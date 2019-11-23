all:
	make -f Makefile.pt
	./gensrclist.sh
	make -f Makefile.sc

clean:
	make clean -f Makefile.sc
	make clean -f Makefile.pt

install:
	make install -f Makefile.sc
