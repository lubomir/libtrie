#include <config.h>
#include "trie.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

void run_loop(Trie *trie)
{
    char buffer[1024];
    while (fgets(buffer, sizeof buffer, stdin)) {
        char *pch = strchr(buffer, '\n');
        *pch = 0;
        const char *data = trie_lookup(trie, buffer);
        if (data) {
            puts(data);
            trie_result_free(trie, data);
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
    if (strcmp(argv[1], "-h") == 0) {
        printf("Usage: %s FILE\n", argv[0]);
        puts("This program has no other options.");
        puts("");
        puts("This is list-query from "PACKAGE" "VERSION".");
        puts("File bug reports at <"PACKAGE_URL">.");
        return 0;
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
