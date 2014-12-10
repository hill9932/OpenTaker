AUTOMAKE_OPTIONS = foreign subdir-objects
include $(top_srcdir)/build/common.am

lib_LTLIBRARIES=libPacketRing.la 

libPacketRing_la_LDFLAGS   = -L$(top_builddir)/sln/LiangZhu
libPacketRing_la_LDADD     = -lLiangZhu

libPacketRing_la_SOURCES = src/packet_ring.cpp
	   

