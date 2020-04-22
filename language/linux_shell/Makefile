COMM_INC = -I../c_cpp
COMM_LIB =  ../c_cpp/libmtreport_api.a  -lm

CC = gcc
WARNINGS := -g -Wall -Wcast-qual -Wsign-compare
CXXFLAGS := $(WARNINGS) $(DEBUG) $(RELEASE) $(COMM_INC)

TARGETS = xrkmonitor_report 

all:$(TARGETS)

SRC := xrkmonitor_report.c

OBJ := $(SRC:.cpp=.o)
DEP := $(SRC:.cpp=.d)

$(TARGETS): ${OBJ} 
	$(CC) -g -Wall -o $@ $^ $(COMM_LIB) $(COMM_INC)  -lrt

install:
	cp ./$(TARGETS) $(TARGETS_DIR)
	cp ./$(TARGETS).conf $(TARGETS_DIR)

uninstall:
	rm -f $(TARGETS_DIR)$(TARGETS)

clean: 
	rm -f *.o
	rm -f *.d
	rm -f ./$(TARGETS)

