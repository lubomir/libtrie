#!/bin/bash -e

. $(dirname $0)/helper.sh

COUNT=10000

echo "foo:bar" | compile_input
echo "Inserted 1 items" | compile_output

strings /dev/urandom | grep -v foo | head -n $COUNT | query_input
yes "Not found" | head -n $COUNT | query_output

runtest "-e"
