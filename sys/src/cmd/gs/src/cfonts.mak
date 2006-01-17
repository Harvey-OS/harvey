#    Copyright (C) 1992, 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.
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

# $Id: cfonts.mak,v 1.5 2004/10/26 02:50:56 giles Exp $
# Makefile for compiling PostScript Type 1 fonts into C.
# For more information about fonts, consult the Fontmap file,
# and also Fonts.htm.
# Users of this makefile must define the following:
#	PSSRCDIR - the source directory holding ccfont.h
#	PSGENDIR - the directory for files generated during building
#	PSOBJDIR - the object code directory

# Define the name of this makefile.
CFONTS_MAK=$(PSSRC)cfonts.mak

# ---------------- End of editable definitions ---------------- #

#CCFONT is defined in int.mak

CFGENDIR=$(PSGENDIR)
CFOBJDIR=$(PSOBJDIR)

CFGEN=$(CFGENDIR)$(D)
CFOBJ=$(CFOBJDIR)$(D)

CFCC=$(CC_) $(I_)$(PSSRCDIR)$(_I) $(I_)$(PSOBJDIR)$(_I)
CFO_=$(O_)$(CFOBJ)

# Define how to invoke the font2c program.
F2CTMP=$(PSGEN)font2c.tmp
F2CDEP=$(MAKEFILE) $(F2CTMP)

$(F2CTMP) : $(MAKEFILE) $(CFONTS_MAK) $(ECHOGS_XE)
	$(EXP)$(ECHOGS_XE) -w $(F2CTMP) -l -q -u -DNODISPLAY -s -u -DWRITESYSTEMDICT
	$(EXP)$(ECHOGS_XE) -a $(F2CTMP) - -- $(PSLIBDIR)$(D)font2c.ps

FONT2C=$(BUILD_TIME_GS) @$(F2CTMP)

# ---------------------------------------------------------------- #

# This file supports two slightly different font sets:
# the de facto commercial standard set of 35 PostScript fonts, and a slightly
# larger set distributed with the free version of the software.

fonts_standard_o : \
AvantGarde_o Bookman_o Courier_o \
Helvetica_o NewCenturySchlbk_o Palatino_o \
TimesRoman_o Symbol_o ZapfChancery_o ZapfDingbats_o
	$(NO_OP)

fonts_standard_c : \
AvantGarde_c Bookman_c Courier_c \
Helvetica_c NewCenturySchlbk_c Palatino_c \
TimesRoman_c Symbol_c ZapfChancery_c ZapfDingbats_c
	$(NO_OP)

fonts_free_o : fonts_standard_o \
CharterBT_o Cyrillic_o Kana_o Utopia_o
	$(NO_OP)

fonts_free_c : fonts_standard_c \
CharterBT_c Cyrillic_c Kana_c Utopia_c
	$(NO_OP)

# ---------------------------------------------------------------- #
#                                                                  #
#                         Standard 35 fonts                        #
#                                                                  #
# ---------------------------------------------------------------- #

# By convention, the names of the 35 standard compiled fonts use '0' for
# the foundry name.  This allows users to substitute different foundries
# without having to change this makefile.

# ---------------- Avant Garde ----------------

AvantGarde_c : $(CFGEN)0agk.c $(CFGEN)0agko.c $(CFGEN)0agd.c $(CFGEN)0agdo.c
	$(NO_OP)

$(CFGEN)0agk.c : $(F2CDEP)
	$(FONT2C) $(Q)AvantGarde-Book$(Q) $(CFGEN)0agk.c agk

$(CFGEN)0agko.c : $(F2CDEP)
	$(FONT2C) $(Q)AvantGarde-BookOblique$(Q) $(CFGEN)0agko.c agko

$(CFGEN)0agd.c : $(F2CDEP)
	$(FONT2C) $(Q)AvantGarde-Demi$(Q) $(CFGEN)0agd.c agd

$(CFGEN)0agdo.c : $(F2CDEP)
	$(FONT2C) $(Q)AvantGarde-DemiOblique$(Q) $(CFGEN)0agdo.c agdo

