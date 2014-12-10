AUTOMAKE_OPTIONS = foreign subdir-objects
noinst_PROGRAMS = UT_ChangQian

include $(top_srcdir)/common.am

UT_ChangQian_LDFLAGS   = -L$(top_builddir)/sln/ChangQian -L$(top_builddir)/sln/LiangZhu
UT_ChangQian_LDADD     = -lChangQian -lLiangZhu

UT_ChangQian_SOURCES = src/global.cpp 	\
		      src/main.cpp 				\
		      src/version.h 			\
		      src/test/inetaddr_ut.cpp 	\
		      src/test/socket_base_ut.cpp 


 
