AUTOMAKE_OPTIONS = foreign subdir-objects
include $(top_srcdir)/build/common.am

lib_LTLIBRARIES=libPacketRing.la 

libPacketRing_la_LDFLAGS   = -shared -fPIC -version-info 1:2:0 -L$(top_builddir)/sln/LiangZhu
libPacketRing_la_LIBADD    = -lLiangZhu

libPacketRing_la_SOURCES = src/packet_ring.cpp			\
						   src/packet_ring_class.cpp 
	   
include_HEADERS =

