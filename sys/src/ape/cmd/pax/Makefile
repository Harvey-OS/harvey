#
# PAX - read and write POSIX conformant tar and cpio archives 
#
# Written by Mark H. Colburn (mark@jhereg.mn.org)
#
# $Id: Makefile,v 1.2 89/02/12 10:08:59 mark Exp $
#

#
# CONFIGURATION SECTION
#
# The following defines may need to be changed for each system which PAX
# is installed on.  Please review these settings before installing on your
# system.

#
# You should define _POSIX_SOURCE if you are running on a POSIX system.  This
# include has to be in the command line because it has to appear before any
# include file is included in the source.  For most systems in use today,
# it should be left blank.
# 
# POSIX= -D_POSIX_SOURCE
POSIX= 

#
# Set CFLAGS to whatever makes your C compiler happy.  Be sure to include 
# the definition of $(POSIX) in the flag.
#
CFLAGS = -O $(POSIX)
CC = cc

#
# Set LIBS to any additional libraries that you need linked in with pax.
#
LIBS=

#
# Set LFLAGS to whatever makes your linker happy
#
#LDFLAGS = -s
LDFLAGS = 

#
# Set COPY to the name of the command to use to copy pax to cpio and
# tar.  Usually it is 'ln'.
#
COPY=ln

#
# Set LINTFLAGS to whatever makes your implementation of lint happy.  If
# you don't undef __STDC__ and you have an ANSI C compiler, lint will choke 
# on the function prototypes present in func.h.
#
LINTFLAGS = -U__STDC__ $(POSIX)

#
# BINDIR - points to the directory in which you want the final pax, tar and
# cpio binaries installed in.
#
BINDIR = /usr/local/bin

#
# MANDIR - specify the directory in which the man pages will be installed
#
MAN5 = /usr/man/man5
MAN1 = /usr/man/man1
MAN5EXT = 5
MAN1EXT = 1

#
# There are three different ways to get POSIX or BSD conformant directory 
# access routines: 1) they are installed in your system library, 2) you 
# are using Doug Gwyn's dirent library (/usr/lib/libdirent.a), or 3) you 
# need the source for the dirent package.  Based on that, pick one of the 
# following three options:
#
# 1. Pick the first dirent line and make sure that config.h is defined
#    correctly for your version of directory access routines.  THIS IS
#    THE LINE WHICH SHOULD BE USED FOR BSD SYSTEMS.
# 2. Chose the second dirent line which  used a library at link time.  You
#    may need to change the name of the library to match your system.
# 3. If you need #3, then you must copy everything in the subdirectory dirent 
#    to this directory and choose the DIROBJ lines.  Please note that this 
#    version of dirent has been modified to work as a stand-alone. 
#
DIRENT=
#DIRENT= -ldirent
#DIROBJ= paxdir.o

#
# END CONFIGURATION SECTION 
#
# Nothing beyond this point should need to be changed.
#

SHELL = /bin/sh
MISC  = Makefile pax.1 tar.5 cpio.5 README PATCHLEVEL
HEADERS= config.h func.h limits.h port.h pax.h 
SOURCE= pax.c append.c buffer.c cpio.c create.c extract.c fileio.c\
	link.c list.c mem.c namelist.c names.c pass.c pathname.c\
	port.c regexp.c replace.c tar.c ttyio.c warn.c wildmat.c
OBJECT= pax.o append.o buffer.o cpio.o create.o extract.o fileio.o\
	link.o list.o mem.o namelist.o names.o pass.o pathname.o\
	port.o regexp.o replace.o tar.o ttyio.o warn.o wildmat.o $(DIROBJ)
PROGS = pax tar cpio
PMAN1 = pax.1 tar.1
PMAN5 = pax.5 tar.5

all: $(PROGS)

install: $(PROGS)
	strip pax
	cp pax $(BINDIR)
	chmod 755 $(BINDIR)/pax
	ln $(BINDIR)/pax $(BINDIR)/tar
	ln $(BINDIR)/pax $(BINDIR)/cpio
	cp $(PMAN1) $(MAN1)
#	cp $(PMAN5) $(MAN5)

clean:
	rm -f $(OBJECT)
	rm -f $(PROGS) a.out *.BAK *.bak 

lint:
	lint $(LINTFLAGS) $(SOURCE)

pax : $(OBJECT)
	$(CC) $(CFLAGS) $(LDFLAGS) -o pax $(OBJECT) $(DIRENT) $(LIBS)

tar: pax
	rm -f tar
	$(COPY) pax tar

cpio: pax
	rm -f cpio
	$(COPY) pax cpio

$(OBJECT): $(HEADERS)