AvantGarde_o : $(CFOBJ)0agk.$(OBJ) $(CFOBJ)0agko.$(OBJ) $(CFOBJ)0agd.$(OBJ) $(CFOBJ)0agdo.$(OBJ)
	$(NO_OP)

$(CFOBJ)0agk.$(OBJ) : $(CFGEN)0agk.c $(CCFONT)
	$(CFCC) $(CFO_)0agk.$(OBJ) $(C_) $(CFGEN)0agk.c

$(CFOBJ)0agko.$(OBJ) : $(CFGEN)0agko.c $(CCFONT)
	$(CFCC) $(CFO_)0agko.$(OBJ) $(C_) $(CFGEN)0agko.c

$(CFOBJ)0agd.$(OBJ) : $(CFGEN)0agd.c $(CCFONT)
	$(CFCC) $(CFO_)0agd.$(OBJ) $(C_) $(CFGEN)0agd.c

$(CFOBJ)0agdo.$(OBJ) : $(CFGEN)0agdo.c $(CCFONT)
	$(CFCC) $(CFO_)0agdo.$(OBJ) $(C_) $(CFGEN)0agdo.c

# ---------------- Bookman ----------------

Bookman_c : $(CFGEN)0bkl.c $(CFGEN)0bkli.c $(CFGEN)0bkd.c $(CFGEN)0bkdi.c
	$(NO_OP)

$(CFGEN)0bkl.c : $(F2CDEP)
	$(FONT2C) $(Q)Bookman-Light$(Q) $(CFGEN)0bkl.c bkl

$(CFGEN)0bkli.c : $(F2CDEP)
	$(FONT2C) $(Q)Bookman-LightItalic$(Q) $(CFGEN)0bkli.c bkli

$(CFGEN)0bkd.c : $(F2CDEP)
	$(FONT2C) $(Q)Bookman-Demi$(Q) $(CFGEN)0bkd.c bkd

$(CFGEN)0bkdi.c : $(F2CDEP)
	$(FONT2C) $(Q)Bookman-DemiItalic$(Q) $(CFGEN)0bkdi.c bkdi

Bookman_o : $(CFOBJ)0bkl.$(OBJ) $(CFOBJ)0bkli.$(OBJ) $(CFOBJ)0bkd.$(OBJ) $(CFOBJ)0bkdi.$(OBJ)
	$(NO_OP)

$(CFOBJ)0bkl.$(OBJ) : $(CFGEN)0bkl.c $(CCFONT)
	$(CFCC) $(CFO_)0bkl.$(OBJ) $(C_) $(CFGEN)0bkl.c

$(CFOBJ)0bkli.$(OBJ) : $(CFGEN)0bkli.c $(CCFONT)
	$(CFCC) $(CFO_)0bkli.$(OBJ) $(C_) $(CFGEN)0bkli.c

$(CFOBJ)0bkd.$(OBJ) : $(CFGEN)0bkd.c $(CCFONT)
	$(CFCC) $(CFO_)0bkd.$(OBJ) $(C_) $(CFGEN)0bkd.c

$(CFOBJ)0bkdi.$(OBJ) : $(CFGEN)0bkdi.c $(CCFONT)
	$(CFCC) $(CFO_)0bkdi.$(OBJ) $(C_) $(CFGEN)0bkdi.c

# ---------------- Courier ----------------

Courier_c : $(CFGEN)0crr.c $(CFGEN)0cri.c $(CFGEN)0crb.c $(CFGEN)0crbi.c
	$(NO_OP)

$(CFGEN)0crr.c : $(F2CDEP)
	$(FONT2C) $(Q)Courier$(Q) $(CFGEN)0crr.c crr

$(CFGEN)0cri.c : $(F2CDEP)
	$(FONT2C) $(Q)Courier-Italic$(Q) $(CFGEN)0cri.c cri

$(CFGEN)0crb.c : $(F2CDEP)
	$(FONT2C) $(Q)Courier-Bold$(Q) $(CFGEN)0crb.c crb

$(CFGEN)0crbi.c : $(F2CDEP)
	$(FONT2C) $(Q)Courier-BoldItalic$(Q) $(CFGEN)0crbi.c crbi

