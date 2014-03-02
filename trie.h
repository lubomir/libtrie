#ifndef TRIE_H
#define TRIE_H

#include <stdio.h>

typedef struct trie Trie;

Trie * trie_new(void);
void trie_free(Trie *trie);
void trie_insert(Trie *trie, const char *key, const char *value);
char *trie_lookup(Trie *trie, const char *key);

void trie_serialize(Trie *trie, const char *filename);

Trie * trie_load(const char *filename);

#endif /* end of include guard: TRIE_H */
