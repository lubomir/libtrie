#!/bin/bash

set -e
set -o pipefail

TRIE=$(mktemp)
COMPILE_INPUT=$(mktemp)
COMPILE_OUTPUT=$(mktemp)
QUERY_INPUT=$(mktemp)
QUERY_OUTPUT=$(mktemp)

cat >"$COMPILE_INPUT" <<EOF
foo|bar
baz|quux
ahoj|baf
foo|foo
EOF

cat >"$COMPILE_OUTPUT" <<EOF
Inserted 4 items
EOF

cat >"$QUERY_INPUT" << EOF
foo
baz
non
EOF

cat >"$QUERY_OUTPUT" <<EOF
bar
foo
quux
Not found
EOF

make

libtool --mode=execute valgrind --error-exitcode=1 -q ./list-compile -d'|' "$COMPILE_INPUT" "$TRIE" \
    | diff -q - "$COMPILE_OUTPUT"

libtool --mode=execute valgrind --error-exitcode=1 -q ./list-query $TRIE <"$QUERY_INPUT" \
    | diff -q - "$QUERY_OUTPUT"

rm -f $TRIE $COMPILE_INPUT $COMPILE_OUTPUT $QUERY_INPUT $QUERY_OUTPUT