Courier_o : $(CFOBJ)0crr.$(OBJ) $(CFOBJ)0cri.$(OBJ) $(CFOBJ)0crb.$(OBJ) $(CFOBJ)0crbi.$(OBJ)
	$(NO_OP)

$(CFOBJ)0crr.$(OBJ) : $(CFGEN)0crr.c $(CCFONT)
	$(CFCC) $(CFO_)0crr.$(OBJ) $(C_) $(CFGEN)0crr.c

$(CFOBJ)0cri.$(OBJ) : $(CFGEN)0cri.c $(CCFONT)
	$(CFCC) $(CFO_)0cri.$(OBJ) $(C_) $(CFGEN)0cri.c

$(CFOBJ)0crb.$(OBJ) : $(CFGEN)0crb.c $(CCFONT)
	$(CFCC) $(CFO_)0crb.$(OBJ) $(C_) $(CFGEN)0crb.c

$(CFOBJ)0crbi.$(OBJ) : $(CFGEN)0crbi.c $(CCFONT)
	$(CFCC) $(CFO_)0crbi.$(OBJ) $(C_) $(CFGEN)0crbi.c

# ---------------- Helvetica ----------------

Helvetica_c : $(CFGEN)0hvr.c $(CFGEN)0hvro.c \
$(CFGEN)0hvb.c $(CFGEN)0hvbo.c $(CFGEN)0hvrrn.c \
$(CFGEN)0hvrorn.c $(CFGEN)0hvbrn.c $(CFGEN)0hvborn.c
	$(NO_OP)

$(CFGEN)0hvr.c : $(F2CDEP)
	$(FONT2C) $(Q)Helvetica$(Q) $(CFGEN)0hvr.c hvr

$(CFGEN)0hvro.c : $(F2CDEP)
	$(FONT2C) $(Q)Helvetica-Oblique$(Q) $(CFGEN)0hvro.c hvro

$(CFGEN)0hvb.c : $(F2CDEP)
	$(FONT2C) $(Q)Helvetica-Bold$(Q) $(CFGEN)0hvb.c hvb

$(CFGEN)0hvbo.c : $(F2CDEP)
	$(FONT2C) $(Q)Helvetica-BoldOblique$(Q) $(CFGEN)0hvbo.c hvbo

$(CFGEN)0hvrrn.c : $(F2CDEP)
	$(FONT2C) $(Q)Helvetica-Narrow$(Q) $(CFGEN)0hvrrn.c hvrrn

$(CFGEN)0hvrorn.c : $(F2CDEP)
	$(FONT2C) $(Q)Helvetica-Narrow-Oblique$(Q) $(CFGEN)0hvrorn.c hvrorn

$(CFGEN)0hvbrn.c : $(F2CDEP)
	$(FONT2C) $(Q)Helvetica-Narrow-Bold$(Q) $(CFGEN)0hvbrn.c hvbrn

$(CFGEN)0hvborn.c : $(F2CDEP)
	$(FONT2C) $(Q)Helvetica-Narrow-BoldOblique$(Q) $(CFGEN)0hvborn.c hvborn

Helvetica_o : $(CFOBJ)0hvr.$(OBJ) $(CFOBJ)0hvro.$(OBJ) $(CFOBJ)0hvb.$(OBJ) $(CFOBJ)0hvbo.$(OBJ) \
$(CFOBJ)0hvrrn.$(OBJ) $(CFOBJ)0hvrorn.$(OBJ) $(CFOBJ)0hvbrn.$(OBJ) $(CFOBJ)0hvborn.$(OBJ)
	$(NO_OP)

$(CFOBJ)0hvr.$(OBJ) : $(CFGEN)0hvr.c $(CCFONT)
	$(CFCC) $(CFO_)0hvr.$(OBJ) $(C_) $(CFGEN)0hvr.c

$(CFOBJ)0hvro.$(OBJ) : $(CFGEN)0hvro.c $(CCFONT)
	$(CFCC) $(CFO_)0hvro.$(OBJ) $(C_) $(CFGEN)0hvro.c

