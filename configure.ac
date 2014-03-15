#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libtrie], [0.1], [lubomir.sedlar@gmail.com])
AC_CONFIG_SRCDIR([trie.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 foreign subdir-objects -Wall])
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

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
