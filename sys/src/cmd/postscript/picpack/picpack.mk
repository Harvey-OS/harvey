MAKE=/bin/make
MAKEFILE=picpack.mk

SYSTEM=V9
VERSION=3.3.2

GROUP=bin
OWNER=bin

MAN1DIR=/tmp
POSTBIN=/usr/bin/postscript

COMMONDIR=../common

CFLGS=-O
LDFLGS=-s

CFLAGS=$(CFLGS) -I$(COMMONDIR)
LDFLAGS=$(LDFLGS)

HFILES=$(COMMONDIR)/ext.h\
       $(COMMONDIR)/gen.h\
       $(COMMONDIR)/path.h

OFILES=picpack.o\
       $(COMMONDIR)/glob.o\
       $(COMMONDIR)/misc.o\
       $(COMMONDIR)/tempnam.o

all : picpack

install : all
	@if [ ! -d "$(POSTBIN)" ]; then \
	    mkdir $(POSTBIN); \
	    chmod 755 $(POSTBIN); \
	    chgrp $(GROUP) $(POSTBIN); \
	    chown $(OWNER) $(POSTBIN); \
	fi
	cp picpack $(POSTBIN)/picpack
	@chmod 755 $(POSTBIN)/picpack
	@chgrp $(GROUP) $(POSTBIN)/picpack
	@chown $(OWNER) $(POSTBIN)/picpack
	cp picpack.1 $(MAN1DIR)/picpack.1
	@chmod 644 $(MAN1DIR)/picpack.1
	@chgrp $(GROUP) $(MAN1DIR)/picpack.1
	@chown $(OWNER) $(MAN1DIR)/picpack.1

clean :
	rm -f *.o

clobber : clean
	rm -f picpack

picpack : $(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o picpack $(OFILES)

picpack.o : $(HFILES)

$(COMMONDIR)/glob.o\
$(COMMONDIR)/misc.o\
$(COMMONDIR)/tempnam.o :
	@cd $(COMMONDIR); $(MAKE) -f common.mk SYSTEM=$(SYSTEM) `basename $@`

changes :
	@trap "" 1 2 3 15; \
	sed \
	    -e "s'^SYSTEM=.*'SYSTEM=$(SYSTEM)'" \
	    -e "s'^VERSION=.*'VERSION=$(VERSION)'" \
	    -e "s'^GROUP=.*'GROUP=$(GROUP)'" \
	    -e "s'^OWNER=.*'OWNER=$(OWNER)'" \
	    -e "s'^MAN1DIR=.*'MAN1DIR=$(MAN1DIR)'" \
	    -e "s'^POSTBIN=.*'POSTBIN=$(POSTBIN)'" \
	$(MAKEFILE) >XXX.mk; \
	mv XXX.mk $(MAKEFILE)

