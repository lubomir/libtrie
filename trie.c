#include "trie.h"

#include <stdbool.h>
#include <stdlib.h>

#define FULL_CHUNK_BYTE_SIZE  64
#define FIRST_CHUNK_BYTE_SIZE 56
/** Number of children pointers in a non-first chunk. */
#define FULL_CHUNK_LEN 6
/** Number of children pointers in the first (inlined) chunk. */
#define FIRST_CHUNK_LEN 5
/** Get a number of children pointers for a given chunk. */
#define GET_CHUNK_SIZE(c) (c->is_inline ? FIRST_CHUNK_LEN : FULL_CHUNK_LEN)

typedef struct trie_node TrieNode;

/**
 * TrieNodeChunk is a pair of small arrays - one for keys, the other
 * for pointers to TrieNodes.
 * */
typedef struct trie_node_chunk {
    /** Pointer to next chunk of NULL if this is the last chunk. */
    struct trie_node_chunk *next;
    bool is_inline : 1;
    /** Number of pointers stored in this chunk. */
    unsigned int size : 7;
    /** Array of keys used in this chunk. */
    char keys[FULL_CHUNK_LEN];
    /** Values associated with the keys. */
    TrieNode *values[FIRST_CHUNK_LEN];
} TrieNodeChunk;

/**
 * TrieNode is a node in the prefix tree. It holds a pointer to stored
 * data and contains a linked list of key arrays.
 */
struct trie_node {
    void *data;
    TrieNodeChunk chunk;
};

Trie * trie_new(void)
{
    TrieNode *node = calloc(sizeof *node, 1);
    node->chunk.is_inline = 1;
    return node;
}

static TrieNode *
find_or_create_trie_node(TrieNode *node, const char key)
{
    TrieNodeChunk *last = NULL;
    for (TrieNodeChunk *chunk = &node->chunk; chunk; chunk = chunk->next) {
        for (size_t i = 0; i < chunk->size; ++i) {
            if (chunk->keys[i] == key) {
                return chunk->values[i];
            }
        }
        last = chunk;
    }

    TrieNode *new_node = malloc(sizeof *new_node);
    new_node->data = NULL;
    new_node->chunk.is_inline = 1;
    new_node->chunk.size = 0;
    new_node->chunk.next = NULL;

    if (last->size < GET_CHUNK_SIZE(last)) {
        last->keys[last->size] = key;
        last->values[last->size] = new_node;
        ++last->size;
    } else {
        last->next = malloc(FULL_CHUNK_BYTE_SIZE);
        last->next->is_inline = 0;
        last->next->size = 1;
        last->next->next = NULL;
        last->next->keys[0] = key;
        last->next->values[0] = new_node;
    }
    return new_node;
}

void
trie_insert(Trie *trie, const char *key, void *val)
{
    TrieNode *node = trie;

    while (*key) {
        node = find_or_create_trie_node(node, *key);
        ++key;
    }
    if (node->data) {
        free(node->data);
    }
    node->data = val;
}

#if 0
#include "utils.h"

static void
trie_node_free(Env *env, TrieNode *tn)
{
    if (!tn) {
        return;
    }
    for (TrieNodeChunk *chunk = &tn->chunk; chunk; ) {
        for (unsigned i = 0; i < chunk->size; ++i) {
            trie_node_free(env, chunk->values[i]);
        }
        TrieNodeChunk *next = chunk->next;
        if (!chunk->is_inline)
            free(chunk);
        chunk = next;
    }

    if (env->destr) {
        env->destr(tn->data);
    }
    free(tn);
}

void env_free(Env *env)
{
    if (env) {
        trie_node_free(env, env->root);
        free(env);
    }
}

static TrieNode *
trie_node_dup(Env *env, TrieNode *node);

static TrieNodeChunk *
chunk_dup(Env *env, TrieNodeChunk *chunk)
{
    if (!chunk) {
        return NULL;
    }
    TrieNodeChunk *new_chunk = malloc(FULL_CHUNK_BYTE_SIZE);
    new_chunk->is_inline = 0;
    new_chunk->size = chunk->size;
    for (size_t i = 0; i < chunk->size; ++i) {
        new_chunk->keys[i] = chunk->keys[i];
        new_chunk->values[i] = trie_node_dup(env, chunk->values[i]);
    }
    new_chunk->next = chunk_dup(env, chunk->next);

    return new_chunk;
}

static TrieNode *
trie_node_dup(Env *env, TrieNode *node)
{
    if (node == NULL) {
        return NULL;
    }

    TrieNode *new = malloc(sizeof *new);
    new->data = env->copy(node->data);
    memcpy(&new->chunk, &node->chunk, FIRST_CHUNK_BYTE_SIZE);
    new->chunk.next = chunk_dup(env, node->chunk.next);
    for (size_t i = 0; i < new->chunk.size; ++i) {
        new->chunk.values[i] = trie_node_dup(env, node->chunk.values[i]);
    }

    return new;
}

Env * env_dup(Env *env)
{
    Env *new = malloc(sizeof *new);
    new->copy = env->copy;
    new->destr = env->destr;
    new->root = trie_node_dup(new, env->root);

    return new;
}

static TrieNode *
find_trie_node(TrieNode *node, const char key)
{
    for (TrieNodeChunk *chunk = &node->chunk; chunk; chunk = chunk->next) {
        for (size_t i = 0; i < chunk->size; ++i) {
            if (chunk->keys[i] == key) {
                return chunk->values[i];
            }
        }
    }
    return NULL;
}

static TrieNode *
find_or_create_trie_node(TrieNode *node, const char key)
{
    TrieNodeChunk *last = NULL;
    for (TrieNodeChunk *chunk = &node->chunk; chunk; chunk = chunk->next) {
        for (size_t i = 0; i < chunk->size; ++i) {
            if (chunk->keys[i] == key) {
                return chunk->values[i];
            }
        }
        last = chunk;
    }

    TrieNode *new_node = malloc(sizeof *new_node);
    new_node->data = NULL;
    new_node->chunk.is_inline = 1;
    new_node->chunk.size = 0;
    new_node->chunk.next = NULL;

    if (last->size < GET_CHUNK_SIZE(last)) {
        last->keys[last->size] = key;
        last->values[last->size] = new_node;
        ++last->size;
    } else {
        last->next = malloc(FULL_CHUNK_BYTE_SIZE);
        last->next->is_inline = 0;
        last->next->size = 1;
        last->next->next = NULL;
        last->next->keys[0] = key;
        last->next->values[0] = new_node;
    }
    return new_node;
}

static void
env_insert(Env *env, const char *key, void *val)
{
    TrieNode *node = env->root;

    while (*key) {
        node = find_or_create_trie_node(node, *key);
        ++key;
    }
    if (node->data) {
        env->destr(node->data);
    }
    node->data = val;
}

void * env_remove(Env *env, const char *key)
{
    TrieNode *node = env->root;

    while (*key) {
        node = find_trie_node(node, *key);
        ++key;
    }

    if (node && node->data) {
        /* This may leave around an empty TrieNode */
        void *val = node->data;
        node->data = NULL;
        return val;
    }
    return NULL;
}

int env_define(Env *env, const char *key, void *val)
{
    env_insert(env, key, val);
    return 1;
}

int env_set(Env *env, const char *key, void *val)
{
    if (env_is_bound(env, key)) {
        return 0;
    }
    env_insert(env, key, val);
    return 1;
}

void * env_get(Env *env, const char *key)
{
    TrieNode *node = env->root;

    while (*key) {
        node = find_trie_node(node, *key++);
    }
    return node ? node->data : NULL;
}
#endif
