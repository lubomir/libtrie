#!/bin/bash -e

. $(dirname $0)/helper.sh

compile_input <<EOF
foo
baz
ahoj
EOF

compile_output <<EOF
Inserted 3 items
EOF

query_input << EOF
foo
baz
non
EOF

query_output <<EOF
Found
Found
Not found
EOF

runtest "-e"
