include ../make_env
COMM_INC = -I../language/c_cpp/
COMM_LIB = ../language/c_cpp/libmtreport_api.a -ldl

CC = g++ 
CXXFLAGS += $(COMM_INC) 

TARGETS = slog_mtreport_client
all:$(TARGETS)

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)
DEP := $(SRC:.cpp=.d)

$(TARGETS): ${OBJ} 
	$(CC) -g -Wall -o $@ $^ $(COMM_LIB) -lrt
	strip $(TARGETS)

clean: 
	rm -f *.o
	rm -f *.d
	rm -f ./$(TARGETS)

