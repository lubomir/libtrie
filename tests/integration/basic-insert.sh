#!/bin/bash -e

. $(dirname $0)/helper.sh

compile_input <<EOF
foo:bar
baz:quux
ahoj:baf
foo:foo
EOF

compile_output <<EOF
Inserted 4 items
EOF

query_input << EOF
foo
baz
non
EOF

query_output <<EOF
bar
foo
quux
Not found
EOF

runtest ""
