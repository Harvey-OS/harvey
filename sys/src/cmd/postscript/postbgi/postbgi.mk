MAKE=/bin/make
MAKEFILE=postbgi.mk

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

HFILES=postbgi.h\
       $(COMMONDIR)/comments.h\
       $(COMMONDIR)/ext.h\
       $(COMMONDIR)/gen.h\
       $(COMMONDIR)/path.h

OFILES=postbgi.o\
       $(COMMONDIR)/glob.o\
       $(COMMONDIR)/misc.o\
       $(COMMONDIR)/request.o

all : postbgi

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
	cp postbgi $(POSTBIN)/postbgi
	@chmod 755 $(POSTBIN)/postbgi
	@chgrp $(GROUP) $(POSTBIN)/postbgi
	@chown $(OWNER) $(POSTBIN)/postbgi
	cp postbgi.ps $(POSTLIB)/postbgi.ps
	@chmod 644 $(POSTLIB)/postbgi.ps
	@chgrp $(GROUP) $(POSTLIB)/postbgi.ps
	@chown $(OWNER) $(POSTLIB)/postbgi.ps
	cp postbgi.1 $(MAN1DIR)/postbgi.1
	@chmod 644 $(MAN1DIR)/postbgi.1
	@chgrp $(GROUP) $(MAN1DIR)/postbgi.1
	@chown $(OWNER) $(MAN1DIR)/postbgi.1

clean :
	rm -f *.o

clobber : clean
	rm -f postbgi

postbgi : $(OFILES)
	$(CC) $(CFLAGS) $(LDFLAGS) -o postbgi $(OFILES) -lm

postbgi.o : $(HFILES)

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
	postbgi.1 >XXX.1; \
	mv XXX.1 postbgi.1

