MAKE=/bin/make
MAKEFILE=postdmd.mk

SYSTEM=V9
VERSION=3.3.2

GROUP=bin
OWNER=bin

MAN1DIR=/tmp
POSTBIN=/usr/bin/postscript
POSTLIB=/usr/lib/postscript

COMMONDIR=../common

CFLGS=-O
LDFLGS=-s

CFLAGS=$(CFLGS) -I$(COMMONDIR)
LDFLAGS=$(LDFLGS)

HFILES=$(COMMONDIR)/comments.h\
       $(COMMONDIR)/ext.h\
       $(COMMONDIR)/gen.h\
       $(COMMONDIR)/path.h

OFILES=postdmd.o\
       $(COMMONDIR)/glob.o\
       $(COMMONDIR)/misc.o\
       $(COMMONDIR)/request.o

all : postdmd

install : all
	@if [ ! -d "$(POSTBIN)" ]; then \
	    mkdir $(POSTBIN); \
	    chmod 755 $(POSTBIN); \
	    chgrp $(GROUP) $(POSTBIN); \
	    chown $(OWNER) $(POSTBIN); \
	fi
	@if [ ! -d "$(POSTLIB)" ]; then \
	    mkdir $(POSTLIB); \
	    chmod 755 $(POSTLIB); \
	    chgrp $(GROUP) $(POSTLIB); \
	    chown $(OWNER) $(POSTLIB); \
	fi
	cp postdmd $(POSTBIN)/postdmd
	@chmod 755 $(POSTBIN)/postdmd
	@chgrp $(GROUP) $(POSTBIN)/postdmd
	@chown $(OWNER) $(POSTBIN)/postdmd
	cp postdmd.ps $(POSTLIB)/postdmd.ps
	@chmod 644 $(POSTLIB)/postdmd.ps
	@chgrp $(GROUP) $(POSTLIB)/postdmd.ps
	@chown $(OWNER) $(POSTLIB)/postdmd.ps
	cp postdmd.1 $(MAN1DIR)/postdmd.1
	@chmod 644 $(MAN1DIR)/postdmd.1
	@chgrp $(GROUP) $(MAN1DIR)/postdmd.1
	@chown $(OWNER) $(MAN1DIR)/postdmd.1

clean :
	rm -f *.o

clobber : clean
	rm -f postdmd

postdmd : $(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o postdmd $(OFILES)

postdmd.o : $(HFILES)

$(COMMONDIR)/glob.o\
$(COMMONDIR)/misc.o\
$(COMMONDIR)/request.o :
	@cd $(COMMONDIR); $(MAKE) -f common.mk `basename $@`

changes :
	@trap "" 1 2 3 15; \
	sed \
	    -e "s'^SYSTEM=.*'SYSTEM=$(SYSTEM)'" \
	    -e "s'^VERSION=.*'VERSION=$(VERSION)'" \
	    -e "s'^GROUP=.*'GROUP=$(GROUP)'" \
	    -e "s'^OWNER=.*'OWNER=$(OWNER)'" \
	    -e "s'^MAN1DIR=.*'MAN1DIR=$(MAN1DIR)'" \
	    -e "s'^POSTBIN=.*'POSTBIN=$(POSTBIN)'" \
	    -e "s'^POSTLIB=.*'POSTLIB=$(POSTLIB)'" \
	$(MAKEFILE) >XXX.mk; \
	mv XXX.mk $(MAKEFILE); \
	sed \
	    -e "s'^.ds dQ.*'.ds dQ $(POSTLIB)'" \
	postdmd.1 >XXX.1; \
	mv XXX.1 postdmd.1

