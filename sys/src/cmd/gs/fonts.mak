#    Copyright (C) 1991, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
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

# Makefile for Ghostscript fonts.
# For more information about Ghostscript fonts, consult the Fontmap file.
 
# Define the top level directory.
DIR=/gs
DOSDIR=\gs

# Define the subdirectories.
AFM=$(DIR)/fonts/afm
BDF=$(DIR)/fonts/bdf
FONTS=$(DIR)/fonts
DOSFONTS=$(DOSDIR)\fonts
IMPORT=$(DIR)/fonts/import

# Uncomment whichever pair of the following lines is appropriate.
#GSBDF=gs386
#CDFONTS=$(DOSFONTS)
GSBDF=gs
CDFONTS=$(FONTS)

default: fonts

fonts: \
  AvantGarde Bookman Charter Courier Cyrillic \
  Helvetica Kana NewCenturySchlbk Palatino Symbol \
  Times Utopia ZapfChancery ZapfDingbats

# ----------------------------------------------------------------

# Each Ghostscript font has a UniqueID (an integer).  This is used
# to identify distinct fonts within the Ghostscript font machinery.
# The UniqueID values in this file are drawn from a block assigned to
# Aladdin Enterprises by Adobe, namely 5066501 to 5066580; the next
# available number in this block is 5066568.  (UniqueIDs 5066533 to
# 5066567 have been assigned to public domain fonts distributed by
# Aladdin that are derived from the public domain Hershey outlines.)

# If you create your own fonts, and are only going to use them within your
# own organization, you should use values between 4000000 and 4999999
# as described just below; if you are going to distribute fonts,
# call Adobe and get them to assign you some UniqueIDs and also an
# XUID for your organization.  The current (September 1993) UniqueID
# Coordinator is Terry O'Donnell; he is very helpful and will probably
# be able to assign you the numbers over the phone.

# XUIDs are a Level 2 PostScript feature that serves the same function
# as UniqueIDs, but is not limited to a single 24-bit integer.
# Ghostscript currently creates XUIDs as [-X- 0 -U-] where -X- is
# the organization XUID and -U- is the UniqueID.  If you are going to
# distribute fonts created by bdftops that have non-temporary UniqueIDs,
# change the the following line to your organization's XUID before
# you run bdftops.  (Aladdin Enterprises' organization XUID is 107;
# do not use this for your own fonts that you distribute.)

XUID=107

# The suggested temporary UniqueID for a font looks like:
#
# 4TTWVE0
#
# where TT is a two-digit number representing the typeface,
# W represents the weight (normal, bold, ...),
# V represents the variant (normal, italic, oblique, ...), and
# E represents the expansion (normal, condensed, ...).
# This scheme will not work forever.  As soon there are more 99
# typefaces, or more than 9 weights or variants, we will have to do
# something else. But it suffices for the near future.

# Ghostscript fonts are stored on files, and the file names must comply
# with the 8-character limit imposed by MS-DOS and other operating systems.
# We therefore construct the filename for a font in a way somewhat similar to
# the construction for temporary UniqueIDs:
#
# FTTWVVVE.gsf
#
# where F is the foundry, TT a two-letter abbreviation for the
# typeface, and W, V, and E the weight, variant, and expansion.  Since a
# font can have multiple variants, we allocate three letters to that
# (for example, Lucida Regular Sans Typewriter Italic).  If a font has
# four variants, you're on your own.  If a font does have multiple
# variants, it's best to add the expansion letter `r', so that it is
# clear which letters are variants and which the expansion.
#
# This scheme is very close to the one proposed in `Filenames for
# fonts', published in the first 1990 issue of TUGboat (the
# journal of the TeX Users Group).
#
# In the following tables, we made no attempt to be exhaustive.
# Instead, we have simply allocated entries for those things that we needed
# for the fonts that we are actually distributing.
#
#
# foundries:
# ----------------------------------------------------------------
# b = Bitstream
# f = freely distributable/public domain fonts
# n = IBM
# p = Adobe (`p' for PostScript)
#
#
#
# typefaces:
# id   name			  filename prefix
# ----------------------------------------------------------------
# 08 = Avant Garde		= pag		(Adobe)
# 11 = Bookman			= pbk		(Adobe)
# 01 = CharterBT		= bch		(Bitstream)
# 02 = Courier			= ncr		(IBM)
# 03 = Helvetica		= phv		(Adobe)
# 04 = New Century Schoolbook	= pnc		(Adobe)
# 09 = Palatino			= ppl		(Adobe)
# 05 = Symbol			= psy		(Adobe)
# 06 = Times			= ptm		(Adobe)
# --   Utopia			= put		(Adobe)
# 07 = Zapf Chancery		= zc		(public domain)
# 10 = Zapf Dingbats		= pzd		(Adobe)
# 12 = public domain Cyrillic	= fcy		(public domain)
# 13 = Kevin Hartig Hiragana	= fhi		(shareware)
# 14 = Kevin Hartig Katakana	= fka		(shareware)
#
# 90 = Hershey Gothic English	= hrge
# 91 = Hershey Gothic Italian	= hrit
# 92 = Hershey Gothic German	= hrgr
# 93 = Hershey Greek		= hrgk
# 94 = Hershey Plain		= hrpl
# 95 = Hershey Script		= hrsc
# 96 = Hershey Symbol		= hrsy
#
#
# weights:
# 0 = normal			= r
# 1 = bold			= b
# 2 = book			= k
# 3 = demi			= d
# 4 = light			= l
#
#
# variants:
# 0 = normal			= r (omitted when the weight is normal)
# 1 = italic			= i
# 2 = oblique			= o
# 3 = script/handwritten/swash	= w
#
#
# expansions:
# 0 = normal			= r (omitted when the weight and variant
#                                    are normal)
# 1 = narrow			= n
#
#

