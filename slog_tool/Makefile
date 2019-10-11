include ../make_env
COMM_INC = -I$(MTLIB_INCLUDE_PATH)/my_proto -I$(MTLIB_INCLUDE_PATH)/mtagent_api_open -I$(MYSQL_INCLUDE)/mysql -I$(MTLIB_INCLUDE_PATH)/Sockets -I$(MTLIB_INCLUDE_PATH)/mtreport_api -I$(MTLIB_INCLUDE_PATH)/mysqlwrapped

COMM_LIB = $(MTLIB_LIB_PATH)/libmysqlwrapped.a $(MTLIB_LIB_PATH)/libmtreport_api_open.a $(MTLIB_LIB_PATH)/libSockets.a -L$(MYSQL_LIB) -lmysqlclient $(MTLIB_LIB_PATH)/libmyproto.a $(MTLIB_LIB_PATH)/libprotobuf.a  $(MTLIB_LIB_PATH)/libmtreport_api.a -lz -lpthread 

CC = g++ 
TARGETS = slog_tool
all:$(TARGETS)

CXXFLAGS += $(COMM_INC)

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)
DEP := $(SRC:.cpp=.d)

$(TARGETS): ${OBJ}
	$(CC) -g -Wall -o $@ $^ $(COMM_LIB) 

clean: 
	rm -f *.o
	rm -f *.d
	rm -f ./$(TARGETS)

