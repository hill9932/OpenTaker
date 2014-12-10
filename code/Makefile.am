AUTOMAKE_OPTIONS = foreign subdir-objects

TARGET_ROOT	= $(top_srcdir)/bin/

if DEBUG
TARGET_PATH = $(TARGET_ROOT)debug/

else
TARGET_PATH = $(TARGET_ROOT)release/

endif

install-exec-local:
	cp $(top_srcdir)/unitTest/UT_LiangZhu/UT_LiangZhu $(TARGET_PATH) && \
	cp $(top_srcdir)/unitTest/UT_ChangQian/UT_ChangQian $(TARGET_PATH)

SUBDIRS = sln/LiangZhu sln/ChangQian unitTest/UT_LiangZhu unitTest/UT_ChangQian
