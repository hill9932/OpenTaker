#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([HILUO-OpenTaker], [0.9.1], [hill9932@gmail.com])
AC_CONFIG_SRCDIR([unitTest/UT_ChangQian/src/version.h])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE()
LT_INIT

AC_ARG_ENABLE([debug],
[ --enable-debug    Turn on debugging],
[case "${enableval}" in
  yes) debug=true ;;
  no)  debug=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],[debug=false])
AM_CONDITIONAL([DEBUG], [test x$debug = xtrue])


# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_RANLIB


# Check thrift library
AC_LANG(C++)
LDFLAGS="$LDFLAGS -L/usr/local/lib"
LIBS="$LIBS -lthrift"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <thrift/protocol/TBinaryProtocol.h>],
        [::apache::thrift::protocol::TBinaryProtocolFactory dummy])],
    [TEST_LIBS="$TEST_LIBS -lthrift"] [HAVE_THRIFT=yes], 
    [AC_MSG_ERROR([No thrift installed.])])

# Check log4cplus library
AC_LANG(C++)
LIBS="$LIBS -llog4cplus "
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <log4cplus/logger.h>],
        [log4cplus::Logger logs = log4cplus::Logger::getInstance("dummy")])],
    [TEST_LIBS="$TEST_LIBS -llog4cplus"] [HAVE_LOG4CPLUS=yes], 
    [AC_MSG_ERROR([No log4cplus installed.])])

# Check boost_thread library
AC_LANG(C++)
SAVED_LIBS=$LIBS
LIBS="$LIBS -lboost_thread -lboost_system"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <boost/thread/thread.hpp>],
        [boost::thread::attributes dummy])],
    [TEST_LIBS="$TEST_LIBS -lboost_thread"] [HAVE_BOOST_THREAD=yes], 
    [AC_MSG_ERROR([No boost_thread library installed.])])
LIBS="$SAVED_LIBS -lboost_thread"

# Check boost_date_time library
AC_LANG(C++)
LIBS="$LIBS -lboost_date_time"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <boost/date_time.hpp>],
        [boost::posix_time::ptime now = boost::posix_time::second_clock::local_time()])],
    [TEST_LIBS="$TEST_LIBS -lboost_date_time"] [HAVE_BOOST_DATE_TIME=yes], 
    [AC_MSG_ERROR([No boost_date_time library installed.])])

# Check boost_system library
AC_LANG(C++)
LIBS="$LIBS -lboost_system"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([#include <boost/system/error_code.hpp>],
        [boost::system::errc::errc_t dummy])],
    [TEST_LIBS="$TEST_LIBS -lboost_system"] [HAVE_BOOST_DATE_TIME=yes], 
    [AC_MSG_ERROR([No boost_system library installed.])])
	
	
# Checks for libraries.
AC_CHECK_LIB([lua5.2], [lua_copy], [], [AC_MSG_ERROR([No liblua5.2 installed.])])
AC_CHECK_LIB([aio], [io_cancel])
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([pcap], [pcap_datalink], [],[AC_MSG_ERROR([No libpcap installed.])])
AC_CHECK_LIB([pq], [main])
AC_CHECK_LIB([pthread], [main])
AC_CHECK_LIB([gtest], [main], [], [AC_MSG_ERROR([No gtest installed.])])
AC_CHECK_LIB([popt], [poptGetContext], [],[AC_MSG_ERROR([No libpopt installed.])])

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
		 sln/ChangQian/Makefile
		 sln/PacketRing/Makefile
		 sln/OpenTaker/Makefile
		 unitTest/UT_LiangZhu/Makefile
		 unitTest/UT_ChangQian/Makefile])

AC_OUTPUT
