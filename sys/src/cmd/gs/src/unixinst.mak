#    Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: unixinst.mak,v 1.4 2000/03/14 06:14:44 lpd Exp $
# Partial makefile common to all Unix and Desqview/X configurations,
# containing the `install' targets.
# This is the very last part of the makefile for these configurations.

install: install-exec install-scripts install-data

# The sh -c in the rules below is required because Ultrix's implementation
# of sh -e terminates execution of a command if any error occurs, even if
# the command traps the error with ||.

# We include mkdirs for datadir, gsdir, and gsdatadir in all 3 install
# rules, just in case bindir or scriptdir is a subdirectory of any of these.

install-exec: $(GS_XE)
	-mkdir $(datadir)
	-mkdir $(gsdir)
	-mkdir $(gsdatadir)
	-mkdir $(bindir)
	$(INSTALL_PROGRAM) $(GS_XE) $(bindir)/$(GS)

install-scripts: $(PSLIBDIR)/gsnd
	-mkdir $(datadir)
	-mkdir $(gsdir)
	-mkdir $(gsdatadir)
	-mkdir $(scriptdir)
	$(SH) -c 'for f in \
gsbj gsdj gsdj500 gslj gslp gsnd \
bdftops dvipdf eps2eps font2c \
pdf2dsc pdf2ps pf2afm pfbtopfa printafm \
ps2ascii ps2epsi ps2pdf ps2pdf12 ps2pdf13 ps2pdfwr ps2ps wftopfa \
fixmswrd.pl lprsetup.sh pj-gs.sh pv.sh sysvlp.sh unix-lpr.sh ;\
	do if ( test -f $(PSLIBDIR)/$$f ); then $(INSTALL_PROGRAM) $(PSLIBDIR)/$$f $(scriptdir); fi;\
	done'

PSDOCDIR=$(PSLIBDIR)/../doc
PSEXDIR=$(PSLIBDIR)/../examples
PSMANDIR=$(PSLIBDIR)/../man

MAN1_PAGES=gs gslp gsnd dvipdf eps2eps font2c pdf2dsc pdf2ps \
	   pf2afm pfbtopfa printafm ps2ascii ps2epsi \
	   ps2pdf ps2pdfwr ps2ps wftopfa