$(CFOBJ)0hvb.$(OBJ) : $(CFGEN)0hvb.c $(CCFONT)
	$(CFCC) $(CFO_)0hvb.$(OBJ) $(C_) $(CFGEN)0hvb.c

$(CFOBJ)0hvbo.$(OBJ) : $(CFGEN)0hvbo.c $(CCFONT)
	$(CFCC) $(CFO_)0hvbo.$(OBJ) $(C_) $(CFGEN)0hvbo.c

$(CFOBJ)0hvrrn.$(OBJ) : $(CFGEN)0hvrrn.c $(CCFONT)
	$(CFCC) $(CFO_)0hvrrn.$(OBJ) $(C_) $(CFGEN)0hvrrn.c

$(CFOBJ)0hvrorn.$(OBJ) : $(CFGEN)0hvrorn.c $(CCFONT)
	$(CFCC) $(CFO_)0hvrorn.$(OBJ) $(C_) $(CFGEN)0hvrorn.c

$(CFOBJ)0hvbrn.$(OBJ) : $(CFGEN)0hvbrn.c $(CCFONT)
	$(CFCC) $(CFO_)0hvbrn.$(OBJ) $(C_) $(CFGEN)0hvbrn.c

$(CFOBJ)0hvborn.$(OBJ) : $(CFGEN)0hvborn.c $(CCFONT)
	$(CFCC) $(CFO_)0hvborn.$(OBJ) $(C_) $(CFGEN)0hvborn.c

# ---------------- New Century Schoolbook ----------------

NewCenturySchlbk_c : $(CFGEN)0ncr.c $(CFGEN)0ncri.c $(CFGEN)0ncb.c \
$(CFGEN)0ncbi.c
	$(NO_OP)

$(CFGEN)0ncr.c : $(F2CDEP)
	$(FONT2C) $(Q)NewCenturySchlbk-Roman$(Q) $(CFGEN)0ncr.c ncr

$(CFGEN)0ncri.c : $(F2CDEP)
	$(FONT2C) $(Q)NewCenturySchlbk-Italic$(Q) $(CFGEN)0ncri.c ncri

$(CFGEN)0ncb.c : $(F2CDEP)
	$(FONT2C) $(Q)NewCenturySchlbk-Bold$(Q) $(CFGEN)0ncb.c ncb

$(CFGEN)0ncbi.c : $(F2CDEP)
	$(FONT2C) $(Q)NewCenturySchlbk-BoldItalic$(Q) $(CFGEN)0ncbi.c ncbi

NewCenturySchlbk_o : $(CFOBJ)0ncr.$(OBJ) $(CFOBJ)0ncri.$(OBJ) $(CFOBJ)0ncb.$(OBJ) $(CFOBJ)0ncbi.$(OBJ)
	$(NO_OP)

$(CFOBJ)0ncr.$(OBJ) : $(CFGEN)0ncr.c $(CCFONT)
	$(CFCC) $(CFO_)0ncr.$(OBJ) $(C_) $(CFGEN)0ncr.c

$(CFOBJ)0ncri.$(OBJ) : $(CFGEN)0ncri.c $(CCFONT)
	$(CFCC) $(CFO_)0ncri.$(OBJ) $(C_) $(CFGEN)0ncri.c

$(CFOBJ)0ncb.$(OBJ) : $(CFGEN)0ncb.c $(CCFONT)
	$(CFCC) $(CFO_)0ncb.$(OBJ) $(C_) $(CFGEN)0ncb.c

$(CFOBJ)0ncbi.$(OBJ) : $(CFGEN)0ncbi.c $(CCFONT)
	$(CFCC) $(CFO_)0ncbi.$(OBJ) $(C_) $(CFGEN)0ncbi.c

# ---------------- Palatino ----------------

Palatino_c : $(CFGEN)0plr.c $(CFGEN)0plri.c $(CFGEN)0plb.c $(CFGEN)0plbi.c
	$(NO_OP)

$(CFGEN)0plr.c : $(F2CDEP)
	$(FONT2C) $(Q)Palatino-Roman$(Q) $(CFGEN)0plr.c plr

