MAKE=/bin/make
MAKEFILE=buildtables.mk

SYSTEM=V9
VERSION=3.3.2

GROUP=bin
OWNER=bin

FONTDIR=/usr/lib/font
POSTBIN=/usr/bin/postscript
POSTLIB=/usr/lib/postscript
MAN1DIR=/tmp

all : buildtables

install : all
	@if [ ! -d $(POSTBIN) ]; then \
	    mkdir $(POSTBIN); \
	    chmod 755 $(POSTBIN); \
	    chgrp $(GROUP) $(POSTBIN); \
	    chown $(OWNER) $(POSTBIN); \
	fi
	cp buildtables $(POSTBIN)/buildtables
	@chmod 755 $(POSTBIN)/buildtables
	@chgrp $(GROUP) $(POSTBIN)/buildtables
	@chown $(OWNER) $(POSTBIN)/buildtables
	cp buildtables.1 $(MAN1DIR)/buildtables.1
	@chmod 644 $(MAN1DIR)/buildtables.1
	@chgrp $(GROUP) $(MAN1DIR)/buildtables.1
	@chown $(OWNER) $(MAN1DIR)/buildtables.1

clean :

clobber : clean
	rm -f buildtables

buildtables : buildtables.sh
	sed \
	    -e "s'^FONTDIR=.*'FONTDIR=$(FONTDIR)'" \
	    -e "s'^POSTBIN=.*'POSTBIN=$(POSTBIN)'" \
	    -e "s'^POSTLIB=.*'POSTLIB=$(POSTLIB)'" \
	buildtables.sh >buildtables
	@chmod 755 buildtables

changes :
	@trap "" 1 2 3 15; \
	sed \
	    -e "s'^SYSTEM=.*'SYSTEM=$(SYSTEM)'" \
	    -e "s'^VERSION=.*'VERSION=$(VERSION)'" \
	    -e "s'^GROUP=.*'GROUP=$(GROUP)'" \
	    -e "s'^OWNER=.*'OWNER=$(OWNER)'" \
	    -e "s'^FONTDIR=.*'FONTDIR=$(FONTDIR)'" \
	    -e "s'^POSTBIN=.*'POSTBIN=$(POSTBIN)'" \
	    -e "s'^POSTLIB=.*'POSTLIB=$(POSTLIB)'" \
	    -e "s'^MAN1DIR=.*'MAN1DIR=$(MAN1DIR)'" \
	$(MAKEFILE) >XXX.mk; \
	mv XXX.mk $(MAKEFILE); \
	sed \
	    -e "s'^.ds dF.*'.ds dF $(FONTDIR)'" \
	    -e "s'^.ds dQ.*'.ds dQ $(POSTLIB)'" \
	buildtables.1 >XXX.1; \
	mv XXX.1 buildtables.1

