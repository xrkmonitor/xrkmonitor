include ../make_env
COMM_INC = -I$(MTLIB_INCLUDE_PATH)/my_proto -I$(MTLIB_INCLUDE_PATH)/mtagent_api_open -I$(MYSQL_INCLUDE)/mysql -I$(MTLIB_INCLUDE_PATH)/Sockets  -I$(MTLIB_INCLUDE_PATH)/mtreport_api -I$(MTLIB_INCLUDE_PATH)/mysqlwrapped

ifneq ($(USE_DLL_COMM_LIB), yes)
COMM_LIB = $(MTLIB_LIB_PATH)/libmysqlwrapped.a $(MTLIB_LIB_PATH)/libmtreport_api_open.a $(MTLIB_LIB_PATH)/libSockets.a -L$(MYSQL_LIB) -lmysqlclient $(MTLIB_LIB_PATH)/libmyproto.a $(MTLIB_LIB_PATH)/libprotobuf.a $(MTLIB_LIB_PATH)/libmtreport_api.a -lz -lpthread 
else
COMM_LIB = -L$(MTLIB_LIB_PATH) -lmysqlwrapped -lmtreport_api_open -lSockets -L$(MYSQL_LIB) -lmysqlclient -L$(MTLIB_LIB_PATH) -lmyproto -lprotobuf -lmtreport_api -lz -lpthread 
endif

CC = g++ 
TARGETS = slog_client
all:$(TARGETS)

CXXFLAGS += $(COMM_INC)

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)
DEP := $(SRC:.cpp=.d)

$(TARGETS): ${OBJ}
	$(CC) -g -Wall -o $@ $^ $(COMM_LIB) 
ifeq ($(USE_DLL_COMM_LIB), yes) 
	strip $(TARGETS)
endif

clean: 
	rm -f *.o
	rm -f *.d
	rm -f ./$(TARGETS)