# ---------------------------------------------------------------- #
#                                                                  #
#            Converting BDF fonts to .gsf (Type 1) fonts           #
#                                                                  #
# ---------------------------------------------------------------- #

# The bdftops conversion program takes the following arguments:
#
#	bdftops xx.bdf [yy1.afm ...] zz.gsf fontname UniqueID [XUID]
#	  [encodingname]
#
# These arguments have the following meanings:
#
#	xx.bdf - the input bitmap file, a BDF file
#	yy*.afm - the AFM files giving the metrics (optional)
#	zz.gsf - the output file
#	fontname - the font name
#	UniqueID - the UniqueID, as described above
#	XUID - the XUID, in the form n1.n2.n3... (optional)
#	encodingname - the encoding for the font (optional)
#
# Currently, the defined encodings are StandardEncoding, ISOLatin1Encoding,
# SymbolEncoding, and DingbatsEncoding. If the encoding is omitted,
# StandardEncoding is assumed.

# ----------------------------------------------------------------

FBEGIN=echogs -w _temp_ -n -x 5b28	# open bracket, open parenthesis
P2=-x 2928				# close and open parenthesis
FNEXT=echogs -a _temp_ -n $(P2)
FLAST=echogs -a _temp_ $(P2)
FEND=-x 295d /ARGUMENTS exch def	# close parenthesis, close bracket
RUNBDF=$(GSBDF) -dNODISPLAY _temp_ bdftops.ps -c quit

# ---------------- Avant Garde ----------------

AvantGarde=$(FONTS)/pagk.gsf $(FONTS)/pagko.gsf $(FONTS)/pagd.gsf \
  $(FONTS)/pagdo.gsf
AvantGarde: $(AvantGarde)

$(FONTS)/pagk.gsf: $(BDF)/avt18.bdf $(AFM)/av-k.afm
	$(FBEGIN) $(BDF)/avt18.bdf $(P2) $(AFM)/av-k.afm
	$(FNEXT) $(FONTS)/pagk.gsf $(P2) AvantGardeGS-Book
	$(FLAST) 5066503 $(P2) $(XUID).0.5066503 $(FEND)
	$(RUNBDF)

$(FONTS)/pagko.gsf: $(BDF)/pagko.bdf $(AFM)/av-ko.afm
	$(FBEGIN) $(BDF)/pagko.bdf $(P2) $(AFM)/av-ko.afm
	$(FNEXT) $(FONTS)/pagko.gsf $(P2) AvantGardeGS-BookOblique
	$(FLAST) 5066504 $(P2) $(XUID).0.5066504 $(FEND)
	$(RUNBDF)

$(FONTS)/pagd.gsf: $(BDF)/pagd.bdf $(AFM)/av-d.afm
	$(FBEGIN) $(BDF)/pagd.bdf $(P2) $(AFM)/av-d.afm
	$(FNEXT) $(FONTS)/pagd.gsf $(P2) AvantGardeGS-Demi
	$(FLAST) 5066505 $(P2) $(XUID).0.5066505 $(FEND)
	$(RUNBDF)

