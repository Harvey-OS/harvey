#    Copyright (C) 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
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

# Makefile for compiling PostScript Type 1 fonts into C.
# For more information about fonts, consult the Fontmap file,
# and also fonts.mak and fonts.doc.

CFONTS=.
FONTS=fonts
FONT2C=font2c

cfonts: AvantGarde_c Bookman_c CharterBT_c Courier_c Cyrillic_c Helvetica_c \
	Kana_c NewCenturySchlbk_c Palatino_c Symbol_c TimesRoman_c \
	Utopia_c ZapfChancery_c ZapfDingbats_c

ofonts: AvantGarde_o Bookman_o CharterBT_o Courier_o Cyrillic_o Helvetica_o \
	Kana_o NewCenturySchlbk_o Palatino_o Symbol_o TimesRoman_o \
	Utopia_o ZapfChancery_o ZapfDingbats_o

# ---------------------------------------------------------------- #
#                                                                  #
#                   Compiling .gsf fonts into C                    #
#                                                                  #
# ---------------------------------------------------------------- #

# ---------------- Avant Garde ----------------

AvantGarde_c: $(CFONTS)/pagk.c $(CFONTS)/pagko.c $(CFONTS)/pagd.c \
	$(CFONTS)/pagdo.c

$(CFONTS)/pagk.c: $(FONTS)/pagk.gsf
	$(FONT2C) AvantGarde-Book $(CFONTS)/pagk.c agk

$(CFONTS)/pagko.c: $(FONTS)/pagko.gsf
	$(FONT2C) AvantGarde-BookOblique $(CFONTS)/pagko.c agko

$(CFONTS)/pagd.c: $(FONTS)/pagd.gsf
	$(FONT2C) AvantGarde-Demi $(CFONTS)/pagd.c agd

$(CFONTS)/pagdo.c: $(FONTS)/pagdo.gsf
	$(FONT2C) AvantGarde-DemiOblique $(CFONTS)/pagdo.c agdo

AvantGarde_o: pagk.$(OBJ) pagko.$(OBJ) pagd.$(OBJ) pagdo.$(OBJ)

pagk.$(OBJ): $(CFONTS)/pagk.c $(CCFONT)
	$(CCCF) $(CFONTS)/pagk.c

pagko.$(OBJ): $(CFONTS)/pagko.c $(CCFONT)
	$(CCCF) $(CFONTS)/pagko.c

pagd.$(OBJ): $(CFONTS)/pagd.c $(CCFONT)
	$(CCCF) $(CFONTS)/pagd.c

pagdo.$(OBJ): $(CFONTS)/pagdo.c $(CCFONT)
	$(CCCF) $(CFONTS)/pagdo.c

# ---------------- Bookman ----------------

Bookman_c: $(CFONTS)/pbkl.c $(CFONTS)/pbkli.c $(CFONTS)/pbkd.c \
	$(CFONTS)/pbkdi.c

$(CFONTS)/pbkl.c: $(FONTS)/pbkl.gsf
	$(FONT2C) Bookman-Light $(CFONTS)/pbkl.c bkl

$(CFONTS)/pbkli.c: $(FONTS)/pbkli.gsf
	$(FONT2C) Bookman-LightItalic $(CFONTS)/pbkli.c bkli

$(CFONTS)/pbkd.c: $(FONTS)/pbkd.gsf
	$(FONT2C) Bookman-Demi $(CFONTS)/pbkd.c bkd

$(CFONTS)/pbkdi.c: $(FONTS)/pbkdi.gsf
	$(FONT2C) Bookman-DemiItalic $(CFONTS)/pbkdi.c bkdi

Bookman_o: pbkl.$(OBJ) pbkli.$(OBJ) pbkd.$(OBJ) pbkdi.$(OBJ)

pbkl.$(OBJ): $(CFONTS)/pbkl.c $(CCFONT)
	$(CCCF) $(CFONTS)/pbkl.c

pbkli.$(OBJ): $(CFONTS)/pbkli.c $(CCFONT)
	$(CCCF) $(CFONTS)/pbkli.c

