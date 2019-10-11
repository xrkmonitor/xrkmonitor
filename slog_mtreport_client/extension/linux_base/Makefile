include ../../../make_env
COMM_INC = -I${MTLIB_INCLUDE_PATH}/mtreport_api
COMM_LIB = -lmtreport_api 

# 注意插件目录名和插件文件名一样，本插件为: linux_base 
TARGETS_DIR = ../../xrkmonitor_plus/linux_base
TARGETS = linux_base.so 

CC = g++ 
CXXFLAGS_LIB += $(COMM_INC)

CONFIG_FILE = linux_base.conf
all:$(TARGETS)

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)

.cpp.o:
	$(CC) $(CXXFLAGS_LIB) -c $(filter %.cpp, $^)

$(TARGETS): ${OBJ} 
	$(CC) $(CXXFLAGS_LIB) -shared -o $@ $^  $(COMM_LIB)

install:
	strip $(TARGETS)
	mkdir -p ../../xrkmonitor_plus/linux_base
	cp $(TARGETS) $(TARGETS_DIR)
	cp $(CONFIG_FILE) $(TARGETS_DIR)

clean: 
	rm -f *.o
	rm -f ./$(TARGETS)

