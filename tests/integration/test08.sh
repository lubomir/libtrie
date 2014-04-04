#!/bin/bash -e

. $(dirname $0)/helper.sh

if ./list-query /non/existant-file; then
    echo "Opening non existing file succeeded" >&2
    exit 1
fi

if ./list-query /etc/shadow; then
    echo "Opening file without read permission succeeded" >&2
    exit 1
fi

echo "I am dummy file" >$TEMP
if ./list-query $TEMP; then
    echo "Opening file with bad format succeeded" >&2
    exit 1
fi
