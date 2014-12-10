AUTOMAKE_OPTIONS = foreign subdir-objects
noinst_PROGRAMS = UT_LiangZhu

include $(top_srcdir)/common.am

UT_LiangZhu_LDFLAGS   = -L$(top_builddir)/sln/LiangZhu
UT_LiangZhu_LDADD     = -lLiangZhu 

UT_LiangZhu_SOURCES = src/global.cpp 	\
		      src/main.cpp 				\
		      src/version.h 			\
		      src/test/file_system_ut.cpp 	\
		      src/test/file_ut.cpp 		\
			  src/test/lua_ut.cpp 		\
		      src/test/math_ut.cpp 		\
		      src/test/string_ut.cpp 	\
		      src/test/system_ut.cpp 


 
