AUTOMAKE_OPTIONS = foreign subdir-objects
include $(top_srcdir)/build/common.am

lib_LTLIBRARIES=libPacketRay.la 

libPacketRing_la_LIBADD    = -lLiangZhu -ltbb

#link against libtool library 
libPacketRing_la_LDADD     = 	
libPacketRing_la_LDFLAGS   = -shared -fPIC -version-info 1:2:0 -L$(top_builddir)/sln/LiangZhu


libPacketRing_la_SOURCES = 	src/packet_ray.cpp			\
							src/pipeline_manager.cpp	\
							src/pipeline_task.cpp
	   
include_HEADERS =

