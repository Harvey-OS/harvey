#    Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: unixinst.mak,v 1.20.2.1 2002/02/01 05:53:50 raph Exp $
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
pdf2dsc pdf2ps pdfopt pf2afm pfbtopfa printafm \
ps2ascii ps2epsi ps2pdf ps2pdf12 ps2pdf13 ps2pdfwr ps2ps wftopfa \
fixmswrd.pl lprsetup.sh pj-gs.sh pv.sh sysvlp.sh unix-lpr.sh ;\
	do if ( test -f $(PSLIBDIR)/$$f ); then $(INSTALL_PROGRAM) $(PSLIBDIR)/$$f $(scriptdir); fi;\
	done'

PSDOCDIR=$(PSLIBDIR)/../doc
PSEXDIR=$(PSLIBDIR)/../examples
PSMANDIR=$(PSLIBDIR)/../man

install-data: install-libdata install-doc install-man install-examples

# There's no point in providing a complete dependency list: we include
# one file from each subdirectory just as a sanity check.

install-libdata: 
	-mkdir $(datadir)
	-mkdir $(gsdir)
	-mkdir $(gsdatadir)
	-mkdir $(gsdatadir)/lib
	$(SH) -c 'for f in \
Fontmap Fontmap.GS \
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
pdf2dsc.ps pdfopt.ps ;\
	do if ( test -f $(PSLIBDIR)/$$f ); then $(INSTALL_DATA) $(PSLIBDIR)/$$f $(gsdatadir)/lib; fi;\
	done'
	$(SH) -c 'for f in $(PSLIBDIR)/gs_*.ps $(PSLIBDIR)/pdf*.ps;\
	do $(INSTALL_DATA) $$f $(gsdatadir)/lib ;\
	done'
	$(SH) -c 'for f in $(PSLIBDIR)/*.ppd $(PSLIBDIR)/*.rpd $(PSLIBDIR)/*.upp $(PSLIBDIR)/*.xbm $(PSLIBDIR)/*.xpm;\
	do $(INSTALL_DATA) $$f $(gsdatadir)/lib ;\
	done'

# install html documentation
DOC_PAGES=PUBLIC README index.html gs.css \
	   API.htm Bug-form.htm Bug-info.htm \
	   C-style.htm Changes.htm Commprod.htm Copying.htm \
	   Current.htm DLL.htm Develop.htm Devices.htm Drivers.htm \
	   Fonts.htm Helpers.htm Hershey.htm \
	   History1.htm History2.htm History3.htm History4.htm \
	   History5.htm History6.htm History7.htm \
	   Htmstyle.htm Humor.htm Install.htm Issues.htm Language.htm \
	   Lib.htm Maintain.htm Make.htm New-user.htm \
	   News.htm Projects.htm Ps-style.htm Ps2epsi.htm Ps2pdf.htm \
	   Psfiles.htm Public.htm Readme.htm Release.htm \
	   Source.htm Tester.htm Unix-lpr.htm Use.htm Xfonts.htm
install-doc: $(PSDOCDIR)/News.htm
	-mkdir $(docdir)
	$(SH) -c 'for f in $(DOC_PAGES) ;\
	do if ( test -f $(PSDOCDIR)/$$f ); then $(INSTALL_DATA) $(PSDOCDIR)/$$f $(docdir); fi;\
	done'

# install the man pages for each locale
MAN_LCDIRS=. de
MAN1_LINKS_PS2PS=eps2eps
MAN1_LINKS_PS2PDF=ps2pdf12 ps2pdf13
MAN1_LINKS_GSLP=gsbj gsdj gsdj500 gslj
install-man: $(PSMANDIR)/gs.1
	$(SH) -c 'test -d $(mandir) || mkdir $(mandir)'
	$(SH) -c 'for d in $(MAN_LCDIRS) ;\
	do man1dir=$(mandir)/$$d/man$(man1ext) ;\
	  ( test -d $$man1dir || mkdir -p $$man1dir ) ;\
	  for f in $(PSMANDIR)/$$d/*.1 ;\
	    do $(INSTALL_DATA) $$f $$man1dir ;\
	    if ( test -f $$man1dir/ps2ps.$(man1ext) ) ;\
	      then for f in $(MAN1_LINKS_PS2PS) ;\
	        do rm -f $$man1dir/$$f.$(man1ext) ;\
		ln -s $$man1dir/ps2ps.$(man1ext) $$man1dir/$$f.$(man1ext) ;\
	      done ;\
	    fi ;\
	    if ( test -f $$man1dir/ps2pdf.$(man1ext) ) ;\
	      then for f in $(MAN1_LINKS_PS2PDF) ;\
	        do rm -f $$man1dir/$$f.$(man1ext) ;\
	        ln -s $$man1dir/ps2pdf.$(man1ext) $$man1dir/$$f.$(man1ext) ;\
 	      done ;\
	    fi ;\
	    if ( test -f $$man1dir/gslp.$(man1ext) ) ;\
	      then for f in $(MAN1_LINKS_GSLP) ;\
	        do rm -f $$man1dir/$$f.$(man1ext) ;\
	        ln -s $$man1dir/gslp.$(man1ext) $$man1dir/$$f.$(man1ext) ;\
 	      done ;\
 	    fi ;\
	  done ;\
	done'

# install the example files
install-examples:
	-mkdir $(exdir)
	for f in \
alphabet.ps chess.ps colorcir.ps doretree.ps escher.ps \
golfer.ps grayalph.ps snowflak.ps tiger.ps vasarely.ps waterfal.ps \
ridt91.eps ;\
	do $(INSTALL_DATA) $(PSEXDIR)/$$f $(exdir) ;\
	done
