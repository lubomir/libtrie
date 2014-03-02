#include "trie.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char * str_dup(const char *c)
{
    char *copy = malloc(strlen(c)+1);
    return strcpy(copy, c);
}

static Trie * load_data(FILE *fh)
{
    char *line = NULL;
    size_t len = 0;
    Trie *trie = trie_new();

    while (getline(&line, &len, fh) > 0) {
        if (strlen(line) <= 1)
            continue;
        char *key = line;
        char *val = strtok(line, ":");
        if (!val)
            continue;
        trie_insert(trie, key, str_dup(val));
    }

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

    return 0;
}
