ACLOCAL_AMFLAGS = -I m4

if ENABLE_SHARED_LIB
lib_LTLIBRARIES = libtrie.la
include_HEADERS = src/trie.h
else
noinst_LTLIBRARIES = libtrie.la
noinst_headers = src/trie.h
endif

libtrie_la_SOURCES = src/trie.c

if ENABLE_TOOLS
bin_PROGRAMS = list-compile list-query

list_compile_SOURCES = src/compile.c
list_compile_LDADD = libtrie.la

list_query_SOURCES = src/query.c
list_query_LDADD = libtrie.la
endif
