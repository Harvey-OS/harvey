#    Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
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

# Partial makefile, common to all Desqview/X configurations.

# This is the last part of the makefile for Desqview/X configurations.
# Since Unix make doesn't have an 'include' facility, we concatenate
# the various parts of the makefile together by brute force (in tar_cat).

# The following prevents GNU make from constructing argument lists that
# include all environment variables, which can easily be longer than
# brain-damaged system V allows.

.NOEXPORT:

# -------------------------------- Library -------------------------------- #

## The Desqview/X platform

dvx__=gp_nofb.$(OBJ) gp_dvx.$(OBJ) gp_unifs.$(OBJ) gp_dosfs.$(OBJ)
dvx_.dev: $(dvx__)
	$(SETMOD) dvx_ $(dvx__)

gp_dvx.$(OBJ): gp_dvx.c $(AK) $(string__h) $(gx_h) $(gsexit_h) $(gp_h) \
  $(time__h) $(dos__h)
	$(CCC) -D__DVX__ gp_dvx.c

# -------------------------- Auxiliary programs --------------------------- #

ansi2knr$(XE): ansi2knr.c $(stdio__h) $(string__h) $(malloc__h)
	$(CC) -o ansi2knr$(XE) $(CFLAGS) ansi2knr.c

echogs$(XE): echogs.c
	$(CC) -o echogs $(CFLAGS) echogs.c
	strip echogs
	coff2exe echogs
	del echogs

genarch$(XE): genarch.c
	$(CC) -o genarch genarch.c
	strip genarch
	coff2exe genarch
	del genarch

genconf$(XE): genconf.c
	$(CC) -o genconf genconf.c
	strip genconf
	coff2exe genconf
	del genconf

# We need to query the environment to construct gconfig_.h.
INCLUDE=/usr/include
gconfig_.h: $(MAKEFILE) echogs$(XE)
	echogs -w gconfig_.h -x 2f2a -s This file was generated automatically. -s -x 2a2f
	echogs -a gconfig_.h -x 23 define SYSTIME_H;
	echogs -a gconfig_.h -x 23 define DIRENT_H;

# ----------------------------- Main program ------------------------------ #

BEGINFILES=
CCBEGIN=$(CCC) *.c

# Interpreter main program

$(GS)$(XE): ld.tr echogs$(XE) gs.$(OBJ) $(INT_ALL) $(LIB_ALL) $(DEVS_ALL)
	cp ld.tr _temp_
	echo $(EXTRALIBS) -lm >>_temp_
	$(CC) $(LDFLAGS) $(XLIBDIRS) -o $(GS) gs.$(OBJ) @_temp_
	strip $(GS)
	coff2exe $(GS)  
	del $(GS)  