$(FONTS)/pagdo.gsf: $(BDF)/pagdo.bdf $(AFM)/av-do.afm
	$(FBEGIN) $(BDF)/pagdo.bdf $(P2) $(AFM)/av-do.afm
	$(FNEXT) $(FONTS)/pagdo.gsf $(P2) AvantGardeGS-DemiOblique
	$(FLAST) 5066506 $(P2) $(XUID).0.5066506 $(FEND)
	$(RUNBDF)

# ---------------- Bookman ----------------

Bookman=$(FONTS)/pbkl.gsf $(FONTS)/pbkli.gsf $(FONTS)/pbkd.gsf \
  $(FONTS)/pbkdi.gsf
Bookman: $(Bookman)

$(FONTS)/pbkl.gsf: $(BDF)/pbkl.bdf $(AFM)/book-l.afm
	$(FBEGIN) $(BDF)/pbkl.bdf $(P2) $(AFM)/book-l.afm
	$(FNEXT) $(FONTS)/pbkl.gsf $(P2) BookmanGS-Light
	$(FLAST) 5066507 $(P2) $(XUID).0.5066507 $(FEND)
	$(RUNBDF)

$(FONTS)/pbkli.gsf: $(BDF)/pbkli.bdf $(AFM)/book-li.afm
	$(FBEGIN) $(BDF)/pbkli.bdf $(P2) $(AFM)/book-li.afm
	$(FNEXT) $(FONTS)/pbkli.gsf $(P2) BookmanGS-LightItalic
	$(FLAST) 5066508 $(P2) $(XUID).0.5066508 $(FEND)
	$(RUNBDF)

$(FONTS)/pbkd.gsf: $(BDF)/pbkd.bdf $(AFM)/book-d.afm
	$(FBEGIN) $(BDF)/pbkd.bdf $(P2) $(AFM)/book-d.afm
	$(FNEXT) $(FONTS)/pbkd.gsf $(P2) BookmanGS-Demi
	$(FLAST) 5066509 $(P2) $(XUID).0.5066509 $(FEND)
	$(RUNBDF)

$(FONTS)/pbkdi.gsf: $(BDF)/pbkdi.bdf $(AFM)/book-di.afm
	$(FBEGIN) $(BDF)/pbkdi.bdf $(P2) $(AFM)/book-di.afm
	$(FNEXT) $(FONTS)/pbkdi.gsf $(P2) BookmanGS-DemiItalic
	$(FLAST) 5066510 $(P2) $(XUID).0.5066510 $(FEND)
	$(RUNBDF)

# ---------------- Charter ----------------

# These are the fonts contributed by Bitstream to X11R5.
# They are already in Type 1 form.

CharterBT=$(FONTS)/bchr.pfa $(FONTS)/bchri.pfa $(FONTS)/bchb.pfa \
  $(FONTS)/bchbi.pfa
Charter: $(CharterBT)

# Old Charter, no longer used.
#$(FONTS)/bchr.gsf: $(BDF)/charR24.bdf	# 4010000

# Old Charter-Italic, no longer used.
#$(FONTS)/bchri.gsf: $(BDF)/charI24.bdf	# 4010100

# Old Charter-Bold, no longer used.
#$(FONTS)/bchb.gsf: $(BDF)/charB24.bdf	# 4011000
		
# Old Charter-BoldItalic, no longer used.
#$(FONTS)/bchbi.gsf: $(BDF)/charBI24.bdf	# 4011100

# ---------------- Courier ----------------

# These are the fonts contributed by IBM to X11R5.
# They are already in Type 1 form.

Courier=$(FONTS)/ncrr.pfa $(FONTS)/ncrri.pfa $(FONTS)/ncrb.pfa \
  $(FONTS)/ncrbi.pfa
Courier: $(Courier)

# Old Courier, longer used.
#$(FONTS)/pcrr.gsf: $(BDF)/courR24.bdf $(AFM)/cour.afm	# 4020000

# Old Courier-Oblique, no longer used.
#$(FONTS)/pcrro.gsf: $(BDF)/courO24.bdf $(AFM)/cour-o.afm	# 4020200

# Old Courier-Bold, no longer used.
#$(FONTS)/_pcrb.gsf: $(BDF)/courB24.bdf $(AFM)/cour-b.afm	# 4021000

