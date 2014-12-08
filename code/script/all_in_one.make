CC 			= gcc
XX 			= g++
CFLAGS 		= -Wno-unused-value -Wno-pointer-arith -fPIC
LFLAGS		=
AR 			= ar
ARFLAGS 	= cr
RM 			= rm -rf

DIR_NAME	:=$(shell pwd | sed 's/^\(.*\)[/]//')
TARGET 		?= lib$(DIR_NAME).a
CONFIG_NAME	?= Debug
BITS		?= 32
TARGET_DIR  =

#
# debug and release version will use different compile flags
#
ifeq ($(CONFIG_NAME), Debug)
	CFLAGS += -ggdb -O -DDEBUG
else
	CFLAGS += -O3  -DNDEBUG
endif

ifeq ($(BITS), 64)
    CFLAGS += -m64
    LFLAGS += -m64
	TARGET_DIR := $(CONFIG_NAME)$(BITS)
else
	CFLAGS += -m32
	LFLAGS += -m32
	TARGET_DIR := $(CONFIG_NAME)
endif

OUTPUT_PATH	?= $(SLN_PATH)/out/objs/$(DIR_NAME)/$(TARGET_DIR)
BIN_PATH	?= $(SLN_PATH)/out/libs/$(TARGET_DIR)

#
#specify compile include and library path
#
USER_INCL_DIRS += -I$(SLN_PATH)/include		\
				  -I$(SLN_PATH)/src			\
				  -I$(SLN_PATH)/src/include	\
				  -I$(SLN_PATH)/../3rdParty	\
				  -I$(SLN_PATH)/../3rdParty/boost_1_55_0	\
				  -I$(SLN_PATH)/../3rdParty/log4cplus-1.1.2/include	\
				  -I$(SLN_PATH)/../3rdParty/lua-5.2.3	\
				  -I$(SLN_PATH)/../3rdParty/thrift-0.9.1/lib/cpp/src	\
				  -I/usr/include/postgresql	\
				  
USER_DEFFLAGS 	= -DLINUX -DTRACE -DLOG4CPP_HAVE_SSTREAM
COMPILE_FLAG 	= $(CFLAGS) $(USER_INCL_DIRS) $(USER_DEFFLAGS)

#
# create the output directory
#
ifeq "$(wildcard $(OUTPUT_PATH))" ""
    $(shell mkdir -p $(OUTPUT_PATH))	
endif

ifeq "$(wildcard $(BIN_PATH))" ""
    $(shell mkdir -p $(BIN_PATH))	
endif


$(OUTPUT_PATH)/%.o:%.c
	$(CC) $(COMPILE_FLAG) -c $< -o $@

$(OUTPUT_PATH)/%.o:%.cpp
	$(XX) $(COMPILE_FLAG) -c $< -o $@

$(OUTPUT_PATH)/%.d:%.cpp
	set -e; rm -f $@; \
	$(XX) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@;\
	rm -f $@.$$$$

	
SOURCES 	= $(wildcard *.c *.cpp)
OBJS 		= $(patsubst %.c, $(OUTPUT_PATH)/%.o, $(patsubst %.cpp, $(OUTPUT_PATH)/%.o, $(SOURCES)))
BUILD_CMDS	= $(AR) $(ARFLAGS) $(BIN_PATH)/$@ $(OBJS)

#include $(patsubst %.c, %.d, $(patsubst %.cpp, $(OUTPUT_PATH)/%.d, $(SOURCES))) 

#
# if target is exe rather than library
#
define build-exe
	@printf "#\n# Building $(TARGET)\n#\n"
	$(XX) $(OBJS) $(USER_LIB_DIRS)	$(USER_LINK_LIBS) -o $(BIN_PATH)/$(TARGET)
	chmod a+x $(BIN_PATH)/$(TARGET)
endef

ifeq "$(TARGET_TYPE)" "EXE"
	BUILD_CMDS = $(build-exe)
endif


all: $(TARGET)
$(TARGET):$(OBJS)
	$(BUILD_CMDS)
	
clean:
	$(RM) $(OUTPUT_PATH) $(BIN_PATH)/$(TARGET)       

.PHONY: all clean