#    Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
# 
# This file is part of AFPL Ghostscript.
# 
# AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
# distributor accepts any responsibility for the consequences of using it, or
# for whether it serves any particular purpose or works at all, unless he or
# she says so in writing.  Refer to the Aladdin Free Public License (the
# "License") for full details.
# 
# Every copy of AFPL Ghostscript must include a copy of the License, normally
# in a plain ASCII text file named PUBLIC.  The License grants you the right
# to copy, modify and redistribute AFPL Ghostscript, but only under certain
# conditions described in the License.  Among other things, the License
# requires that the copyright notice and this notice be preserved on all
# copies.

# $Id: unix-end.mak,v 1.2 2000/09/19 19:00:52 lpd Exp $
# Partial makefile common to all Unix and Desqview/X configurations.
# This is the next-to-last part of the makefile for these configurations.

# Define the rule for building standard configurations.
STDDIRS:
	@if test ! -d $(BINDIR); then mkdir $(BINDIR); fi
	@if test ! -d $(GLGENDIR); then mkdir $(GLGENDIR); fi
	@if test ! -d $(GLOBJDIR); then mkdir $(GLOBJDIR); fi
	@if test ! -d $(PSGENDIR); then mkdir $(PSGENDIR); fi
	@if test ! -d $(PSOBJDIR); then mkdir $(PSOBJDIR); fi

# Define a rule for building profiling configurations.
PGDIRS: STDDIRS
	@if test ! -d $(BINDIR)/$(PGRELDIR); then mkdir $(BINDIR)/$(PGRELDIR); fi
	@if test ! -d $(GLGENDIR)/$(PGRELDIR); then mkdir $(GLGENDIR)/$(PGRELDIR); fi
	@if test ! -d $(GLOBJDIR)/$(PGRELDIR); then mkdir $(GLOBJDIR)/$(PGRELDIR); fi
	@if test ! -d $(PSGENDIR)/$(PGRELDIR); then mkdir $(PSGENDIR)/$(PGRELDIR); fi
	@if test ! -d $(PSOBJDIR)/$(PGRELDIR); then mkdir $(PSOBJDIR)/$(PGRELDIR); fi

PGDEFS=GENOPT='-DPROFILE' CFLAGS='$(CFLAGS_PROFILE) $(GCFLAGS) $(XCFLAGS)'\
 LDFLAGS='$(XLDFLAGS) -pg' XLIBS='Xt SM ICE Xext X11' CC_LEAF='$(CC_LEAF_PG)'\
 BINDIR=$(BINDIR)/$(PGRELDIR)\
 GLGENDIR=$(GLGENDIR)/$(PGRELDIR) GLOBJDIR=$(GLOBJDIR)/$(PGRELDIR)\
 PSGENDIR=$(PSGENDIR)/$(PGRELDIR) PSOBJDIR=$(PSOBJDIR)/$(PGRELDIR)

pg: PGDIRS
	$(MAKE) $(PGDEFS) default

pgclean: PGDIRS
	$(MAKE) $(PGDEFS) clean

# Define a rule for building debugging configurations.
DEBUGDIRS: STDDIRS
	@if test ! -d $(BINDIR)/$(DEBUGRELDIR); then mkdir $(BINDIR)/$(DEBUGRELDIR); fi
	@if test ! -d $(GLGENDIR)/$(DEBUGRELDIR); then mkdir $(GLGENDIR)/$(DEBUGRELDIR); fi
	@if test ! -d $(GLOBJDIR)/$(DEBUGRELDIR); then mkdir $(GLOBJDIR)/$(DEBUGRELDIR); fi
	@if test ! -d $(PSGENDIR)/$(DEBUGRELDIR); then mkdir $(PSGENDIR)/$(DEBUGRELDIR); fi
	@if test ! -d $(PSOBJDIR)/$(DEBUGRELDIR); then mkdir $(PSOBJDIR)/$(DEBUGRELDIR); fi

DEBUGDEFS=GENOPT='-DDEBUG' CFLAGS='$(CFLAGS_DEBUG) $(GCFLAGS) $(XCFLAGS)'\
 BINDIR=$(BINDIR)/$(DEBUGRELDIR)\
 GLGENDIR=$(GLGENDIR)/$(DEBUGRELDIR) GLOBJDIR=$(GLOBJDIR)/$(DEBUGRELDIR)\
 PSGENDIR=$(PSGENDIR)/$(DEBUGRELDIR) PSOBJDIR=$(PSOBJDIR)/$(DEBUGRELDIR)

debug: DEBUGDIRS
	$(MAKE) $(DEBUGDEFS) default

debugclean: DEBUGDIRS
	$(MAKE) $(DEBUGDEFS) clean

# The rule for gconfigv.h is here because it is shared between Unix and
# DV/X environments.
$(gconfigv_h): $(GLSRC)unix-end.mak $(TOP_MAKEFILES) $(ECHOGS_XE)
	$(ECHOGS_XE) -w $(gconfigv_h) -x 23 define USE_ASM -x 2028 -q $(USE_ASM)-0 -x 29
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define USE_FPU -x 2028 -q $(FPU_TYPE)-0 -x 29
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define EXTEND_NAMES 0$(EXTEND_NAMES)
	$(ECHOGS_XE) -a $(gconfigv_h) -x 23 define SYSTEM_CONSTANTS_ARE_WRITABLE 0$(SYSTEM_CONSTANTS_ARE_WRITABLE)

# Emacs tags maintenance.

TAGS:
	etags -t $(GLSRC)*.[ch] $(PSSRC)*.[ch]
