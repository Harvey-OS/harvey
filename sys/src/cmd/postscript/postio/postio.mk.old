MAKE=/bin/make
MAKEFILE=postio.mk

SYSTEM=V9
VERSION=3.3.1

GROUP=bin
OWNER=bin

MAN1DIR=/tmp
POSTBIN=/usr/bin/postscript

COMMONDIR=../common

DKLIB=-lipc
CFLGS=-O
LDFLGS=-s

CFLAGS=$(CFLGS) -D$(SYSTEM) -I$(COMMONDIR)
LDFLAGS=$(LDFLGS)

#
# Need dk.h and libdk.a for Datakit support on System V. We recommend you put
# them in standard places. If it's not possible define DKHOSTDIR (below) and
# try uncommenting the following lines:
#
#	DKHOSTDIR=/usr
#	CFLAGS=$(CFLGS) -D$(SYSTEM) -I$(COMMONDIR) -I$(DKHOSTDIR)/include
#	EXTRA=-Wl,-L$(DKHOSTDIR)/lib
#

HFILES=postio.h\
       ifdef.h\
       $(COMMONDIR)/gen.h

OFILES=postio.o\
       ifdef.o\
       slowsend.o

all : postio

install : all
	@if [ ! -d "$(POSTBIN)" ]; then \
	    mkdir $(POSTBIN); \
	    chmod 755 $(POSTBIN); \
	    chgrp $(GROUP) $(POSTBIN); \
	    chown $(OWNER) $(POSTBIN); \
	fi
	cp postio $(POSTBIN)/postio
	@chmod 755 $(POSTBIN)/postio
	@chgrp $(GROUP) $(POSTBIN)/postio
	@chown $(OWNER) $(POSTBIN)/postio
	cp postio.1 $(MAN1DIR)/postio.1
	@chmod 644 $(MAN1DIR)/postio.1
	@chgrp $(GROUP) $(MAN1DIR)/postio.1
	@chown $(OWNER) $(MAN1DIR)/postio.1

clean :
	rm -f *.o

clobber : clean
	rm -f postio

postio : $(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o postio $(OFILES) $(EXTRA) $(DKLIB)

postio.o : $(HFILES)
slowsend.o : postio.h $(COMMONDIR)/gen.h
ifdef.o : ifdef.h $(COMMONDIR)/gen.h

changes :
	@trap "" 1 2 3 15; \
	sed \
	    -e "s'^SYSTEM=.*'SYSTEM=$(SYSTEM)'" \
	    -e "s'^VERSION=.*'VERSION=$(VERSION)'" \
	    -e "s'^GROUP=.*'GROUP=$(GROUP)'" \
	    -e "s'^OWNER=.*'OWNER=$(OWNER)'" \
	    -e "s'^CFLGS=.*'CFLGS=$(CFLGS)'" \
	    -e "s'^DKLIB=.*'DKLIB=$(DKLIB)'" \
	    -e "s'^MAN1DIR=.*'MAN1DIR=$(MAN1DIR)'" \
	    -e "s'^POSTBIN=.*'POSTBIN=$(POSTBIN)'" \
	$(MAKEFILE) >XXX.mk; \
	mv XXX.mk $(MAKEFILE)

