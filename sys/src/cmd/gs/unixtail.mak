#    Copyright (C) 1990, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
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

# Partial makefile common to all Unix configurations.

# This is the last part of the makefile for Unix configurations.
# Since Unix make doesn't have an 'include' facility, we concatenate
# the various parts of the makefile together by brute force (in tar_cat).

# The following prevents GNU make from constructing argument lists that
# include all environment variables, which can easily be longer than
# brain-damaged system V allows.

.NOEXPORT:

# -------------------------------- Library -------------------------------- #

## The Unix platforms

# We have to include a test for the existence of sys/time.h,
# because some System V platforms don't have it.

# Unix platforms other than System V, and also System V Release 4
# (SVR4) platforms.
unix__=gp_nofb.$(OBJ) gp_unix.$(OBJ) gp_unifs.$(OBJ) gp_unifn.$(OBJ) gdevpipe.$(OBJ)
unix_.dev: $(unix__)
	$(SETMOD) unix_ $(unix__)
	$(ADDMOD) unix_ -iodev pipe

gp_unix.$(OBJ): gp_unix.c $(AK) $(string__h) $(gx_h) $(gsexit_h) $(gp_h) \
  $(time__h)

gdevpipe.$(OBJ): gdevpipe.c $(AK) $(stdio__h) \
  $(gserror_h) $(gsmemory_h) $(gstypes_h) $(gxiodev_h) $(stream_h)

# System V platforms other than SVR4, which lack some system calls,
# but have pipes.
sysv__=gp_nofb.$(OBJ) gp_unix.$(OBJ) gp_unifs.$(OBJ) gp_unifn.$(OBJ) gp_sysv.$(OBJ) gdevpipe.$(OBJ)
sysv_.dev: $(sysv__)
	$(SETMOD) sysv_ $(sysv__)
	$(ADDMOD) sysv_ -iodev pipe

gp_sysv.$(OBJ): gp_sysv.c $(time__h) $(AK)

# -------------------------- Auxiliary programs --------------------------- #

ansi2knr$(XE): ansi2knr.c $(stdio__h) $(string__h) $(malloc__h)
	$(CCAUX) $(O)ansi2knr$(XE) ansi2knr.c

echogs$(XE): echogs.c
	$(CCAUX) $(O)echogs$(XE) echogs.c

# On the RS/6000 (at least), compiling genarch.c with gcc with -O
# produces a buggy executable.
genarch$(XE): genarch.c
	$(CCAUX) $(O)genarch$(XE) genarch.c

genconf$(XE): genconf.c
	$(CCAUX) $(O)genconf$(XE) genconf.c

# We need to query the environment to construct gconfig_.h.
INCLUDE=/usr/include
gconfig_.h: $(MAKEFILE) echogs$(XE)
	./echogs -w gconfig_.h -x 2f2a -s This file was generated automatically. -s -x 2a2f
	if ( test -f $(INCLUDE)/sys/time.h ) then ./echogs -a gconfig_.h -x 23 define SYSTIME_H;\
	fi
	if ( test -f $(INCLUDE)/dirent.h ) then ./echogs -a gconfig_.h -x 23 define DIRENT_H;\
	elif ( test -f $(INCLUDE)/sys/dir.h ) then ./echogs -a gconfig_.h -x 23 define SYSDIR_H;\
	elif ( test -f $(INCLUDE)/sys/ndir.h ) then ./echogs -a gconfig_.h -x 23 define SYSNDIR_H;\
	elif ( test -f $(INCLUDE)/ndir.h ) then ./echogs -a gconfig_.h -x 23 define NDIR_H;\
	fi

# ----------------------------- Main program ------------------------------ #

BEGINFILES=
CCBEGIN=$(CCC) *.c

# Interpreter main program

NONDEVS_ALL=gs.$(OBJ) gsmain.$(OBJ) $(INT_ALL) $(LIBGS) gconfig.$(OBJ)

$(GS)$(XE): ld.tr echogs $(NONDEVS_ALL) $(DEVS_ALL)
	./echogs -w ldt.tr -n - $(CCLD) $(LDFLAGS) $(XLIBDIRS) -o $(GS)$(XE)
	./echogs -a ldt.tr -n -s gs.$(OBJ) -s
	cat ld.tr >>ldt.tr
	./echogs -a ldt.tr -s - $(EXTRALIBS) -lm
	$(SH) <ldt.tr
