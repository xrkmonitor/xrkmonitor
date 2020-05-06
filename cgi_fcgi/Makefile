include ../make_env
COMM_INC = -I$(MTLIB_INCLUDE_PATH)/mtagent_api_open -I$(MYSQL_INCLUDE)/mysql -I$(MTLIB_INCLUDE_PATH)/Sockets -I/usr/include/fastcgi/ -I$(MTLIB_INCLUDE_PATH)/cgi -I$(MTLIB_INCLUDE_PATH)/my_proto -I$(MTLIB_INCLUDE_PATH)/mtreport_api -I$(MTLIB_INCLUDE_PATH)/mysqlwrapped -I$(MTLIB_INCLUDE_PATH)/cgicomm

ifneq ($(USE_DLL_COMM_LIB), yes)
COMM_LIB = $(MTLIB_LIB_PATH)/libfcgi.a $(MTLIB_LIB_PATH)/libneo_cgi.a $(MTLIB_LIB_PATH)/libneo_cs.a $(MTLIB_LIB_PATH)/libneo_utl.a -lz $(MTLIB_LIB_PATH)/libcgicomm.a $(MTLIB_LIB_PATH)/libmysqlwrapped.a $(MTLIB_LIB_PATH)/libmtreport_api_open.a $(MTLIB_LIB_PATH)/libSockets.a -L$(MYSQL_LIB) -lmysqlclient $(MTLIB_LIB_PATH)/libmyproto.a $(MTLIB_LIB_PATH)/libprotobuf.a $(MTLIB_LIB_PATH)/libmtreport_api.a -lz -lpthread  -lrt
else
COMM_LIB = -L$(MTLIB_LIB_PATH) -lfcgi -lneo_cgi -lneo_cs -lneo_utl -lcgicomm -lmysqlwrapped -lmtreport_api_open -lSockets -L$(MYSQL_LIB) -lmysqlclient -L$(MTLIB_LIB_PATH) -lmyproto -lprotobuf -lmtreport_api -lz -lpthread -lrt
endif

CC = g++
OBJECT=$(patsubst %.cpp,%,$(wildcard *.cpp))
all: $(OBJECT) 

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)
#DEP := $(SRC:.cpp=.d)

%:%.cpp
	$(CC) $(CXXFLAGS) -o $@  $< $(OBJ_COMM) $(COMM_LIB) $(COMM_INC) 
ifeq ($(USE_DLL_COMM_LIB), yes) 
	strip $@ 
endif

MYCGI_PATH=/srv/www/cgi-bin

libcomm:
	make -C comm
	make -C comm install

install:
	apachectl stop
	sleep 1
	cp $(OBJECT) $(MYCGI_PATH) 
#	cp *.conf $(MYCGI_PATH)
	chmod 777 $(MYCGI_PATH)/$(OBJECT)
	apachectl start

clean: 
	rm -f *.o
	rm -f *.d
	rm -f ./$(OBJECT)

