#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([HILUO-OpenTaker], [0.9.1], [hill9932@gmail.com])
AC_CONFIG_SRCDIR([unitTest/UT_ChangQian/src/version.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE
LT_INIT


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([lua5.2], [lua_copy], [], [AC_MSG_ERROR([No liblua5.2 installed.])])
AC_CHECK_LIB([aio], [io_cancel])
AC_CHECK_LIB([pthread], [main])

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h malloc.h netdb.h netinet/in.h stdlib.h string.h sys/ioctl.h sys/socket.h sys/time.h sys/timeb.h sys/vfs.h unistd.h wchar.h wctype.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MKTIME
AC_CHECK_FUNCS([bzero dup2 ftruncate getcwd gethostbyaddr gethostname gettimeofday inet_ntoa memset mkdir select setlocale socket strerror strstr])

AC_CONFIG_FILES([Makefile
		 sln/LiangZhu/Makefile
		 unitTest/UT_LiangZhu/Makefile])

AC_OUTPUT