pbkd.$(OBJ): $(CFONTS)/pbkd.c $(CCFONT)
	$(CCCF) $(CFONTS)/pbkd.c

pbkdi.$(OBJ): $(CFONTS)/pbkdi.c $(CCFONT)
	$(CCCF) $(CFONTS)/pbkdi.c

# ---------------- Charter ----------------

CharterBT_c: $(CFONTS)/bchr.c $(CFONTS)/bchri.c $(CFONTS)/bchb.c \
	$(CFONTS)/bchbi.c

$(CFONTS)/bchr.c: $(FONTS)/bchr.pfa
	$(FONT2C) Charter-Roman $(CFONTS)/bchr.c chr

$(CFONTS)/bchri.c: $(FONTS)/bchri.pfa
	$(FONT2C) Charter-Italic $(CFONTS)/bchri.c chri

$(CFONTS)/bchb.c: $(FONTS)/bchb.pfa
	$(FONT2C) Charter-Bold $(CFONTS)/bchb.c chb

$(CFONTS)/bchbi.c: $(FONTS)/bchbi.pfa
	$(FONT2C) Charter-BoldItalic $(CFONTS)/bchbi.c chbi

CharterBT_o: bchr.$(OBJ) bchri.$(OBJ) bchb.$(OBJ) bchbi.$(OBJ)

bchr.$(OBJ): $(CFONTS)/bchr.c $(CCFONT)
	$(CCCF) $(CFONTS)/bchr.c

bchri.$(OBJ): $(CFONTS)/bchri.c $(CCFONT)
	$(CCCF) $(CFONTS)/bchri.c

bchb.$(OBJ): $(CFONTS)/bchb.c $(CCFONT)
	$(CCCF) $(CFONTS)/bchb.c

bchbi.$(OBJ): $(CFONTS)/bchbi.c $(CCFONT)
	$(CCCF) $(CFONTS)/bchbi.c

# ---------------- Cyrillic ----------------

Cyrillic_c: $(CFONTS)/fcyr.c $(CFONTS)/fcyri.c

$(CFONTS)/fcyr.c: $(FONTS)/fcyr.gsf
	$(FONT2C) Cyrillic $(CFONTS)/fcyr.c fcyr

$(CFONTS)/fcyri.c: $(FONTS)/fcyri.gsf
	$(FONT2C) Cyrillic-Italic $(CFONTS)/fcyri.c fcyri

Cyrillic_o: fcyr.$(OBJ) fcyri.$(OBJ)

fcyr.$(OBJ): $(CFONTS)/fcyr.c $(CCFONT)
	$(CCCF) $(CFONTS)/fcyr.c

fcyri.$(OBJ): $(CFONTS)/fcyri.c $(CCFONT)
	$(CCCF) $(CFONTS)/fcyri.c

# ---------------- Courier ----------------

Courier_c: $(CFONTS)/ncrr.c $(CFONTS)/ncrri.c $(CFONTS)/ncrb.c \
	$(CFONTS)/ncrbi.c

$(CFONTS)/ncrr.c: $(FONTS)/ncrr.pfa
	$(FONT2C) Courier $(CFONTS)/ncrr.c crr

$(CFONTS)/ncrri.c: $(FONTS)/ncrri.pfa
	$(FONT2C) Courier-Italic $(CFONTS)/ncrri.c cri

$(CFONTS)/ncrb.c: $(FONTS)/ncrb.pfa
	$(FONT2C) Courier-Bold $(CFONTS)/ncrb.c crb

$(CFONTS)/ncrbi.c: $(FONTS)/ncrbi.pfa
	$(FONT2C) Courier-Italic $(CFONTS)/ncrbi.c crbi

Courier_o: ncrr.$(OBJ) ncrri.$(OBJ) ncrb.$(OBJ) ncrbi.$(OBJ)

ncrr.$(OBJ): $(CFONTS)/ncrr.c $(CCFONT)
	$(CCCF) $(CFONTS)/ncrr.c

