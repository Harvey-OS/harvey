The following files are included

tm.sty	-- Contains the main macros for the TM document style. 
		Requires art10.sty (one of the standard file for
		the article style) and local.sty (see below).

	tm.sty should be placed in the .../inputs directory or
	wherever your TeX looks for input files that are not in
	the current directory.

expicture.sty -- Contains some extra macro that can be used in 
		the picture environment.

	expicture.sty should be placed in the .../inputs directory
	or wherever your TeX looks for input files that are not
	in the current directory.

local.sty  -- Used by tm.sty to set some printer dependent
		local features.  Can be edited to add other
		local macros or change parameters.  

	local.sty should be placed in the .../inputs directory
	or wherever your TeX looks for input files athat are not
	in the current directory.

localpatch.sty -- Used to make local modifications to macro
		definitions.  The distributed file has only a comment.
		Macro definitions placed here will override those in
		tm.sty and tmaddon.sty.  This is preferable to editing
		tm.sty.  

	localpatch.sty should be placed in the .../inputs directory
	or wherever your TeX looks for input files that are not
	in the current directory.

attslides.sty -- Contains a style file for use with SLiTeX for
		preparing viewgraphs (see doc in tmdoc.tex).

mw.sty -- Contains a style file for preparing documents in Multiweight
		Design. For documentation see mwdoc.tex.  

att36.tfm -- The metric file for the AT&T logo font.

	att36.tfm should be placed in .../fonts directory
	or wherever your TeX looks for font metric (*.tfm)
	files.

att36.300pk -- The AT&T logo font in pk format; created with
		METAFONT.  This version if for the Imagen
		8/300 laser printer which has a 300 dpi
		resolution and prints a logo with the 12-line
		AT&T ball 36 pt in diameter (the size used
		on TMs and letterheads).  This font may be
		usable on other 300 dpi printers but may not
		look as nice as one optimized for the other
		printer.

att36.*pk -- 	The same AT&T logo in other magnifications.  
		The 118pk is typically used by screen previewers.

att36.*pk.10 -- The older 10-line version of the AT&T logo.  Identical
		that that printed by most troff implementation.

att36.pxl -- The same font at att36.pk but in pxl format

	either att36.pk or att36.pxl depending on which your
	dvi program uses should be installed in your font 
	directory for 300 dpi fonts, eg. 
	../pixels/canon300/dpi300, or some similar path.

att36.pke, att36.pxe -- The pk and pxl files for Epson LQ series
	printers.  They should be in a directory for 180 dpi.

att36.mf -- The METAFONT input file to create the AT&T
		logo.  You must have the METAFONT program
		to use this file.  Can be used to create
		pk or pxl files for other printers.

ldiffmk -- a version of diffmk that produces LaTeX output.
		Notice the limitations mentioned in the
		man page.

ldiffmk.1 -- a man page for the above.  

bitmap.sty -- 	A style file to be used as an option with other styles
		such as article or tm to print bitmaps.   Use 
		\input bitmap.sty or \documentstyle[bitmap]{yourstyle}.
		You can then print bitmaps converted by bitmap2tex.
 
		Then put \vbox{}\hskip3.5in\input texfile.tex 
		or something similar that uses \input texfile.tex
		wherever a \vbox could be used.
		
		The default bitmap pixel size is 1 pt, but can be
		redefined by \bitdotsize=2pt or whatever size you
		wish.

bitmap2tex --	A filter (written in awk) to convert X-window bitmaps
		to a LaTeX routine that produces a vbox.  Use

		  bitmap2tex <bitfile >texfile.tex

		The pixel dimensions (48x48 or whatever) are read from
		the bitmap file and set latex parameters automatically.

