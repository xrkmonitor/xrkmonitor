include ../make_env
COMM_INC = -I${MTLIB_INCLUDE_PATH}/mtreport_api
COMM_LIB = -lmtreport_api -ldl

CC = g++ 
CXXFLAGS += $(COMM_INC) 

TARGETS = slog_mtreport_client
all:$(TARGETS)

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)
DEP := $(SRC:.cpp=.d)

$(TARGETS): ${OBJ} 
	$(CC) -o $@ $^ $(COMM_LIB) -lrt

clean: 
	rm -f *.o
	rm -f *.d
	rm -f ./$(TARGETS)