ncrri.$(OBJ): $(CFONTS)/ncrri.c $(CCFONT)
	$(CCCF) $(CFONTS)/ncrri.c

ncrb.$(OBJ): $(CFONTS)/ncrb.c $(CCFONT)
	$(CCCF) $(CFONTS)/ncrb.c

ncrbi.$(OBJ): $(CFONTS)/ncrbi.c $(CCFONT)
	$(CCCF) $(CFONTS)/ncrbi.c

# ---------------- Helvetica ----------------

Helvetica_c: $(CFONTS)/phvr.c $(CFONTS)/phvro.c $(CFONTS)/phvrrn.c \
	$(CFONTS)/phvb.c $(CFONTS)/phvbo.c

$(CFONTS)/phvr.c: $(FONTS)/phvr.gsf
	$(FONT2C) Helvetica $(CFONTS)/phvr.c hvr

$(CFONTS)/phvro.c: $(FONTS)/phvro.gsf
	$(FONT2C) Helvetica-Oblique $(CFONTS)/phvro.c hvro

$(CFONTS)/phvrrn.c: $(FONTS)/phvrrn.gsf
	$(FONT2C) Helvetica-Narrow $(CFONTS)/phvrrn.c hvrrn

$(CFONTS)/phvb.c: $(FONTS)/phvb.gsf
	$(FONT2C) Helvetica-Bold $(CFONTS)/phvb.c hvb

$(CFONTS)/phvbo.c: $(FONTS)/phvbo.gsf
	$(FONT2C) Helvetica-BoldOblique $(CFONTS)/phvbo.c hvbo

Helvetica_o: phvr.$(OBJ) phvro.$(OBJ) phvrrn.$(OBJ) phvb.$(OBJ) phvbo.$(OBJ)

phvr.$(OBJ): $(CFONTS)/phvr.c $(CCFONT)
	$(CCCF) $(CFONTS)/phvr.c

phvro.$(OBJ): $(CFONTS)/phvro.c $(CCFONT)
	$(CCCF) $(CFONTS)/phvro.c

phvrrn.$(OBJ): $(CFONTS)/phvrrn.c $(CCFONT)
	$(CCCF) $(CFONTS)/phvrrn.c

phvb.$(OBJ): $(CFONTS)/phvb.c $(CCFONT)
	$(CCCF) $(CFONTS)/phvb.c

phvbo.$(OBJ): $(CFONTS)/phvbo.c $(CCFONT)
	$(CCCF) $(CFONTS)/phvbo.c

# ---------------- Kana ----------------

Kana_c: $(CFONTS)/fhirw.c $(CFONTS)/fkarw.c

$(CFONTS)/fhirw.c: $(FONTS)/fhirw.gsf
	$(FONT2C) Calligraphic-Hiragana $(CFONTS)/fhirw.c fhirw

$(CFONTS)/fkarw.c: $(FONTS)/fkarw.gsf
	$(FONT2C) Calligraphic-Katakana $(CFONTS)/fkarw.c fkarw

Kana_o: fhirw.$(OBJ) fkarw.$(OBJ)

fhirw.$(OBJ): $(CFONTS)/fhirw.c $(CCFONT)
	$(CCCF) $(CFONTS)/fhirw.c

fkarw.$(OBJ): $(CFONTS)/fkarw.c $(CCFONT)
	$(CCCF) $(CFONTS)/fkarw.c

# ---------------- New Century Schoolbook ----------------

NewCenturySchlbk_c: $(CFONTS)/pncr.c $(CFONTS)/pncri.c $(CFONTS)/pncb.c \
	$(CFONTS)/pncbi.c

$(CFONTS)/pncr.c: $(FONTS)/pncr.gsf
	$(FONT2C) NewCenturySchlbk-Roman $(CFONTS)/pncr.c ncr

$(CFONTS)/pncri.c: $(FONTS)/pncri.gsf
	$(FONT2C) NewCenturySchlbk-Italic $(CFONTS)/pncri.c ncri