MAN1_PS2PDF_LINKS=ps2pdf12 ps2pdf13
MAN1_GSLP_LINKS=gsbj gsdj gsdj500 gslj
# There's no point in providing a complete dependency list: we include
# one file from each subdirectory just as a sanity check.
install-data: $(PSDOCDIR)/Use.htm $(PSEXDIR)/golfer.ps $(PSMANDIR)/gs.1
	-mkdir $(mandir)
	-mkdir $(man1dir)
	$(SH) -c 'for f in $(MAN1_PAGES) ;\
	do if ( test -f $(PSMANDIR)/$$f.1 ); then $(INSTALL_DATA) $(PSMANDIR)/$$f.1 $(man1dir)/$$f.$(man1ext); fi;\
	done'
	$(SH) -c 'for f in $(MAN1_PS2PDF_LINKS) ;\
	do rm -f $(man1dir)/$$f.$(man1ext); ln -s ps2pdf.1 $(man1dir)/$$f.$(man1ext);\
	done'
	$(SH) -c 'for f in $(MAN1_GSLP_LINKS) ;\
	do rm -f $(man1dir)/$$f.$(man1ext); ln -s gslp.1 $(man1dir)/$$f.$(man1ext);\
	done'
	-mkdir $(datadir)
	-mkdir $(gsdir)
	-mkdir $(gsdatadir)
	-mkdir $(gsdatadir)/lib
	$(SH) -c 'for f in \
Fontmap Fontmap.GS \
gs_init.ps \
gs_btokn.ps gs_ccfnt.ps gs_cff.ps gs_cidfn.ps gs_cmap.ps gs_cmdl.ps \
gs_diskf.ps gs_dpnxt.ps gs_dps.ps gs_dps1.ps gs_dps2.ps gs_epsf.ps \
gs_fform.ps gs_fonts.ps gs_kanji.ps gs_lev2.ps gs_ll3.ps \
gs_pfile.ps gs_rdlin.ps gs_res.ps gs_setpd.ps gs_statd.ps \
gs_trap.ps gs_ttf.ps gs_typ32.ps gs_typ42.ps gs_type1.ps \
gs_ce_e.ps gs_dbt_e.ps gs_il1_e.ps gs_il2_e.ps gs_ksb_e.ps \
gs_std_e.ps gs_sym_e.ps gs_wl1_e.ps gs_wl2_e.ps gs_wl5_e.ps \
ht_ccsto.ps \
acctest.ps addxchar.ps align.ps bdftops.ps \
caption.ps cid2code.ps decrypt.ps docie.ps \
errpage.ps font2c.ps font2pcl.ps gslp.ps gsnup.ps image-qa.ps impath.ps \
jispaper.ps landscap.ps level1.ps lines.ps markhint.ps markpath.ps \
packfile.ps pcharstr.ps pf2afm.ps pfbtopfa.ps ppath.ps prfont.ps printafm.ps \
ps2ai.ps ps2ascii.ps ps2epsi.ps quit.ps rollconv.ps \
showchar.ps showpage.ps stcinfo.ps stcolor.ps stocht.ps \
traceimg.ps traceop.ps type1enc.ps type1ops.ps uninfo.ps unprot.ps \
viewcmyk.ps viewgif.ps viewjpeg.ps viewmiff.ps \
viewpcx.ps viewpbm.ps viewps2a.ps \
winmaps.ps wftopfa.ps wrfont.ps zeroline.ps \
gs_l2img.ps \
pdf2dsc.ps pdfeof.ps \
pdf_base.ps pdf_draw.ps pdf_font.ps pdf_main.ps pdf_ops.ps pdf_sec.ps \
gs_lgo_e.ps gs_lgx_e.ps gs_mex_e.ps gs_mgl_e.ps gs_mro_e.ps \
gs_pdf_e.ps gs_wan_e.ps \
gs_pdfwr.ps ;\
	do if ( test -f $(PSLIBDIR)/$$f ); then $(INSTALL_DATA) $(PSLIBDIR)/$$f $(gsdatadir)/lib; fi;\
	done'
	$(SH) -c 'for f in $(PSLIBDIR)/*.ppd $(PSLIBDIR)/*.upp $(PSLIBDIR)/*.xbm $(PSLIBDIR)/*.xpm;\
	do $(INSTALL_DATA) $$f $(gsdatadir)/lib ;\
	done'
	-mkdir $(docdir)
	$(SH) -c 'for f in \
PUBLIC README \
ps2epsi.txt \
Bug-form.htm C-style.htm Commprod.htm Copying.htm Current.htm \
DLL.htm Devices.htm Drivers.htm Fonts.htm \
Helpers.htm Hershey.htm \
History1.htm History2.htm History3.htm History4.htm History5.htm History6.htm \
Htmstyle.htm Humor.htm Install.htm Language.htm Lib.htm Make.htm New-user.htm \
News.htm Ps2pdf.htm Psfiles.htm Public.htm Readme.htm Release.htm \
Source.htm Tester.htm Unix-lpr.htm Use.htm Xfonts.htm ;\
	do if ( test -f $(PSDOCDIR)/$$f ); then $(INSTALL_DATA) $(PSDOCDIR)/$$f $(docdir); fi;\
	done'
	-mkdir $(exdir)
	for f in \
alphabet.ps chess.ps cheq.ps colorcir.ps doretree.ps escher.ps \
golfer.ps grayalph.ps snowflak.ps tiger.ps vasarely.ps waterfal.ps \
ridt91.eps ;\
	do $(INSTALL_DATA) $(PSEXDIR)/$$f $(exdir) ;\
	done
