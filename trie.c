#include "trie.h"

#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>

#define VERSION 3

#define INIT_SIZE 4096

typedef struct {
    size_t len;
    size_t used;
    char data[1];
} String;

typedef uint32_t NodeId;
typedef uint32_t ChunkId;
typedef size_t DataId;

typedef struct {
    ChunkId next;
    NodeId value;
    char key;
} __attribute__((__packed__)) TrieNodeChunk;


typedef struct {
    ChunkId chunk;
    union {
        DataId offset;
        String *string;
    } data;
} TrieNode;

struct trie {
    int version;
    TrieNode *nodes;
    size_t len;
    size_t idx;

    TrieNodeChunk *chunks;
    size_t chunks_len;
    size_t chunks_idx;

    char *data;
    size_t data_idx;
    size_t data_len;

    void *base_mem;
    size_t file_len;
};

static ChunkId chunk_alloc(Trie *t)
{
    if (t->chunks_idx >= t->chunks_len) {
        t->chunks_len *= 2;
        TrieNodeChunk *tmp = realloc(t->chunks, sizeof *tmp * t->chunks_len);
        t->chunks = tmp;
    }
    memset(&t->chunks[t->chunks_idx], 0, sizeof t->chunks[0]);
    assert(t->chunks_idx < UINT32_MAX - 1);
    return t->chunks_idx++;
}

static NodeId node_alloc(Trie *t)
{
    if (t->idx >= t->len) {
        t->len *= 2;
        TrieNode *tmp = realloc(t->nodes, sizeof *tmp * t->len);
        if (!tmp) {
            perror("Failed to allocate node");
            free(t->nodes);
            exit(1);
        }
        t->nodes = tmp;
    }
    memset(&t->nodes[t->idx], 0, sizeof t->nodes[t->idx]);
    t->nodes[t->idx].chunk = 0;
    assert(t->idx < UINT32_MAX - 1);
    return t->idx++;
}

Trie * trie_new(void)
{
    struct trie *t = calloc(sizeof *t, 1);
    t->version = VERSION;
    t->nodes = calloc(sizeof t->nodes[0], INIT_SIZE);
    t->len = INIT_SIZE;
    t->idx = 1;

    t->chunks = calloc(sizeof t->chunks[0], INIT_SIZE);
    t->chunks_len = INIT_SIZE;
    t->chunks_idx = 0;

    t->data_len = INIT_SIZE;
    t->data = calloc(1, INIT_SIZE);
    t->data_idx = 1;

    node_alloc(t);
    chunk_alloc(t);

    return t;
}

void trie_free(Trie *trie)
{
    if (trie->base_mem) {
        munmap(trie->base_mem, trie->file_len);
        free(trie);
    } else {
        free(trie->nodes);
        free(trie->chunks);
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
static NodeId find_or_create_node(Trie *trie, NodeId current, char key)
{
    ChunkId last = 0;
    assert(current < trie->idx);
    for (ChunkId chunk_idx = trie->nodes[current].chunk;
            chunk_idx > 0 && chunk_idx < trie->chunks_idx;
            chunk_idx = trie->chunks[chunk_idx].next) {

        if (trie->chunks[chunk_idx].key == key) {
            return trie->chunks[chunk_idx].value;
        }
        last = chunk_idx;
    }

    ChunkId continuation = chunk_alloc(trie);
    if (last > 0) {
        trie->chunks[last].next = continuation;
    } else {
        trie->nodes[current].chunk = continuation;
    }

    TrieNodeChunk *next = trie->chunks + continuation;
    next->next = 0;
    next->key = key;
    next->value = node_alloc(trie);

    return next->value;
}

void trie_insert(Trie *trie, const char *key, const char *value)
{
    NodeId current = 1;

    while (*key) {
        current = find_or_create_node(trie, current, *key);
        ++key;
    }
    insert_data(trie, trie->nodes + current, value);
}

static NodeId find_trie_node(Trie *trie, NodeId current, char key)
{
    assert(current < trie->idx);
    for (ChunkId chunk_idx = trie->nodes[current].chunk;
            chunk_idx > 0 && chunk_idx < trie->chunks_idx;
            chunk_idx = trie->chunks[chunk_idx].next) {

        if (trie->chunks[chunk_idx].key == key) {
            return trie->chunks[chunk_idx].value;
        }
    }
    return 0;
}

char * trie_lookup(Trie *trie, const char *key)
{
    NodeId current = 1;

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
    for (NodeId idx = 1; idx < trie->idx; ++idx) {
        if (trie->nodes[idx].data.string == NULL) {
            continue;
        }
        char *str = trie->nodes[idx].data.string->data;
        size_t len = trie->nodes[idx].data.string->used;
        if (trie->data_idx + len >= trie->data_len) {
            trie->data_len *= 2;
            trie->data = realloc(trie->data, trie->data_len);
        }
        strcpy(trie->data + trie->data_idx, str);
        free(trie->nodes[idx].data.string);
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
    fwrite(trie->chunks, sizeof *trie->chunks, trie->chunks_idx, fh);
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
    trie->chunks = (TrieNodeChunk *) ((char *)trie->nodes + sizeof *trie->nodes * trie->idx);
    trie->data = (char *)trie->chunks + sizeof *trie->chunks * trie->chunks_idx;
    trie->file_len = info.st_size;
    trie->base_mem = mem;
    return trie;

err2:
    free(trie);
err1:
    munmap(mem, info.st_size);
    return NULL;
}
