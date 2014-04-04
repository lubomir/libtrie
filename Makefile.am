ACLOCAL_AMFLAGS = -I m4

NULL=
AM_CFLAGS=

if ENABLE_SHARED_LIB
lib_LTLIBRARIES = libtrie.la
include_HEADERS = src/trie.h

pylibtrie_PYTHON = python/libtrie/__init__.py \
		   python/libtrie/config.py \
		   $(NULL)

pylibtriedir = $(pythondir)/libtrie
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

TESTS = \
	tests/integration/test01.sh \
	tests/integration/test02.sh \
	tests/integration/test03.sh \
	tests/integration/test04.sh \
	tests/integration/test05.sh \
	$(NULL)

if ENABLE_COVERAGE
AM_CFLAGS+=--coverage --no-inline

clean-gcda:
	rm -rf coverage coverage.info
	find . -name '*.gcda' -exec rm {} \+
	lcov --directory $(top_builddir)/src --zerocounters

coverage :
	@make clean-gcda
	@make USE_VALGRIND="no" check
	lcov -c -d $(top_builddir)/src -o coverage.info
	genhtml -o coverage coverage.info
clean-local:
	@make clean-gcda
endif

EXTRA_DIST = $(TESTS) README.markdown