# Old Courier-BoldOblique, no longer used.
#$(FONTS)/pcrbo.gsf: $(BDF)/courBO24.bdf $(AFM)/cour-bo.afm	# 4021200

# ---------------- Cyrillic ----------------

# These are shareware fonts of questionable quality.
# They have UniqueIDs 5066501-02.

Cyrillic=$(FONTS)/fcyr.gsf $(FONTS)/fcyri.gsf
Cyrillic: $(Cyrillic)

$(Cyrillic): $(IMPORT)/fcyr.taz
	cd $(CDFONTS)
	gunzip -c import/fcyr.taz | tar -xvf -

# ---------------- Helvetica ----------------

Helvetica=$(FONTS)/phvr.gsf $(FONTS)/phvro.gsf $(FONTS)/phvrrn.gsf \
  $(FONTS)/phvb.gsf $(FONTS)/phvbo.gsf
Helvetica: $(Helvetica)

$(FONTS)/phvr.gsf: $(BDF)/helvR24.bdf $(AFM)/helv.afm
	$(FBEGIN) $(BDF)/helvR24.bdf $(P2) $(AFM)/helv.afm
	$(FNEXT) $(FONTS)/phvr.gsf $(P2) HelveticaGS
	$(FLAST) 5066511 $(P2) $(XUID).0.5066511 $(FEND)
	$(RUNBDF)

$(FONTS)/phvro.gsf: $(BDF)/helvO24.bdf $(AFM)/helv-o.afm
	$(FBEGIN) $(BDF)/helvO24.bdf $(P2) $(AFM)/helv-o.afm
	$(FNEXT) $(FONTS)/phvro.gsf $(P2) HelveticaGS-Oblique
	$(FLAST) 5066512 $(P2) $(XUID).0.5066512 $(FEND)
	$(RUNBDF)

$(FONTS)/phvrrn.gsf: $(BDF)/hvmrc14.bdf $(AFM)/helv-n.afm
	$(FBEGIN) $(BDF)/hvmrc14.bdf $(P2) $(AFM)/helv-n.afm
	$(FNEXT) $(FONTS)/phvrrn.gsf $(P2) HelveticaGS-Narrow
	$(FLAST) 5066513 $(P2) $(XUID).0.5066513 $(FEND)
	$(RUNBDF)

$(FONTS)/phvb.gsf: $(BDF)/helvB24.bdf $(AFM)/helv-b.afm
	$(FBEGIN) $(BDF)/helvB24.bdf $(P2) $(AFM)/helv-b.afm
	$(FNEXT) $(FONTS)/phvb.gsf $(P2) HelveticaGS-Bold
	$(FLAST) 5066514 $(P2) $(XUID).0.5066514 $(FEND)
	$(RUNBDF)

$(FONTS)/phvbo.gsf: $(BDF)/helvBO24.bdf $(AFM)/helv-bo.afm
	$(FBEGIN) $(BDF)/helvBO24.bdf $(P2) $(AFM)/helv-bo.afm
	$(FNEXT) $(FONTS)/phvbo.gsf $(P2) HelveticaGS-BoldOblique
	$(FLAST) 5066515 $(P2) $(XUID).0.5066515 $(FEND)
	$(RUNBDF)

# ---------------- Kana ----------------

# These are free fonts posted by a user to comp.fonts.
# They have their own UniqueIDs.

Kana=$(FONTS)/fhirw.gsf $(FONTS)/fkarw.gsf
Kana: $(Kana)

$(Kana): $(IMPORT)/fxxrw.taz
	cd $(CDFONTS)
	gunzip -c import/fxxrw.taz | tar -xvf -

# ---------------- New Century Schoolbook ----------------

NewCenturySchlbk=$(FONTS)/pncr.gsf $(FONTS)/pncri.gsf $(FONTS)/pncb.gsf \
  $(FONTS)/pncbi.gsf
NewCenturySchlbk: $(NewCenturySchlbk)

$(FONTS)/pncr.gsf: $(BDF)/ncenR24.bdf $(AFM)/ncs-r.afm
	$(FBEGIN) $(BDF)/ncenR24.bdf $(P2) $(AFM)/ncs-r.afm
	$(FNEXT) $(FONTS)/pncr.gsf $(P2) NewCenturySchlbkGS-Roman
	$(FLAST) 5066516 $(P2) $(XUID).0.5066516 $(FEND)
	$(RUNBDF)

