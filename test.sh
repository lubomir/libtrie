#!/bin/bash

set -e
TRIE=$(mktemp)

make
valgrind ./list-compile /dev/stdin $TRIE <<EOF
foo:bar
baz:quux
ahoj:baf
foo:foo
EOF
valgrind ./list-query $TRIE <<EOF
foo
baz
non
EOF
rm -vf $TRIE
