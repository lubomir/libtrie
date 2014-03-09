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
typedef uint32_t DataId;

typedef struct {
    ChunkId next;
    NodeId value;
    char key;
} __attribute__((__packed__)) TrieNodeChunk;

static_assert(sizeof(TrieNodeChunk) == 9, "TrieNodeChunk has wrong size");

typedef struct {
    ChunkId chunk;
    DataId data;
} __attribute__((packed)) TrieNode;

static_assert(sizeof(TrieNode) == 8, "TrieNodeChunk has wrong size");

struct trie {
    int version;
    TrieNode *nodes;
    size_t len;
    size_t idx;

    TrieNodeChunk *chunks;
    size_t chunks_len;
    size_t chunks_idx;

    char *data;
    String **data_builder;
    size_t data_idx;
    size_t data_len;

    void *base_mem;
    size_t file_len;
};

#define ERROR_STAT      1
#define ERROR_OPEN      2
#define ERROR_MMAP      3
#define ERROR_VERSION   4

static int last_error = 0;
static const char *errors[] = {
    NULL,
    "Failed to stat the file",
    "Failed to open file",
    "Mapping file to memory failed",
    "File has bad version"
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

    t->data_builder = calloc(sizeof *t->data_builder, INIT_SIZE);
    t->data_len = INIT_SIZE;
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

/**
 * @param trie  trie that is being inserted to
 * @param node  to which node we are inserting
 * @param data  actual inserted data
 */
static void insert_data(Trie *trie, TrieNode *node, const char *data)
{
    assert(trie->base_mem == NULL);

    size_t len = strlen(data);
    /* No string exists for this node yet. */
    if (node->data == 0) {
        /* Resize array of strings. */
        if (trie->data_idx + 1 >= trie->data_len) {
            trie->data_len *= 2;
            trie->data_builder = realloc(trie->data_builder,
                    trie->data_len * sizeof *trie->data_builder);
        }
        node->data = trie->data_idx++;
        trie->data_builder[node->data] = calloc(256, 1);
        trie->data_builder[node->data]->len = 256;
    }
    String *s = trie->data_builder[node->data];
    if (s->used + len + 1 >= s->len - 16) {
        s->len *= 2;
        trie->data_builder[node->data] = s = realloc(s, s->len);
    }
    if (s->used > 0) {
        s->data[s->used++] = '\n';
    }
    strcpy(s->data + s->used, data);
    s->used += len;
}

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
    assert(trie->base_mem);
    return trie->data + trie->nodes[current].data;
}

static void trie_consolidate(Trie *trie)
{
    assert(trie->base_mem == NULL);
    trie->data_len = INIT_SIZE;
    trie->data_idx = 1;
    trie->data = calloc(1, INIT_SIZE);

    for (NodeId idx = 1; idx < trie->idx; ++idx) {
        if (trie->nodes[idx].data == 0) {
            continue;
        }
        String *s = trie->data_builder[trie->nodes[idx].data];
        if (trie->data_idx + s->used >= trie->data_len) {
            trie->data_len *= 2;
            trie->data = realloc(trie->data, trie->data_len);
        }
        strcpy(trie->data + trie->data_idx, s->data);
        trie->data[trie->data_idx + s->used] = 0;
        trie->nodes[idx].data = trie->data_idx;
        trie->data_idx += s->used + 1;
        free(s);
    }
    free(trie->data_builder);
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
        last_error = ERROR_STAT;
        return NULL;
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        last_error = ERROR_OPEN;
        return NULL;
    }

    void *mem = mmap(NULL, info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);

    if (!mem) {
        last_error = ERROR_MMAP;
        return NULL;
    }

    Trie *trie = malloc(sizeof *trie);

    memcpy(trie, mem, sizeof *trie);
    if (trie->version != VERSION) {
        last_error = ERROR_VERSION;
        goto err;
    }
    trie->nodes = (TrieNode *) ((char *)mem + sizeof *trie);
    trie->chunks = (TrieNodeChunk *) ((char *)trie->nodes + sizeof *trie->nodes * trie->idx);
    trie->data = (char *)trie->chunks + sizeof *trie->chunks * trie->chunks_idx;
    trie->file_len = info.st_size;
    trie->base_mem = mem;
    return trie;

err:
    free(trie);
    munmap(mem, info.st_size);
    return NULL;
}

const char * trie_get_last_error(void)
{
    return errors[last_error];
}
