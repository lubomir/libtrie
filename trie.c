#include "trie.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

#define VERSION 1

#define INIT_SIZE 4096

#define FULL_CHUNK_BYTE_SIZE 64
#define FIRST_CHUNK_BYTE_SIZE 56
#define FULL_CHUNK_LEN 6
#define FIRST_CHUNK_LEN 5
#define GET_CHUNK_SIZE(c) ((c)->is_inline ? FIRST_CHUNK_LEN : FULL_CHUNK_LEN)

typedef struct {
    size_t len;
    size_t used;
    char data[1];
} String;

typedef struct trie_node TrieNode;

typedef struct trie_node_chunk {
    size_t next;
    bool is_inline : 1;
    unsigned int size : 7;
    char keys[FULL_CHUNK_LEN];
    size_t values[FIRST_CHUNK_LEN];
} TrieNodeChunk;


struct trie_node {
    TrieNodeChunk chunk;
    union {
        size_t offset;
        String *string;
    } data;
};

struct trie {
    int version;
    TrieNode *nodes;
    size_t len;
    size_t idx;

    char *data;
    size_t data_idx;
    size_t data_len;

    void *base_mem;
    size_t file_len;
};


static size_t node_alloc(Trie *t, int is_inline)
{
    if (t->idx >= t->len) {
        t->len *= 2;
        struct trie_node *tmp = realloc(t->nodes, sizeof *tmp * t->len);
        if (!tmp) {
            perror("Failed to allocate node");
            free(t->nodes);
            exit(1);
        }
        t->nodes = tmp;
    }
    memset(&t->nodes[t->idx], 0, sizeof t->nodes[t->idx]);
    t->nodes[t->idx].chunk.is_inline = is_inline;
    return t->idx++;
}

Trie * trie_new(void)
{
    struct trie *t = calloc(sizeof *t, 1);
    t->version = VERSION;
    t->nodes = calloc(sizeof t->nodes[0], INIT_SIZE);
    t->len = INIT_SIZE;
    t->idx = 1;

    t->data_len = INIT_SIZE;
    t->data = calloc(1, INIT_SIZE);
    t->data_idx = 1;

    node_alloc(t, 1);

    return t;
}

void trie_free(Trie *trie)
{
    if (trie->base_mem) {
        munmap(trie->base_mem, trie->file_len);
        free(trie);
    } else {
        for (size_t idx = 1; idx < trie->idx; ++idx) {
            free(trie->nodes[idx].data.string);
        }
        free(trie->nodes);
        free(trie->data);
        free(trie);
    }
}

static void insert_data(Trie *trie, TrieNode *node, const char *data)
{
    assert(trie->base_mem == NULL);

    size_t len = strlen(data);

    if (node->data.string == NULL) {
        node->data.string = calloc(256, 1);
        node->data.string->len = 256;
    }

    if (node->data.string->used + len + 1 >= node->data.string->len - 16) {
        node->data.string->len *= 2;
        String *tmp = realloc(node->data.string, node->data.string->len);
        node->data.string = tmp;
    }

    if (node->data.string->used > 0) {
        node->data.string->data[node->data.string->used++] = '\n';
    }
    strcpy(node->data.string->data + node->data.string->used, data);
    node->data.string->used += len;
}

#define CHUNK(x) ((TrieNodeChunk*)(&x))

/**
 * The `current` node is always in the trie.
 */
static size_t find_or_create_node(Trie *trie, size_t current, char key)
{
    size_t last = 0;
    assert(current < trie->idx);
    for (size_t chunk_idx = current;
            chunk_idx > 0 && chunk_idx < trie->idx;
            chunk_idx = CHUNK(trie->nodes[chunk_idx])->next) {
        TrieNodeChunk *chunk = CHUNK(trie->nodes[chunk_idx]);
        for (size_t i = 0; i < chunk->size; ++i) {
            if (chunk->keys[i] == key) {
                return chunk->values[i];
            }
        }
        last = chunk_idx;
    }

    size_t new_idx = node_alloc(trie, 1);
    trie->nodes[new_idx].chunk.is_inline = 1;

    TrieNodeChunk *last_chunk = &trie->nodes[last].chunk;
    if (last_chunk->size < GET_CHUNK_SIZE(last_chunk)) {
        last_chunk->values[last_chunk->size] = new_idx;
        last_chunk->keys[last_chunk->size++] = key;
    } else {
        size_t continuation = node_alloc(trie, 0);
        last_chunk = &trie->nodes[last].chunk;
        last_chunk->next = continuation;
        TrieNodeChunk *next = &trie->nodes[continuation].chunk;
        next->size = 1;
        next->next = 0;
        next->keys[0] = key;
        next->values[0] = new_idx;
    }
    assert(new_idx < trie->idx);
    return new_idx;
}

