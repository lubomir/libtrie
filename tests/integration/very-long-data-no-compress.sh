#!/bin/bash -e

. $(dirname $0)/helper.sh

COUNT=10000

yes "very-very-very-long-key:some data" | head -n $COUNT | compile_input
echo "Inserted $COUNT items" | compile_output

echo "very-very-very-long-key" | query_input
yes "some data" | head -n $COUNT | query_output

runtest "-u"