$(CFGEN)0plri.c : $(F2CDEP)
	$(FONT2C) $(Q)Palatino-Italic$(Q) $(CFGEN)0plri.c plri

$(CFGEN)0plb.c : $(F2CDEP)
	$(FONT2C) $(Q)Palatino-Bold$(Q) $(CFGEN)0plb.c plb

$(CFGEN)0plbi.c : $(F2CDEP)
	$(FONT2C) $(Q)Palatino-BoldItalic$(Q) $(CFGEN)0plbi.c plbi

Palatino_o : $(CFOBJ)0plr.$(OBJ) $(CFOBJ)0plri.$(OBJ) $(CFOBJ)0plb.$(OBJ) $(CFOBJ)0plbi.$(OBJ)
	$(NO_OP)

$(CFOBJ)0plr.$(OBJ) : $(CFGEN)0plr.c $(CCFONT)
	$(CFCC) $(CFO_)0plr.$(OBJ) $(C_) $(CFGEN)0plr.c

$(CFOBJ)0plri.$(OBJ) : $(CFGEN)0plri.c $(CCFONT)
	$(CFCC) $(CFO_)0plri.$(OBJ) $(C_) $(CFGEN)0plri.c

$(CFOBJ)0plb.$(OBJ) : $(CFGEN)0plb.c $(CCFONT)
	$(CFCC) $(CFO_)0plb.$(OBJ) $(C_) $(CFGEN)0plb.c

$(CFOBJ)0plbi.$(OBJ) : $(CFGEN)0plbi.c $(CCFONT)
	$(CFCC) $(CFO_)0plbi.$(OBJ) $(C_) $(CFGEN)0plbi.c

# ---------------- Times Roman ----------------

TimesRoman_c : $(CFGEN)0tmr.c $(CFGEN)0tmri.c $(CFGEN)0tmb.c $(CFGEN)0tmbi.c
	$(NO_OP)

$(CFGEN)0tmr.c : $(F2CDEP)
	$(FONT2C) $(Q)Times-Roman$(Q) $(CFGEN)0tmr.c tmr

$(CFGEN)0tmri.c : $(F2CDEP)
	$(FONT2C) $(Q)Times-Italic$(Q) $(CFGEN)0tmri.c tmri

$(CFGEN)0tmb.c : $(F2CDEP)
	$(FONT2C) $(Q)Times-Bold$(Q) $(CFGEN)0tmb.c tmb

$(CFGEN)0tmbi.c : $(F2CDEP)
	$(FONT2C) $(Q)Times-BoldItalic$(Q) $(CFGEN)0tmbi.c tmbi

TimesRoman_o : $(CFOBJ)0tmr.$(OBJ) $(CFOBJ)0tmri.$(OBJ) $(CFOBJ)0tmb.$(OBJ) $(CFOBJ)0tmbi.$(OBJ)
	$(NO_OP)

$(CFOBJ)0tmr.$(OBJ) : $(CFGEN)0tmr.c $(CCFONT)
	$(CFCC) $(CFO_)0tmr.$(OBJ) $(C_) $(CFGEN)0tmr.c

$(CFOBJ)0tmri.$(OBJ) : $(CFGEN)0tmri.c $(CCFONT)
	$(CFCC) $(CFO_)0tmri.$(OBJ) $(C_) $(CFGEN)0tmri.c

$(CFOBJ)0tmb.$(OBJ) : $(CFGEN)0tmb.c $(CCFONT)
	$(CFCC) $(CFO_)0tmb.$(OBJ) $(C_) $(CFGEN)0tmb.c

$(CFOBJ)0tmbi.$(OBJ) : $(CFGEN)0tmbi.c $(CCFONT)
	$(CFCC) $(CFO_)0tmbi.$(OBJ) $(C_) $(CFGEN)0tmbi.c

# ---------------- Symbol ----------------

Symbol_c : $(CFGEN)0syr.c
	$(NO_OP)

$(CFGEN)0syr.c : $(F2CDEP)
	$(FONT2C) $(Q)Symbol$(Q) $(CFGEN)0syr.c syr

