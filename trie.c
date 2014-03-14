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

#define VERSION 11

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
    int with_content;
    TrieNode *nodes;
    uint32_t len;
    uint32_t idx;

    TrieNodeChunk *chunks;
    uint32_t chunks_len;
    uint32_t chunks_idx;

    char *data;
    String **data_builder;
    uint32_t data_idx;
    uint32_t data_len;

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

Trie * trie_new(int with_content)
{
    Trie *t = calloc(sizeof *t, 1);
    t->version = VERSION;
    t->with_content = with_content;
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
    if (!trie)
        return;
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

static void
compress(char *buffer, const char *data, const char *key)
{
    size_t key_len = strlen(key);
    size_t data_len = strlen(data);
    size_t common = 0;
    while (common < key_len && common < data_len && key[common] == data[common]) {
        ++common;
    }
    buffer[0] = (char) common + '0';
    memcpy(buffer + 1, data + common, data_len - common + 1);
}

static void
decompress(char *buffer, const char *data, const char *key)
{
    char tmp[strlen(data) + 1];
    strcpy(tmp, data);

    buffer[0] = 0;

    char *line = strtok(tmp, "\n");
    int counter = 0;
    while (line) {
        if (counter > 0) {
            strcat(buffer, "\n");
        }
        size_t len = strlen(buffer);
        strncat(buffer, key, line[0] - '0');
        buffer[len + line[0] - '0'] = 0;
        strcat(buffer, line + 1);
        ++counter;
        line = strtok(NULL, "\n");
    }
}

/**
 * @param trie  trie that is being inserted to
 * @param node  to which node we are inserting
 * @param data  actual inserted data
 */
static void
insert_data(Trie *trie, TrieNode *node, const char *data, const char *key)
{
    assert(trie->base_mem == NULL);

    if (!trie->with_content) {
        node->data = 1;
        return;
    }

    size_t len = strlen(data);
    char buffer[len+1];
    strcpy(buffer, data);
    compress(buffer, data, key);
    len = strlen(buffer);

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
    strcpy(s->data + s->used, buffer);
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
    const char *orig_key = key;

    while (*key) {
        current = find_or_create_node(trie, current, *key);
        ++key;
    }
    insert_data(trie, trie->nodes + current, value, orig_key);
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

char * trie_lookup(Trie *trie, const char *key, char *buffer)
{
    NodeId current = 1;
    const char *orig_key = key;

    while (*key && current < trie->idx) {
        current = find_trie_node(trie, current, *key++);
    }
    assert(current < trie->idx);
    if (current == 0) {
        return NULL;
    }
    if (!trie->with_content) {
        return strcpy(buffer, "Found");
    }
    assert(trie->base_mem);
    char *data = trie->data + trie->nodes[current].data;
    decompress(buffer, data, orig_key);
    return buffer;
}

static int string_compare(const void *a, const void *b)
{
    const char *s1 = * (char * const *) a;
    const char *s2 = * (char * const *) b;
    return strcmp(s1, s2);
}

/**
 * Find index of a string in the data section of the trie.
 */
static DataId
search_string(Trie *trie, char **strings, size_t len, const char *string)
{
    char **ptr = bsearch(&string, strings, len, sizeof *strings, string_compare);
    assert(ptr);
    return *ptr - trie->data;
}

/**
 * Move all unique strings from the array to data section of a trie. This
 * function assumes the data section is already allocated and is big enough.
 * The array will be changed to only have a single copy of each string.
 * @return new length of the array
 */
static size_t
strings_deduplicate(Trie *trie, char **arr, size_t len)
{
    size_t read = 0;
    size_t write = 0;
    trie->data_idx = 1;

    while (read < len) {
        while (read < len - 1 && strcmp(arr[read], arr[read+1]) == 0) {
            ++read;
        }
        size_t string_length = strlen(arr[read]) + 1;
        arr[write++] = memcpy(trie->data + trie->data_idx, arr[read++], string_length);
        trie->data_idx += string_length;
    }
    return write;
}

/**
 * Move all strings from data builders into the data section of the trie.
 * @param len   (out) length of returned array
 * @return      (transfer container) ordered array of strings
 */
static char **
create_strings(Trie *trie, size_t *len)
{
    char **strings = malloc(trie->data_idx * sizeof *strings);
    trie->data_len = 1;
    for (size_t idx = 1; idx < trie->data_idx; ++idx) {
        strings[idx - 1] = trie->data_builder[idx]->data;
        trie->data_len += strlen(strings[idx - 1]) + 1;
    }
    trie->data = calloc(1, trie->data_len);

    qsort(strings, trie->data_idx - 1, sizeof *strings, string_compare);
    *len = strings_deduplicate(trie, strings, trie->data_idx - 1);
    return strings;
}

static void trie_consolidate(Trie *trie)
{
    assert(trie->base_mem == NULL);

    size_t s_len;
    char **strings = create_strings(trie, &s_len);

    for (NodeId idx = 1; idx < trie->idx; ++idx) {
        if (trie->nodes[idx].data == 0) {
            continue;
        }
        String *s = trie->data_builder[trie->nodes[idx].data];
        trie->nodes[idx].data = search_string(trie, strings, s_len, s->data);
        free(s);
    }
    free(strings);
    free(trie->data_builder);
}

void trie_serialize(Trie *trie, const char *filename)
{
    FILE *fh = fopen(filename, "w");
    if (!fh) {
        perror("Failed to open output file");
        return;
    }
    if (trie->with_content) {
        trie_consolidate(trie);
    }
    fwrite(trie, sizeof *trie, 1, fh);
    fwrite(trie->nodes, sizeof *trie->nodes, trie->idx, fh);
    fwrite(trie->chunks, sizeof *trie->chunks, trie->chunks_idx, fh);
    if (trie->with_content) {
        fwrite(trie->data, 1, trie->data_idx, fh);
    }
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
    if (trie->with_content) {
        trie->data = (char *)trie->chunks + sizeof *trie->chunks * trie->chunks_idx;
    } else {
        trie->data = NULL;
    }
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
