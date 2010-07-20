AC_INIT([cli-browser],[0.0],[himself@prof-maad.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

m4_include([libev/libev.m4])
m4_include([m4/ax_prog_doxygen.m4])

AC_PROG_CXXCPP(clang)
AC_PROG_CXX(clang)

# Checks for libraries.
AC_CHECK_LIB([pthread], [pthread_cond_init])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h stddef.h stdint.h stdlib.h string.h sys/socket.h sys/statfs.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_REALLOC
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([clock_gettime dup2 gettimeofday memset mkdir select socket strrchr uname])

PKG_CHECK_MODULES([DEPS], [yaml-cpp >= 0.2.5])

DX_INIT_DOXYGEN(cli-browser, Doxyfile, doc)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile
		server/src/Makefile
		renderers/webkit-renderer/src/Makefile])
AC_OUTPUT
