MAKE=/bin/make
MAKEFILE=devpost.add.mk

SYSTEM=SYSV
VERSION=3.2

GROUP=bin
OWNER=bin

FONTDIR=/usr/lib/font
FONTFILES=??

all :

install : all
	@if [ ! -d $(FONTDIR) ]; then \
	    mkdir $(FONTDIR); \
	    chmod 755 $(FONTDIR); \
	    chgrp $(GROUP) $(FONTDIR); \
	    chown $(OWNER) $(FONTDIR); \
	fi
	@if [ ! -d $(FONTDIR)/devpost ]; then \
	    mkdir $(FONTDIR)/devpost; \
	    chmod 755 $(FONTDIR)/devpost; \
	    chgrp $(GROUP) $(FONTDIR)/devpost; \
	    chown $(OWNER) $(FONTDIR)/devpost; \
	fi
	cp $(FONTFILES) $(FONTDIR)/devpost
	@for i in $(FONTFILES); do \
	    chmod 644 $(FONTDIR)/devpost/$$i; \
	    chgrp $(GROUP) $(FONTDIR)/devpost/$$i; \
	    chown $(OWNER) $(FONTDIR)/devpost/$$i; \
	done

clean :

clobber : clean

changes :
	@trap "" 1 2 3 15; \
	sed \
	    -e "s'^SYSTEM=.*'SYSTEM=$(SYSTEM)'" \
	    -e "s'^VERSION=.*'VERSION=$(VERSION)'" \
	    -e "s'^GROUP=.*'GROUP=$(GROUP)'" \
	    -e "s'^OWNER=.*'OWNER=$(OWNER)'" \
	    -e "s'^FONTDIR=.*'FONTDIR=$(FONTDIR)'" \
	$(MAKEFILE) >XXX.mk; \
	mv XXX.mk $(MAKEFILE)

