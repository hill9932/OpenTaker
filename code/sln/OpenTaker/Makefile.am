AUTOMAKE_OPTIONS = foreign subdir-objects
include $(top_srcdir)/build/common.am

noinst_PROGRAMS = OpenTaker

OpenTaker_LDFLAGS   = -L$(top_builddir)/sln/ChangQian -L$(top_builddir)/sln/LiangZhu -L$(top_builddir)/sln/PacketRing -L$(top_builddir)/sln/PacketRay
OpenTaker_LDADD     = -lPacketRay -lPacketRing -lChangQian -lLiangZhu -lpopt

OpenTaker_SOURCES = src/global.cpp 				\
					src/main.cpp 				\
					src/capture.cpp				\
					src/block_capture.cpp		\
					src/pcap_capture.cpp		\
					src/virtual_capture.cpp		\
					src/capture_store.cpp		\
					src/pcap_file.cpp			\
					src/pcap_file2.cpp			\
					src/pcap_file_fake.cpp		\
					src/storage.cpp 
	


 
