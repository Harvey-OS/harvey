% Copyright (C) 1996, 1999 Aladdin Enterprises.  All rights reserved.
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

% $Id: Fontmap.GS,v 1.1 2000/03/09 08:40:39 lpd Exp $
% Fontmap - standard font catalog for Ghostscript.

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
%
% Fonts contributed by:
%	URW++ Design and Development Incorporated
%	Poppenbuetteler Bogen 29A
%	D-22399 Hamburg
%	Germany
%	tel. +49 (40) 60 60 50
%	fax +49 (40) 60 60 51 11
%	http://www.urwpp.de
% for distribution under the GNU License and Aladdin Free Public License.
% See the notice at the head of this Fontmap file for licensing terms.
% Each of these fonts is individually covered by the license:
% for licensing purposes, they are not "part of" any larger entity.
% The following notice applies to these fonts:
%
%	Copyright URW Software, Copyright 1994 by URW.
%

% Actual fonts

/URWBookmanL-DemiBold	(b018015l.pfb)	;
/URWBookmanL-DemiBoldItal	(b018035l.pfb)	;
/URWBookmanL-Ligh	(b018012l.pfb)	;
/URWBookmanL-LighItal	(b018032l.pfb)	;

/NimbusMonL-Regu	(n022003l.pfb)	;
/NimbusMonL-ReguObli	(n022023l.pfb)	;
/NimbusMonL-Bold	(n022004l.pfb)	;
/NimbusMonL-BoldObli	(n022024l.pfb)	;

/URWGothicL-Book	(a010013l.pfb)	;
/URWGothicL-BookObli	(a010033l.pfb)	;
/URWGothicL-Demi	(a010015l.pfb)	;
/URWGothicL-DemiObli	(a010035l.pfb)	;

/NimbusSanL-Regu	(n019003l.pfb)	;
/NimbusSanL-ReguItal	(n019023l.pfb)	;
/NimbusSanL-Bold	(n019004l.pfb)	;
/NimbusSanL-BoldItal	(n019024l.pfb)	;

/NimbusSanL-ReguCond	(n019043l.pfb)	;
/NimbusSanL-ReguCondItal	(n019063l.pfb)	;
/NimbusSanL-BoldCond	(n019044l.pfb)	;
/NimbusSanL-BoldCondItal	(n019064l.pfb)	;

/URWPalladioL-Roma	(p052003l.pfb)	;
/URWPalladioL-Ital	(p052023l.pfb)	;
/URWPalladioL-Bold	(p052004l.pfb)	;
/URWPalladioL-BoldItal	(p052024l.pfb)	;

/CenturySchL-Roma	(c059013l.pfb)	;
/CenturySchL-Ital	(c059033l.pfb)	;
/CenturySchL-Bold	(c059016l.pfb)	;
/CenturySchL-BoldItal	(c059036l.pfb)	;

/NimbusRomNo9L-Regu	(n021003l.pfb)	;
/NimbusRomNo9L-ReguItal	(n021023l.pfb)	;
/NimbusRomNo9L-Medi	(n021004l.pfb)	;
/NimbusRomNo9L-MediItal	(n021024l.pfb)	;

/StandardSymL	(s050000l.pfb)	;

/URWChanceryL-MediItal	(z003034l.pfb)	;

/Dingbats	(d050000l.pfb)	;

% Aliases

/Bookman-Demi			/URWBookmanL-DemiBold	;
/Bookman-DemiItalic		/URWBookmanL-DemiBoldItal	;
/Bookman-Light			/URWBookmanL-Ligh	;
/Bookman-LightItalic		/URWBookmanL-LighItal	;

/Courier			/NimbusMonL-Regu	;
/Courier-Oblique		/NimbusMonL-ReguObli	;
/Courier-Bold			/NimbusMonL-Bold	;
/Courier-BoldOblique		/NimbusMonL-BoldObli	;

/AvantGarde-Book		/URWGothicL-Book	;
/AvantGarde-BookOblique		/URWGothicL-BookObli	;
/AvantGarde-Demi		/URWGothicL-Demi	;
/AvantGarde-DemiOblique		/URWGothicL-DemiObli	;

/Helvetica			/NimbusSanL-Regu	;
/Helvetica-Oblique		/NimbusSanL-ReguItal	;
/Helvetica-Bold			/NimbusSanL-Bold	;
/Helvetica-BoldOblique		/NimbusSanL-BoldItal	;

/Helvetica-Narrow		/NimbusSanL-ReguCond	;
/Helvetica-Narrow-Oblique	/NimbusSanL-ReguCondItal	;
/Helvetica-Narrow-Bold		/NimbusSanL-BoldCond	;
/Helvetica-Narrow-BoldOblique	/NimbusSanL-BoldCondItal	;

/Palatino-Roman			/URWPalladioL-Roma	;
/Palatino-Italic		/URWPalladioL-Ital	;
/Palatino-Bold			/URWPalladioL-Bold	;
/Palatino-BoldItalic		/URWPalladioL-BoldItal	;

/NewCenturySchlbk-Roman		/CenturySchL-Roma	;
/NewCenturySchlbk-Italic	/CenturySchL-Ital	;
/NewCenturySchlbk-Bold		/CenturySchL-Bold	;
/NewCenturySchlbk-BoldItalic	/CenturySchL-BoldItal	;

/Times-Roman			/NimbusRomNo9L-Regu	;
/Times-Italic			/NimbusRomNo9L-ReguItal	;
/Times-Bold			/NimbusRomNo9L-Medi	;
/Times-BoldItalic		/NimbusRomNo9L-MediItal	;

/Symbol				/StandardSymL	;

/ZapfChancery-MediumItalic	/URWChanceryL-MediItal	;

/ZapfDingbats			/Dingbats	;
