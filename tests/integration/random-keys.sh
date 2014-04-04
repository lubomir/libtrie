#!/bin/bash -e

. $(dirname $0)/helper.sh

COUNT=10000

strings </dev/urandom | head -n $COUNT | compile_input

num_keys=$(wc -l <$COMPILE_INPUT)
echo "Inserted $num_keys items" | compile_output

$SHUF $COMPILE_INPUT | query_input
yes "Found" | head -n $COUNT | query_output

runtest "-e"
