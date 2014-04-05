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
	tests/integration/basic-insert.sh \
	tests/integration/basic-insert-no-compress.sh \
	tests/integration/custom-delimiter.sh \
	tests/integration/custom-delimiter-no-compress.sh \
	tests/integration/no-data-with-keys.sh \
	tests/integration/random-keys.sh \
	tests/integration/random-keys-not-found.sh \
	tests/integration/very-long-data.sh \
	tests/integration/very-long-data-no-compress.sh \
	tests/integration/very-many-keys.sh \
	tests/integration/very-many-keys-no-compress.sh \
	tests/integration/querying-bad-file.sh \
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
