#include "trie.h"

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
    size_t data;
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
        free(trie->nodes);
        free(trie->data);
        free(trie);
    }
}

static size_t insert_data(Trie *trie, const char *data)
{
    size_t len = strlen(data) + 1;
    if (trie->data_idx + len >= trie->data_len) {
        trie->data_len *= 2;
        trie->data = realloc(trie->data, trie->data_len);
    }
    size_t idx = trie->data_idx;
    strcpy(trie->data + trie->data_idx, data);
    trie->data[trie->data_idx + len - 1] = 0;
    trie->data_idx += len;
    return idx;
}

#define CHUNK(x) ((TrieNodeChunk*)(&x))

/**
 * The `current` node is always in the trie.
 */
static size_t find_or_create_node(Trie *trie, size_t current, char key)
{
    size_t last = 0;
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
    trie->nodes[new_idx].data = 0;
    trie->nodes[new_idx].chunk.is_inline = 1;
    trie->nodes[new_idx].chunk.size = 0;
    trie->nodes[new_idx].chunk.next = 0;

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
    return new_idx;
}

void trie_insert(Trie *trie, const char *key, const char *value)
{
    size_t current = 1;

    while (*key) {
        current = find_or_create_node(trie, current, *key);
        ++key;
    }
    trie->nodes[current].data = insert_data(trie, value);
}

static size_t find_trie_node(Trie *trie, size_t current, char key)
{
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
    return current == 0 ? NULL : trie->data + trie->nodes[current].data;
}

void trie_serialize(Trie *trie, const char *filename)
{
    FILE *fh = fopen(filename, "w");
    if (!fh) {
        perror("Failed to open output file");
        return;
    }
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
    if (!mem) {
        perror("mmap");
        return NULL;
    }

    close(fd);

    Trie *trie = malloc(sizeof *trie);
    if (!trie) {
        perror("malloc");
        return NULL;
    }

    memcpy(trie, mem, sizeof *trie);
    if (trie->version != VERSION) {
        fprintf(stderr, "Bad version\n");
        return NULL;
    }
    trie->nodes = (TrieNode *) ((char *)mem + sizeof *trie);
    trie->data = (char *)mem + sizeof trie->nodes[0] * trie->idx + sizeof *trie;
    trie->file_len = info.st_size;
    trie->base_mem = mem;
    return trie;
}
