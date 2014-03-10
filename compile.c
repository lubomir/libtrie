#include "trie.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static Trie * load_data(FILE *fh, const char *delimiter)
{
    char *line = NULL;
    size_t len = 0;
    Trie *trie = trie_new();
    unsigned count = 0;

    while (getline(&line, &len, fh) > 0) {
        char *pch = strchr(line, '\n');
        *pch = 0;
        if (strlen(line) <= 1)
            continue;
        char *key = strtok(line, delimiter);
        char *val = strtok(NULL, "\n");
        if (!val)
            continue;
        trie_insert(trie, key, val);
        if (isatty(STDOUT_FILENO) && (++count % 1000) == 0) {
            printf("\rInserted %u items", count);
        }
    }

    if (isatty(STDOUT_FILENO)) {
        putchar('\r');
    }
    printf("Inserted %u items\n", count);

    free(line);

    return trie;
}

int main(int argc, char *argv[])
{
    const char *delimiter = ":";

    int opt;
    while ((opt = getopt(argc, argv, "d:")) != -1) {
        switch (opt) {
        case 'd':
            delimiter = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s [OPTIONS...] INPUT OUTPUT\n", argv[0]);
            return 1;
        }
    }

    if (optind >= argc - 1) {
        fprintf(stderr, "Expected input and output file names\n");
        return 1;
    }

    FILE *infile = fopen(argv[optind], "r");
    if (!infile) {
        perror("Failed to open input file");
        return 2;
    }

    Trie *trie = load_data(infile, delimiter);
    fclose(infile);

    trie_serialize(trie, argv[optind + 1]);

    trie_free(trie);

    return 0;
}