$(CFONTS)/pncb.c: $(FONTS)/pncb.gsf
	$(FONT2C) NewCenturySchlbk-Bold $(CFONTS)/pncb.c ncb

$(CFONTS)/pncbi.c: $(FONTS)/pncbi.gsf
	$(FONT2C) NewCenturySchlbk-BoldItalic $(CFONTS)/pncbi.c ncbi

NewCenturySchlbk_o: pncr.$(OBJ) pncri.$(OBJ) pncb.$(OBJ) pncbi.$(OBJ)

pncr.$(OBJ): $(CFONTS)/pncr.c $(CCFONT)
	$(CCCF) $(CFONTS)/pncr.c

pncri.$(OBJ): $(CFONTS)/pncri.c $(CCFONT)
	$(CCCF) $(CFONTS)/pncri.c

pncb.$(OBJ): $(CFONTS)/pncb.c $(CCFONT)
	$(CCCF) $(CFONTS)/pncb.c

pncbi.$(OBJ): $(CFONTS)/pncbi.c $(CCFONT)
	$(CCCF) $(CFONTS)/pncbi.c

# ---------------- Palatino ----------------

Palatino_c: $(CFONTS)/pplr.c $(CFONTS)/pplri.c $(CFONTS)/pplb.c \
	$(CFONTS)/pplbi.c

$(CFONTS)/pplr.c: $(FONTS)/pplr.gsf
	$(FONT2C) Palatino-Roman $(CFONTS)/pplr.c plr

$(CFONTS)/pplri.c: $(FONTS)/pplri.gsf
	$(FONT2C) Palatino-Italic $(CFONTS)/pplri.c plri

$(CFONTS)/pplb.c: $(FONTS)/pplb.gsf
	$(FONT2C) Palatino-Bold $(CFONTS)/pplb.c plb

$(CFONTS)/pplbi.c: $(FONTS)/pplbi.gsf
	$(FONT2C) Palatino-BoldItalic $(CFONTS)/pplbi.c plbi

Palatino_o: pplr.$(OBJ) pplri.$(OBJ) pplb.$(OBJ) pplbi.$(OBJ)

pplr.$(OBJ): $(CFONTS)/pplr.c $(CCFONT)
	$(CCCF) $(CFONTS)/pplr.c

pplri.$(OBJ): $(CFONTS)/pplri.c $(CCFONT)
	$(CCCF) $(CFONTS)/pplri.c

pplb.$(OBJ): $(CFONTS)/pplb.c $(CCFONT)
	$(CCCF) $(CFONTS)/pplb.c

pplbi.$(OBJ): $(CFONTS)/pplbi.c $(CCFONT)
	$(CCCF) $(CFONTS)/pplbi.c

# ---------------- Symbol ----------------

Symbol_c: $(CFONTS)/psyr.c

$(CFONTS)/psyr.c: $(FONTS)/psyr.gsf
	$(FONT2C) Symbol $(CFONTS)/psyr.c syr

Symbol_o: psyr.$(OBJ)

psyr.$(OBJ): $(CFONTS)/psyr.c $(CCFONT)
	$(CCCF) $(CFONTS)/psyr.c

# ---------------- Times Roman ----------------

TimesRoman_c: $(CFONTS)/ptmr.c $(CFONTS)/ptmri.c $(CFONTS)/ptmb.c \
	$(CFONTS)/ptmbi.c

$(CFONTS)/ptmr.c: $(FONTS)/ptmr.gsf
	$(FONT2C) Times-Roman $(CFONTS)/ptmr.c tmr

$(CFONTS)/ptmri.c: $(FONTS)/ptmri.gsf
	$(FONT2C) Times-Italic $(CFONTS)/ptmri.c tmri

$(CFONTS)/ptmb.c: $(FONTS)/ptmb.gsf
	$(FONT2C) Times-Bold $(CFONTS)/ptmb.c tmb

$(CFONTS)/ptmbi.c: $(FONTS)/ptmbi.gsf
	$(FONT2C) Times-BoldItalic $(CFONTS)/ptmbi.c tmbi

