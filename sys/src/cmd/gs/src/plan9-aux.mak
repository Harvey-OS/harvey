#    Copyright (C) 1999-2004 artofcode LLC. All rights reserved.
# 
# This software is provided AS-IS with no warranty, either express or
# implied.
# 
# This software is distributed under license and may not be copied,
# modified or distributed except as expressly authorized under the terms
# of the license contained in the file LICENSE in this distribution.
# 
# For more information about licensing, please refer to
# http://www.ghostscript.com/licensing/. For information on
# commercial licensing, go to http://www.artifex.com/licensing/ or
# contact Artifex Software, Inc., 101 Lucas Valley Road #110,
# San Rafael, CA  94903, U.S.A., +1(415)492-9861.

# $Id: unix-aux.mak,v 1.12 2004/01/22 09:16:30 giles Exp $
# Partial makefile common to all Unix configurations.
# This makefile contains the build rules for the auxiliary programs such as
# echogs, and the 'platform' modules.

# Define the name of this makefile.
UNIX_AUX_MAK=$(GLSRC)unix-aux.mak

# -------------------------------- Library -------------------------------- #

## The Unix platforms

# We have to include a test for the existence of sys/time.h,
# because some System V platforms don't have it.

# Unix platforms other than System V, and also System V Release 4
# (SVR4) platforms.
unix__=$(GLOBJ)gp_getnv.$(OBJ) $(GLOBJ)gp_unix.$(OBJ) $(GLOBJ)gp_unifs.$(OBJ) $(GLOBJ)gp_unifn.$(OBJ) $(GLOBJ)gp_stdia.$(OBJ) $(GLOBJ)gp_unix_cache.$(OBJ)
$(GLGEN)unix_.dev: $(unix__) $(GLD)nosync.dev
	$(SETMOD) $(GLGEN)unix_ $(unix__) -include $(GLD)nosync

$(GLOBJ)gp_unix.$(OBJ): $(GLSRC)gp_unix.c $(AK)\
 $(pipe__h) $(string__h) $(time__h)\
 $(gx_h) $(gsexit_h) $(gp_h)
	$(GLCC) $(GLO_)gp_unix.$(OBJ) $(C_) $(GLSRC)gp_unix.c

$(GLOBJ)gp_unix_cache.$(OBJ): $(GLSRC)gp_unix_cache.c $(AK)\
 $(stdio__h) $(string__h) $(time__h) $(gconfigd_h) $(gp_h) $(md5_h)
	$(GLCC) $(GLO_)gp_unix_cache.$(OBJ) $(C_) $(GLSRC)gp_unix_cache.c

# assume all Unix platforms support unbuffered read
$(GLOBJ)gp_stdia.$(OBJ): $(GLSRC)gp_stdia.c $(AK)\
  $(stdio__h) $(time__h) $(unistd__h) $(gx_h) $(gp_h)
	$(GLCC) $(GLO_)gp_stdia.$(OBJ) $(C_) $(GLSRC)gp_stdia.c

# System V platforms other than SVR4, which lack some system calls,
# but have pipes.
sysv__=$(GLOBJ)gp_getnv.$(OBJ) $(GLOBJ)gp_unix.$(OBJ) $(GLOBJ)gp_unifs.$(OBJ) $(GLOBJ)gp_unifn.$(OBJ) $(GLOBJ)gp_sysv.$(OBJ)
$(GLGEN)sysv_.dev: $(sysv__) $(GLD)nosync.dev
	$(SETMOD) $(GLGEN)sysv_ $(sysv__) -include $(GLD)nosync

$(GLOBJ)gp_sysv.$(OBJ): $(GLSRC)gp_sysv.c $(stdio__h) $(time__h) $(AK)
	$(GLCC) $(GLO_)gp_sysv.$(OBJ) $(C_) $(GLSRC)gp_sysv.c

# -------------------------- Auxiliary programs --------------------------- #

$(ECHOGS_XE): $(GLSRC)echogs.c $(AK) $(stdpre_h)
	$(CCAUX) $(I_)$(GLSRCDIR)$(_I) $(O_)$(ECHOGS_XE) $(GLSRC)echogs.c

# On the RS/6000 (at least), compiling genarch.c with gcc with -O
# produces a buggy executable.
$(GENARCH_XE): $(GLSRC)genarch.c $(AK) $(GENARCH_DEPS)
	$(CCAUX) $(I_)$(GLSRCDIR)$(_I) $(O_)$(GENARCH_XE) $(GLSRC)genarch.c

$(GENCONF_XE): $(GLSRC)genconf.c $(AK) $(GENCONF_DEPS)
	$(CCAUX) $(I_)$(GLSRCDIR)$(_I) $(O_)$(GENCONF_XE) $(GLSRC)genconf.c

$(GENDEV_XE): $(GLSRC)gendev.c $(AK) $(GENDEV_DEPS)
	$(CCAUX) $(I_)$(GLSRCDIR)$(_I) $(O_)$(GENDEV_XE) $(GLSRC)gendev.c

$(GENHT_XE): $(GLSRC)genht.c $(AK) $(GENHT_DEPS)
	$(CCAUX) $(GENHT_CFLAGS) $(O_)$(GENHT_XE) $(GLSRC)genht.c

$(GENINIT_XE): $(GLSRC)geninit.c $(AK) $(GENINIT_DEPS)
	$(CCAUX) $(I_)$(GLSRCDIR)$(_I) $(O_)$(GENINIT_XE) $(GLSRC)geninit.c

# Query the environment to construct gconfig_.h.
# The "else true;" is required because Ultrix's implementation of sh -e
# terminates execution of a command if any error occurs, even if the command
# traps the error with ||.
INCLUDE=/sys/include/ape
$(gconfig__h): $(UNIX_AUX_MAK) $(ECHOGS_XE)
	$(ECHOGS_XE) -w $(gconfig__h) -x 2f2a -s This file was generated automatically by unix-aux.mak. -s -x 2a2f
	if ( test -f $(INCLUDE)/dirent.h ); then $(ECHOGS_XE) -a $(gconfig__h) -x 23 define HAVE_DIRENT_H; else true; fi
	if ( test -f $(INCLUDE)/ndir.h ); then $(ECHOGS_XE) -a $(gconfig__h) -x 23 define HAVE_NDIR_H; else true; fi
	if ( test -f $(INCLUDE)/sys/dir.h ); then $(ECHOGS_XE) -a $(gconfig__h) -x 23 define HAVE_SYS_DIR_H; else true; fi
	if ( test -f $(INCLUDE)/sys/ndir.h ); then $(ECHOGS_XE) -a $(gconfig__h) -x 23 define HAVE_SYS_NDIR_H; else true; fi
	if ( test -f $(INCLUDE)/sys/time.h ); then $(ECHOGS_XE) -a $(gconfig__h) -x 23 define HAVE_SYS_TIME_H; else true; fi
	if ( test -f $(INCLUDE)/sys/times.h ); then $(ECHOGS_XE) -a $(gconfig__h) -x 23 define HAVE_SYS_TIMES_H; else true; fi
	if ( test -f $(JSRCDIR)/jmemsys.h); then true; else $(ECHOGS_XE) -a $(gconfig__h) -x 23 define DONT_HAVE_JMEMSYS_H; fi
