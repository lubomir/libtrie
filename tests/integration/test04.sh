#!/bin/bash -e

. $(dirname $0)/helper.sh

strings </dev/urandom | head -n1000 | compile_input

num_keys=$(wc -l <$COMPILE_INPUT)
echo "Inserted $num_keys items" | compile_output

query_input <$COMPILE_INPUT
yes "Found" | head -n 1000 | query_output

runtest "-e"
