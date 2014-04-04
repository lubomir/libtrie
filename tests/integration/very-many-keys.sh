#!/bin/bash -e

. $(dirname $0)/helper.sh

COUNT=10000

for n in $(seq 1 $COUNT); do
    echo "my-key-$n:my-data-$n"
done | $SHUF | compile_input
echo "Inserted $COUNT items" | compile_output

cut -d: -f1 $COMPILE_INPUT | query_input
cut -d: -f2 $COMPILE_INPUT | query_output

runtest ""
