COMM_INC = -I/usr/include/mtagent_api_open -I/usr/include/Sockets -I/usr/include/mysql -I/usr/include/my_proto
COMM_LIB =  /usr/lib64/libmtreport_api_open.a 

CC = g++ 
WARNINGS := -g -Wall -Wcast-qual -Wsign-compare
CXXFLAGS := $(WARNINGS) $(DEBUG) $(RELEASE) $(COMM_INC)

TARGETS = test_main 

all:$(TARGETS)

SRC := main.cpp

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

