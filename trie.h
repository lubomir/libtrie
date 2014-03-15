#ifndef TRIE_H
#define TRIE_H

#include <stdio.h>

/**
 * Opaque type for the trie. Do not access any members directly.
 *
 * The trie is always in one of two possible modes. Either it is loaded from a
 * file via `trie_load()`. In this case the trie is read only and no data can
 * be inserted into it.
 *
 * If the trie is created from scratch via `trie_new()`, it is write-only and
 * does not support searching. The only thing that can be done to such a
 * structure is serialize it to a file via `trie_serialize()`.
 */
typedef struct trie Trie;

/**
 * Create new empty write-only trie. Free with `trie_free()` when no longer
 * needed.
 *
 * @param with_content  whether there will be data associated with nodes
 * @return              new empty trie
 */
Trie * trie_new(int with_content);

/**
 * Free all memory held by the trie.
 *
 * @param trie  trie to be freed
 */
void trie_free(Trie *trie);

/**
 * Insert data into the trie. The trie must have been created via `trie_new()`.
 * If the keys is already present in the trie, the data will be appended to the
 * current data (delimited by a new line.
 *
 * @param trie  trie to insert into
 * @param key   under which key to insert the data
 * @param value data to be inserted
 */
void trie_insert(Trie *trie, const char *key, const char *value);

/**
 * Look up a value under given key. The trie must have been loaded from a file.
 * The result is dynamically allocated and it is the caller's responsibility to
 * free it.
 *
 * @param trie  trie to search
 * @param key   what key is wanted
 * @return      associated value or NULL
 */
char * trie_lookup(Trie *trie, const char *key);

/**
 * Store the trie into a file. The trie must have been created from scratch via
 * `trie_new()`.
 *
 * @param trie      trie to be serialized
 * @param filename  to which file we want to save
 */
void trie_serialize(Trie *trie, const char *filename);

/**
 * Load the trie from given file. The file must have been created by calling
 * to `trie_serialize()`. Free the result with `trie_free()` when no longer
 * needed.
 *
 * @param filename  file to be loaded
 * @return          loaded trie or NULL
 */
Trie * trie_load(const char *filename);

/**
 * If some function failed, use this function to get user-friendly error
 * message. The result is a static string that should not be free'd.
 *
 * @return string message
 */
const char * trie_get_last_error(void);

#endif /* end of include guard: TRIE_H */
