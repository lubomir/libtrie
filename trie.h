#ifndef TRIE_H
#define TRIE_H

typedef struct trie_node Trie;

Trie * trie_new(void);
void trie_free(Trie *trie);
void trie_insert(Trie *trie, const char *key, void *value);

#endif /* end of include guard: TRIE_H */