Symbol_o : $(CFOBJ)0syr.$(OBJ)
	$(NO_OP)

$(CFOBJ)0syr.$(OBJ) : $(CFGEN)0syr.c $(CCFONT)
	$(CFCC) $(CFO_)0syr.$(OBJ) $(C_) $(CFGEN)0syr.c

# ---------------- Zapf Chancery ----------------

ZapfChancery_c : $(CFGEN)0zcmi.c
	$(NO_OP)

$(CFGEN)0zcmi.c : $(F2CDEP)
	$(FONT2C) $(Q)ZapfChancery-MediumItalic$(Q) $(CFGEN)0zcmi.c zcmi

ZapfChancery_o : $(CFOBJ)0zcmi.$(OBJ)
	$(NO_OP)

$(CFOBJ)0zcmi.$(OBJ) : $(CFGEN)0zcmi.c $(CCFONT)
	$(CFCC) $(CFO_)0zcmi.$(OBJ) $(C_) $(CFGEN)0zcmi.c

# ---------------- Zapf Dingbats ----------------

ZapfDingbats_c : $(CFGEN)0zdr.c
	$(NO_OP)

$(CFGEN)0zdr.c : $(F2CDEP)
	$(FONT2C) $(Q)ZapfDingbats$(Q) $(CFGEN)0zdr.c zdr

ZapfDingbats_o : $(CFOBJ)0zdr.$(OBJ)
	$(NO_OP)

$(CFOBJ)0zdr.$(OBJ) : $(CFGEN)0zdr.c $(CCFONT)
	$(CFCC) $(CFO_)0zdr.$(OBJ) $(C_) $(CFGEN)0zdr.c

# ---------------------------------------------------------------- #
#                                                                  #
#                         Additional fonts                         #
#                                                                  #
# ---------------------------------------------------------------- #

# ---------------- Bitstream Charter ----------------

CharterBT_c : $(CFGEN)bchr.c $(CFGEN)bchri.c $(CFGEN)bchb.c $(CFGEN)bchbi.c
	$(NO_OP)

$(CFGEN)bchr.c : $(F2CDEP)
	$(FONT2C) $(Q)Charter-Roman$(Q) $(CFGEN)bchr.c chr

$(CFGEN)bchri.c : $(F2CDEP)
	$(FONT2C) $(Q)Charter-Italic$(Q) $(CFGEN)bchri.c chri

$(CFGEN)bchb.c : $(F2CDEP)
	$(FONT2C) $(Q)Charter-Bold$(Q) $(CFGEN)bchb.c chb

$(CFGEN)bchbi.c : $(F2CDEP)
	$(FONT2C) $(Q)Charter-BoldItalic$(Q) $(CFGEN)bchbi.c chbi

CharterBT_o : $(CFOBJ)bchr.$(OBJ) $(CFOBJ)bchri.$(OBJ) $(CFOBJ)bchb.$(OBJ) $(CFOBJ)bchbi.$(OBJ)
	$(NO_OP)

$(CFOBJ)bchr.$(OBJ) : $(CFGEN)bchr.c $(CCFONT)
	$(CFCC) $(CFO_)bchr.$(OBJ) $(C_) $(CFGEN)bchr.c

$(CFOBJ)bchri.$(OBJ) : $(CFGEN)bchri.c $(CCFONT)
	$(CFCC) $(CFO_)bchri.$(OBJ) $(C_) $(CFGEN)bchri.c

$(CFOBJ)bchb.$(OBJ) : $(CFGEN)bchb.c $(CCFONT)
	$(CFCC) $(CFO_)bchb.$(OBJ) $(C_) $(CFGEN)bchb.c

$(CFOBJ)bchbi.$(OBJ) : $(CFGEN)bchbi.c $(CCFONT)
	$(CFCC) $(CFO_)bchbi.$(OBJ) $(C_) $(CFGEN)bchbi.c

# ---------------- Cyrillic ----------------

Cyrillic_c : $(CFGEN)fcyr.c $(CFGEN)fcyri.c
	$(NO_OP)

