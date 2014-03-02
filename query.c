#include "trie.h"

#include <stdio.h>
#include <string.h>

void run_loop(Trie *trie)
{
    char buffer[1024];
    while (fgets(buffer, sizeof buffer, stdin)) {
        char *pch = strchr(buffer, '\n');
        *pch = 0;
        char *data = trie_lookup(trie, buffer);
        if (data) {
            puts(data);
        } else {
            puts("Not found");
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s FILE\n", argv[0]);
        return 1;
    }
    Trie *trie = trie_load(argv[1]);
    if (!trie) {
        fprintf(stderr, "Failed to load trie\n");
        return 2;
    }

    run_loop(trie);

    trie_free(trie);

    return 0;
}
