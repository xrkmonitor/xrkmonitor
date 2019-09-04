include ../make_env
COMM_INC = -I../lib/mtreport_api
COMM_LIB = ../lib/mtreport_api/libmtreport_api.a -ldl

TARGETS_DIR = linux_monitor_agent

CC = g++ 
CXXFLAGS += $(COMM_INC) 

TARGETS = slog_mtreport_client
all:$(TARGETS)

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)
DEP := $(SRC:.cpp=.d)

$(TARGETS): ${OBJ} 
	$(CC) -g -Wall -o $@ $^ $(COMM_LIB) -lrt

install:
	strip $(TARGETS)
	cp *.sh $(TARGETS_DIR)
	cp $(TARGETS) $(TARGETS_DIR)

uninstall:
	rm -f $(TARGETS_DIR)/$(TARGETS)

clean: 
	rm -f *.o
	rm -f *.d
	rm -f ./$(TARGETS)

