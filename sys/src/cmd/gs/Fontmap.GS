%    Copyright (C) 1990, 1992, 1993 Aladdin Enterprises.  All rights reserved.
% 
% This file is part of Aladdin Ghostscript.
% 
% Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
% or distributor accepts any responsibility for the consequences of using it,
% or for whether it serves any particular purpose or works at all, unless he
% or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
% License (the "License") for full details.
% 
% Every copy of Aladdin Ghostscript must include a copy of the License,
% normally in a plain ASCII text file named PUBLIC.  The License grants you
% the right to copy, modify and redistribute Aladdin Ghostscript, but only
% under certain conditions described in the License.  Among other things, the
% License requires that the copyright notice and this notice be preserved on
% all copies.

% fontmap aka Fontmap - standard font catalog for Ghostscript.

% ----------------------------------------------------------------

% This file is a catalog of fonts known to Ghostscript.  Any font
% that is to be loaded automatically when named must be in this catalog,
% except for fonts that Ghostscript finds automatically in directories
% named in the GS_FONTPATH environment variable.

% Each font has an entry consisting of three items:
%
%	- The name by which the font is known inside Ghostscript
%	(a Ghostscript name preceded by a `/', or a string enclosed
%	in parentheses).  This is used to find the file from which
%	a font of a given name should be loaded.
%
%	- Information depending on whether this is a real font or a
%	font alias:
%
%		- For real fonts, the name of the Ghostscript font
%		file (a Ghostscript string, enclosed in parentheses).
%		The filename should include the extension, which (by
%		convention) is `.gsf'.  `.pfa' and `.pfb' files are
%		also usable as fonts for Ghostscript.
%
%		- For font aliases, the name of the font which should
%		be used when this one is requested, preceded by a
%		`/'.  See the entry for Charter below for an example.
%		Note that an alias name cannot be enclosed in parentheses.
%
%	- At least one space or tab, and a terminating semicolon.

% Because of limitations in the MS-DOS environment, Ghostscript font
% file names must be no more than 8 characters long, must consist only
% of LOWER CASE letters, digits, and underscores, and must start with a
% letter.  Font names, on the other hand, need only obey the syntax of
% names in the Ghostscript language, which is much more liberal.

% Most of the Ghostscript fonts were created automatically from freely
% available bitmaps.  There is a makefile (fonts.mak) that specifies
% how this conversion was done.  fonts.mak also specifies, for each such
% converted font, its uniqueID (an integer used to identify distinct fonts
% within the Ghostscript font machinery), and its encoding (the mapping
% from character codes in a string to character names).  For more detailed
% information, read fonts.mak.

% The following table is actually a Ghostscript data structure.
% If you add new entries, be sure to copy the punctuation accurately;
% in particular, you must leave at least one space or tab between each
% field in the entry.  Also, please read fonts.doc for important information
% about font names.

% Note that .pfa and .pfb fonts are compatible with Adobe Type Manager
% and other programs that don't include full PostScript interpreters,
% as well as with PostScript interpreters; .gsf fonts are compatible with
% PostScript interpreters, but not with ATM or similar programs.

% 
% Fonts converted from bitmaps.

/AvantGarde-Book		(pagk.gsf)	;
/AvantGarde-BookOblique		(pagko.gsf)	;
/AvantGarde-Demi		(pagd.gsf)	;
/AvantGarde-DemiOblique		(pagdo.gsf)	;

/Bookman-Light			(pbkl.gsf)	;
/Bookman-LightItalic		(pbkli.gsf)	;
/Bookman-Demi			(pbkd.gsf)	;
/Bookman-DemiItalic		(pbkdi.gsf)	;

% The converted Bitstream Charter fonts have been replaced by Type 1 fonts
% contributed to the X11R5 distribution; see below.
%/Charter-Roman			(bchr.gsf)	;
%/Charter-Italic		(bchri.gsf)	;
%/Charter-Bold			(bchb.gsf)	;
%/Charter-BoldItalic		(bchbi.gsf)	;

% The converted Courier fonts have been replaced by Type 1 fonts
% contributed to the X11R5 distribution; see below.
%/Courier			(pcrr.gsf)	;
%/Courier-Oblique		(pcrro.gsf)	;
%/Courier-Bold			(pcrb.gsf)	;
%/Courier-BoldOblique		(pcrbo.gsf)	;
% Quite a few PostScript files reference Courier-Oblique rather than
% Courier-Italic, so we need some aliases here.
/Courier-Oblique		/Courier-Italic		;
/Courier-BoldOblique		/Courier-BoldItalic	;

/Helvetica			(phvr.gsf)	;
/Helvetica-Oblique		(phvro.gsf)	;
/Helvetica-Bold			(phvb.gsf)	;
/Helvetica-BoldOblique		(phvbo.gsf)	;
% We don't have a full set of Helvetica-Narrow....
/Helvetica-Narrow		(phvrrn.gsf)	;
	/Helvetica-Narrow-Bold		/Helvetica-Narrow	;

/NewCenturySchlbk-Roman		(pncr.gsf)	;
/NewCenturySchlbk-Italic	(pncri.gsf)	;
/NewCenturySchlbk-Bold		(pncb.gsf)	;
/NewCenturySchlbk-BoldItalic	(pncbi.gsf)	;

/Palatino-Roman			(pplr.gsf)	;
/Palatino-Italic		(pplri.gsf)	;
/Palatino-Bold			(pplb.gsf)	;
/Palatino-BoldItalic		(pplbi.gsf)	;

/Symbol				(psyr.gsf)	;

/Times-Roman			(ptmr.gsf)	;
/Times-Italic			(ptmri.gsf)	;
/Times-Bold			(ptmb.gsf)	;
/Times-BoldItalic		(ptmbi.gsf)	;

/ZapfChancery			(zcr.gsf)	;
/ZapfChancery-Oblique		(zcro.gsf)	;
/ZapfChancery-Bold		(zcb.gsf)	;
% Some PostScript files reference the MediumItalic font....
/ZapfChancery-MediumItalic	/ZapfChancery-Oblique	;

/ZapfDingbats			(pzdr.gsf)	;


% 
%
% Type 1 fonts contributed to the X11R5 distribution.
%

% The following notice accompanied the Charter fonts.
%
% (c) Copyright 1989-1992, Bitstream Inc., Cambridge, MA.
%
% You are hereby granted permission under all Bitstream propriety rights
% to use, copy, modify, sublicense, sell, and redistribute the 4 Bitstream
% Charter (r) Type 1 outline fonts and the 4 Courier Type 1 outline fonts
% for any purpose and without restriction; provided, that this notice is
% left intact on all copies of such fonts and that Bitstream's trademark
% is acknowledged as shown below on all unmodified copies of the 4 Charter
% Type 1 fonts.
%
% BITSTREAM CHARTER is a registered trademark of Bitstream Inc.

% The Bitstream Charter fonts have different names (CharterBT-
% instead of Charter-), but Ghostscript doesn't care.
/Charter-Roman			(bchr.pfa)	;
/Charter-Italic			(bchri.pfa)	;
/Charter-Bold			(bchb.pfa)	;
/Charter-BoldItalic		(bchbi.pfa)	;

% The following notice accompanied the Courier font:
%
%   IBM Courier - Copyright (c) IBM Corporation 1990, 1991
%
%   You are hereby granted permission under the terms of the IBM/MIT X
%   Consortium Courier Typefont agreement to execute, reproduce,
%   distribute, display, market, sell and otherwise transfer copies of
%   the IBM Courier font to third parties.
%
%   The font is provided "AS IS" without charge.  NO WARRANTIES OR
%   INDEMNIFICATION ARE GIVEN, WHETHER EXPRESS OR IMPLIED INCLUDING, BUT
%   LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
%   A PARTICULAR PURPOSE.

/Courier		(ncrr.pfa)	;
/Courier-Italic		(ncrri.pfa)	;
/Courier-Bold		(ncrb.pfa)	;
/Courier-BoldItalic	(ncrbi.pfa)	;

% The following notice accompanied the Utopia font:
%
%   Permission to use, reproduce, display and distribute the listed
%   typefaces is hereby granted, provided that the Adobe Copyright notice
%   appears in all whole and partial copies of the software and that the
%   following trademark symbol and attribution appear in all unmodified
%   copies of the software:
%
%           Copyright (c) 1989 Adobe Systems Incorporated
%           Utopia (R)
%           Utopia is a registered trademark of Adobe Systems Incorporated
%
%   The Adobe typefaces (Type 1 font program, bitmaps and Adobe Font
%   Metric files) donated are:
%
%           Utopia Regular
%           Utopia Italic
%           Utopia Bold
%           Utopia Bold Italic

/Utopia-Regular		(putr.pfa)	;
/Utopia-Italic		(putri.pfa)	;
/Utopia-Bold		(putb.pfa)	;
/Utopia-BoldItalic	(putbi.pfa)	;

% 
%
% Fonts contributed by URW GmbH for distribution under the GNU License.
% The following notice accompanied these fonts:
%
% N019003L Nimbus Sans L Regular PostScript Type 1 Font Program
% N021003L Nimbus Roman No9 L Regular PostScript Type 1 Font Program
% U004006T URW Grotesk 2031 Bold PostScript Type 1 Font Program
% U003043T URW Antiqua 2051 Regular Condensed PostScript Type 1 Font Program
%
% Copyright (c) 1992 URW GmbH, Hamburg, Germany
%
% This program is free software; you can redistribute it and/or modify
% it under the terms of the GNU General Public License as published by
% the Free Software Foundation; either version 2 of the License, or
% (at your option) later version.
%
% This program is distributed in the hope that it will be useful,
% but WITHOUT ANY WARRANTY; wihtout even the implied warranty of
% MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
% See the GNU General Public License for more details.
%
% You should have received a copy of the GNU General Public License
% along with this program; if not, write to the Free Software
% Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
%
% Address:
% URW GmbH
% PC Support
% Harksheider Strasse 102
% 2000 Hamburg 65
% Germany
% Phone: +49 40 60 60 50       (Reception)
% Phone: +49 40 60 60 52 30   (PC Support)
% Fax    : +49 40 60 60 52 52
%

% Nimbus Sans L Regular is a good work-alike for Helvetica:
/Helvetica			/NimbusSansL-Regular	;
/NimbusSansL-Regular		(n019003l.gsf)	;
% Nimbus Roman No9 L Regular is a good work-alike for Times Roman:
/Times-Roman			/NimbusRomanNo9L-Regular	;
/NimbusRomanNo9L-Regular	(n021003l.gsf)	;
% The other fonts don't correspond to any in the base set of 35....
/URWAntiquaT-RegularCondensed	(u003043t.gsf)	;
/URWGroteskT-Bold		(u004006t.gsf)	;

% 
%
% Shareware Kana fonts.  These are subject to the following notice:
%
% These copyrighted fonts were developed by Kevin Hartig. Permission is
% granted to freely distribute them in entirety along with this statement.
% This is shareware. If you decide to use these fonts please contribute
% $10 US to help support further freeware and shareware software development.
% Questions and comments may be sent to:
%
% hartig@fsl.noaa.gov
% khartig@nyx.cs.du.edu
%
% Kevin Hartig
% 1126 Collyer Street
% Longmont, CO 80501 USA
% 
% copyright 1993.

% Hiragana and Katakana fonts.  The character names are inappropriate,
% and the encoding is probably not related to any known standard.

/Calligraphic-Hiragana		(fhirw.gsf)	;
/Calligraphic-Katakana		(fkarw.gsf)	;

% 
%
% Public-domain fonts.  These have no copyright, and are of unknown quality.

% Cyrillic fonts.  The character names are inappropriate,
% and the encoding is probably not related to any known standard.

/Shareware-Cyrillic-Regular	(fcyr.gsf)	;
/Shareware-Cyrillic-Italic	(fcyri.gsf)	;
% Aliases
/Cyrillic			/Cyrillic-Regular	;
/Cyrillic-Regular		/Shareware-Cyrillic-Regular	;
/Cyrillic-Italic		/Shareware-Cyrillic-Italic	;

% 
%
% Fonts converted from Hershey outlines.  These are constructed and
% maintained manually.  These are also in the public domain.
%
% The suggested UniqueID's and filenames are constructed differently for
% these than for the ones above, because of the strange way that the Hershey
% fonts were constructed.  The scheme for these looks like:
%
% 42TTXY0
%
% TT = typeface, X = ``class'', Y = variation
%
% The typeface names and numbers are listed in fonts.mak.
%
% class:
% 0 = normal			= r
% 1 = simplex			= s
% 2 = complex			= c
% 3 = triplex			= t
% 4 = duplex			= d
%
% variation:
% 0 = normal			(omitted)
% 1 = oblique			= o
% 2 = italic			= i
% 3 = bold			= b
% 4 = bold oblique		= bo
% 5 = bold italic		= bi
%

% Fonts created by Thomas Wolff <wolff@inf.fu-berlin.de>, by adding
% accents, accented characters, and various other non-alphabetics
% to the original Hershey fonts.  These are "freeware", not to be sold.

/Hershey-Gothic-English		(hrger.pfa)	;	% 5066533
/Hershey-Gothic-German		(hrgrr.pfa)	;
/Hershey-Gothic-Italian		(hritr.pfa)	; 

/Hershey-Plain-Duplex		(hrpld.pfa)	;
/Hershey-Plain-Duplex-Italic	(hrpldi.pfa)	;
/Hershey-Plain-Triplex		(hrplt.pfa)	;
/Hershey-Plain-Triplex-Italic	(hrplti.pfa)	;

/Hershey-Script-Complex		(hrscc.pfa)	;
/Hershey-Script-Simplex		(hrscs.pfa)	;	% 5066541

% Fonts created algorithmically from the above.

/Hershey-Gothic-English-Bold		(hrgerb.gsf)	;	% 5066542
/Hershey-Gothic-English-Oblique		(hrgero.gsf)	;
/Hershey-Gothic-English-SemiBold	(hrgerd.gsf)	;
/Hershey-Gothic-German-Bold		(hrgrrb.gsf)	;
/Hershey-Gothic-German-Oblique		(hrgrro.gsf)	;
/Hershey-Gothic-Italian-Bold		(hritrb.gsf)	;
/Hershey-Gothic-Italian-Oblique		(hritro.gsf)	;

/Hershey-Plain-Duplex-Bold		(hrpldb.gsf)	;
/Hershey-Plain-Duplex-Bold-Italic	(hrpldbi.gsf)	;
/Hershey-Plain-Triplex-Bold		(hrpltb.gsf)	;
/Hershey-Plain-Triplex-Bold-Italic	(hrpltbi.gsf)	;

/Hershey-Script-Complex-Bold		(hrsccb.gsf)	;
/Hershey-Script-Complex-Oblique		(hrscco.gsf)	;
/Hershey-Script-Simplex-Bold		(hrscsb.gsf)	;
/Hershey-Script-Simplex-Oblique		(hrscso.gsf)	;	% 5066556

% Fonts consisting only of characters from the original Hershey
% distribution.  These are Type 3 fonts.

/Hershey-Greek-Complex			(hrgkc.gsf)	;	% 5066557
/Hershey-Greek-Simplex			(hrgks.gsf)	;

/Hershey-Plain				(hrplr.gsf)	;
/Hershey-Plain-Simplex			(hrpls.gsf)	;	% 5066560

% Fonts created algorithmically from the above.

/Hershey-Plain-Bold			(hrplrb.gsf)	;	% 5066561
/Hershey-Plain-Bold-Oblique		(hrplrbo.gsf)	;
/Hershey-Plain-Oblique			(hrplro.gsf)	;
/Hershey-Plain-Simplex-Bold		(hrplsb.gsf)	;
/Hershey-Plain-Simplex-Bold-Oblique	(hrplsbo.gsf)	;
/Hershey-Plain-Simplex-Oblique		(hrplso.gsf)	;	% 5066566

% This font, and only this font among the Hershey fonts, uses
% the SymbolEncoding.

/Hershey-Symbol				(hrsyr.gsf)	;	% 5066567
