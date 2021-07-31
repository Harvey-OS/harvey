# This file makes routines that are used in more than one place.

SHELL=/bin/sh

CC=gcc
cflags=-g -I.
CFLAGS=$(cflags)

RANLIB=true

lib=lib.a

# We don't make `openinout.o' or `texmf.o', since TeX and Metafont need
# to use different routine names, hence they need different .o files.

objs=alloca.o concat.o concat3.o dir-p.o eofeoln.o file-p.o fprintreal.o\
getopt.o getopt1.o inputint.o main.o ourpaths.o pathsrch.o strpascal.o	\
uexit.o xfopen-pas.o xmalloc.o xopendir.o xrealloc.o xstat.o xstrdup.o	\
zround.o


.SUFFIXES:
.SUFFIXES: .o .c
.c.o:
	$(CC) $(CFLAGS) -c $*.c

default: all
triptrap: all

all: $(lib)

$(lib): $(objs)
	rm -f $(lib)
	ar qc $(lib) $(objs)
	$(RANLIB) $(lib)

mostlyclean:
	rm -f *.o $(lib)

clean distclean realclean: mostlyclean

depend:
	$(CC) $(CFLAGS) -MM *.c >> /tmp/depend

# Dependencies.
alloca.o : alloca.c 
concat.o : concat.c config.h site.h c-auto.h c-std.h c-memstr.h c-errno.h \
  c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h getopt.h lib.h \
  types.h ourpaths.h pascal.h 
dir-p.o : dir-p.c config.h site.h c-auto.h c-std.h c-memstr.h c-errno.h \
  c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h getopt.h lib.h \
  types.h ourpaths.h pascal.h dirio.h xstat.h 
eofeoln.o : eofeoln.c config.h site.h c-auto.h c-std.h c-memstr.h c-errno.h \
  c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h getopt.h lib.h \
  types.h ourpaths.h pascal.h 
file-p.o : file-p.c config.h site.h c-auto.h c-std.h c-memstr.h c-errno.h \
  c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h getopt.h lib.h \
  types.h ourpaths.h pascal.h xstat.h 
fprintreal.o : fprintreal.c config.h site.h c-auto.h c-std.h c-memstr.h \
  c-errno.h c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h \
  getopt.h lib.h types.h ourpaths.h pascal.h 
getopt.o : getopt.c getopt.h 
getopt1.o : getopt1.c getopt.h 
inputint.o : inputint.c config.h site.h c-auto.h c-std.h c-memstr.h \
  c-errno.h c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h \
  getopt.h lib.h types.h ourpaths.h pascal.h 
main.o : main.c config.h site.h c-auto.h c-std.h c-memstr.h c-errno.h \
  c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h getopt.h lib.h \
  types.h ourpaths.h pascal.h 
ourpaths.o : ourpaths.c config.h site.h c-auto.h c-std.h c-memstr.h \
  c-errno.h c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h \
  getopt.h lib.h types.h ourpaths.h pascal.h
pathsrch.o : pathsrch.c config.h site.h c-auto.h c-std.h c-memstr.h \
  c-errno.h c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h \
  getopt.h lib.h types.h ourpaths.h pascal.h c-pathch.h c-namemx.h c-pathmx.h \
  paths.h dirio.h pathsrch.h 
strpascal.o : strpascal.c config.h site.h c-auto.h c-std.h c-memstr.h \
  c-errno.h c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h \
  getopt.h lib.h types.h ourpaths.h pascal.h 
xfopen-pas.o : xfopen-pas.c config.h site.h c-auto.h c-std.h c-memstr.h \
  c-errno.h c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h \
  getopt.h lib.h types.h ourpaths.h pascal.h 
xmalloc.o : xmalloc.c config.h site.h c-auto.h c-std.h c-memstr.h c-errno.h \
  c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h getopt.h lib.h \
  types.h ourpaths.h pascal.h 
xopendir.o : xopendir.c config.h site.h c-auto.h c-std.h c-memstr.h \
  c-errno.h c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h \
  getopt.h lib.h types.h ourpaths.h pascal.h dirio.h 
xrealloc.o : xrealloc.c config.h site.h c-auto.h c-std.h c-memstr.h \
  c-errno.h c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h \
  getopt.h lib.h types.h ourpaths.h pascal.h 
xstat.o : xstat.c config.h site.h c-auto.h c-std.h c-memstr.h c-errno.h \
  c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h getopt.h lib.h \
  types.h ourpaths.h pascal.h xstat.h 
xstrdup.o : xstrdup.c config.h site.h c-auto.h c-std.h c-memstr.h c-errno.h \
  c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h getopt.h lib.h \
  types.h ourpaths.h pascal.h 
zround.o : zround.c config.h site.h c-auto.h c-std.h c-memstr.h c-errno.h \
  c-minmax.h c-limits.h c-seek.h c-fopen.h c-proto.h common.h getopt.h lib.h \
  types.h ourpaths.h pascal.h 