$(CFGEN)fcyr.c : $(F2CDEP)
	$(FONT2C) $(Q)Cyrillic$(Q) $(CFGEN)fcyr.c fcyr

$(CFGEN)fcyri.c : $(F2CDEP)
	$(FONT2C) $(Q)Cyrillic-Italic$(Q) $(CFGEN)fcyri.c fcyri

Cyrillic_o : $(CFOBJ)fcyr.$(OBJ) $(CFOBJ)fcyri.$(OBJ)
	$(NO_OP)

$(CFOBJ)fcyr.$(OBJ) : $(CFGEN)fcyr.c $(CCFONT)
	$(CFCC) $(CFO_)fcyr.$(OBJ) $(C_) $(CFGEN)fcyr.c

$(CFOBJ)fcyri.$(OBJ) : $(CFGEN)fcyri.c $(CCFONT)
	$(CFCC) $(CFO_)fcyri.$(OBJ) $(C_) $(CFGEN)fcyri.c

# ---------------- Kana ----------------

Kana_c : $(CFGEN)fhirw.c $(CFGEN)fkarw.c
	$(NO_OP)

$(CFGEN)fhirw.c : $(F2CDEP)
	$(FONT2C) $(Q)Calligraphic-Hiragana$(Q) $(CFGEN)fhirw.c fhirw

$(CFGEN)fkarw.c : $(F2CDEP)
	$(FONT2C) $(Q)Calligraphic-Katakana$(Q) $(CFGEN)fkarw.c fkarw

Kana_o : $(CFOBJ)fhirw.$(OBJ) $(CFOBJ)fkarw.$(OBJ)
	$(NO_OP)

$(CFOBJ)fhirw.$(OBJ) : $(CFGEN)fhirw.c $(CCFONT)
	$(CFCC) $(CFO_)fhirw.$(OBJ) $(C_) $(CFGEN)fhirw.c

$(CFOBJ)fkarw.$(OBJ) : $(CFGEN)fkarw.c $(CCFONT)
	$(CFCC) $(CFO_)fkarw.$(OBJ) $(C_) $(CFGEN)fkarw.c

# ---------------- Utopia ----------------

Utopia_c : $(CFGEN)putr.c $(CFGEN)putri.c $(CFGEN)putb.c $(CFGEN)putbi.c
	$(NO_OP)

$(CFGEN)putr.c : $(F2CDEP)
	$(FONT2C) $(Q)Utopia-Regular$(Q) $(CFGEN)putr.c utr

$(CFGEN)putri.c : $(F2CDEP)
	$(FONT2C) $(Q)Utopia-Italic$(Q) $(CFGEN)putri.c utri

$(CFGEN)putb.c : $(F2CDEP)
	$(FONT2C) $(Q)Utopia-Bold$(Q) $(CFGEN)putb.c utb

$(CFGEN)putbi.c : $(F2CDEP)
	$(FONT2C) $(Q)Utopia-BoldItalic$(Q) $(CFGEN)putbi.c utbi

Utopia_o : $(CFOBJ)putr.$(OBJ) $(CFOBJ)putri.$(OBJ) $(CFOBJ)putb.$(OBJ) $(CFOBJ)putbi.$(OBJ)
	$(NO_OP)

$(CFOBJ)putr.$(OBJ) : $(CFGEN)putr.c $(CCFONT)
	$(CFCC) $(CFO_)putr.$(OBJ) $(C_) $(CFGEN)putr.c

$(CFOBJ)putri.$(OBJ) : $(CFGEN)putri.c $(CCFONT)
	$(CFCC) $(CFO_)putri.$(OBJ) $(C_) $(CFGEN)putri.c

$(CFOBJ)putb.$(OBJ) : $(CFGEN)putb.c $(CCFONT)
	$(CFCC) $(CFO_)putb.$(OBJ) $(C_) $(CFGEN)putb.c

$(CFOBJ)putbi.$(OBJ) : $(CFGEN)putbi.c $(CCFONT)
	$(CFCC) $(CFO_)putbi.$(OBJ) $(C_) $(CFGEN)putbi.c
