# This file makes an archive file consisting of the window support for
# all the devices.  To make Metafont support another device, just add a
# file here, and the obvious declarations in ../../common/extra.c (the device
# support is the last section in that file).
# 
# By making the result into an archive file, the final binary has only
# the devices which are actually desired, thus saving some small amount
# of time and space.

SHELL=/bin/sh

CC=gcc
cflags=-g -I../../lib
CFLAGS=$(cflags) $(xincludedir) -I../../$(srcdir)/lib

RANLIB=true

lib=window.a
objs=hp2627.o sun.o tek.o uniterm.o x10.o x11.o

.SUFFIXES:
.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) -c $*.c

default:	all

all:	$(lib)

$(lib):	$(objs)
	rm -f $(lib)
	ar qc $(lib) $(objs)
	$(RANLIB) $(lib)

$(objs): ../mfd.h


install:

mostlyclean:
	rm -f *.o $(lib)

clean: mostlyclean
	rm -f \#*\# *~ *.bak *.ckp core

realclean: clean