$(FONTS)/pncri.gsf: $(BDF)/ncenI24.bdf $(AFM)/ncs-i.afm
	$(FBEGIN) $(BDF)/ncenI24.bdf $(P2) $(AFM)/ncs-i.afm
	$(FNEXT) $(FONTS)/pncri.gsf $(P2) NewCenturySchlbkGS-Italic
	$(FLAST) 5066517 $(P2) $(XUID).0.5066517 $(FEND)
	$(RUNBDF)

$(FONTS)/pncb.gsf: $(BDF)/ncenB24.bdf $(AFM)/ncs-b.afm
	$(FBEGIN) $(BDF)/ncenB24.bdf $(P2) $(AFM)/ncs-b.afm
	$(FNEXT) $(FONTS)/pncb.gsf $(P2) NewCenturySchlbkGS-Bold
	$(FLAST) 5066518 $(P2) $(XUID).0.5066518 $(FEND)
	$(RUNBDF)

$(FONTS)/pncbi.gsf: $(BDF)/ncenBI24.bdf $(AFM)/ncs-bi.afm
	$(FBEGIN) $(BDF)/ncenBI24.bdf $(P2) $(AFM)/ncs-bi.afm
	$(FNEXT) $(FONTS)/pncbi.gsf $(P2) NewCenturySchlbkGS-BoldItalic
	$(FLAST) 5066519 $(P2) $(XUID).0.5066519 $(FEND)
	$(RUNBDF)

# ---------------- Palatino ----------------

Palatino=$(FONTS)/pplr.gsf $(FONTS)/pplri.gsf $(FONTS)/pplb.gsf \
  $(FONTS)/pplbi.gsf
Palatino: $(Palatino)

$(FONTS)/pplr.gsf: $(BDF)/pal18.bdf $(AFM)/pal-r.afm
	$(FBEGIN) $(BDF)/pal18.bdf $(P2) $(AFM)/pal-r.afm
	$(FNEXT) $(FONTS)/pplr.gsf $(P2) PalatinoGS-Roman
	$(FLAST) 5066520 $(P2) $(XUID).0.5066520 $(FEND)
	$(RUNBDF)

$(FONTS)/pplri.gsf: $(BDF)/pplri.bdf $(AFM)/pal-i.afm
	$(FBEGIN) $(BDF)/pplri.bdf $(P2) $(AFM)/pal-i.afm
	$(FNEXT) $(FONTS)/pplri.gsf $(P2) PalatinoGS-Italic
	$(FLAST) 5066521 $(P2) $(XUID).0.5066521 $(FEND)
	$(RUNBDF)

$(FONTS)/pplb.gsf: $(BDF)/pplb.bdf $(AFM)/pal-b.afm
	$(FBEGIN) $(BDF)/pplb.bdf $(P2) $(AFM)/pal-b.afm
	$(FNEXT) $(FONTS)/pplb.gsf $(P2) PalatinoGS-Bold
	$(FLAST) 5066522 $(P2) $(XUID).0.5066522 $(FEND)
	$(RUNBDF)

$(FONTS)/pplbi.gsf: $(BDF)/pplbi.bdf $(AFM)/pal-bi.afm
	$(FBEGIN) $(BDF)/pplbi.bdf $(P2) $(AFM)/pal-bi.afm
	$(FNEXT) $(FONTS)/pplbi.gsf $(P2) PalatinoGS-BoldItalic
	$(FLAST) 5066523 $(P2) $(XUID).0.5066523 $(FEND)
	$(RUNBDF)

# ---------------- Symbol ----------------

Symbol=$(FONTS)/psyr.gsf
Symbol: $(Symbol)

$(FONTS)/psyr.gsf: $(BDF)/symb24.bdf $(AFM)/symbol.afm
	$(FBEGIN) $(BDF)/symb24.bdf $(P2) $(AFM)/symbol.afm
	$(FNEXT) $(FONTS)/psyr.gsf $(P2) SymbolGS
	$(FLAST) 5066524 $(P2) $(XUID).0.5066524 $(P2) SymbolEncoding $(FEND)
	$(RUNBDF)

# ---------------- Times ----------------

Times=$(FONTS)/ptmr.gsf $(FONTS)/ptmri.gsf $(FONTS)/ptmb.gsf \
  $(FONTS)/ptmbi.gsf
Times: $(Times)

