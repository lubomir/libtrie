#include "trie.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Trie * load_data(FILE *fh)
{
    char *line = NULL;
    size_t len = 0;
    Trie *trie = trie_new();

    while (getline(&line, &len, fh) > 0) {
        char *pch = strchr(line, '\n');
        *pch = 0;
        if (strlen(line) <= 1)
            continue;
        char *key = strtok(line, ":");
        char *val = strtok(NULL, "\n");
        if (!val)
            continue;
        trie_insert(trie, key, val);
    }

    free(line);

    return trie;
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "USAGE: %s INPUT OUTPUT\n", argv[0]);
        return 1;
    }
    FILE *infile = fopen(argv[1], "r");
    if (!infile) {
        perror("Failed to open input file");
        return 2;
    }

    Trie *trie = load_data(infile);
    fclose(infile);

    trie_serialize(trie, argv[2]);

    trie_free(trie);

    return 0;
}
