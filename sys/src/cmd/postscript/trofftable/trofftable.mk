MAKE=/bin/make
MAKEFILE=trofftable.mk

SYSTEM=V9
VERSION=3.3.2

GROUP=bin
OWNER=bin

FONTDIR=/usr/lib/font
POSTBIN=/usr/bin/postscript
POSTLIB=/usr/lib/postscript
MAN1DIR=/tmp

all : trofftable

install : all
	@if [ ! -d $(POSTBIN) ]; then \
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
	cp trofftable $(POSTBIN)/trofftable
	@chmod 755 $(POSTBIN)/trofftable
	@chgrp $(GROUP) $(POSTBIN)/trofftable
	@chown $(OWNER) $(POSTBIN)/trofftable
	cp trofftable.ps $(POSTLIB)/trofftable.ps
	@chmod 644 $(POSTLIB)/trofftable.ps
	@chgrp $(GROUP) $(POSTLIB)/trofftable.ps
	@chown $(OWNER) $(POSTLIB)/trofftable.ps
	cp trofftable.1 $(MAN1DIR)/trofftable.1
	@chmod 644 $(MAN1DIR)/trofftable.1
	@chgrp $(GROUP) $(MAN1DIR)/trofftable.1
	@chown $(OWNER) $(MAN1DIR)/trofftable.1

clean :

clobber : clean
	rm -f trofftable

trofftable : trofftable.sh
	sed \
	    -e "s'^FONTDIR=.*'FONTDIR=$(FONTDIR)'" \
	    -e "s'^POSTBIN=.*'POSTBIN=$(POSTBIN)'" \
	    -e "s'^POSTLIB=.*'POSTLIB=$(POSTLIB)'" \
	trofftable.sh >trofftable
	@chmod 755 trofftable

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
	trofftable.1 >XXX.1; \
	mv XXX.1 trofftable.1

