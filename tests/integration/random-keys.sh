#!/bin/bash -e

. $(dirname $0)/helper.sh

COUNT=10000

strings </dev/urandom | head -n $COUNT | compile_input

num_keys=$(wc -l <$COMPILE_INPUT)
echo "Inserted $num_keys items" | compile_output

query_input <$COMPILE_INPUT
yes "Found" | head -n $COUNT | query_output

runtest "-e"
