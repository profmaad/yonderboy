AC_INIT([cli-browser],[0.0],[himself@prof-maad.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

m4_include([libev/libev.m4])

AC_PROG_CXX

PKG_CHECK_MODULES([DEPS], [yaml-cpp >= 0.2.5])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile server/src/Makefile])
AC_OUTPUT
