#!/bin/bash -e

set -o pipefail

COMPILE_INPUT=$(mktemp)
COMPILE_OUTPUT=$(mktemp)
QUERY_INPUT=$(mktemp)
QUERY_OUTPUT=$(mktemp)

cleanup()
{
    rm -f $TRIE $COMPILE_INPUT $COMPILE_OUTPUT $QUERY_INPUT $QUERY_OUTPUT
}

trap cleanup EXIT

compile_input()
{
    cat >$COMPILE_INPUT
}

compile_output()
{
    cat >$COMPILE_OUTPUT
}

query_input()
{
    cat >$QUERY_INPUT
}

query_output()
{
    cat >$QUERY_OUTPUT
}

runtest()
{
    ARGS=$1

    TRIE=$(mktemp)

    if [ "x${USE_VALGRIND:-yes}" = "xno" ] || ! which valgrind; then
        VALGRIND=""
    else
        VALGRIND="valgrind --error-exitcode=1 -q"
    fi

    libtool --mode=execute $VALGRIND ./list-compile $ARGS "$COMPILE_INPUT" "$TRIE" \
        | diff -q - "$COMPILE_OUTPUT"

    libtool --mode=execute $VALGRIND ./list-query $TRIE <"$QUERY_INPUT" \
        | diff -q - "$QUERY_OUTPUT"
}
