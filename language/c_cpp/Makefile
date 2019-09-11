all:
	./gensrclist.sh
	make -f Makefile.sc

debug:
	./gensrclist.sh
	make -f Makefile.sc makecmd=debug

clean:
	make clean -f Makefile.sc
	rm -f Makefile.srcs

install:
	make install -f Makefile.sc
