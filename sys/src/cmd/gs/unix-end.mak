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

# Partial makefile common to all Unix and Desqview/X configurations.

# This is the very last part of the makefile for these configurations.
# Since Unix make doesn't have an 'include' facility, we concatenate
# the various parts of the makefile together by brute force (in tar_cat).

# Installation

TAGS:
	etags -t *.c *.h

install: install-exec install-data

install-exec: $(GS)
	-mkdir $(bindir)
	$(INSTALL_PROGRAM) $(GS) $(bindir)/$(GS)
	-mkdir $(scriptdir)
	for f in gsbj gsdj gsdj500 gslj gslp gsnd bdftops font2c \
ps2ascii ps2epsi ;\
	do $(INSTALL_PROGRAM) $$f $(scriptdir)/$$f ;\
	done

install-data: gs.1
	-mkdir $(mandir)
	-mkdir $(man1dir)
	for f in gs ps2epsi ;\
	do $(INSTALL_DATA) $$f.1 $(man1dir)/$$f.$(manext) ;\
	done
	-mkdir $(datadir)
	-mkdir $(gsdir)
	-mkdir $(gsdatadir)
	for f in gslp.ps gs_init.ps gs_btokn.ps gs_ccfnt.ps gs_dps1.ps \
gs_fonts.ps gs_kanji.ps gs_lev2.ps gs_pfile.ps gs_res.ps \
gs_setpd.ps gs_statd.ps gs_type0.ps gs_type1.ps \
gs_dbt_e.ps gs_iso_e.ps gs_ksb_e.ps gs_std_e.ps gs_sym_e.ps \
quit.ps Fontmap bdftops.ps decrypt.ps font2c.ps impath.ps landscap.ps \
level1.ps packfile.ps prfont.ps printafm.ps ps2ascii.ps ps2epsi.ps \
ps2image.ps pstoppm.ps showpage.ps type1enc.ps type1ops.ps wrfont.ps ;\
	do $(INSTALL_DATA) $$f $(gsdatadir)/$$f ;\
	done
	-mkdir $(docdir)
	for f in COPYING NEWS PUBLIC README current.doc devices.doc \
drivers.doc fonts.doc gs.1 hershey.doc history1.doc history2.doc humor.doc \
language.doc lib.doc make.doc ps2epsi.1 ps2epsi.doc psfiles.doc use.doc \
xfonts.doc ;\
	do $(INSTALL_DATA) $$f $(docdir)/$$f ;\
	done
	-mkdir $(exdir)
	for f in chess.ps cheq.ps colorcir.ps golfer.ps escher.ps \
snowflak.ps tiger.ps ;\
	do $(INSTALL_DATA) $$f $(exdir)/$$f ;\
	done