TimesRoman_o: ptmr.$(OBJ) ptmri.$(OBJ) ptmb.$(OBJ) ptmbi.$(OBJ)

ptmr.$(OBJ): $(CFONTS)/ptmr.c $(CCFONT)
	$(CCCF) $(CFONTS)/ptmr.c

ptmri.$(OBJ): $(CFONTS)/ptmri.c $(CCFONT)
	$(CCCF) $(CFONTS)/ptmri.c

ptmb.$(OBJ): $(CFONTS)/ptmb.c $(CCFONT)
	$(CCCF) $(CFONTS)/ptmb.c

ptmbi.$(OBJ): $(CFONTS)/ptmbi.c $(CCFONT)
	$(CCCF) $(CFONTS)/ptmbi.c

# ---------------- Utopia ----------------

Utopia_c: $(CFONTS)/putr.c $(CFONTS)/putri.c $(CFONTS)/putb.c \
	$(CFONTS)/putbi.c

$(CFONTS)/putr.c: $(FONTS)/putr.gsf
	$(FONT2C) Utopia-Regular $(CFONTS)/putr.c utr

$(CFONTS)/putri.c: $(FONTS)/putri.gsf
	$(FONT2C) Utopia-Italic $(CFONTS)/putri.c utri

$(CFONTS)/putb.c: $(FONTS)/putb.gsf
	$(FONT2C) Utopia-Bold $(CFONTS)/putb.c utb

$(CFONTS)/putbi.c: $(FONTS)/putbi.gsf
	$(FONT2C) Utopia-BoldItalic $(CFONTS)/putbi.c utbi

Utopia_o: putr.$(OBJ) putri.$(OBJ) putb.$(OBJ) putbi.$(OBJ)

putr.$(OBJ): $(CFONTS)/putr.c $(CCFONT)
	$(CCCF) $(CFONTS)/putr.c

putri.$(OBJ): $(CFONTS)/putri.c $(CCFONT)
	$(CCCF) $(CFONTS)/putri.c

putb.$(OBJ): $(CFONTS)/putb.c $(CCFONT)
	$(CCCF) $(CFONTS)/putb.c

putbi.$(OBJ): $(CFONTS)/putbi.c $(CCFONT)
	$(CCCF) $(CFONTS)/putbi.c

# ---------------- Zapf Chancery ----------------

ZapfChancery_c: $(CFONTS)/zcr.c $(CFONTS)/zcro.c $(CFONTS)/zcb.c

$(CFONTS)/zcr.c: $(FONTS)/zcr.gsf
	$(FONT2C) ZapfChancery $(CFONTS)/zcr.c zcr

$(CFONTS)/zcro.c: $(FONTS)/zcro.gsf
	$(FONT2C) ZapfChancery-Oblique $(CFONTS)/zcro.c zcro

$(CFONTS)/zcb.c: $(FONTS)/zcb.gsf
	$(FONT2C) ZapfChancery-Bold $(CFONTS)/zcb.c zcb

ZapfChancery_o: zcr.$(OBJ) zcro.$(OBJ) zcb.$(OBJ)

zcr.$(OBJ): $(CFONTS)/zcr.c $(CCFONT)
	$(CCCF) $(CFONTS)/zcr.c

zcro.$(OBJ): $(CFONTS)/zcro.c $(CCFONT)
	$(CCCF) $(CFONTS)/zcro.c

zcb.$(OBJ): $(CFONTS)/zcb.c $(CCFONT)
	$(CCCF) $(CFONTS)/zcb.c

# ---------------- Zapf Dingbats ----------------

ZapfDingbats_c: $(CFONTS)/pzdr.c

$(CFONTS)/pzdr.c: $(FONTS)/pzdr.gsf
	$(FONT2C) ZapfDingbats $(CFONTS)/pzdr.c zdr

ZapfDingbats_o: pzdr.$(OBJ)

pzdr.$(OBJ): $(CFONTS)/pzdr.c $(CCFONT)
	$(CCCF) $(CFONTS)/pzdr.c