$(FONTS)/ptmr.gsf: $(BDF)/timR24.bdf $(AFM)/times-r.afm
	$(FBEGIN) $(BDF)/timR24.bdf $(P2) $(AFM)/times-r.afm
	$(FNEXT) $(FONTS)/ptmr.gsf $(P2) TimesGS-Roman
	$(FLAST) 5066525 $(P2) $(XUID).0.5066525 $(FEND)
	$(RUNBDF)

$(FONTS)/ptmri.gsf: $(BDF)/timI24.bdf $(AFM)/times-i.afm
	$(FBEGIN) $(BDF)/timI24.bdf $(P2) $(AFM)/times-i.afm
	$(FNEXT) $(FONTS)/ptmri.gsf $(P2) TimesGS-Italic
	$(FLAST) 5066526 $(P2) $(XUID).0.5066526 $(FEND)
	$(RUNBDF)

$(FONTS)/ptmb.gsf: $(BDF)/timB24.bdf $(AFM)/times-b.afm
	$(FBEGIN) $(BDF)/timB24.bdf $(P2) $(AFM)/times-b.afm
	$(FNEXT) $(FONTS)/ptmb.gsf $(P2) TimesGS-Bold
	$(FLAST) 5066527 $(P2) $(XUID).0.5066527 $(FEND)
	$(RUNBDF)

$(FONTS)/ptmbi.gsf: $(BDF)/timBI24.bdf $(AFM)/times-bi.afm
	$(FBEGIN) $(BDF)/timBI24.bdf $(P2) $(AFM)/times-bi.afm
	$(FNEXT) $(FONTS)/ptmbi.gsf $(P2) TimesGS-BoldItalic
	$(FLAST) 5066528 $(P2) $(XUID).0.5066528 $(FEND)
	$(RUNBDF)

# ---------------- Utopia ----------------

# These are fonts contributed by Adobe to X11R5.
# They are already in Type 1 form.

Utopia=$(FONTS)/putr.pfa $(FONTS)/putri.pfa $(FONTS)/putb.pfa \
  $(FONTS)/putbi.pfa
Utopia: $(Utopia)

# ---------------- Zapf Chancery ----------------

ZapfChancery=$(FONTS)/zcr.gsf $(FONTS)/zcro.gsf $(FONTS)/zcb.gsf
ZapfChancery: $(ZapfChancery)

$(FONTS)/zcr.gsf: $(BDF)/zcr24.bdf $(AFM)/zc-r.afm
	$(FBEGIN) $(BDF)/zcr24.bdf $(P2) $(AFM)/zc-r.afm
	$(FNEXT) $(FONTS)/zcr.gsf $(P2) ZapfChanceryGS
	$(FLAST) 5066529 $(P2) $(XUID).0.5066529 $(FEND)
	$(RUNBDF)

$(FONTS)/zcro.gsf: $(BDF)/zcro24.bdf $(AFM)/zc-mi.afm
	$(FBEGIN) $(BDF)/zcro24.bdf $(P2) $(AFM)/zc-mi.afm
	$(FNEXT) $(FONTS)/zcro.gsf $(P2) ZapfChanceryGS-Oblique
	$(FLAST) 5066530 $(P2) $(XUID).0.5066530 $(FEND)
	$(RUNBDF)

$(FONTS)/zcb.gsf: $(BDF)/zcb30.bdf $(AFM)/zc-b.afm
	$(FBEGIN) $(BDF)/zcb30.bdf $(P2) $(AFM)/zc-b.afm
	$(FNEXT) $(FONTS)/zcb.gsf $(P2) ZapfChanceryGS-Bold
	$(FLAST) 5066531 $(P2) $(XUID).0.5066531 $(FEND)
	$(RUNBDF)

# ---------------- Zapf Dingbats ----------------

ZapfDingbats=$(FONTS)/pzdr.gsf
ZapfDingbats: $(ZapfDingbats)

$(FONTS)/pzdr.gsf: $(BDF)/ding24.bdf $(AFM)/zd.afm
	$(FBEGIN) $(BDF)/ding24.bdf $(P2) $(AFM)/zd.afm
	$(FNEXT) $(FONTS)/pzdr.gsf $(P2) ZapfDingbatsGS
	$(FLAST) 5066532 $(P2) $(XUID).0.5066532 $(P2) DingbatsEncoding $(FEND)
	$(RUNBDF)
