ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libtrie.la

include_HEADERS = trie.h

libtrie_la_SOURCES = trie.c

if ENABLE_TOOLS
bin_PROGRAMS = list-compile list-query

list_compile_SOURCES = compile.c
list_compile_LDADD = libtrie.la

list_query_SOURCES = query.c
list_query_LDADD = libtrie.la
endif
