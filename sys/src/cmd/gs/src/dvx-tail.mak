#    Copyright (C) 1994, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
# 
# This file is part of Aladdin Ghostscript.
# 
# Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
# or distributor accepts any responsibility for the consequences of using it,
# or for whether it serves any particular purpose or works at all, unless he
# or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
# License (the "License") for full details.
# 
# Every copy of Aladdin Ghostscript must include a copy of the License,
# normally in a plain ASCII text file named PUBLIC.  The License grants you
# the right to copy, modify and redistribute Aladdin Ghostscript, but only
# under certain conditions described in the License.  Among other things, the
# License requires that the copyright notice and this notice be preserved on
# all copies.

# $Id: dvx-tail.mak,v 1.1 2000/03/09 08:40:40 lpd Exp $
# Partial makefile, common to all Desqview/X configurations.
# This is the last part of the makefile for Desqview/X configurations.

# The following prevents GNU make from constructing argument lists that
# include all environment variables, which can easily be longer than
# brain-damaged system V allows.

.NOEXPORT:

# -------------------------------- Library -------------------------------- #

## The Desqview/X platform

dvx__=$(GLOBJ)gp_getnv.$(OBJ) $(GLOBJ)gp_nofb.$(OBJ) $(GLOBJ)gp_dvx.$(OBJ) $(GLOBJ)gp_unifs.$(OBJ) $(GLOBJ)gp_dosfs.$(OBJ)
$(GLGEN)dvx_.dev: $(dvx__) nosync.dev
	$(SETMOD) $(GLGEN)dvx_ $(dvx__) -include nosync

$(GLOBJ)gp_dvx.$(OBJ): $(GLSRC)gp_dvx.c $(AK) $(string__h) $(gx_h) $(gsexit_h) $(gp_h) \
  $(time__h) $(dos__h)
	$(CC_) -D__DVX__ -c $(GLSRC)gp_dvx.c -o $(GLOBJ)gp_dvx.$(OBJ)

# -------------------------- Auxiliary programs --------------------------- #

$(ANSI2KNR_XE): ansi2knr.c $(stdio__h) $(string__h) $(malloc__h)
	$(CC) -o ansi2knr $(CFLAGS) ansi2knr.c

$(ECHOGS_XE): echogs.c
	$(CC) -o echogs $(CFLAGS) echogs.c
	strip echogs
	coff2exe echogs
	del echogs

$(GENARCH_XE): genarch.c $(GENARCH_DEPS)
	$(CC) -o genarch genarch.c
	strip genarch
	coff2exe genarch
	del genarch

$(GENCONF_XE): genconf.c $(GENCONF_DEPS)
	$(CC) -o genconf genconf.c
	strip genconf
	coff2exe genconf
	del genconf

$(GENDEV_XE): gendev.c $(GENDEV_DEPS)
	$(CC) -o gendev gendev.c
	strip gendev
	coff2exe gendev
	del gendev

$(GENHT_XE): genht.c $(GENHT_DEPS)
	$(CC) -o genht $(GENHT_CFLAGS) genht.c
	strip genht
	coff2exe genht
	del genht

$(GENINIT_XE): geninit.c $(GENINIT_DEPS)
	$(CC) -o geninit geninit.c
	strip geninit
	coff2exe geninit
	del geninit

# Construct $(gconfig__h) to reflect the environment.
INCLUDE=/djgpp/include
$(gconfig__h): $(GLSRCDIR)/dvx-tail.mak $(ECHOGS_XE)
	$(ECHOGS_XE) -w $(gconfig__h) -x 2f2a -s This file was generated automatically. -s -x 2a2f
	$(ECHOGS_XE) -a $(gconfig__h) -x 23 define HAVE_SYS_TIME_H
	$(ECHOGS_XE) -a $(gconfig__h) -x 23 define HAVE_DIRENT_H

# ----------------------------- Main program ------------------------------ #

# Interpreter main program

$(GS_XE): ld.tr gs.$(OBJ) $(INT_ALL) $(LIB_ALL) $(DEVS_ALL)
	$(CP_) ld.tr _temp_
	echo $(EXTRALIBS) $(STDLIBS) >>_temp_
	$(CC) $(LDFLAGS) -o $(GS) gs.$(OBJ) @_temp_
	strip $(GS)
	coff2exe $(GS)  
	del $(GS)  
