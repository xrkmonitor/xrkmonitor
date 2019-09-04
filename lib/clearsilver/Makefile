include rock_install

all:
	make -C cgi
	make -C cs
	make -C util

clean:
	make clean -C cgi 
	make clean -C cs 
	make clean -C util 

install:
	mkdir -p $(INSTALL_INC_DIR)
	cp ClearSilver.h $(INSTALL_INC_DIR)
	cp cs_config.h $(INSTALL_INC_DIR)
	make install -C cgi install
	make install -C cs install
	make install -C util install

