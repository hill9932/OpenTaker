AUTOMAKE_OPTIONS = foreign subdir-objects
noinst_LIBRARIES = libLiangZhu.a

AM_CPPFLAGS = -I$(top_srcdir)/inc -I$(top_srcdir)/inc/common -I$(top_srcdir)/3rdParty/inc
AM_CPPFLAGS += -DLINUX -fPIC

libLiangZhu_a_SOURCES = src/main.cpp \
			src/common/aio_.cpp \
			src/common/common.cpp \
			src/common/file.cpp \
			src/common/file_pool.cpp \
			src/common/file_system.cpp \
			src/common/log.cpp \
			src/common/lua_.cpp \
			src/common/math_.cpp \
			src/common/mutex.cpp \
			src/common/numa_.cpp \
			src/common/platform_linux.cpp \
			src/common/platform_win32.cpp \
			src/common/string_.cpp \
			src/common/system_.cpp \
			src/common/thread.cpp \
			src/common/thread_pool.cpp \
			src/common/time_.cpp \
			src/common/versoin.h 

 
