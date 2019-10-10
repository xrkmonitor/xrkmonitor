
COMM_INC = -I/usr/include/mtagent_api
COMM_LIB = -lmtreport_api

# 注意插件目录名和插件文件名一样，本插件为: linux_base 
TARGETS_DIR = ../../xrkmonitor_plus/linux_base
TARGETS = linux_base.so 

ifeq ($(findstring debug, $(makecmd)), debug)
DEBUG := -g -DMT_DEBUG
else
RELEASE :=  
endif

CC = g++ 

WARNINGS := -Wall -Wsign-compare -Wno-strict-aliasing -Wno-write-strings -Wno-array-bounds -Wno-unused-but-set-variable -fpermissive
CXXFLAGS := -fPIC $(WARNINGS) $(DEBUG) $(RELEASE) $(COMM_INC) 

CONFIG_FILE = linux_base.conf
all:$(TARGETS)

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)

.cpp.o:
	$(CC) $(CXXFLAGS) $(COMM_INC) -c $(filter %.cpp, $^)

$(TARGETS): ${OBJ} 
	$(CC) $(CXXFLAGS) -fPIC -shared -o $@ $^  $(COMM_LIB)

install:
	strip $(TARGETS)
	mkdir -p ../../xrkmonitor_plus/linux_base
	cp $(TARGETS) $(TARGETS_DIR)
	cp $(CONFIG_FILE) $(TARGETS_DIR)

clean: 
	rm -f *.o
	rm -f ./$(TARGETS)

