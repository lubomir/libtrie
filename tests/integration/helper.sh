#!/bin/bash -e

TRIE=$(mktemp)
COMPILE_INPUT=$(mktemp)
COMPILE_OUTPUT=$(mktemp)
QUERY_INPUT=$(mktemp)
QUERY_OUTPUT=$(mktemp)
TEMP=$(mktemp)

cleanup()
{
    rm -f $TRIE $COMPILE_INPUT $COMPILE_OUTPUT $QUERY_INPUT $QUERY_OUTPUT $TEMP
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

run()
{
    ACTION=$1
    shift
    if ! $*; then
        echo "When running action $ACTION:  <$*> failed" >&2
        exit 1
    fi
}

runtest()
{
    ARGS=$1

    VALGRIND="valgrind --error-exitcode=1 -q"
    if [ "x${USE_VALGRIND:-yes}" = "xno" ] || ! which valgrind >/dev/null; then
        VALGRIND=""
    fi
    RUNNER="libtool --mode=execute $VALGRIND"

    run compile $RUNNER ./list-compile $ARGS $COMPILE_INPUT $TRIE >$TEMP
    run compile diff $TEMP $COMPILE_OUTPUT

    run query $RUNNER ./list-query $TRIE <$QUERY_INPUT >$TEMP
    run query diff $TEMP $QUERY_OUTPUT
}
