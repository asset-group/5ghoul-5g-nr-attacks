AC_PREREQ([2.65])
AC_INIT([libmpsse],[1.3.2],[https://github.com/l29ah/libmpsse/issues])

AC_ARG_ENABLE([python],AS_HELP_STRING([--disable-python], [Do not build Python wrappers]))

AC_CONFIG_SRCDIR([mpsse.c])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AM_PROG_AR
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_LANG(C)

AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_FUNC_MALLOC

# Requirements/checks detected by autoscan
AC_CHECK_FUNCS([floor memset strchr strdup strstr])
AC_CHECK_HEADERS([float.h limits.h])
AC_CHECK_HEADER_STDBOOL

LT_INIT
AC_ENABLE_SHARED
AC_ENABLE_STATIC

AX_APPEND_FLAG([-L$ac_abs_confdir])
AX_APPEND_FLAG([-I$ac_abs_confdir])
AX_APPEND_FLAG([-D_GNU_SOURCE])

LDFLAGS="-l:libftdi1-static.a -ludev -lpthread $LDFLAGS"

AC_PATH_PROG([SWIG], [swig], [no])
AM_PATH_PYTHON
AC_ARG_VAR([PYTHON_INCLUDE], [Include flags for Python, bypassing python-config])
AC_ARG_VAR([PYTHON_LIBS], [Libs flags for Python, bypassing python-config])
AC_ARG_VAR([PYTHON_CONFIG], [Path to python config])

AM_CONDITIONAL([BUILD_PYTHON_WRAPPERS], [test "x$enable_python" != "xno"])
AS_IF([test "x$enable_python" != "xno"], [
    AS_IF([test "$SWIG" = no], [AC_MSG_ERROR([swig not found])])

    AS_IF([test -z "$PYTHON_CONFIG"], [
           AC_PATH_PROGS([PYTHON_CONFIG], [python${PYTHON_VERSION}-config python-config], [no])
           AS_IF([test "$PYTHON_CONFIG" = no], AC_MSG_ERROR([can't find python-config]))
           ])

    AS_IF([test -z "$PYTHON_INCLUDE"], [
           AC_MSG_CHECKING([Python include flags])
           PYTHON_INCLUDE=`$PYTHON_CONFIG --includes`
           AC_MSG_RESULT([$PYTHON_INCLUDE])
           ])

    AS_IF([test -z "$PYTHON_LIBS"], [
           AC_MSG_CHECKING([Python libs flags])
           PYTHON_LIBS=`$PYTHON_CONFIG --libs`
           AC_MSG_RESULT([$PYTHON_LIBS])
           ])
])


AC_SUBST(PYTHON_INCLUDE)
AC_SUBST(PYTHON_LIBS)
AC_SUBST(LIBFTDI1)
AC_CONFIG_FILES([Makefile examples/Makefile])
AC_CONFIG_HEADERS([mpsse_config.h])
AC_OUTPUT