void trie_insert(Trie *trie, const char *key, const char *value)
{
    size_t current = 1;

    while (*key) {
        current = find_or_create_node(trie, current, *key);
        ++key;
    }
    insert_data(trie, trie->nodes + current, value);
}

static size_t find_trie_node(Trie *trie, size_t current, char key)
{
    assert(current < trie->idx);
    for (size_t chunk_idx = current;
            chunk_idx > 0 && chunk_idx < trie->idx;
            chunk_idx = trie->nodes[chunk_idx].chunk.next) {
        TrieNodeChunk *chunk = &trie->nodes[chunk_idx].chunk;
        for (size_t i = 0; i < chunk->size; ++i) {
            if (chunk->keys[i] == key) {
                return chunk->values[i];
            }
        }
    }
    return 0;
}

char * trie_lookup(Trie *trie, const char *key)
{
    size_t current = 1;

    while (*key && current < trie->idx) {
        current = find_trie_node(trie, current, *key++);
    }
    assert(current < trie->idx);
    if (current == 0) {
        return NULL;
    }
    if (trie->base_mem) {
        return trie->data + trie->nodes[current].data.offset;
    } else {
        return trie->nodes[current].data.string->data;
    }
}

static void trie_consolidate(Trie *trie)
{
    assert(trie->base_mem == NULL);
    for (size_t idx = 1; idx < trie->idx; ++idx) {
        if (!trie->nodes[idx].chunk.is_inline ||
                trie->nodes[idx].data.string == NULL) {
            /* Non-inline chunks have no data */
            continue;
        }
        char *str = trie->nodes[idx].data.string->data;
        size_t len = trie->nodes[idx].data.string->used;
        if (trie->data_idx + len >= trie->data_len) {
            trie->data_len *= 2;
            trie->data = realloc(trie->data, trie->data_len);
        }
        strcpy(trie->data + trie->data_idx, str);
        trie->data[trie->data_idx + len] = 0;
        trie->nodes[idx].data.offset = trie->data_idx;
        trie->data_idx += len + 1;
    }
}

void trie_serialize(Trie *trie, const char *filename)
{
    FILE *fh = fopen(filename, "w");
    if (!fh) {
        perror("Failed to open output file");
        return;
    }
    trie_consolidate(trie);
    fwrite(trie, sizeof *trie, 1, fh);
    fwrite(trie->nodes, sizeof *trie->nodes, trie->idx, fh);
    fwrite(trie->data, 1, trie->data_idx, fh);
    fclose(fh);
}

Trie * trie_load(const char *filename)
{
    struct stat info;
    if (stat(filename, &info) < 0) {
        perror("stat");
        return NULL;
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("open");
        return NULL;
    }

    void *mem = mmap(NULL, info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (!mem) {
        perror("mmap");
        return NULL;
    }

    Trie *trie = malloc(sizeof *trie);
    if (!trie) {
        perror("malloc");
        goto err1;
    }

    memcpy(trie, mem, sizeof *trie);
    if (trie->version != VERSION) {
        fprintf(stderr, "Bad version\n");
        goto err2;
    }
    trie->nodes = (TrieNode *) ((char *)mem + sizeof *trie);
    trie->data = (char *)mem + sizeof trie->nodes[0] * trie->idx + sizeof *trie;
    trie->file_len = info.st_size;
    trie->base_mem = mem;
    return trie;

err2:
    free(trie);
err1:
    munmap(mem, info.st_size);
    return NULL;
}
