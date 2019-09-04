############################################################
#
# rules.mk  is  A U T O   G E N E R A T E D 
#
# you must edit:     rules.mk.in
#
############################################################

##
## Global Makefile Rules
##
## Before including this file, you must set NEOTONIC_ROOT
##

OSNAME := $(shell uname -rs | cut -f 1-2 -d "." | cut -f 1 -d "-")
OSTYPE := $(shell uname -s)

LIB_DIR    = $(NEOTONIC_ROOT)/libs/

## Installation Directories
srcdir = .
top_srcdir = .

prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include

DESTDIR =

cs_includedir = ${includedir}/ClearSilver

## NOTE: The wdb code in util will tickle a bug in SleepyCat 2.4.5,
## which ships with various versions of Linux as part of glibc.  If you
## are going to use that code, you should compile against SleepyCat
## 2.7.7 instead
USE_DB2 = 1

USE_ZLIB = 1



PICFLG = -fPIC
ifeq ($(OSTYPE),OSF1)
PICFLG =
endif
ifeq ($(OSNAME),MINGW32_NT)
PICFLG =
endif


## -------------- base (Linux/Neotonic) options

PYTHON_INC = 
PYTHON_LIB = 
PYTHON_SITE = 
JAVA_PATH  = 
JAVA_INCLUDE_PATH = 
CSHARP_PATH = 

## Programs

INSTALL    = /usr/bin/install -c
CC	   = gcc
MKDIR      = mkdir -p
RM         = rm -f
CPP        = g++
JAVAC      = $(JAVA_PATH)/bin/javac
JAVAH      = $(JAVA_PATH)/bin/javah
JAR        = $(JAVA_PATH)/bin/jar
APXS       = 
PYTHON     = 
PERL	   = /usr/bin/perl
RUBY       = /usr/bin/ruby

CPPFLAGS   = -I$(NEOTONIC_ROOT) 
CFLAGS     = -g -O2 -Wall $(CPPFLAGS) $(PICFLG)
OUTPUT_OPTION = -o $@
LD         = $(CC) -o
LDFLAGS    = -L$(LIB_DIR) 
LDSHARED   = $(CC) -shared $(PICFLG)
CPPLDSHARED   = $(CPP) -shared $(PICFLG)
AR         = ar cr
RANLIB     = ranlib
DEP_LIBS   = $(DLIBS:-l%=$(LIB_DIR)lib%.a)
DBI_LIBS   = -ldbi -ldl -lz
LIBS       =  -lz
LS         = /bin/ls
XARGS      = xargs -i%
BUILD_WRAPPERS =  perl ruby
EXTRA_UTL_OBJS = 
EXTRA_UTL_SRC  =  ulocks.c rcfs.c skiplist.c dict.c filter.c neo_net.c neo_server.c

## I don't really feel like writing a configure thing for this yet
ifeq ($(OSNAME),SunOS)
LDSHARED   = ld -G -fPIC
endif
ifeq ($(OSTYPE),Darwin)
LDSHARED   = $(CC) -bundle -flat_namespace -undefined suppress $(PICFLG)
CPPLDSHARED   = $(CPP) -bundle -flat_namespace -undefined suppress $(PICFLG)
endif

## --------------win32 options

## ifeq ($(OSTYPE),WindowsNT)
## CFLAGS += -D__WINDOWS_GCC__
## USE_DB2 = 0
## USE_ZLIB = 0
## # SHELL=cmd.exe
## LS = ls
## PYTHON_INC = -Ic:/Python22/include
## LDSHARED= dllwrap
## endif
## 
## ## --------------
## 
## ifeq ($(OSTYPE),FreeBSD)
## XARGS = xargs -J%
## # This should work on freebsd... but I wouldn't worry too much about it
## USE_DB2 = 0
## PYTHON_INC = -I/usr/local/include/python2.2
## endif
## 
## ifeq ($(USE_ZLIB),1)
## LIBS += -lz
## endif
## 
## ifeq ($(USE_DB2),1)
## DB2_INC = -I$(HOME)/src/db-2.7.7/dist
## DB2_LIB = -L$(HOME)/src/db-2.7.7/dist -ldb
## CFLAGS += $(DB2_INC)
## endif

.c.o:
	$(CC) $(CFLAGS) $(OUTPUT_OPTION) -c $<

everything: depend all

.PHONY: depend
depend: Makefile.depends

SOURCE_FILES := $(wildcard *.c)
Makefile.depends: $(NEOTONIC_ROOT)/rules.mk Makefile
	@echo "*******************************************"
	@echo "** Building Dependencies "
	@echo "** OSNAME: $(OSTYPE)"
	@rm -f Makefile.depends
	@touch Makefile.depends
	@if test "x" != "x$(SOURCE_FILES)"; then \
	  for II in "$(SOURCE_FILES)"; do \
		gcc -M -MG ${CFLAGS} $$II >> Makefile.depends; \
	  done; \
	 fi
	@echo "** (done) "

DEPEND_FILE := $(shell find . -name Makefile.depends -print)
ifneq ($(DEPEND_FILE),)
include Makefile.depends
endif
