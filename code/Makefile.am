AUTOMAKE_OPTIONS = foreign subdir-objects

TARGET_PATH	= $(top_srcdir)/bin/

if DEBUG
TARGET_PATH = $(TARGET_PATH)debug/

else
TARGET_PATH =$(TARGET_PATH)release/

endif

install-exec-local:
	cp $(top_srcdir)/unitTest/UT_LiangZhu/UT_LiangZhu $(TARGET_PATH)

SUBDIRS = sln/LiangZhu unitTest/UT_LiangZhu
