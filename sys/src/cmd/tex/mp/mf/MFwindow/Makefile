# This file makes an archive file consisting of the window support for
# all the devices.  To make Metafont support another device, just add a
# file here, and the obvious declarations in ../extra.c (the device
# support is the last section in that file).
# 
# By making the result into an archive file, the final binary has only
# the devices which are actually desired, thus saving some small amount
# of time and space.
#

SITEDIR=..
CC=/usr/ape/apebin/pcc
OPT=-g
CFLAGS=$(OPT) -I../$(SITEDIR)
SHELL=/bin/sh
RANLIB=/usr/bin/ranlib

lib=window.a
objs=hp2627.o sun.o tek.o x10.o x11.o

.SUFFIXES:
.SUFFIXES: .o .c
.c.o:
		$(CC) $(CFLAGS) -c $*.c

default:	all

all:		$(lib)

window.a:	$(objs)
		rm -f $(lib)
		ar rc $(lib) $(objs)
		$(SHELL) -c "if ( test -f $(RANLIB) ) then $(RANLIB) $(lib);fi"

$(objs):	../mfd.h

clean:
		rm -f *.o $(lib)

veryclean:	clean
		rm -f \#*\# *~ *.bak *.ckp core