makeface.c -- A c++ program to convert 9th Edition faces to X-window
		bitmaps (may report false file opening errors, due to
		a bug in some version of iostreams).  After compiling
		makeface.c you may convert a 9th ed face by:

		  makeface face-file bitmap-file

		bitmap files can be viewed and edited using the
		X-window program `bitmap'.

square.map, tlaface.map -- Two sample X-window bitmaps that can be
		converted to tex files with bitmap2tex.

square.tex, tlaface.tex -- The above converted to tex for use with
		bitmap.sty. 

noface.tex --	a face file with a black face.  This may be used for
		authors that do not have a face file for use with
		\makefacesignature (when some authors do and some do
		not).

tmdoc.tex --	The LaTeX source for the TM that documents tm.sty.
		You must have tm.sty installed to re-LaTeX this
		document. (previously called tm.doc) 

tmdoc.dvi -- 	A dvi form of the above.  You should be able to print
		this before installing tm.sty, but you must install
		the att36 font first.

tmexample.tex -- An illustrative example of a brief tm using most of
		the features of tm.sty.  It is printed as source in an
		appendix of tm.doc, but can also be LaTeX'ed directly
		to see the document it produces.  A printed copy
		should be inserted into the appendix B of the document
		produced by tm.doc.

tmexample.dvi -- The dvi form of the above.  The att36 fonts must be
		installed before printing this.

letexample.tex -- An example of letters using attletter.sty.  It is
		printed as source in an appendix of tm.doc, but can
		also be LaTeX'ed directly to see the document it
		produces.  A printed copy should be inserted into the
		appendix D of the document produced by tm.doc.

letexample.dvi -- The dvi form of the above. 

attletter.sty -- A letter style that prints AT&T letterhead.

authors.sty -- A database for data on authors.

atttm.bst -- A BibTeX style file similar to unsrt.bst but somewhat
		closer to the style specified in The Bell Labs Style
		Guide by Cormaci and Trenner of Kelly Educ & Training
		Ctr, Feb 1988 edition.  For bibtex 0.99a or later.

atttm98.bst -- bibtex 0.99 was a major revision and *.bst files for
		0.99 and later versions are not compatible with
		earlier releases.  This file is for those sites still
		using bibtex 0.98.

lclocal.tex -- A local guide for LaTeX at AT&T LC 59112, 59114.

lclocal.dvi -- The dvi version of the above.

lclocal.bbl -- The bibliography for above produced by BibTeX.

tm.el -- Additions to GNUemacs latex.el to support tm related stuff.

my-tm.el -- Additions that are personal or site dependent.

my-latex.el -- None tm related additions to latex.el that are site dependent.

manpage.sty -- a style file for creating man pages using LaTeX

manpage.tex -- documentation for manpage.sty

manpage.dvi -- the dvi version of the above

voucher.sty -- a style for preparing expense vouchers.

vouchform.tex -- a template for vouchers.

vouch.Makefile -- a convenient make file for preparing vouchers

voucher -- a script to call vouch.Makefile with a parameter

voucher.README -- a readme for voucher.Makefile

showvoucher -- a script for displaying a voucher in postscript on a
        sun running X using pageview.

mm2tex -- a shell script for running the mm/troff to tm.sty/LaTeX
	translator; this should put in a directory in your path,
	perhaps the same directory as tex, latex, ...  Use as
	mm2tex mm_file.  Creates mm_file.tex (removing the extension
	mm if it exists).

mm2tex*.tex -- documentation for the translator

mm2tex.sh, mm2tex.awk, mm2tex.sed, mm2tex.sed2 -- parts of the
	translator. These should be placed in the macro directory
	($TEXINPUTS) or the env variable, MM2TEX should point to their
	directory.

README -- This file.

Recent changes to tm.sty

%	88-???-??	added copytocov & copytohere
%	88-Nov-02	added support for copyto lists longer than 1 pg
%	88-Nov-??	added makefacesignature
%	88-Dec-27	added labeled type list env
%	89-Jan-17	LaTeX extension that are of interest in other
%			  tm related styles moved to tmaddon.sty
%	89-Jan-20	fixed makeautherhead to allow for author's
%			  names that will not fit on one line.
%	89-Jan-20	fixed error for coversheet when no
%			  documentno's specified
%	89-Jan-25	added memo for file style coversheet, mffcoversheet
%	89-Jan-25	fixed bug in makesignature that removed parindent
%	89-Feb-28	fixed bug; undef tm@keywords if keywords not called
%	89-Mar-24	changed \topnumber from 2 to 5
%			changed \bottomnumber from 1 to 5
%			changed \totalnumber from 3 to 10
%			to allow more figures and tables per page
%	89-Apr-10	fixed bug(?) in tm*.sty (is also in art*.sty)
%			 that causes \part to clear even user heads 
%			 with markright in ps@myhead and ps@headandfoot.
%
%	89-May-19	fixed a bug in covereheet printing of document
%			 when two authors from same dept but diff 
%			 document numbers (esp first with none)
%	89-May-19	fixed abstract to allow it to extend across
%			 page break on first page.
%	89-May-19	input localpatch.sty at end to allow local
%			 variants
%	89-Sep-05	removed call to \bibstyle in \makehead since
%			 newer (>0.98) versions of bibtex do not
%			 tolerate redefining.
%	89-Sep-12	fixed bug in number of pages when restofcopyto...
%	89-Oct-11	added additional ITDS locations
%	89-Oct-11	changed makeauthorhead to better handle long
%			 names and eaddresses
%	89-Oct-11	touched up coversheet spaces for names,eaddr
%	89-Oct-12	simplified makesignature
%       89-Nov-21       make facesignature autoload bitmap.sty
%       89-Dec-13       moved \@cite redef to tmaddon
%       89-Dec-19       fixed \title*
%       90-Jan-15       fixed wrong quote mark on coversheet
%       90-Jan-16       broke up coversheet into smaller pieces (ideas
%                        from Peter F. Patel-Schneider)
%       90-Jan-16       fixed coversheet font size regardless of
%                        document font size (ideas from pfps)
%       90-Jan-23       minor changes to appearence of cover sheet
%       90-Feb-11       added bibliography* to allow use of bibtex
%                        with thebibliography* (no new page)
%       90-Feb-15       shortened pan line on 2nd pg of coversheet
%       90-Feb-22       fixed marginparwidth in tm*.sty
%       90-Apr-05       fixed the spacing of facesig's with null faces
%       90-Sep-11       changed to new ``from'' format
%       90-Oct-26       add extrapages macro to add to page count
%                        without effecting page numbering.
%       90-Nov-29       start coversheet with clearpage rather than newpage
%       91-Feb-05       made \date{\today} a default.
%       91-Apr-08       fixed raggedright in makehead esp subject
%       91-May-31       added \marknone to cancel propr marks etc
%
Recent changes to tmaddon.sty

%	89-Feb-01	add tmnotice
%	89-Feb-27	change draftmark to add date/time option;
%			  contributed by Dennis DeBruler
%	89-Feb-28	added \hhmm and \hhmm*
%	89-Aug-04	added simpleheadandfoot pagestyle
%	89-Aug-24	added varhline (var width hlines)
%       89-Dec-13       moved \@cite redef here from tm.sty
%       90-May-30       fixed markdraft to add bottommark even if
%                         propietary level not set
%       90-Jul-05       added inlinelist environment
%       90-Aug-27       changed UNIX tm notice
%       91-Feb-25       corrected spelling of propr notice USL
%       91-May-02       added numenum to force arabic numbered lists
%       91-May-17       removed right indent from the added enumerations
%       91-May-28       added symbol for cents.
%       91-Jun-05       added typed caption to override float type
%                        this is for generated TeX where type may not
%                        be known when float begun.

Recent changes to other distributed files

	88-Jun		added support for printing bitmaps and faces
	88-Dec-23	added authors.sty - an author database
	89-Jan-17	added attletter.sty - a letter style with
			  AT&T letterhead
	89-Mar-3	added lclocal.tex - a local guide for LC
	89-Sep-7	changed atttm.bst to be compatible with bibtex
			  .99.  Bibtex .98 compatible version renamed
			  atttm98.bst
	90-May-29	added FAX to attletter letterhead
	90-May-29	made attletter support \date
	91-Jan-09       made attletter support authors.sty
	91-Jun--19	added mm2tex

If you have problems installing any of these files or getting them
to work correctly contact:

	Terry L Anderson
	AT&T-BL 
	Dept 59112 
	LC 4N-E01
	184 Liberty Corner Rd
	Warren, NJ 07059 
	908 580-4428
	tla@bartok.att.com
	

These files may be freely distributed within AT&T, but are


                   AT&T -- PROPRIETARY
           Use pursuant to Company Instructions
