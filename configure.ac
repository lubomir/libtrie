#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libtrie], [0.1], [lubomir.sedlar@gmail.com])
AC_CONFIG_SRCDIR([src/trie.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 foreign subdir-objects -Wall dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC_C99
AM_PROG_CC_C_O
AM_PROG_AR
LT_INIT

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memset munmap strchr])

AC_ARG_ENABLE([tools],
    AS_HELP_STRING([--disable-tools], [Disable CLI tools]))
AM_CONDITIONAL([ENABLE_TOOLS], [test "x$enable_tools" != "xno"])

AC_ARG_ENABLE([shared-lib],
    AS_HELP_STRING([--disable-shared-lib], [Do not install shared library]))
AM_CONDITIONAL([ENABLE_SHARED_LIB], [test "x$enable_shared_lib" != "xno"])

AS_IF([test "x$enable_shared_lib" != "xno"], [
       AM_PATH_PYTHON([2.7])
       ])

AC_CONFIG_FILES([Makefile python/libtrie/config.py])
AC_OUTPUT

echo ""
echo "libtrie $VERSION"
echo ""
echo " Prefix..........................: ${prefix}"
echo " CFLAGS..........................: ${CFLAGS}"
echo " Building tools..................: ${enable_tools:-yes}"
echo " Installing shared library.......: ${enable_shared_lib:-yes}"
