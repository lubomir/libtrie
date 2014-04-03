#!/bin/bash -e

set -o pipefail

TRIE=$(mktemp)
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

    VALGRIND="valgrind --error-exitcode=1 -q"
    if [ "x${USE_VALGRIND:-yes}" = "xno" ] || ! which valgrind; then
        VALGRIND=""
    fi
    RUNNER="libtool --mode=execute $VALGRIND"

    $RUNNER ./list-compile $ARGS $COMPILE_INPUT $TRIE \
        | diff -q - "$COMPILE_OUTPUT"

    $RUNNER ./list-query $TRIE <$QUERY_INPUT \
        | diff -q - "$QUERY_OUTPUT"
}
