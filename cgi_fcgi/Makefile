include ../make_env
COMM_INC = -I$(MTLIB_INCLUDE_PATH)/mtagent_api_open -I$(MYSQL_INCLUDE)/mysql -I$(MTLIB_INCLUDE_PATH)/Sockets -I/usr/include/fastcgi/ -I$(MTLIB_INCLUDE_PATH)/cgi -I$(MTLIB_INCLUDE_PATH)/my_proto 

LIBPATH=$(MTLIB_LIB_PATH)
COMM_LIB += -lrt $(LIBPATH)/libfcgi.a $(LIBPATH)/libneo_cgi.a $(LIBPATH)/libneo_cs.a $(LIBPATH)/libneo_utl.a -lz  $(LIBPATH)/libcgicomm.a $(LIBPATH)/libmysqlwrapped.a -L$(MYSQL_LIB) -lmysqlclient $(LIBPATH)/libmtreport_api_open.a $(LIBPATH)/libSockets.a  $(LIBPATH)/libmyproto.a /usr/lib64/libprotobuf.a  -lpthread 

CC = g++
OBJECT=$(patsubst %.cpp,%,$(wildcard *.cpp))
all: $(OBJECT) 

SRC := $(wildcard *.cpp)
OBJ := $(SRC:.cpp=.o)
#DEP := $(SRC:.cpp=.d)

%:%.cpp
	$(CC) $(CXXFLAGS) -o $@  $< $(OBJ_COMM) $(COMM_LIB) $(COMM_INC) 

#MYCGI_PATH=/srv/www/cgi-bin
MYCGI_PATH=/var/www/cgi-bin
APACHE_EXE=/usr/sbin/apache2ctl

libcomm:
	make -C comm
	make -C comm install

install:
	$(APACHE_EXE) stop 
	sleep 1
	cp $(OBJECT) $(MYCGI_PATH) 
#	cp *.conf $(MYCGI_PATH)
	chmod 777 $(MYCGI_PATH)/$(OBJECT)
	$(APACHE_EXE) start

clean: 
	rm -f *.o
	rm -f *.d
	rm -f ./$(OBJECT)

