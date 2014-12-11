AUTOMAKE_OPTIONS = foreign subdir-objects

TARGET_ROOT	 = $(top_srcdir)/bin/
LIBRARY_ROOT = $(top_srcdir)/lib/

if DEBUG
TARGET_PATH  = $(TARGET_ROOT)debug/
LIBRARY_PATH = $(LIBRARY_ROOT)debug/

else
TARGET_PATH = $(TARGET_ROOT)release/
LIBRARY_PATH = $(LIBRARY_ROOT)release/

endif

install-exec-local:
	cp $(top_srcdir)/unitTest/UT_LiangZhu/UT_LiangZhu $(TARGET_PATH) 	&& \
	cp $(top_srcdir)/unitTest/UT_ChangQian/UT_ChangQian $(TARGET_PATH) 	&& \
	cp $(top_srcdir)/sln/OpenTaker/OpenTaker $(TARGET_PATH)

SUBDIRS = sln/LiangZhu sln/ChangQian sln/PacketRing sln/OpenTaker unitTest/UT_LiangZhu unitTest/UT_ChangQian
