.nr *% \n(%#u+7u
.ds NR "\f2nroff\fP
.ds TR "\f2troff\|\fP
.ds Tr \f2Troff\|\fP
.ds Nr \f2Nroff\fP
.		\" CW - constant width font not from -ms
.de T&
.X "END US
.X "US T&
..
.de CW
.nr PQ \\n(.f
.if \\n(.$=0 .ft CW
.if \\n(.$>0 \%\&\\$3\f(CW\\$1\\f\\n(PQ\\$2
..
.de BI
.nr PQ \\n(.f
.if \\n(.$=0 .ft 4
.if \\n(.$>0 \%\&\\$3\f2\\$1\\f\\n(PQ\\$2
..
.de UC
\\$3\s-2\\$1\s+2\\$2
..
.am NH
.nr p \\np+1
.nr s 0 1
..
.fp 4 BI LucidaSansI
.bd 4 3
.de sc
.LP
\f4\\np.\\n+s.\ \ \\$1\f1\0
..
.de bt
.SP .25
.LP
.NE 2.1
.ta 1.5i 2.5i 3.5i 4.5i
\\$1	\\$2	\\$3	\\$4
.IP "" 0.8i
....br
\\$5
..
.
.
.
.
.
.
.
.TL
Troff User's Manual
.AU
Joseph F. Ossanna
Brian W. Kernighan
.sp
bwk@research.bell-labs.com
.EQ
delim @@
define cw % "\&" font CW %
.EN
.SH
Introduction
.PP
\*(Tr and \*(NR are text processors 
that format text for typesetter- and
typewriter-like terminals, respectively.
They accept lines of text interspersed with lines of
format control information and
format the text into a printable, paginated document
having a user-designed style.
\*(Tr and \*(NR offer
unusual freedom in document styling:
arbitrary style headers and footers;
arbitrary style footnotes;
multiple automatic sequence numbering for paragraphs, sections, etc;
multiple column output;
dynamic font and point-size control;
arbitrary horizontal and vertical local motions at any point;
and
a family of automatic overstriking, bracket construction, and
line-drawing functions.
.
.de TL
.LP
.ce
.ps +2
.ft B
..
.
.PP
.I Troff
produces its output in a device-independent form,
although parameterized for a specific device;
\*(TR output must be processed by a driver for that
device to produce printed output.
.PP
\*(Tr and \*(NR are highly compatible with each other and it is almost always
possible to prepare input acceptable to both.
Conditional input is provided to enable
the user to embed input expressly destined for either program.
\*(Nr can prepare output directly for a variety of terminal types and
is capable of utilizing the full resolution of each terminal.
\*(Nr is the same program as \*(TR; in fact, on Plan 9 
\*(NR is a shell script that calls \*(TR with the
.CW -N
argument.
.SH
Background to the Plan 9 Edition
.PP
The primary change to \*(TR and \*(NR for Plan 9 is
support of the Unicode Standard, which was added during
1992 and 1993.  There are two results.  First, there is much
less need for the myriad of two-character names that are so
much a part of \*(TR lore; in Plan 9, for example, one naturally uses the
Unicode character ½ instead of \*(TR\|'s
.CW \\e(12 .
Second, the output device, though called
.CW utf ,
is almost always a form of PostScript printer;
the panoply of special drivers for different typesetters
has largely disappeared.
Unfortunately, not all PostScript printers can cope
with Unicode characters, so there remains a need for
programs that synthesize PostScript characters from bitmaps;
this is especially true for Asian languages.
.SH
Background to the Second Edition
.PP
\*(Tr
was originally written by the late Joe Ossanna
in about 1973, in assembly language for the
.UC PDP -11,
to drive the Graphic Systems CAT typesetter.
It was rewritten in C around 1975,
and underwent slow but steady evolution until
Ossanna's death late in 1977.
.PP
In 1979, Brian Kernighan
modified
\*(TR
so that it would produce output for a variety of typesetters,
while retaining its input specifications.
Over the decade from 1979 to 1989,
the internals
have been modestly revised,
though much of the code remains as it was when Ossanna wrote it.
.PP
\*(Tr
reads parameter files
each time it is invoked, to
set values for machine resolution,
legal type sizes and fonts, and character names,
character widths
and the like.
\*(Tr
output is
.UC ASCII
characters
in a simple language
that describes where each character is to be placed
and in what size and font.
A post-processor must be written for each device
to convert this typesetter-independent language
into specific instructions for that device.
.PP
The output language contains information that was not readily
identifiable in the older output.
In the newer language, the beginning of each page, line, and word
is marked,
so post-processors can do device-specific optimizations
such as sorting the data vertically or printing it boustrophedonically,
independent of
\*(TR.
.PP
Capabilities for graphics have been added:
\*(TR
recognizes commands for drawing diagonal lines,
circles, ellipses, circular arcs,
and quadratic B-splines.
There are also ways to pass arbitrary information to the output,
unprocessed by
\*(TR.
.PP
A number of limitations have been eased or eliminated.
A document may have an arbitrary number of fonts on any page
(if the output device permits it, of course).
Fonts may be accessed merely by naming them;
``mounting'' is no longer necessary.
There are no limits on the number of characters.
\H'8'Character height\H'10' and \S'-1'sl\S'0'a\S'1'nt\S'0' may be set
independently of width.
.PP
The remainder of this document contains a description of
usage and command-line options;
a summary of requests, escape sequences, and pre-defined number registers;
a reference manual;
tutorial examples;
and a list of commonly-available characters.
.SH
Acknowledgements
.PP
Joe Ossanna's
\*(TR
remains a remarkable accomplishment.
For more than twenty years, it has proven a robust tool,
taking unbelievable abuse from a variety of preprocessors
and being forced into uses that were never conceived of
in the original design,
all with considerable grace under fire.
.PP
Recent versions of \*(TR have profited from
significant code improvements by
Jaap Akkerhuis, Dennis Ritchie, Ken Thompson, and Molly Wagner.
UTF facilities owe much to Jaap Akkerhuis.
Andrew Hume, Doug McIlroy, Peter Nelson and Ravi Sethi made valuable suggestions on the manual.
I fear that the remaining bugs are my fault.
.sp 100
.BP
.TL
Usage
.SP
.PP
\*(Tr or \*(NR is invoked as
.P1
troff  \fIoptions  files\fP
nroff  \fIoptions  files\fP
.P2
where @options@ represents any of a number of option arguments
and @files@ represents the list of files containing the document
to be formatted.
An argument consisting of a single minus
.CW - ' `
represents standard input.
If no filenames are given input is taken from the standard input.
The options, which may appear in any order so long as they appear
before the files, are:
.TS
center;
lfCW lw(4.5i).
-m@name@	T{
Read the macro file
@cw /sys/lib/tmac. name@
before the input @files@.
T}
-T@name@	T{
Specifies
the type of the output device.
Specific devices are site-dependent.
For
\*(TR,
the most useful name is
.CW utf .
For
\*(NR,
useful names include
@cw "37"@ for the (default) Model 37 Teletype,
@cw lp@ for ``dumb'' line printer terminals (no half-line motions,
no reverse motions),
and @cw think@ for the HP ThinkJet printer.
T}
-i	T{
Read standard input after the input files are exhausted.
T}
-o@list@	T{
Print only pages whose page numbers appear in @list@,
which consists of comma-separated numbers and number ranges.
A number range has the form @N-M@
and means pages @N@ through @M@;
a initial @-N@ means
from the beginning to page @N@; and a final @N-@ means
from @N@ to the end.
T}
-n@N@	T{
Number first generated page @N@.
T}
-r@aN@	T{
Set number register @a@ (one-character) to @N@.
T}
-s@N@	T{
Stop every @N@ pages.
\*(Nr will halt prior to every @N@ pages (default @N=1@)
to allow paper loading or
changing, and will resume upon receipt of a newline.
\*(Tr will include a ``pause'' code every @N@ pages;
its meaning, if any, depends on the output device.
T}
-u@N@	T{
Set amount of emboldening for the
.CW bd
request to @N@.
T}
-F@path@	T{
Look in directory @path@ for font information;
the defaults are
.CW /sys/lib/troff/font
and
.CW /sys/lib/troff/term
for \*(TR
and \*(NR respectively.
T}
.sp .5
	T{
                  \*(TR Only
T}
-a	T{
Send a printable approximation
of the results to the standard output.
T}
.sp .5
	T{
                  \*(NR Only
T}
-e	T{
Produce equally-spaced words in adjusted
lines, using full terminal resolution.
T}
-h	T{
Use tabs instead of spaces
to speed up printing.
T}
-q	T{
Invoke the simultaneous input-output mode of the @cw rd@ request.
T}
.TE
.PP
Each option is a separate argument;
for example,
.P1
troff -Tutf -ms -mpictures -o4,6,8-10 \f2file1 file2\fP
.P2
requests formatting of pages 4, 6, and 8 through 10 of a document contained in the files
named \f2file1\fP and \f2file2\fP,
specifies the output in UTF,
and invokes the macro packages
.CW -ms
and
.CW -mpictures .
.PP
Various pre- and post-processors are available for use with \*(NR and \*(TR.
These include the equation preprocessor
.I eqn
(for \*(TR only),
the table-construction preprocessor
.I tbl ,
and
.I pic 
and
.I grap
for various forms of graphics.
.sp 100
.BP
.TL
Request Summary
.PP
In the following table,
the notation @+- N@ in the
.BI "Request Form
column means that the forms @N@, @+N@, or @-N@ are permitted,
to set the parameter to @N@, increment it by @N@, or decrement it by @N@,
respectively.
Plain @N@ means that the value is used to set the parameter.
.BI "Initial Values
separated by 
.CW ;
are for
\*(TR
and
\*(NR
respectively.
In the 
.BI Notes
column,
.TS
center;
c lw(4.5i).
B	T{
Request normally causes a break.
The use of
.CW ' \&
as control character (instead of
.CW . )\&
suppresses the break function.
T}
D	T{
Mode or relevant parameters associated with current diversion level.
T}
E	T{
Relevant parameters are a part of the current environment.
T}
O	T{
Must stay in effect until logical output.
T}
P	T{
Mode must be still or again in effect at the time of physical output.
T}
T	T{
\*(TR only; no effect in \*(NR.
T}
@bold v@, @bold p@, @bold m@, @bold u@	T{
Default scale indicator; if not specified, scale indicators are ignored.
T}
.TE
.sp
.tr &.
.ps 9
.vs 11
.nr z 0 1
.TS
lf2 lf2 lf2 lf2 lf2
lf2 lf2 lf2 lf2 lf2
lfCW l l l l.
Request	Initial	If No
Form	Value	Argument	Notes	Explanation
.sp .5
.T&
lf3 s s s s.
\\n+z.  General Information
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Font and Character Size Control
.sp .5
&ps @+- N@	10 point	previous	E,T	Point size; also @cw "\es" +- N@.
&ss @N@	12/36\fBm\fP	ignored	E,T	Space-character size set to @N/36@ em.
&cs @ F~N~ M@	off	-	P,T	Constant character space (width) mode (font @F@).
&bd @F~N@	off	-	P,T	Embolden font @F@ by @N-1@ units.
&bd S@~F~N@	off	-	P,T	Embolden Special Font when current font is @F@.
&ft@~F@	Roman	previous	E	Change to font @F@; also @cw "\ef" x@, @cw "\ef(" xx@, @cw "\ef" N@.
&fp@~N~F~L@	R,I,B,...,S	ignored	-	Mount font named @F@ on physical position @N <= 1@;
				  long name is @L@ if given.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Page Control
&pl @+- N@	11i	11i	@bold v@	Page length.
&bp @+- N@	@N=1@	-	B,@bold v@	Eject current page; next page number @N@.
&pn @+- N@	@N=1@	ignored	-	Next page number @N@.
&po @+- N@	1i; 0	previous	@bold v@	Page offset.
&ne @N@	-	@N=1 roman v@	D,@bold v@	Need @N@ vertical space.
&mk @R@	none	internal	D	Mark current vertical place in register @R@.
&rt @+- N@	none	internal	D,@bold v@	Return (upward only) to marked vertical place.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Text Filling, Adjusting, and Centering
&br	-	-	B	Break.
&fi	fill	-	B,E	Fill output lines.
&nf	fill	-	B,E	No filling or adjusting of output lines.
&ad @c@	adj, both	adjust	E	Adjust output lines with mode @c@;  @c = cw l , cw r , cw c , cw b , none@
&na	adjust	-	E	No output line adjusting.
&ce @N@	off	@N=1@	B,E	Center next @N@ input text lines.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Vertical Spacing
&vs @N@	12p; 1/6i	previous	E,@bold p@	Vertical baseline spacing (@V@).
&ls @N@	@N=1@	previous	E	Output @N-1@ @bold v@'s after each text output line.
&sp @N@	-	@N=1@v	B,@bold v@	Space vertical distance @N@ in either direction.
&sv @N@	-	@N=1@v	@bold v@	Save vertical distance @N@.
&os	-	-	-	Output saved vertical distance.
&ns	space	-	D	Turn no-space mode on.
&rs	-	-	D	Restore spacing; turn no-space mode off.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Line Length and Indenting
&ll @+- N@	6.5i	previous	E,@bold m@	Line length.
&in @+- N@	@N=0@	previous	B,E,@bold m@	Indent.
&ti @+- N@	-	ignored	B,E,@bold m@	Temporary indent.
.sp .5
.ne 2.1
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Macros, Strings, Diversion, and Position Traps
&de @xx~yy@	-	@.yy= cw ".."@	-	Define or redefine macro @xx@; end at call of @yy@.
&am @xx~yy@	-	@.yy= cw ".."@	-	Append to a macro.
&ds @xx~string@	-	ignored	-	Define a string @xx@ containing @string@.
&as @xx~string@	-	ignored	-	Append @string@ to string @xx@.
&rm @xx@	-	ignored	-	Remove request, macro, or string.
&rn @xx~yy@	-	ignored	-	Rename request, macro, or string @xx@ to @yy@.
&di @xx@	-	end	D	Divert output to macro @xx@.
&da @xx@	-	end	D	Divert and append to @xx@.
&wh @N~xx@	-	-	@bold v@	Set location trap; negative is w.r.t. page bottom.
&ch @xx~N@	-	-	@bold v@	Change trap location.
&dt @N~xx@	-	off	D,@bold v@	Set a diversion trap.
&it @N~xx@	-	off	E	Set an input-line count trap.
&em @xx@	none	none	-	End macro is @xx@.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Number Registers
&nr @R~+- N~M@		-	@bold u@	Define and set number register @R@;
				  auto-increment by @M@.
&af @R~c@	arabic	-	-	Assign format to register @R@ (@c= cw "1" , cw i , cw I , cw a , cw A@).
&rr @R@	-	-	-	Remove register @R@.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Tabs, Leaders, and Fields
&ta@~Nt~. . .@	0.5i; 0.8n	none	E,@bold m@	Tab settings; left-adjusting, unless
				  @t= cw R@ (right), @cw C@ (centered).
&tc@~c@	none	none	E	Tab repetition character.
&lc@~c@	@cw "."@	none	E	Leader repetition character.
&fc@~a~b@	off	off	-	Set field delimiter @a@ and pad character @b@.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Input and Output Conventions and Character Translations
&ec@~c@	\e	\e	-	Set escape character.
&eo	on	-	-	Turn off escape character mechanism.
&lg@~N@	on; -	on	T	Ligature mode on if @N>0@.
&ul@~N@	off	@N=1@	E	Underline (italicize in \*(TR\^) @N@ input lines.
&cu@~N@	off	@N=1@	E	Continuous underline in \*(NR; in \*(TR, like @cw ul@.
&uf@~F@	Italic	Italic	-	Underline font set to @F@ (to be switched to by @cw ul@).
&cc@~c@	@cw .@	@cw .@	E	Set control character to @c@.
&c2@~c@	@cw "'"@	@cw "'"@	E	Set no-break control character to @c@.
&tr@~abcd....@	none	-	O	Translate @a@ to @b@, etc., on output.
.sp .5
.T&
lf3 s s s s.
\\n+z.  Local Horizontal and Vertical Motions, and the Width Function
.sp .5
.T&
lf3 s s s s.
\\n+z.  Overstrike, Bracket, Line-drawing, Graphics, and Zero-width Functions
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Hyphenation.
&nh	hyphenate	-	E	No hyphenation.
&hy@~N@	hyphenate	hyphenate	E	Hyphenate; @N =@ mode.
&hc@~c@	@cw "\e%"@	@cw "\e%"@	E	Hyphenation indicator character @c@.
&hw@~word~. . .@		ignored	-	Add words to hyphenation dictionary.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Three-Part Titles.
&tl@~'l'c'r'@		-	-	Three-part title; delimiter may be any character.
&pc@~c@	@cw %@	off	-	Page number character.
&lt@~+- N@	6.5i	previous	E,@bold m@	Length of title.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Output Line Numbering.
&nm@~+- N^M^S^I@		off	E	Number mode on or off, set parameters.
&nn@~N@	-	@N=1@	E	Do not number next @N@ lines.
.sp .5
.ne 2
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Conditional Acceptance of Input
&if@~c~any@		-	-	If condition @c@ true, accept @any@ as input;
				  for multi-line, use @cw "\e{" any cw "\e}"@.
&if !@c~any@		-	-	If condition @c@ false, accept @any@.
&if@~N~any@		-	@bold u@	If expression @N > 0@, accept @any@.
&if !@N~any@		-	@bold u@	If expression @N <= 0@ [sic], accept @any@.
&if@~ 's1 's2 '~any@		-	-	If string @s1@ identical to @s2@, accept @any@.
&if !@ 's1 's2 '~any@		-	-	If string @s1@ not identical to @s2@, accept @any@.
&ie@~c~any@		-	@bold u@	If portion of if-else; all above forms (like @cw "if"@).
&el@~any@		-	-	Else portion of if-else.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Environment Switching
&ev@~N@	@N=0@	previous	-	Environment switch (push down).
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Insertions from the Standard Input
&rd@~prompt@	-	@prompt@=\s-1BEL\s+1	-	Read insertion.
&ex	-	-	-	Exit.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Input/Output File Switching
&so@~filename@		-	-	Switch source file (push down).
&nx@~filename@		end-of-file	-	Next file.
&sy@~string@		-	-	Execute program @string@.  Output not interpolated.
&pi@~string@		-	-	Pipe output to program @string@.
&cf@~filename@		-	-	Copy file contents to \*(TR output.
.sp .5
.T&
lf3 s s s s
lfCW l l l l.
\\n+z.  Miscellaneous
&mc@~c~N@	-	off	E,@bold m@	Set margin character @c@ and separation @N@.
&tm@~string@	-	newline	-	Print @string@ on terminal (standard error).
&ab@~string@	-	newline	-	Print @string@ on standard error, exit program.
&ig@~yy@	-	@.yy= cw ".."@	-	Ignore input until call of @yy@.
&lf@~N ~f@		-	-	Set input line number to @N@ and filename to @f@.
&pm@~t@	-	all	-	Print macro names, sizes; if @t@ present, print total.
&fl	-	-	B	Flush output buffer.
.sp .5
.T&
lf3 s s s s.
\\n+z.  Output and Error Messages
.sp .5
\\n+z.  Output Language
.sp .5
\\n+z.  Device and Font Description Files
.TE
.br
.nr zz 9
.de cl
.ie \\n+(cl<\n(zz \{\
.	po +\\n(.lu/\n(zzu
.	rt\}
.el \{.po 1i\}
..
.nr cl 0 1
.di zz
.ta .45iR
... was .35
.nf
.ps 9
.vs 10.5
\f(CWab\fP	20
\f(CWad\fP	4
\f(CWaf\fP	8
\f(CWam\fP	7
\f(CWas\fP	7
\f(CWbd\fP	2
\f(CWbp\fP	3
\f(CWbr\fP	4
\f(CWc2\fP	10
\f(CWcc\fP	10
\f(CWce\fP	4
\f(CWcf\fP	19
\f(CWch\fP	7
\f(CWcs\fP	2
\f(CWcu\fP	10
\f(CWda\fP	7
\f(CWde\fP	7
\f(CWdi\fP	7
\f(CWds\fP	7
\f(CWdt\fP	7
\f(CWec\fP	10
\f(CWel\fP	16
\f(CWem\fP	7
\f(CWeo\fP	10
\f(CWev\fP	17
\f(CWex\fP	18
\f(CWfc\fP	9
\f(CWfi\fP	4
\f(CWfl\fP	20
\f(CWfp\fP	2
\f(CWft\fP	2
\f(CWhc\fP	13
\f(CWhw\fP	13
\f(CWhy\fP	13
\f(CWie\fP	16
\f(CWif\fP	16
\f(CWig\fP	20
\f(CWin\fP	6
\f(CWit\fP	7
\f(CWlc\fP	9
\f(CWlg\fP	10
\f(CWlf\fP	20
\f(CWll\fP	6
\f(CWls\fP	5
\f(CWlt\fP	14
\f(CWmc\fP	20
\f(CWmk\fP	3
\f(CWna\fP	4
\f(CWne\fP	3
\f(CWnf\fP	4
\f(CWnh\fP	13
\f(CWnm\fP	15
\f(CWnn\fP	15
\f(CWnr\fP	8
\f(CWns\fP	5
\f(CWnx\fP	19
\f(CWos\fP	5
\f(CWpc\fP	14
\f(CWpi\fP	19
\f(CWpl\fP	3
\f(CWpm\fP	20
\f(CWpn\fP	3
\f(CWpo\fP	3
\f(CWps\fP	2
\f(CWrd\fP	18
\f(CWrm\fP	7
\f(CWrn\fP	7
\f(CWrr\fP	8
\f(CWrs\fP	5
\f(CWrt\fP	3
\f(CWso\fP	19
\f(CWsp\fP	5
\f(CWss\fP	2
\f(CWsv\fP	5
\f(CWsy\fP	19
\f(CWta\fP	9
\f(CWtc\fP	9
\f(CWti\fP	6
\f(CWtl\fP	14
\f(CWtm\fP	20
\f(CWtr\fP	10
\f(CWuf\fP	10
\f(CWul\fP	10
\f(CWvs\fP	5
\f(CWwh\fP	7
.di
.nr aa \n(dn/\n(zz
.ne \\n(aau+10p
.sp
.SP 2
.TL
Alphabetical Request and Section Number Cross Reference
.SP .5
.LP
.sp .5
.nf
.wh \n(nlu+\n(aau cl
.nr qq \n(nlu+\n(aau
.ps
.vs
.mk
.zz
.rt
.sp \n(.tu
.ch cl 12i
.sp 100
.BP
.TL
Escape Sequences for Characters, Indicators, and Functions
.SP .5
.LP
.ps -1
.vs -1
.TS
center;
c2 l
c2 l2 l
n2 l2fCW l.
.ft 4
Section	Escape
Reference	Sequence	Meaning
.ft
.sp .5
10.1	\e\e	\&\f(CW\e\fP prevents or delays the interpretation of \&\f(CW\e\fP
10.1	\ee	Printable version of the current escape character.
2.1	\e'	\' (acute accent); equivalent to \&\f(CW\e(aa\fP
2.1	\e`	\` (grave accent); equivalent to \&\f(CW\e(ga\fP
2.1	\e\-	\- Minus sign in the current font
7.	\e\^.	Period (dot) (see \&\f(CWde\fP)
11.1	\e\f2space\fP	Unpaddable space-size space character
11.1	\e0	Digit width space
11.1	\e|	1/6 em narrow space character (zero width in \*(NR\^)
11.1	\e^	1/12 em half-narrow space character (zero width in \*(NR\^)
.tr &&
4.1	\e&	Non-printing, zero width character
.tr &.
10.6	\e!	Transparent line indicator
10.8	\e"	Beginning of comment; continues to end of line
13.	\e%	Default optional hyphenation character
2.1	\e(@xx@	Character named @xx@
7.1	\e*@x,~@\e*(@xx@	Interpolate string @x@ or @xx@
7.3	\e$@N@	Interpolate argument @1 <= N <= 9@
9.1	\ea	Non-interpreted leader character
12.3	\eb'@abc...@'	Bracket building function
4.2	\ec	Connect to next input text
2.1	\eC'@xyz@'	Character named @xyz@
11.1	\ed	Downward 1/2 em vertical motion (1/2 line in \*(NR\^)
12.5	\eD'@c...@'	Draw graphics function @c@ with parameters @. . .@; @c= cw l , cw c , cw e , cw a , cw "~"@
2.2	\ef@x,~@\ef(@xx,~@\ef@N@	Change to font named @x@ or @xx@, or position @N@
8.	\eg@x,~@\eg(@xx@	Format of number register @x@ or @xx@
11.1	\eh'@N@'	Local horizontal motion; move right @N@ (negative left)
2.3	\eH'@N@'	Height of current font is @N@
11.3	\ek@x@	Mark horizontal input place in register @x@
12.4	\el'@Nc@'	Horizontal line drawing function (optionally with @c@ )
12.4	\eL'@Nc@'	Vertical line drawing function (optionally with @c@ )
8.	\en@x,~@\en(@xx@	Contents of number register @x@ or @xx@
2.1	\eN'@N@'	Character number @N@ on current font
12.1	\eo'@abc...@'	Overstrike characters @a,~ b,~ c@, ...
4.1	\ep	Break and spread output line
11.1	\er	Reverse 1 em vertical motion (reverse line in \*(NR\^)
2.3	\es@N,~@\es@+- N@	Point-size change function; also @cw "\es(" nn@, @cw "\es" +- cw "(" nn@
2.2	\eS'@N@'	Slant output @N@ degrees
9.1	\et	Non-interpreted horizontal tab
11.1	\eu	Reverse (up) 1/2 em vertical motion (1/2 line in \*(NR\^)
11.1	\ev'@N@'	Local vertical motion; move down N (negative up)
11.2	\ew'@string@'	Width of @string@
5.2	\ex'@N@'	Extra line-space function (negative before, positive after)
10.7	\eX'@string@'	Output @string@ as device control function
12.2	\ez@c@	Print @c@ with zero width (without spacing)
16.	\e{	Begin conditional input
16.	\e}	End conditional input
10.8	\e@newline@	Concealed (ignored) newline
-	\e@Z@	@Z@, any character not listed above
.TE
.ps +1
.vs +1
.LP
The escape sequences
.CW \e\e ,
.CW \e\^. ,
.CW \e" ,
.CW \e$ ,
.CW \e* ,
.CW \ea ,
.CW \en ,
.CW \et ,
.CW \eg ,
and
.CW \e@newline@
are interpreted in copy mode (§7.2).
.SP .5i
\0
.sp 100
.BP
.TL
Predefined Number Registers
.LP
.ps -1
.vs -1
.TS
c2l
c2 l2 l
n2 l2fCW l.
.ft 4
Section	Register
Reference	Name	Description
.ft
.sp .5
3.	%	Current page number.
11.2	ct	Character type (set by \&\f(CW\ew\fP function).
7.4	dl	Width (maximum) of last completed diversion.
7.4	dn	Height (vertical size) of last completed diversion.
-	dw	Current day of the week (1-7).
-	dy	Current day of the month (1-31).
15.	ln	Output line number.
-	mo	Current month (1-12).
4.1	nl	Vertical position of last printed text baseline.
11.2	sb	Depth of string below baseline (generated by \&\f(CW\ew\fP function).
11.2	st	Height of string above baseline (generated by \&\f(CW\ew\fP function).
-	yr	Last two digits of current year.
.TE
.ps +1
.vs +1


.TL
Predefined Read-Only Number Registers
.LP
.ps -1
.vs -1
.TS
c2 l
c2 l2 l
n2 l2fCW l.
.ft 4
Section	Register
Reference	Name	Description
.ft
.sp .5
19.	$$	Process id of \*(TR or \*(NR.
7.3	&$	Number of arguments available at the current macro level.
5.2	&a	Post-line extra line-space most recently used in @cw "\ex'" N cw "'" @.
-	&A	Set to 1 in \*(TR, if @cw -a@ option used; always 1 in \*(NR.
2.3	&b	Emboldening level.
20.	&c	Number of lines read from current input file.
7.4	&d	Current vertical place in current diversion; equal to @cw nl@, if no diversion.
2.2	&f	Current font number.
20.	&F	Current input file name [sic].
4.	&h	Text baseline high-water mark on current page or diversion.
11.1	&H	Available horizontal resolution in basic units.
6.	&i	Current indent.
4.2	&j	Current @cw ad@ mode.
4.1	&k	Current output horizontal position.
6.	&l	Current line length.
5.1	&L	Current @cw ls@ value.
4.	&n	Length of text portion on previous output line.
3.	&o	Current page offset.
3.	&p	Current page length.
7.5	.R	Number of unused number registers.
-	&T	Set to 1 in \*(NR, if \&\f(CW-T\fP option used; always 0 in \*(TR.
2.3	&s	Current point size.
7.5	&t	Distance to the next trap.
4.1	&u	Equal to 1 in fill mode and 0 in nofill mode.
5.1	&v	Current vertical line spacing.
11.1	&V	Available vertical resolution in basic units.
11.2	&w	Width of previous character.
-	&x	Reserved version-dependent register.
-	&y	Reserved version-dependent register.
7.4	&z	Name [sic] of current diversion.
.TE
.ps +1
.vs +1
.sp 100
.BP
.TL
Reference Manual
.NH
General Explanation
.sc "Form of input.
Input consists of \fItext lines\fR, which are destined to be printed,
interspersed with \fIcontrol lines\fR,
which set parameters or otherwise control subsequent processing.
Control lines begin with a \fIcontrol character\fR\(em\
normally \&\f(CW.\fR (period) or \&\f(CW'\fR (single quote)\(em\
followed by a one or two character name that specifies
a basic \fIrequest\fR or the substitution of
a user-defined \fImacro\fR in place of the control line.
The control character \&\f(CW'\fR suppresses the \fIbreak\fR function\(em\
the forced output of a partially filled line\(em\
caused by certain requests.
The control character may be separated from the request/macro name by
white space (spaces and/or tabs) for aesthetic reasons.
Names should be followed by either
space or newline.
Control lines with unrecognized names are ignored.
.PP
Various special functions may be introduced anywhere in the input by
means of an \fIescape\fR character, normally \&\f(CW\e\fR.
For example, the function
.CW \en@R@
causes the interpolation of the contents of the
\fInumber register R\fR
in place of the function;
here @R@ is either a single character name
as in \&\f(CW\en\fIx\fR,
or a two-character name introduced by
a left-parenthesis, as in \&\f(CW\en(\fIxx\fR.
.sc "Formatter and device resolution.
\*(Tr internally stores and processes dimensions in units that correspond to
the particular device for which output is being prepared;
values from 300 to 1200/inch are typical.
See §23.
\*(Nr internally uses 240 units/inch,
corresponding to the least common multiple of the
horizontal and vertical resolutions of various
typewriter-like output devices.
\*(Tr rounds horizontal/vertical numerical parameter input to the actual
horizontal/vertical resolution of the output device indicated by the \&\f(CW-T\fR option
(default
.CW post ).
\*(Nr similarly rounds numerical input to the actual resolution
of its output device
(default Model 37 Teletype).
.sc "Numerical parameter input.
Both \*(NR and \*(TR
accept numerical input with the appended scale
indicators
shown in the following table,
where
\fIS\fR is the current type size in points and
\fIV\fR is the current vertical line spacing in
basic units.
.TS
center box;
c|c
c|c
c|l.
Scale
Indicator	Meaning
_
\&\f(CWi\fR	Inch
\&\f(CWc\fR	Centimeter
\&\f(CWP\fR	Pica = 1/6 inch
\&\f(CWm\fR	Em = \fIS\fR points
\&\f(CWn\fR	En = Em/2
\&\f(CWp\fR	Point = 1/72 inch
\&\f(CWu\fR	Basic unit
\&\f(CWv\fR	Vertical line space \fIV\fR
none	Default, see below
.TE
In \*(NR, both the em and the en are taken to be equal to the
nominal character width,
which is output-device dependent;
common values are 1/10 and 1/12 inch.
Actual character widths in \*(NR need not be all the same and constructed characters
such as -> (→) are often extra wide.
The default scaling is
.CW m
for the horizontally-oriented requests
and functions
.CW ll ,
.CW in ,
.CW ti ,
.CW ta ,
.CW lt ,
.CW po ,
.CW mc ,
.CW \eh ,
.CW \el ,
and horizontal coordinates of
.CW \eD ;
.CW v
for the vertically-oriented requests and functions
.CW pl ,
.CW wh ,
.CW ch ,
.CW dt ,
.CW sp ,
.CW sv ,
.CW ne ,
.CW rt ,
.CW \ev ,
.CW \ex ,
.CW \eL ,
and vertical coordinates of
.CW \eD ;
.CW p
for the
.CW vs
request;
and
.CW u
for the requests
.CW nr ,
.CW if ,
and
.CW ie .
\fIAll\fR other requests ignore any scale indicators.
When a number register containing an already appropriately scaled number
is interpolated to provide numerical input,
the unit scale indicator
\&\f(CWu\fR may need to be appended to prevent
an additional inappropriate default scaling.
The number, @N@, may be specified in decimal-fraction form
but the parameter finally stored is rounded to an integer number of basic units.
Internal computations are performed in integer arithmetic.
.PP
The \fIabsolute position\fR indicator \&\f(CW|\fR may be prefixed
to a number @N@
to generate the distance to the vertical or horizontal place @N@.
For vertically-oriented requests and functions, \&\f(CW|\fP@N@
becomes the distance in basic units from the current vertical place on the page or in a \fIdiversion\fR (§7.4)
to the vertical place @N@.
For \fIall\fR other requests and functions,
\&\f(CW|\fP@N@
becomes the distance from
the current horizontal place on the \fIinput\fR line to the horizontal place @N@.
For example,
.P1
\&.sp |3.2c
.P2
will space in the required direction to 3.2 centimeters from the top of the page.
.sc "Numerical expressions.
.tr &&
Wherever numerical input is expected,
an expression involving parentheses,
the arithmetic operators \&\f(CW+\fR, \&\f(CW-\fR, \&\f(CW/\fR, \&\f(CW\(**\fR, \&\f(CW%\fR (mod),
and the logical operators
\&\f(CW<\fR,
\&\f(CW>\fR,
\&\f(CW<=\fR,
\&\f(CW>=\fR,
\&\f(CW=\fR (or \&\f(CW==\fR),
\&\f(CW&\fR\ (and),
\&\f(CW:\fR\ (or)
may be used.
Except where controlled by parentheses, evaluation of expressions is left-to-right;
there is no operator precedence.
In the case of certain requests, an initial \&\f(CW+\fR or \&\f(CW-\fR is stripped
and interpreted as an increment or decrement indicator respectively.
In the presence of default scaling, the desired scale indicator must be
attached to \fIevery\fR number in an expression
for which the desired and default scaling differ.
For example,
if the number register \&\f(CWx\fR contains 2
and the current point size is 10,
then
.P1
\&.ll (4.25i+\enxP+3)/2u
.P2
will set the line length to 1/2 the sum of 4.25 inches + 2 picas + 3 ems.
.sc "Notation.
Numerical parameters are indicated in this manual in two ways.
@+- N@ means that the argument may take the forms @N@, @+N@, or @-N@ and
that the corresponding effect is to set the parameter
to @N@, to increment it by @N@, or to decrement it by @N@ respectively.
Plain @N@ means that an initial algebraic sign is \fInot\fR
an increment indicator,
but merely the sign of @N@.
Generally, unreasonable numerical input is either ignored
or truncated to a reasonable value.
For example,
most requests expect to set parameters to non-negative
values;
exceptions are
.CW sp ,
.CW wh ,
.CW ch ,
.CW nr ,
and
.CW if .
The requests
.CW ps ,
.CW ft ,
.CW po ,
.CW vs ,
.CW ls ,
.CW ll ,
.CW in ,
and
.CW lt 
restore the previous parameter value in the absence
of an argument.
.PP
Single character arguments are indicated by single lower case letters
and
one/two character arguments are indicated by a pair of lower case letters.
Character string arguments are indicated by multi-character mnemonics.
.NH
Font and Character Size Control
.sc "Character set.
The \*(TR character set is defined by a description file specific to each output device (§23).
There are normally several regular fonts and one or more special fonts.
Characters are input as themselves,
as @cw "\e(" xx@, as @cw "\eC'" name cw "'"@,
or as 
.CW \eN'@n@' .
The form
.CW \eC'@name@'
permits a name of any length;
the form
.CW \eN'@n@'
refers to the @n@-th character on the current font,
whether named or not.
.PP
Normally the input characters
.CW ` ,
.CW ' ,
and
.CW -
are printed as `, ', and - respectively;
.CW \e` ,
.CW \e' ,
and
.CW \e-
produce \`, \', and \-.
If the character does not exist in the font, \*(TR assumes the width is 1 em and
outputs the character with a
.CW C
name as defined in Section 22.
(This is independent of how the device handles characters unknown to it.)
.PP
\*(Nr has an analogous, but different, mechanism for defining legal characters
and how to print them.
By default all characters are valid.
There are such
additional characters as may be available on
the output device,
such characters as may be constructed
by overstriking or other combination,
and those that can reasonably be mapped
into other printable characters.
The exact behavior is determined by a driving
table prepared for each device.
.sc "Fonts.
\*(Tr
begins execution by reading information for a set of defaults fonts,
said to be
.I mounted ;
conventionally, the first four are
Times Roman (\&\f(CWR\fR),
Times Italic
(\&\f(CWI\fR),
Times Bold
(\&\f(CWB\fR),
and
Times Bold Italic
(\&\f(CWBI\fR) ,
and the last is a Special font
.CW S ) (
containing miscellaneous characters.
(This document uses Lucida Sans in place of Times.)
The set of fonts and positions is determined by the device description file,
described in §23.
.PP
The current font, initially Roman, may be changed
by the \&\f(CWft\fR request,
or by embedding at any desired point
\&\f(CW\ef\fIx\fR, \&\f(CW\ef(\fIxx\fR, or \&\f(CW\ef\fP@N@,
where
\fIx\fR and \fIxx\fR are the name of a font
and @N@ is a numerical font position.
.PP
It is not necessary to change to the Special font;
characters on that font are automatically handled
as if they were physically part of the current font.
The Special font may actually be several fonts;
the name
.CW S
is reserved and is generally used for one of these.
All special fonts must be mounted after regular fonts.
.PP
\*(Tr can be informed that any particular font is mounted
by use of the \&\f(CWfp\fR request.
The list of known fonts is installation dependent.
In the subsequent discussion of font-related requests,
@F@ represents either a one/two-character
font name or the numerical font position.
The current font is available (as a numerical position) in the read-only number register \&\f(CW.f\fR.
.PP
A request for a named but not-mounted font is honored
if the font description information exists.
In this way, there is no limit on the number of fonts that may be printed
in any part of a document.
Mounted fonts may be handled more efficiently,
and they may be referred to by their mount positions,
but there is no other difference.
Mention of an unmounted font loads it temporarily at font position
zero, which serves as a one-font cache.
.PP
The function
.CW \eS'@+- N@'
causes the current font to be slanted by
@+- N@
degrees.
Not all devices support slanting.
.PP
\*(Nr understands font control
and normally underlines italic characters (see §10.5).
.sc "Character size.
Character point sizes available depend on the specific output device;
a typical (historical) set of values is
6, 7, 8, 9, 10, 11, 12, 14, 16, 18, 20, 22, 24, 28, and 36.
This is a range of 1/12 inch to 1/2 inch.
The \&\f(CWps\fR request is used to change or restore the point size.
Alternatively the point size may be changed between any two characters
by embedding a
.CW \es@N@
at the desired point
to set the size to @N@,
or a
.CW "\&\f(CW\es@+- N@
(@1 <= N <= 9@)
to increment/decrement the size by @N@;
.CW \es0
restores the previous size.
Requested point size values that are between two valid
sizes yield the larger of the two.
.PP
Note that through an accident of history, a construction like
.CW \es39
is parsed as size 39, and thus converted to size 36 (given the sizes above),
while
.CW \es40
is parsed as size 4 followed by
.CW 0 .
The forms
@cw "\es(" nn@ and @cw "\es" +- cw "(" nn@
permit specification of sizes that would otherwise be ambiguous.
.PP
The current size is available in the \&\f(CW.s\fR register.
\*(Nr ignores type size requests.
.PP
The function
.CW "\eH'@+- N@'
sets \H'+2'the height of the current font\H'0' to
@N@, or increments it by @+N@, or decrements it by @-N@;
if @N=0@, the height is restored to the current point size.
In each case, the width is unchanged.
Not all devices support independent height and width for characters.
.FS
*The fields have the same meaning as described earlier in the Request Summary.
.FE
.SP .5
.LP
.ne 2.1
.ta 1.5i 2.5i 3.5i 4.5i
\f2Request\fR	\f2Initial\fR	\f2If\ No\fR
.br
\f2Form\fR	\f2Value\fR	\f2Argument\fR	\f2Notes\fR
.bt "\&\f(CW.ps\fI \(+-N\fR*" "10\|point" "previous" "E" "Point size
set to @+- N@.
Alternatively, embed
.CW \es@N@
or
.CW "\&\f(CW\es@+- N@" .
Any positive size value may be requested;
if invalid, the next larger valid size will result, with a
maximum of 36.
A paired sequence
@+N@, @-N@
will work because the previous requested value is also remembered.
Ignored in \*(NR.
.bt "\&\f(CW.ss\fI N\fR" "12/36\|em" "ignored" "E" "Space-character size
(i.e., inter-word gap)
is set to @N@/36 ems.
This size is the minimum word spacing in adjusted text.
Ignored in \*(NR.
.bt "\&\f(CW.cs\fI\|F\|N\|M\fR" "off" "-" "P" "Constant character space
(width) mode is
set on for font @F@ (if mounted); the width of every character will be
taken to be @N@/36 ems.
If @M@ is absent,
the em is that of the character's point size;
if @M@ is given,
the em is @M@ points.
All affected characters
are centered in this space, including those with an actual width
larger than this space.
Special Font characters occurring while the current font
is @F@ are also so treated.
If @N@ is absent, the mode is turned off.
The mode must be in effect when the characters are physically printed.
Ignored in \*(NR.
.bt "\&\f(CW.bd\fI F N\fR" "off" "-" "P" "The characters in font @F@ will be artificially
emboldened by printing each one twice, separated by @N-1@ basic units.
A reasonable value for @N@ is 3 when the character size is near 10 points.
If @N@ is missing the embolden mode is turned off.
The emboldening value @N@ is in the \&\f(CW.b\fP register.
.IP
.bd R 3
This paragraph is printed with \&\f(CW.bd R 3\fR.
The mode must be in effect when the characters are physically printed.
Ignored in \*(NR.
.br
.bd R
.bt "\&\f(CW.bd S \fIF N\fR" "off" "-" "P" "The characters in the Special font
will be emboldened whenever the current font is @F@.
The mode must be in effect when the characters are physically printed.
Ignored in \*(NR.
.bt "\&\f(CW.ft\fP @F@" "Roman" "previous" "E" "Font changed to
@F@.
Alternatively, embed
.CW \ef@F@ .
The font name \&\f(CWP\fR is reserved to mean the previous font,
and the name
.CW S
for the special font.
.bt "\&\f(CW.fp \fIN F L\fR" "R,I,B,...,S" "ignored" "-" "Font position.
This is a statement
that a font named @F@ is associated with position @N@.
It is a fatal error if @F@ is not known.
For fonts with names longer than two characters,
.I L
refers to the long name,
and
.I F
becomes a synonym.
There is generally a limit of about 10 mounted fonts.
.NH
Page control
.PP
Top and bottom margins are not automatically provided;
it is conventional to define two \fImacros\fR and to set \fItraps\fR
for them at vertical positions 0 (top) and @-N@ (distance @N@ up from the bottom).
See §7 and Tutorial Examples §T2.
A pseudo-page transition onto the first page occurs
either when the first \fIbreak\fR occurs or
when the first \fInon-diverted\fR text processing occurs.
Arrangements
for a trap to occur at the top of the first page
must be completed before this transition.
In the following, references to the \fIcurrent diversion\fR (§7.4)
mean that the mechanism being described works during both
ordinary and diverted output (the former considered as the top diversion level).
.PP
The limitations on \*(TR and \*(NR output dimensions
are device dependent.
.bt "\&\f(CW.pl\fI \(+-N\fR" "11\|in" "11\|in" "\fBv\fR" "Page length set to @+- N@.
The current page length is available in the \&\f(CW.p\fR register.
.bt "\&\f(CW.bp\fI \(+-N\fR" "\fIN\(eq\fR1" "-" "B,\fBv\fR" "Begin page.
The current page is ejected and a new page is begun.
If @+- N@ is given, the new page number will be @+- N@.
Also see request \&\f(CWns\fR.
.bt "\&\f(CW.pn\fI \(+-N\fR" "@N@\(eq1" "ignored" "-" "Page number.
The next page (when it occurs) will have the page number @+- N@.
A \&\f(CWpn\fR must occur before the initial pseudo-page transition
to affect the page number of the first page.
The current page number is in the \&\f(CW%\fR register.
.bt "\&\f(CW.po\fI \(+-N\fR" "1\|in; 0" "previous" "\fBv\fR" "Page offset.
The current \fIleft margin\fR is set to @+- N@.
The \*(TR initial value provides 1 inch of paper margin
on a typical device.
The current page offset is available in the \&\f(CW.o\fR register.
.bt "\&\f(CW.ne\fI N\fR" "-" "\fIN\(eq\fR1\|\fIV\fR" "D,\fBv\fR" "Need @N@ vertical space.
If the distance \fID\fR to the next trap position (see §7.5) is less than @N@,
a forward vertical space of size \fID\fR occurs,
which will spring the trap.
If there are no remaining
traps on the page,
\fID\fR is the distance to the bottom of the page.
If @D<V@, another line could still be output
and spring the trap.
In a diversion, \fID\fR is the distance to the \fIdiversion trap\fR, if any,
or is very large.
.bt "\&\f(CW.mk\fI R\fR" "none" "internal" "D" "Mark the current vertical place
in an internal register (both associated with the current diversion level),
or in register @R@, if given.
See \&\f(CWrt\fR request.
.bt "\&\f(CW.rt\fI \(+-N\fR" "none" "internal" "D,\fBv\fR" "Return \fIupward only\fR to a marked vertical place
in the current diversion.
If @+- N@ (with respect to current place) is given,
the place is @+- N@ from the top of the page or diversion
or, if @N@ is absent, to a
place marked by a previous \&\f(CWmk\fR.
The \&\f(CWsp\fR request (§5.3) may be used
instead of \&\f(CWrt\fR
by spacing to the absolute place stored in a explicit register,
e.g., using
.CW ".mk
@R@ ...\&
.CW ".sp
.CW |\en@R@u ;
this also works when the motion is downwards.
.NH
Text Filling, Adjusting, and Centering
.sc "Filling and adjusting.
Normally,
words are collected from input text lines
and assembled into a output text line
until some word does not fit.
An attempt is then made
to hyphenate the word to put part
of it into the output line.
The spaces between the words on the output line
are then increased to spread out the line
to the current \fIline length\fR
minus any current \fIindent\fR.
A \fIword\fR is any string of characters delimited by
the \fIspace\fR character or the beginning/end of the input line.
Any adjacent pair of words that must be kept together
(neither split across output lines nor spread apart
in the adjustment process)
can be tied together by separating them with the
\fIunpaddable space\fR character
``\&\f(CW\e\ \fR'' (backslash-space).
The adjusted word spacings are uniform in \*(TR
and the minimum interword spacing can be controlled
with the \&\f(CWss\fR request (§2).
In \*(NR, they are normally nonuniform because of
quantization to character-size spaces;
however,
the command line option \&\f(CW-e\fR causes uniform
spacing with full output device resolution.
Filling, adjustment, and hyphenation (§13) can all be
prevented or controlled.
The text length on the last line output is available in the \&\f(CW.n\fR register,
and text baseline position on the page for this line is in the \&\f(CWnl\fR register.
The text baseline high-water mark (lowest place) on the current page is in
the \&\f(CW.h\fR register.
The current horizontal output position is in the \&\f(CW.k\fP register.
.PP
An input text line
.I ending
with \&\f(CW.\fR\^, \&\f(CW?\fR, or \&\f(CW!\fR,
optionally followed by any number of
.CW \&" ,
.CW ' ,
.CW ) ,
.CW ] ,
.CW * ,
or
†,
is taken
to be the end of a sentence, and an additional space character is
automatically provided during filling.
To prevent this, add
.CW \e&
to the end of the input line.
Multiple inter-word space characters found in the input are retained,
except for trailing spaces;
initial spaces also cause a break.
.PP
When filling is in effect, a \&\f(CW\ep\fR may be embedded or attached to a word to
cause a break at the end of the word and have the resulting output
line spread out to fill the current line length.
.PP
.tr &&
A text input line that happens to begin
with a control character can
be made not to look like a control line
by prefixing it with
the non-printing, zero-width filler character \&\f(CW\e&\fR.
Still another way is to specify output translation of some
convenient character into the control character
using \&\f(CWtr\fR (§10.5).
.tr &.
.sc "Interrupted text.
The copying of a input line in \fInofill\f (non-fill) mode can be interrupted
by terminating
the partial line with a \&\f(CW\ec\fR.
The next encountered input text line will be considered to be a continuation
of the same line of input text.
Similarly,
a word within \fIfilled\fR text may be interrupted by terminating the
word (and line) with \&\f(CW\ec\fR;
the next encountered text will be taken as a continuation of the
interrupted word.
If the intervening control lines cause a break,
any partial line will be forced out along with any partial word.
.bt "\&\f(CW.br\fR" "-" "-" "B" "Break.
The filling of the line currently
being collected is stopped and
the line is output without adjustment.
Text lines beginning with space characters
(but not tabs)
and empty text lines (blank lines) also cause a break.
.bt "\&\f(CW.fi\fR" "fill on" - B,E "Fill subsequent output lines.
The register \&\f(CW.u\fR is 1 in fill mode and 0 in nofill mode.
.bt "\&\f(CW.nf\fR" "fill on" "-" "B,E" "Nofill.
Subsequent output lines are neither filled nor adjusted.
Input text lines are copied directly to output lines
without regard for the current line length.
.bt "\&\f(CW.ad\fI c\fR" "adj, both" "adjust" "E" "Line adjustment is begun.
If fill mode is not on, adjustment will be deferred until
fill mode is back on.
If the type indicator @c@ is present,
the adjustment type is changed as shown in the following table.
.TS
center box;
c|c
c|l.
Indicator	Adjust Type
_
\&\f(CWl\fR	adjust left margin only
\&\f(CWr\fR	adjust right margin only
\&\f(CWc\fR	center
\&\f(CWb\fR or \&\f(CWn\fR	adjust both margins
absent	unchanged
.TE
The number register
.CW .j
contains the current value of the
.CW ad
setting;
its value can be recorded and used subsequently to set adjustment.
.bt "\&\f(CW.na\fR" "adjust" "-" "E" "Noadjust.
Adjustment is turned off;
the right margin will be ragged.
The adjustment type for \&\f(CWad\fR is not changed.
Output line filling still occurs if fill mode is on.
.bt "\&\f(CW.ce\fI N\fR" "off" "@N=1@" "B,E" "Center the next @N@ input text lines
within the current available horizontal space (line-length minus indent).
If @N=0@, any residual count is cleared.
A break occurs after each of the @N@ input lines.
If the input line is too long,
it will be left adjusted.
.NH
Vertical Spacing
.sc "Baseline spacing.
The vertical spacing @(V)@ between the baselines of successive
output lines can be set
using the \&\f(CWvs\fR request.
\fIV\fR should be large enough to accommodate the character sizes
on the affected output lines.
For the common type sizes (9-12 points),
usual typesetting practice is to set \fIV\fR to 2 points greater than the
point size;
\*(TR default is 10-point type on a 12-point spacing
(as in this document).
The current \fIV\fR is available in the \&\f(CW.v\fR register.
Multiple-\fIV\|\fR line separation (e.g., double spacing) may be requested
with \&\f(CWls\fR,
but it is better to use a large
.CW vs
instead;
certain preprocessors assume single spacing.
The current line spacing is available in the \&\f(CW.L\fP register.
.sc "Extra line-space.
If a word contains a tall construct requiring
the output line containing it to have extra vertical space
before and/or after it,
the \fIextra-line-space\fR function \&\f(CW\ex'\fIN\fP'\fR
can be embedded in or attached to that word.
If @N@ is negative,
the output line containing the word will
be preceded by @N@ extra vertical space;
if @N@ is positive,
the output line containing the word
will be followed by @N@ extra vertical space.
If successive requests for extra space apply to the same line,
the maximum values are used.
The most recently utilized post-line extra line-space is available in the \&\f(CW.a\fR register.
.PP
In
.CW \ex'\f2...\fP'
and other functions having a pair of delimiters around
their parameter,
the delimiter choice (here 
.CW ' )
is arbitrary,
except that it can not look like the continuation of a number expression for @N@.
.sc "Blocks of vertical space.
A block of vertical space is ordinarily requested using \&\f(CWsp\fR,
which honors the \fIno-space\fR mode and which does
not space past a trap.
A contiguous block of vertical space may be reserved using \&\f(CWsv\fR.
.bt "\&\f(CW.vs \fIN\fR" "12pts; 1/6in" "previous" "E,\fBp\fR" "Set vertical baseline spacing size \fIV\fR.
Transient extra vertical space is available with \&\f(CW\ex\fI'N\|'\fR (see above).
.bt "\&\f(CW.ls \fIN\fR" "@N=1@" "previous" "E" "\fILine\fR spacing
set to @+- N@.
@N-1@ \fIV\fR\^s (blank lines) are
appended to each output text line.
Appended blank lines are omitted, if the text or previous appended blank line reached a trap position.
.bt "\&\f(CW.sp \fIN\fR" "-" "@N=1~V@" "B,\fBv\fR" "Space vertically in either direction.
If @N@ is negative, the motion is backward (upward)
and is limited to the distance to the top of the page.
Forward (downward) motion is truncated to the distance to the
nearest trap.
(Recall the use of
.CW ".sp |\f2N\fP
from §1.3.)
If the no-space mode is on,
no spacing occurs (see \&\f(CWns\fR and \&\f(CWrs\fR below).
.bt "\&\f(CW.sv\fI N\fR" "-" "@N=1~V@" "\fBv\fR" "Save a contiguous vertical block of size @N@.
If the distance to the next trap is greater
than @N@, @N@ vertical space is output.
No-space mode has no effect.
If this distance is less than @N@,
no vertical space is immediately output,
but @N@ is remembered for later output (see \&\f(CWos\fR).
Subsequent \&\f(CWsv\fR requests will overwrite any still remembered @N@.
.bt "\&\f(CW.os\fR" "-" "-" "-" "Output saved vertical space.
No-space mode has no effect.
Used to finally output a block of vertical space requested
by an earlier \&\f(CWsv\fR request.
.bt "\&\f(CW.ns\fR" "space" "-" "D" "No-space mode turned on.
When on, no-space mode inhibits \&\f(CWsp\fR requests and
\&\f(CWbp\fR requests \fIwithout\fR a next page number.
No-space mode is turned off when a line of
output occurs, or with \&\f(CWrs\fR.
.bt "\&\f(CW.rs\fR" "space" "-" "D" "Restore spacing.
The no-space mode is turned off.
.bt "\&Blank text line." "" "-" "B" "Causes a break and
output of a blank line exactly like \&\f(CWsp 1\fR.
.NH
Line Length and Indenting
.PP
The maximum line length for fill mode may be set with \&\f(CWll\fR.
The indent may be set with \&\f(CWin\fR;
an indent applicable to only the next output line may be set with \&\f(CWti\fR.
The line length includes indent space but not
page offset space.
The line length minus the indent is the basis for centering with \&\f(CWce\fR.
The effect of \&\f(CWll\fR, \&\f(CWin\fR, or \&\f(CWti\fR
is delayed, if a partially collected line exists,
until after that line is output.
In fill mode the length of text on an output line is less than or equal to
the line length minus the indent.
The current line length and indent are available in registers \&\f(CW.l\fR and \&\f(CW.i\fR respectively.
The length of \fIthree-part titles\fR produced by \&\f(CWtl\fR
(see §14) is independently set by \&\f(CWlt\fR.
.bt "\&\f(CW.ll\fI \(+-N\fR" "6.5\|in" "previous" "E,\fBm\fR" "Line length is set to \(+-@N@.
.bt "\&\f(CW.in\fI \(+-N\fR" "\fIN\(eq\^\fR0" "previous" "B,E,\fBm\fR" "Indent is set to @+- N@.
The indent is prefixed to each output line.
.bt "\&\f(CW.ti\fI \(+-N\fR" "-" "ignored" "B,E,\fBm\fR" "Temporary indent.
The next output text line will be indented a distance @+- N@
with respect to the current indent.
The resulting total indent may not be negative.
The current indent is not changed.
.NH
Macros, Strings, Diversion, and Position Traps
.sc "Macros and strings.
A \fImacro\fR is a named set of arbitrary \fIlines\fR that may be invoked by name or
with a \fItrap\fR.
A \fIstring\fR is a named string of \fIcharacters\fR,
not including a newline character,
that may be interpolated by name at any point.
Request, macro, and string names share the same name list.
Macro and string names
may be one or two characters long and may usurp previously defined
request, macro, or string names;
this implies that built-in operations may be (irrevocably) redefined.
Any of these entities may be renamed with \&\f(CWrn\fR
or removed with \&\f(CWrm\fR.
.PP
Macros are created by \&\f(CWde\fR and \&\f(CWdi\fR, and appended to by \&\f(CWam\fR and \&\f(CWda\fR;
\&\f(CWdi\fR and \&\f(CWda\fR cause normal output to be stored in a macro.
A macro is invoked in the same way as a request;
a control line beginning \&\f(CW.\fIxx\fR will interpolate the contents of macro \fIxx\fR.
The remainder of the line may contain up to nine \fIarguments\fR.
.PP
Strings are created by \&\f(CWds\fR and appended to by \&\f(CWas\fR.
The strings \fIx\fR and \fIxx\fR are interpolated at any desired point with
\&\f(CW\e\(**\fIx\fR and \&\f(CW\e\(**(\fIxx\fR respectively.
String references and macro invocations may be nested.
.sc "Copy mode input interpretation.
During the definition and extension
of strings and macros (not by diversion)
the input is read in \fIcopy mode\fR.
In copy mode, input is copied without interpretation
except that:
.IP
.ds + \v'-.1m'\s-4\(bu\s+4\v'+.1m'
.nf
\*+ The contents of number registers indicated by \&\f(CW\en\fR are interpolated.
\*+ Strings indicated by \&\f(CW\e\(**\fR are interpolated.
\*+ Arguments indicated by \&\f(CW\e$\fR are interpolated.
\*+ Concealed newlines indicated by \&\f(CW\e\fP\f2newline\fP are eliminated.
\*+ Comments indicated by \&\f(CW\e"\fR are eliminated.
\*+ \&\f(CW\et\fR and \&\f(CW\ea\fR are interpreted as \s-1ASCII\s+1 horizontal tab and \s-1SOH\s+1 respectively (§9).
\*+ \&\f(CW\e\e\fR is interpreted as \&\f(CW\e\fR.
\*+ \&\f(CW\e.\fR is interpreted as ``\&\f(CW.\fR''.
.LP
These interpretations can be suppressed by
prefixing
a \&\f(CW\e\fR.
For example, since \&\f(CW\e\e\fR maps into a \&\f(CW\e\fR, \&\f(CW\e\en\fR will copy as \&\f(CW\en\fR, which
will be interpreted as a number register indicator when the
macro or string is reread.
.sc "Arguments.
When a macro is invoked by name, the remainder of the line is
taken to contain up to nine arguments.
The argument separator is the space character (not tab), and arguments
may be surrounded by double quotes to permit embedded space characters.
Pairs of double quotes may be embedded in double-quoted arguments to
represent a single double-quote character.
The argument
.CW \&""
is explicitly null.
If the desired arguments won't fit on a line,
a concealed newline may be used to continue on the next line.
A trailing double quote may be omitted.
.PP
When a macro is invoked the \fIinput level\fR is \fIpushed down\fR and
any arguments available at the previous level become unavailable
until the macro is completely read and the previous level is restored.
A macro's own arguments can be interpolated at any point
within the macro with
.CW \e$@N@ ,
which interpolates the @N@\^th
argument
(@1 <= N <= 9@).
If an invoked argument does not exist,
a null string results.
For example, the macro \fIxx\fR may be defined by
.P1
.ta .75i
&de xx	\e" begin definition
Today is \e\e$1 the \e\e$2.
&.	\e" end definition
.P2
and called by
.P1
&xx Monday 14th
.P2
to produce the text
.P1
Today is Monday the 14th.
.P2
Note that each \&\f(CW\e$\fR
was concealed in the definition with a prefixed \&\f(CW\e\fR.
The number of
arguments is in the \&\f(CW.$\fR register.
.PP
No arguments are available at the top (non-macro) level,
within a string, or within a trap-invoked macro.
.PP
Arguments are copied in copy mode onto a stack
where they are available for reference.
It is advisable to
conceal string references (with an extra \&\f(CW\e\fR\|)
to delay interpolation until argument reference time.
.sc "Diversions.
Processed output may be diverted into a macro for purposes
such as footnote processing (see Tutorial §T5)
or determining the horizontal and vertical size of some text for
conditional changing of pages or columns.
A single diversion trap may be set at a specified vertical position.
The number registers \&\f(CWdn\fR and \&\f(CWdl\fR respectively contain the
vertical and horizontal size of the most
recently ended diversion.
Processed text that is diverted into a macro
retains the vertical size of each of its lines when reread
in \fInofill\fR mode
regardless of the current \fIV\fR.
Constant-spaced (\&\f(CWcs\fR) or emboldened (\&\f(CWbd\fR) text that is diverted
can be reread correctly only if these modes are again or still in effect
at reread time.
One way to do this is to embed in the diversion the appropriate
\&\f(CWcs\fR or \&\f(CWbd\fR requests with the \fItransparent\fR
mechanism described in §10.6.
.PP
Diversions may be nested
and certain parameters and registers
are associated
with the current diversion level
(the top non-diversion level may be thought of as the
0th diversion level).
These are the diversion trap and associated macro,
no-space mode,
the internally-saved marked place (see \&\f(CWmk\fR and \&\f(CWrt\fR),
the current vertical place (\&\f(CW.d\fR register),
the current high-water text baseline (\&\f(CW.h\fR register),
and the current diversion name (\&\f(CW.z\fR register).
.sc "Traps.
Three types of trap mechanisms are available\(empage traps, a diversion trap, and
an input-line-count trap.
Macro-invocation traps may be planted using \&\f(CWwh\fR at any page position including the top.
This trap position may be changed using \&\f(CWch\fR.
Trap positions at or below the bottom of the page
have no effect unless or until
moved to within the page or rendered effective by an increase in page length.
Two traps may be planted at the same position only by first planting them at different
positions and then moving one of the traps;
the first planted trap will conceal the second unless and until the first one is moved
(see Tutorial Examples).
If the first one is moved back, it again conceals the second trap.
The macro associated with a page trap is automatically
invoked when a line of text is output whose vertical size reaches
or sweeps past the trap position.
Reaching the bottom of a page springs the top-of-page trap, if any,
provided there is a next page.
The distance to the next trap position is available in the \&\f(CW.t\fR register;
if there are no traps between the current position and the bottom of the page,
the distance returned is the distance to the page bottom.
.PP
A macro-invocation trap effective in the current diversion may be planted using \&\f(CWdt\fR.
The \&\f(CW.t\fR register works in a diversion; if there is no subsequent trap a large
distance is returned.
For a description of input-line-count traps, see \&\f(CWit\fR below.
.bt "\&\f(CW&de\fI xx yy\fR" "-" "\fI.yy=\&\f(CW..\fR" "-" "Define or redefine the macro \fIxx\fR.
The contents of the macro begin on the next input line.
Input lines are copied in \fIcopy mode\fR until the definition is terminated by a
line beginning with \&\f(CW.\fIyy\fR,
whereupon the macro \fIyy\fR is called.
In the absence of \fIyy\fR, the definition
is terminated by a
line beginning with ``\&\f(CW..\fR''.
A macro may contain \&\f(CWde\fR requests
provided the terminating macros differ
or the contained definition terminator is concealed.
\&``\&\f(CW..\fR'' can be concealed as
\&\f(CW\e\e..\fR which will copy as \&\f(CW\e..\fR and be reread as ``\&\f(CW..\fR''.
.bt "\&\f(CW&am\fI xx yy\fR" "-" "\fI.yy=\&\f(CW..\fR" "-" "Append to macro
.I xx
(append version of \&\f(CWde\fR).
.bt "\&\f(CW&ds\fI xx string\fR" "-" "ignored" "-" "Define a string
\fIxx\fR containing \fIstring\fR.
Any initial double quote in \fIstring\fR is stripped off to permit
initial blanks.
.bt "\&\f(CW&as\fI xx string\fR" "-" "ignored" "-" "Append
\fIstring\fR to string \fIxx\fR
(append version of \&\f(CWds\fR).
.bt "\&\f(CW&rm\fI xx\fR" "-" "ignored" "-" "Remove
request, macro, or string.
The name \fIxx\fR is removed from the name list and
any related storage space is freed.
Subsequent references will have no effect.
If many macros and strings are being created dynamically, it
may become necessary to remove unused ones
to recapture internal storage space for newer registers.
.bt "\&\f(CW&rn\fI xx yy\fR" "-" "ignored" "-" "Rename request, macro, or string
\fIxx\fR to \fIyy\fR.
If \fIyy\fR exists, it is first removed.
.bt "\&\f(CW&di\fI xx\fR" "-" "end" "D" "Divert output to macro \fIxx\fR.
Normal text processing occurs during diversion
except that page offsetting is not done.
The diversion ends when the request \&\f(CWdi\fR or \&\f(CWda\fR is encountered without an argument;
extraneous
requests of this type should not appear when nested diversions are being used.
.bt "\&\f(CW&da \fIxx\fR" "-" "end" "D" "Divert, appending to macro \fIxx\fR
(append version of \&\f(CWdi\fR).
.bt "\&\f(CW&wh\fI N xx\fR" "-" "-" "\fBv\fR" "Install
a trap to invoke \fIxx\fR at page position \fIN\fR;
a negative N will be interpreted as a distance from the
page bottom.
Any macro previously planted at @N@ is replaced by \fIxx\fR.
A zero @N@ refers to the top of a page.
In the absence of \fIxx\fR, the first trap found at @N@, if any, is removed.
.bt "\&\f(CW&ch\fI xx N\fR" "-" "-" "\fBv\fR" "Change
the trap position for macro \fIxx\fR to be @N@.
In the absence of @N@, the trap, if any, is removed.
.bt "\&\f(CW&dt\fI N xx\fR" "-" "off" "D,\fBv\fR" "Install a diversion trap
at position @N@ in the \fIcurrent\fR diversion to invoke
macro \fIxx\fR.
Another \&\f(CWdt\fR will redefine the diversion trap.
If no arguments are given, the diversion trap is removed.
.bt "\&\f(CW&it\fI N xx\fR" "-" "off" "E" "Set an input-line-count trap
to invoke the macro \fIxx\fR after @N@ lines of \fItext\fR input
have been read
(control or request lines do not count).
The text may be inline text or
text interpolated by inline or trap-invoked macros.
.bt "\&\f(CW&em\fI xx\fR" "none" "none" "-" "The
macro \fIxx\fR will be invoked
when all input has ended.
The effect is almost as if the contents of \fIxx\fR had been at the end
of the last file processed,
but all processing ceases at the next page eject.
.NH
Number Registers
.PP
A variety of parameters are available to the user as
predefined \fInumber registers\fR (see Summary, page \n(*%).
In addition, users may define their own registers.
Register names are one or two characters long and do not conflict
with request, macro, or string names.
Except for certain predefined read-only registers,
a number register can be read, written, automatically
incremented or decremented, and interpolated
into the input in a variety of formats.
One common use of user-defined registers is to
automatically number sections, paragraphs, lines, etc.
A number register may be used any time numerical input is expected or desired
and may be used in numerical \fIexpressions\fR (§1.4).
.PP
Number registers are created and modified using \&\f(CWnr\fR, which
specifies the name, numerical value, and the auto-increment size.
Registers are also modified, if accessed
with an auto-incrementing sequence.
If the registers \fIx\fR and \fIxx\fR both contain
@N@ and have the auto-increment size @M@,
the following access sequences have the effect shown:
.TS
center box;
c2|c2|c
c2|c2|c2
l2|c2|c2
l2|c2|c2
l2|l2|c2.
	Effect on	Value
Sequence	Register	Interpolated
_
\&\f(CW\en\fIx\fR	none	@N@
\&\f(CW\en(\fIxx\fR	none	@N@
\&\f(CW\en+\fIx\fR	\fIx\fR incremented by @M@	\fIN+M\fR
\&\f(CW\en-\fIx\fR	\fIx\fR decremented by @M@	\fIN-M\fR
\&\f(CW\en+(\fIxx\fR	\fIxx\fR incremented by @M@	\fIN+M\fR
\&\f(CW\en-(\fIxx\fR	\fIxx\fR decremented by @M@	\fIN-M\fR
.TE
When interpolated, a number register is converted to
decimal (default),
decimal with leading zeros,
lower-case Roman,
upper-case Roman,
lower-case sequential alphabetic,
or
upper-case sequential alphabetic
according to the format specified by \&\f(CWaf\fR.
.bt "\&\f(CW&nr\fI R \(+-N M\fR" "" "-" "\fBu\fR" "The number register
@R@ is assigned the value @+- N@
with respect to the previous value, if any.
The increment for auto-incrementing is set to @M@.
.bt "\&\f(CW&af\fI R c\fR" "arabic" "-" "-" "Assign
format @c@ to register @R@.
The available formats are:
.Tm number register format	s
.TS
center box;
c2|c
c2|c
c2|l.
	Numbering
Format	Sequence
_
\&\f(CW1\fR	0, 1, 2, 3, 4, 5, ...
\&\f(CW001\fR	000, 001, 002, 003, 004, 005, ...
\&\f(CWi\fR	0, i, ii, iii, iv, v, ...
\&\f(CWI\fR	0, I, II, III, IV, V, ...
\&\f(CWa\fR	0, a, b, c, ..., z, aa, ab, ..., zz, aaa, ...
\&\f(CWA\fR	0, A, B, C, ..., Z, AA, AB, ..., ZZ, AAA, ...
.TE
An arabic format having @N@ digits
specifies a field width of @N@ digits (example 2 above).
The read-only registers and the width function
.CW \ew
(§11.2)
are always arabic.
Warning: the value of a number register in a non-Arabic format
is not numeric, and will not produce the expected results in expressions.
.IP
The function
.CW \eg@x@
or
.CW \eg(@xx@
returns the format of a number register in a form suitable for
.CW af ;
it returns nothing if the register has not been used.
.bt "\&\f(CW&rr\fI R\fR" "-" "ignored" "-" "Remove number register @R@.
If many registers are being created dynamically, it
may become necessary to remove unused registers
to recapture internal storage space for newer registers.
The register
.CW .R
contains the number of number registers still available.
.NH
Tabs, Leaders, and Fields
.sc "Tabs and leaders.
The \s-1ASCII\s+1 horizontal tab character and the \s-1ASCII\s+1
\s-1SOH\s+1 (control-A, hereafter called the \fIleader\fR character)
can both be used to generate either horizontal motion or
a string of repeated characters.
The length of the generated entity is governed
by internal \fItab stops\fR specifiable
with \&\f(CWta\fR.
The default difference is that tabs generate motion and leaders generate
a string of periods;
\&\f(CWtc\fR and \&\f(CWlc\fR
offer the choice of repeated character or motion.
There are three types of internal tab stops\(em\
\fIleft\fR adjusting, \fIright\fR adjusting,
and \fIcentering\fR.
In the following table,
\fID\fR is the distance from the current position on the \fIinput\fR line
(where a tab or leader was found)
to the next tab stop,
\fInext-string\fR consists
of the input characters following the tab (or leader) up to the next tab (or leader) or end of line,
and
\fIW\fR is the width of \fInext-string\fR.
.TS
center box;
c2|c2|c
c2|c2|c
c2|c2|l.
Tab	Length of motion or	Location of
type	repeated characters	\fInext-string\fR
_
Left	\fID\fR	Following \fID\fR
Right	\fID-W\fR	Right adjusted within \fID\fR
Centered	\fID-W/\fR2	Centered on right end of \fID\fR
.TE
The length of generated motion is allowed to be negative, but
that of a repeated character string cannot be.
Repeated character strings contain an integer number of characters, and
any residual distance is prepended as motion.
Tabs or leaders found after the last tab stop are ignored, but may be used
as \fInext-string\fR terminators.
.PP
Tabs and leaders are not interpreted in copy mode.
\&\f(CW\et\fR and \&\f(CW\ea\fR always generate a non-interpreted
tab and leader respectively, and
are equivalent to actual tabs and leaders in copy mode.
.sc "Fields.
A \fIfield\fR is contained between
a pair of \fIfield delimiter\fR characters,
and consists of substrings
separated by \fIpadding\fR indicator characters.
The field length is the distance on the
\fIinput\fR line from the position where the field begins to the next tab stop.
The difference between the total length of all the substrings
and the field length is incorporated as horizontal
padding space that is divided among the indicated
padding places.
The incorporated padding is allowed to be negative.
For example,
if the field delimiter is \&\f(CW#\fR and the padding indicator is \&\f(CW^\fR,
\&\f(CW#^\fIxxx\&\f(CW^\fIright\|\&\f(CW#\fR
specifies a right-adjusted string with the string \fIxxx\fR centered
in the remaining space.
.h1
.bt "\&\f(CW&ta\fI Nt ...\fR" "0.8; 0.5in" "none" "E,\fBm\fR" "Set tab stops and types.
\fIt=\&\f(CWR\fR, right adjusting;
\fIt=\&\f(CWC\fR, centering;
\fIt\fR absent, left adjusting.
\*(Tr tab stops are preset every 0.5in.,
\*(NR every 0.8in.
The stop values are separated by spaces, and
a value preceded by \&\f(CW+\fR
is treated as an increment to the previous stop value.
.bt "\&\f(CW&tc\fI c\fR" "none" "none" "E" "The tab repetition character
becomes @c@,
or is removed, thus specifying motion.
.bt "\&\f(CW&lc\fI c\fR" "\&\f(CW.\fR" "none" "E" "The leader repetition character
becomes @c@,
or is removed, thus specifying motion.
.bt "\&\f(CW&fc\fI a b\fR" "off" "off" "-" "The field delimiter
is set to \fIa\fR;
the padding indicator is set to the space character or to
\fIb\fR, if given.
In the absence of arguments the field mechanism is turned off.
.NH
Input and Output Conventions and Character Translations
.sc "Input character translations.
Ways of inputting the valid character set were
discussed in §2.1.
The \s-1ASCII\s+1 control characters horizontal tab (§9.1),
\s-1SOH\s+1 (§9.1), and backspace (§10.3) are discussed elsewhere.
The newline delimits input lines.
In addition,
\s-1STX\s+1, \s-1ETX\s+1, \s-1ENQ\s+1, \s-1ACK\s+1, and \s-1BEL\s+1
are accepted,
and may be used as delimiters or translated into a graphic with \&\f(CWtr\fR (§10.5).
All others are ignored.
.PP
The \fIescape\fR character \&\f(CW\e\fR
introduces \fIescape sequences\fR,
which cause the following character to mean
another character, or to indicate
some function.
.nr %% \n(*%-1
A complete list of such sequences is given in the Summary on page \n(*%.
The escape character \&\f(CW\e\fR
should not be confused with the \s-1ASCII\s+1 control character \s-1ESC\s+1.
The escape character \&\f(CW\e\fR can be input with the sequence \&\f(CW\e\e\fR.
The escape character can be changed with \&\f(CWec\fR,
and all that has been said about the default \&\f(CW\e\fR becomes true
for the new escape character.
\&\f(CW\ee\fR can be used to print whatever the current escape character is.
The escape mechanism may be turned off with \&\f(CWeo\fR,
and restored with \&\f(CWec\fR.
.h1
.bt "\&\f(CW&ec\fI c\fR" "\&\f(CW\e\fR" "\&\f(CW\e\fR" "-" "Set escape character
to \&\f(CW\e\fR, or to @c@, if given.
.bt "\&\f(CW&eo\fR" "on" "-" "-" "Turn escape mechanism off.
.sc "Ligatures.
.lg0
The set of available ligatures is device and font dependent,
but is often a subset of
\&\fBfi\fR, \&\fBfl\fR, \&\fBff\fR, \&\fBffi\fR, and \&\fBffl\fR.
They may be input by
\&\f(CW\e(fi\fR, \&\f(CW\e(fl\fR, \&\f(CW\e(ff\fR, \&\f(CW\e(Fi\fR, and \&\f(CW\e(Fl\fR respectively.
.lg
The ligature mode is normally on in \*(TR, and automatically invokes 
ligatures during input.
.h1
.bt "\&\f(CW&lg\fI N\fR" "on; off" "on" "-" "Ligature mode
is turned on if @N@ is absent or non-zero,
and turned off if @N=0@.
If @N=2@, only the two-character ligatures are automatically invoked.
Ligature mode is inhibited for
request, macro, string, register, or file names,
and in copy mode.
No effect in \*(NR.
.sc "Backspacing, underlining, overstriking, etc.
Unless in copy mode, the \s-1ASCII\s+1 backspace character is replaced
by a backward horizontal motion having the width of the
space character.
Underlining as a form of line-drawing is discussed in §12.4.
A generalized overstriking function is described in §12.1.
.PP
\*(Nr automatically underlines
characters in the \fIunderline\fR font,
specifiable with \&\f(CWuf\fR,
normally that on font position 2.
In addition to \&\f(CWft\fR and
.CW \ef@F@ ,
the underline font may be selected by \&\f(CWul\fR and \&\f(CWcu\fR.
Underlining is restricted to an output-device-dependent
subset of reasonable characters.
.bt "\&\f(CW&ul\fI N\fR" "off" "@N=1@" "E" "Italicize in \*(TR
(underline in \*(NR) the next @N@
input text lines.
Actually, switch to underline font, saving the
current font for later restoration;
other font changes within the span of a \&\f(CWul\fR
will take effect,
but the restoration will undo the last change.
Output generated by \&\f(CWtl\fR (§14) is affected by the
font change, but does not decrement @N@.
If @N>1@, there is the risk that
a trap interpolated macro may provide text
lines within the span;
environment switching can prevent this.
.bt "\&\f(CW&cu\fI N\fR" "off" "@N=1@" "E" "Continuous underline.
A variant
of \&\f(CWul\fR that causes \fIevery\fR character to be underlined in \*(NR.
Identical to \&\f(CWul\fR in \*(TR.
.bt "\&\f(CW&uf\fI F\fR" "Italic" "Italic" "-" "Underline font set to @F@.
In \*(NR,
@F@ may not be on position 1.
.sc "Control characters.
Both the control character \&\f(CW.\fR and the \fIno-break\fR
control character \&\f(CW'\fR may be changed.
Such a change must be compatible with the design
of any macros used in the span of the change,
and
particularly of any trap-invoked macros.
.bt "\&\f(CW&cc\fI c\fR" "\&\f(CW.\fR" "\&\f(CW.\fR" "E" "The basic control character
is set to @c@,
or reset to ``\&\f(CW.\fR''.
.bt "\&\f(CW&c2\fI c\fR" "\&\f(CW'" "'\fR" "E" "The \fIno-break\fR control character is set
to @c@, or reset to ``\&\f(CW'\fR''.
.sc "Output translation.
One character can be made a stand-in for another character using \&\f(CWtr\fR.
All text processing (e.g., character comparisons) takes place
with the input (stand-in) character, which appears to have the width of the final
character.
The graphic translation occurs at the moment of output
(including diversion).
.bt "\&\f(CW&tr\fI abcd....\fR" "none" "-" "O" "Translate
\fIa\fR into \fIb\fR, @c@ into \fId\fR, etc.
If an odd number of characters is given,
the last one will be mapped into the space character.
To be consistent, a particular translation
must stay in effect from \fIinput\fR to \fIoutput\fR time.
.sc "Transparent throughput.
An input line beginning with a \&\f(CW\e!\fR is read in copy mode and \fItrans\%parently\fR output
(without the initial \&\f(CW\e!\fR);
the text processor is otherwise unaware of the line's presence.
This mechanism may be used to pass control information to a post-processor
or to embed control lines in a macro created by a diversion.
.sc "Transparent output
The sequence
.CW \eX'@anything@'
copies
.I anything
to the output, as a device control function of the form
.CW x
.CW X 
.I anything 
(§22).
Escape sequences in
.I anything
are processed.
.sc "Comments and concealed newlines.
An uncomfortably long input line that must stay
one line (e.g., a string definition, or nofilled text)
can be split into several physical lines by ending all but
the last one with the escape \&\f(CW\e\fR.
The sequence \&\f(CW\e\fR@newline@ is always ignored,
except in a comment.
Comments may be embedded at the end of any line by
prefacing them with \&\f(CW\e"\fR.
The newline at the end of a comment cannot be concealed.
A line beginning with \&\f(CW\e"\fR will appear as a blank line and
behave like
.CW ".sp\ 1" ;
a comment can be on a line by itself by beginning the line with \&\f(CW.\e"\fR.
.NH
Local Horizontal and Vertical Motions, and the Width Function
.sc "Local Motions.
The functions \&\f(CW\ev'\fIN\&\f(CW'\fR and
\&\f(CW\eh'\fIN\&\f(CW'\fR
can be used for \fIlocal\fR vertical and horizontal motion respectively.
The distance @N@ may be negative; the positive directions
are rightward and downward.
A local motion is one contained within a line.
To avoid unexpected vertical dislocations, it is necessary that
the net vertical local motion within a word in filled text
and otherwise within a line balance to zero.
The escape sequences providing local motion are
summarized in the following table.
.ds Y \0\0\0
.KS
.TS
center box;
c2|cs2||c2|cs2
c1|c2c2||c2|c2c2.
Vertical	Effect in	Horizontal	Effect in
Local Motion	\*(TR	\*(NR	Local Motion	\*(TR	\*(NR
_
.sp.4
.TC
l2|ls2||l2|ls2.
\&\f(CW\*Y\ev'\fIN\|\f(CW'\fR	Move distance @N@	\
\&\f(CW\*Y\eh'\fIN\|\f(CW'\fR	Move distance @N@
.TC
_2|_2_2||l2|ls2.
x	x	x	\&\f(CW\*Y\e\fP\f2space\fP	Unpaddable space-size space
.TC
l2|l2|l2||l2|ls2.
\&\f(CW\*Y\eu\fR	½ em up	½ line up	\&\f(CW\*Y\e0\fR	Digit-size space
.TC
l2|l2|l2||_2|_2_2.
\&\f(CW\*Y\ed\fR	½ em down	½ line down	x	x	x
.TC
l2|l2|l2||l2|l2|l2.
\&\f(CW\*Y\er\fR	1 em up	1 line up	\&\f(CW\*Y\e|\fR	1/6 em space	ignored
			\&\f(CW\*Y\e^\fR	1/12 em space	ignored
.sp.4
.TE
.KE
As an example,
\&\f(CWE\s-2\v'-.4m'2\v'.4m'\s+2\fR
could be generated by a sequence of size changes and motions:
\&\f(CWE\es-2\ev'-0.4m'2\ev'0.4m'\es+2\fR;
note that
the 0.4 em vertical motions are at the smaller size.
.sc "Width Function.
The \fIwidth\fR function \&\f(CW\ew'\fIstring\&\f(CW'\fR
generates the numerical width of \fIstring\fR (in basic units).
Size and font changes may be embedded in \fIstring\fR,
and will not affect the current environment.
For example,
\&\&\f(CW.ti\ -\ew'\efB1.\ 'u\fR could be used to
temporarily indent leftward a distance equal to the
size of the string ``\&\f(CW1.\ \fR'' in font
.CW B .
.PP
The width function also sets three number registers.
The registers \&\f(CWst\fR and \&\f(CWsb\fR are set respectively to the highest and
lowest extent of \fIstring\fR relative to the baseline;
then, for example,
the total height of the string is \&\f(CW\en(stu-\en(sbu\fR.
In \*(TR the number register \&\f(CWct\fR is set to a value
between 0 and 3.
The value
0 means that all of the characters in \fIstring\fR were short lower
case characters without descenders (like \&\f(CWe\fR);
1 means that at least one character has a descender (like \&\f(CWy\fR);
2 means that at least one character is tall (like \&\f(CWH\fR);
and 3 means that both tall characters and characters with
descenders are present.
.sc "Mark horizontal place.
The function \&\f(CW\ek\fIx\fR causes the current horizontal
position in the \fIinput line\fR to be stored in register \fIx\fR.
For example,
the construction \&\f(CW\ekx\fIword\f(CW\eh'|\enxu+3u'\fIword\&\f(CW\fR
will embolden \fIword\fR by backing up to almost its beginning and overprinting it,
resulting in \kz\fIword\fR\h'|\nzu+3u'\fIword\fR.
.NH
Overstrike, Bracket, Line-drawing, Graphics, and Zero-width Functions
.sc "Overstriking.
Automatically centered overstriking of up to nine characters
is provided by the \fIoverstrike\fR function
\&\f(CW\eo'\fIstring\&\f(CW\|'\fR.
The characters in \fIstring\fR are overprinted with centers aligned; the total width
is that of the widest character.
\fIstring\fR may not contain local vertical motion.
As examples,
\&\f(CW\eo'e\e''\fR produces \o'e\'', and
\&\f(CW\eo'\e(mo\e(sl'\fR produces \o'\(mo\(sl'.
.sc "Zero-width characters.
The function
.CW \ez@c@
will output @c@ without spacing over
it, and can be used to produce left-aligned overstruck
combinations.
As examples,
\&\f(CW\ez□+\fR will produce \z□+, and
\&\f(CW\e(br\ez\e(rn\e(ul\e(br\fR will produce a small
badly constructed box \&\(br\z\(rn\(ul\(br\|.
.sc "Large Brackets.
The Special Font usually contains a number of bracket construction pieces
\|\|\(lt\|\|\(lb\|\|\(rt\|\|\(rb\|\|\(lk\|\|\(rk\|\|\(bv\|\|\(lf\|\|\(rf\|\|\(lc\|\|\(rc\|\|
that can be combined into various bracket styles.
The function \&\f(CW\eb'\fIstring\&\f(CW\|'\fR may be used to pile
up vertically the characters in \fIstring\fR
(the first character on top and the last at the bottom);
the characters are vertically separated by 1 em and the total
pile is centered 1/2 em above the current baseline
(½ line in \*(NR).
For example,
.P1
\eb'\e(lc\e(lf'E\eb'\e(rc\e(rf'\ex'-0.5m'\ex'0.5m'
.P2
produces
\x'-.5m'\x'.5m'\b'\(lc\(lf'E\b'\(rc\(rf'.
.sc "Line drawing.
.tr &&
The function \&\f(CW\el'\fINc\f(CW'\fR (backslash-ell) draws a string of repeated @c@'s towards the right for a distance @N@.
If @c@ looks like a continuation of
an expression for @N@, it may be insulated from @N@ with \&\f(CW\e&\fR.
If @c@ is not specified, the \&\f(CW\(ru\fR (baseline rule) is used
(underline character in \*(NR).
If @N@ is negative, a backward horizontal motion
of size @N@ is made before drawing the string.
Any space resulting from @N@/(size of @c@) having a remainder is put at the beginning (left end)
of the string.
If @N@ is less than the width of @c@,
a single @c@ is centered on a distance @N@.
In the case of characters
that are designed to be connected, such as
baseline-rule\ \&\f(CW\(ru\fR\|,
under-rule\ \&\f(CW\(ul\fR\|,
and
root-en\ \&\f(CW\(rn\fR\|,
the remainder space is covered by overlapping.
As an example, a macro to underscore a string can be written
.tr &.
.P1
.ne 2.1
&de us
\e\e$1\e\|l\|'|0\e(ul'
&&
.P2
.ne2.1
.de xu
\\$1\l'|0\(ul'
..
or one to draw a box around a string
.P1
&de bx
\e(br\e|\e\e$1\e|\e(br\e\|l\|'|0\e(rn'\e\|l\|'|0\e(ul'
&&
.P2
.de bx
\(br\|\\$1\|\(br\l'|0\(rn'\l'|0\(ul'
..
such that
.P1
&ul "underlined words"
.P2
and
.P1
&bx "words in a box"
.P2
yield
.xu "underlined words"
and
.bx "words in a box"
\h'-\w'.'u'.
.PP
The function \&\f(CW\eL'\fINc\&\f(CW'\fR draws a vertical line consisting
of the (optional) character @c@ stacked vertically apart 1\|em
(1 line in \*(NR),
with the first two characters overlapped,
if necessary, to form a continuous line.
The default character is the \fIbox rule\fR \|\(br\| (\&\f(CW\|\e(br\fR);
the other suitable character is the \fIbold vertical\fR \|\(bv\| (\&\f(CW\|\e(bv\fR).
The line is begun without any initial motion relative to the
current baseline.
A positive @N@ specifies a line drawn downward and
a negative @N@ specifies a line drawn upward.
After the line is drawn no compensating
motions are made;
the instantaneous baseline is at the end of the line.
.PP
.de eb
.sp -1
.nf
\h'-.5n'\L'|\\nzu-1'\l'\\n(.lu+1n\(ul'\L'-|\\nzu+1'\l'|0u-.5n\(ul'
.fi
..
.ne 2i
.mk z
.nr z \nz+1
The horizontal and vertical line drawing functions may be used
in combination to produce large boxes.
The zero-width \fIbox-rule\fR and the ½-em wide \fIunder-rule\fR
were designed to form corners when using 1-em vertical
spacings.
For example the macro
.nr x \n(DV
.nr DV 0
.P1 .15i
.ps -1
\&.de eb
\&.sp -1	\e"compensate for next automatic baseline spacing
\&.nf	\e"avoid possibly overflowing word buffer
\&\eh'-.5n'\eL'|\e\enau-1'\el'\e\en(.lu+1n\e(ul'\eL'-|\e\enau+1'\el'|0u-.5n\e(ul'
\&.fi
\&..
.ps +1
.P2
.nr DV \nx 
will draw a box around some text whose beginning vertical place was
saved in number register \fIa\fR
(e.g., using \&\f(CW.mk\ a\fR)
as was done for this paragraph.
.eb
.sc "Graphics.
The function
.CW \eD'@c...@'
draws a graphic object of type @c@
according to a sequence of parameters,
which are generally pairs of numbers.
.IP
.nf
.ta 1.7i
\f(CW\eD'l @dh~ dv@'	\f1draw line from current position by @dh,~dv@\f(CW
\f(CW\eD'c @d@'	\f1draw circle of diameter @d@ with left side at current position\f(CW
\f(CW\eD'e @d sub 1 d sub 2@'	\f1draw ellipse of diameters @d sub 1@ and @d sub 2@\f(CW
\f(CW\eD'a @dh sub 1~ dv sub 1~ dh sub 2~ dv sub 2@'\f(CW	\f1draw arc from current position to @dh sub 1 +dh sub 2@, @dv sub 1 +dv sub 2@,\f(CW
	   \f1with center at @dh sub 1 ,~ dv sub 1@ from current position\f(CW
\f(CW\eD'~ @dh sub 1 dv sub 1 dh sub 2 dv sub 2 "..."@'\f(CW	\f1draw B-spline from current position by @dh sub 1, dv sub 1@,\f(CW
	   \f1then by @dh sub 2 , dv sub 2@, then by @dh sub 2 , dv sub 2@, then ...\f(CW
.LP
For example,
.CW "\eD'e0.2i 0.1i'"
draws the ellipse
\D'e.2i .1i'\|,
and
.CW "\eD'l.2i -.1i'\eD'l.1i .1i'"
the line
\D'l.2i -.1i'\D'l.1i .1i'\|.
A
.CW \\eD
with an unknown @c@ is processed and copied through to the output
for unspecified interpretation;
coordinates are interpreted alternately as horizontal and vertical
values.
.PP
Numbers taken as horizontal (first, third, etc.) have default scaling of ems;
vertical numbers (second, fourth, etc.) have default scaling of @V^@s (§1.3).
The position after a graphical object has been drawn is
at its end; for circles and ellipses, the ``end''
is at the right side.
.NH
Hyphenation.
.PP
Automatic hyphenation may be switched off and on.
When switched on with \&\f(CWhy\fR,
several variants may be set.
A \fIhyphenation indicator\fR character may be embedded in a word to
specify desired hyphenation points,
or may be prefixed to suppress hyphenation.
In addition,
the user may specify a small list of exception words.
.PP
Only words that consist of a central alphabetic string
surrounded by (usually null) non-alphabetic strings
are candidates for automatic hyphenation.
Words that contain hyphens
(minus),
em-dashes (\&\f(CW\e(em\fR),
or hyphenation indicator characters
are always subject to splitting after those characters,
whether automatic hyphenation is on or off.
.bt "\&\f(CW&nh\fR" "hyphenate" "-" "E" "Automatic hyphenation is turned off.
.bt "\&\f(CW&hy\fP@~N@" "on, @N=1@" "on, @N=1@" "E" "Automatic hyphenation is turned on
for @N >= 1@, or off for @N=0@.
If @N=2@, last lines (ones that will cause a trap)
are not hyphenated.
For @N=4@ and 8, the last and first two characters
respectively of a word are not split off.
These values are additive;
i.e., @N=14@ will invoke all three restrictions.
.bt "\&\f(CW&hc\fI c\fR" "\&\f(CW\e%" "\e%\fR" "E" "Hyphenation indicator character is set
to @c@ or to the default \&\f(CW\e%\fR.
The indicator does not appear in the output.
.bt "\&\f(CW&hw\fI word ...\fR" "" "ignored" "-" "Specify
hyphenation points in words
with embedded minus signs.
Versions of a word with terminal \fIs\fR are implied;
i.e.,
.CW dig-it
implies
.CW dig-its .
This list is examined initially and after
each suffix stripping.
The space available is small.
.NH
Three-Part Titles.
.PP
The titling function \&\f(CWtl\fR provides for automatic placement
of three fields at the left, center, and right of a line
with a title length
specifiable with \&\f(CWlt\fR.
\&\f(CWtl\fR may be used anywhere, and is independent of the
normal text collecting process.
A common use is in header and footer macros.
.h1
.bt "\&\f(CW&tl '\fIleft\fP'\fIcenter\fP'\fIright\fP'\fR" "-" "-" "" "The strings
\fIleft\fR, \fIcenter\fR, and \fIright\fR are
respectively left-adjusted, centered, and right-adjusted
in the current title length.
Any of the strings may be empty,
and overlapping is permitted.
If the page-number character (initially \&\f(CW%\fR) is found within any of the fields it is replaced
by the current page number in the format assigned to register \&\f(CW%\fR.
Any character may be used in place of
.CW '
as the string delimiter.
.bt "\&\f(CW&pc\fI c\fR" "\&\f(CW%\fR" "off" "-" "The page number character is set to @c@,
or removed.
The page number register remains \&\f(CW%\fR.
.bt "\&\f(CW&lt\fI \(+-N\fR" "6.5\|in" "previous" "E,\fBm\fR" "Length of title
is set to @+- N@.
The line length and the title length are independent.
Indents do not apply to titles; page offsets do.
.NH
Output Line Numbering.
.PP
.ll -\w'0000'u
.nm 1 3
Automatic sequence numbering of output lines may be
requested with \&\f(CWnm\fR.
When in effect,
a three-digit, arabic number plus a digit-space
is prefixed to output text lines.
The text lines are thus offset by four digit-spaces,
and otherwise retain their line length;
a reduction in line length may be desired to keep the right margin
aligned with an earlier margin.
Blank lines, other vertical spaces, and lines generated by \&\f(CWtl\fR
are not numbered.
Numbering can be temporarily suspended with \&\f(CWnn\fR,
or with an \&\f(CW.nm\fR followed by a later \&\f(CW.nm +0\fR.
In addition,
a line number indent \fII\fR, and the number-text separation \fIS\fR
may be specified in digit-spaces.
Further, it can be specified that only those line numbers that are
multiples of some number @M@ are to be printed (the others will appear
as blank number fields).
.br
.nm
.ll
.bt "\&\f(CW&nm\fI \(+-N M S I\fR" "" "off" "E" "Line number mode.
If @+- N@ is given,
line numbering is turned on,
and the next output line numbered is numbered @+- N@.
Default values are @M=1@, @S=1@, and @I=0@.
Parameters corresponding to missing arguments are unaffected;
a non-numeric argument is considered missing.
In the absence of all arguments, numbering is turned off;
the next line number is preserved for possible further use
in number register \&\f(CWln\fR.
.bt "\&\f(CW&nn\fI N\fR" "-" "@N=1@" "E" "The next @N@ text output lines are not
numbered.
.PP
.ll -\w'0000'u
.nm +0
As an example, the paragraph portions of this section
are numbered with \fIM=\fR\|3:
\&\&\f(CW.nm\ 1\ 3\fR was placed at the beginning;
\&\&\f(CW.nm\fR was placed at the end of the first paragraph;
and \&\f(CW.nm\ +0\fR was placed in front of this paragraph;
and \&\f(CW.nm\fR finally placed at the end.
Line lengths were also changed (by \&\f(CW\ew'0000'u\fR) to keep the right side aligned.
Another example is
.CW .nm
.CW +5
.CW 5
.CW x
.CW 3 ,
which turns on numbering with the line number of the next
line to be 5 greater than the last numbered line,
with @M=5@, with spacing \fIS\fR untouched, and with the indent \fII\fR set to 3.
.br
.ll
.nm
.NH
Conditional Acceptance of Input
.PP
In the following,
@c@ is a one-character built-in \fIcondition\fR name,
\&\f(CW!\fR signifies \fInot\fR,
@N@ is a numerical expression,
\fIstring1\fR and \fIstring2\fR are strings delimited by any non-blank, non-numeric character not in the strings,
and
\fIanything\fR represents what is conditionally accepted.
.bt "\&\f(CW&if\fI c anything\fR" "-" "-" "" "If condition
@c@ true, accept \fIanything\fR as input;
in multi-line case use \e{\fIanything\|\fR\e}.
.bt "\&\f(CW&if !\fIc anything\fR" "-" "-" "" "If condition @c@ false, accept \fIanything\fR.
.bt "\&\f(CW&if\fI N anything\fR" "" "-" "\fBu\fR" "If expression @N@ > 0, accept \fIanything\fR.
.bt "\&\f(CW&if !\fIN anything\fR" "" "-" "\fBu\fR" "If expression @N@ ≤ 0 [sic], accept \fIanything\fR.
.bt "\&\f(CW&if '\fIstring1\f(CW'\fIstring2\f(CW'\fI anything\fR" "-" "" "" "If \fIstring1\fR identical to \fIstring2\fR,
accept \fIanything\fR.
.bt "\&\f(CW&if !'\fIstring1\f(CW'\fIstring2\f(CW'\fI anything\fR" "-" "" "" "If \fIstring1\fR not identical to \fIstring2\fR,
accept \fIanything\fR.
.bt "\&\f(CW&ie\fI c anything\fR" "" "-" "\fBu\fR" "If portion of if-else;
all of the forms for \&\f(CWif\fR above are valid.
.bt "\&\f(CW&el\fI anything\fR" "-" "-" "" "Else portion of if-else.
.PP
The built-in condition names are:
.TS
center box;
c2|c2
c2|c2
c2|l2.
Condition
Name	True If
_
\&\f(CWo\fR	Current page number is odd
\&\f(CWe\fR	Current page number is even
\&\f(CWt\fR	Formatter is \*(TR
\&\f(CWn\fR	Formatter is \*(NR
.TE
If the condition @c@ is true, or if the number @N@ is greater than zero,
or if the strings compare identically (including motions and character size and font),
\fIanything\fR is accepted as input.
If a \&\f(CW!\fR precedes the condition, number, or string comparison,
the sense of the acceptance is reversed.
.PP
Any spaces between the condition and the beginning of \fIanything\fR are skipped over.
The \fIanything\fR can be either a single input line (text, macro, or whatever)
or a number of input lines.
In the multi-line case,
the first line must begin with a left delimiter \&\f(CW\e{\fR and
the last line must end with a right delimiter \&\f(CW\e}\fR.
.PP
The request \&\f(CWie\fR (if-else) is identical to \&\f(CWif\fR
except that the acceptance state is remembered.
A subsequent and matching \&\f(CWel\fR (else) request then uses the reverse sense of that state.
\&\f(CWie\fR-\&\f(CWel\fR pairs may be nested.
.PP
Some examples are:
.P1
&if e .tl '\|Even Page %'''
.P2
which outputs a title if the page number is even; and
.P1
&ie \en%>1 \e{\e
\&'	sp 0.5i
&	tl 'Page %'''
\&'	sp |1.2i \e}
&el .sp |2.5i
.P2
which treats page 1 differently from other pages.
.NH
Environment Switching.
.PP
A number of the parameters that
control the text processing are gathered together into an
\fIenvironment\fR, which can be switched by the user.
The environment parameters are those associated
with requests noting E in their \fINotes\fR column;
in addition, partially collected lines and words are in the environment.
Everything else is global; examples are page-oriented parameters,
diversion-oriented parameters, number registers, and macro and string definitions.
All environments are initialized with default parameter values.
.bt "\&\f(CW&ev\fI N\fR" "@N=0@" "previous" "-" "Environment switched to
environment @0 <= N <= 2@.
Switching is done in push-down fashion so that
restoring a previous environment \fImust\fR be done with \&\f(CW.ev\fR
rather than specific reference.
Note that what is pushed down and restored is the environment
.I number,
not its contents.
.NH
Insertions from the Standard Input
.PP
The input can be temporarily switched to the system standard input
with \&\f(CWrd\fR,
which will switch back when two consecutive newlines
are found (the extra blank line is not used).
This mechanism is intended for insertions in form-letter-like documentation.
The standard input can be the user's keyboard,
a pipe, or a file.
.bt "\&\f(CW&rd\fI prompt\fR" "-" "\fIprompt=\fR\s-1BEL\s+1" "-" "Read insertion
from the standard input until two newlines in a row are found.
If the standard input is the user's keyboard, \fIprompt\fR (or a \s-1BEL\s+1)
is written onto the standard output.
\&\f(CWrd\fR behaves like a macro,
and arguments may be placed after \fIprompt\fR.
.bt "\&\f(CW&ex\fR" "-" "-" "-" "Exit from \*(NR/\*(TR.
Text processing is terminated exactly as if all input had ended.
.PP
If insertions are to be
taken from the terminal keyboard while output is being printed
on the terminal, the command line option \&\f(CW-q\fR will turn off the echoing
of keyboard input and prompt only with \s-1BEL\s+1.
The regular input and insertion input cannot
simultaneously come from the standard input.
.PP
As an example,
multiple copies of a form letter may be prepared by entering the insertions
for all the copies in one file to be used as the standard input,
and causing the file containing the letter to reinvoke itself with \&\f(CWnx\fR (§19);
the process would ultimately be ended by an \&\f(CWex\fR in the insertion file.
.NH
Input/Output File Switching
.bt "\&\f(CW&so\fI filename\fR" "" "-" "-" "Switch source file.
The top input (file reading) level is switched to \fIfilename\fR.
When the new file ends,
input is again taken from the original file.
\&\f(CWso\fR's may be nested.
.bt "\&\f(CW&nx\fI filename\fR" "" "end-of-file" "-" "Next file is \fIfilename\fR.
The current file is considered ended, and the input is immediately switched
to \fIfilename\fR.
.bt "\&\f(CW&sy\fI string\fR" "" "-" "-" "Execute program from \fIstring\fR,
which is the rest of the input line.
The output is not collected automatically.
The number register
.CW $$ ,
which contains the process id of the \*(TR process,
may be useful in generating unique filenames for output.
.bt "\&\f(CW&pi\fI string\fR" "" "-" "-" "Pipe output to \fIstring\fR,
which is the rest of the input line.
This request must occur before any printing occurs;
typically it is the first line of input.
.bt "\&\f(CW&cf\fI filename\fR" "" "-" "-" "Copy
contents of file
.I filename
to output, completely unprocessed.
The file is assumed to contain something meaningful
to subsequent processes.
.NH
Miscellaneous
.br
.mc \s12\(br\s0
.bt "\&\f(CW.mc\fI c N\fR" - off E,\fBm\fR "Specifies
that a \fImargin\fR character @c@ appear a distance
@N@ to the right of the right margin
after each non-empty text line (except those produced by \&\f(CWtl\fR).
If the output line is too long (as can happen in nofill mode)
the character will be appended to the line.
If @N@ is not given, the previous @N@ is used; the initial @N@ is
0.2 inches in \*(NR and 1 em in \*(TR.
The margin character used with this paragraph was a 12-point box-rule.
.br
.mc
.bt "\&\f(CW.tm\fI string\fR" "-" "newline" "-" "After skipping initial blanks,
\fIstring\fR (rest of the line) is read in copy mode
and written on the standard error.
.bt "\&\f(CW&ab\fI string\fR" "-" "newline" "-" "After skipping initial blanks,
\fIstring\fR (rest of the line) is read in copy mode
and written on the standard error.
\*(Tr or \*(NR then exit.
.bt "\&\f(CW.ig\fI yy\fR" "-" "\fI.yy=\&\f(CW..\fR" "-" "Ignore
input lines.
\&\f(CWig\fR behaves exactly like \&\f(CWde\fR (§7) except that the
input is discarded.
The input is read in copy mode, and any auto-incremented
registers will be affected.
.bt "\&\f(CW.lf\fI N filename\fR" "" "-" "-" "Set
line number to @N@ and filename to @filename@
for purposes of subsequent error messages, etc.
The number register [sic]
.CW .F
contains the name of the current input file,
as set by command line argument,
.CW so ,
.CW nx ,
or
.CW lf .
The number register
.CW .c
contains the number of input lines read from the current file,
again perhaps as modified by
.CW lf .
.CW 
.bt "\&\f(CW.pm\fI t\fR" "-" "all" "-" "Print macros.
The names and sizes of all of the defined macros and strings are printed
on the standard error;
if \fIt\fR is given, only the total of the sizes is printed.
The sizes is given in blocks
of 128 characters.
.bt "\&\f(CW.fl\fR" - - B "Flush output buffer.
Force output, including any pending position information.
......
.NH
Output and Error Messages.
.PP
The output from \&\f(CWtm\fR, \&\f(CWpm\fR, and the prompt from \&\f(CWrd\fR,
as well as various error messages, are written onto
the standard error.
The latter is different from the standard output,
where formatted text goes.
By default, both are written onto the user's terminal,
but they can be independently redirected.
.PP
Various error conditions may occur during
the operation of \*(NR and \*(TR.
Certain less serious errors having only local impact do not
cause processing to terminate.
Two examples are \fIword overflow\fR, caused by a word that is too large
to fit into the word buffer (in fill mode), and
\fIline overflow\fR, caused by an output line that grew too large
to fit in the line buffer.
In both cases, a message is printed, the offending excess
is discarded,
and the affected word or line is marked at the point of truncation
with a \(** in \*(NR and a \(lh in \*(TR.
Processing continues if possible,
on the grounds that output useful for debugging may be produced.
If a serious error occurs, processing terminates,
and a message is printed, along with a list of the macro names currently active.
Examples of serious errors include the inability to create, read, or write files,
and the exceeding of certain internal limits that
make future output unlikely to be useful.
.NH
Output Language
.PP
\*(Tr
produces its output in a language that is independent of any
specific output device,
except that the numbers in it have been computed on the basis
of the resolution of the device,
and the sizes, fonts, and characters that that device can print.
Nevertheless it is quite possible to interpret that output
on a different device, within the latter's capabilities.
.IP
.nf
.ta .7i
@cw s n@	set point size to @n@
@cw f n@	set font to @n@
@cw c c@	print character @c@
@cw C name@	print the character called @name@; terminate @name@ by white space
@cw N n@	print character @n@ on current font
@cw H n@	go to absolute horizontal position \f2n\fP (@n >= 0@)
@cw V n@	go to absolute vertical position \f2n\fP (@n >= 0@, down is positive)
@cw h n@	go \f2n\fP units horizontally; @n < 0@ is to the left
@cw v n@	go \f2n\fP units vertically; @n < 0@ is up
@nnc@	move right \f2nn\fP, then print \s-1UTF\s0 character \f2c\fP;  \f2nn\fP must be exactly 2 digits
@cw p n@	new page \f2n\fP begins\(emset vertical position to 0
@cw n b~a@	end of line (information only\(emno action);  \f2b\fP = space before line, \f2a\fP = after
@cw w@	paddable word space (information only\(emno action)
@cw D c@ ...\en	graphics function @c@; see below
@cw x@ ...\en	device control functions; see below
@cw "#"@ ...\en	comment
.LP
All position values are in units.
Sequences that end in digits must be followed by a non-digit.
Blanks, tabs and newlines may occur as separators
in the input, and are mandatory to separate constructions
that would otherwise be confused.
Graphics functions, device control functions, and comments extend to the
end of the line they occur on.
.PP
The device control and graphics commands are intended as open-ended
families, to be expanded as needed.
The graphics functions coincide directly with the
.CW \eD 
sequences:
.IP
.nf
.ta 1.7i
@cw Dl@ \f2dh dv\fP	draw line from current position by @dh,~ dv@
@cw Dc@ \f2d\fP	draw circle of diameter \f2d\fP with left side here
@cw De@ @dh sub 1~dv sub 2@	draw ellipse of diameters @dh sub 1@ and @ dv sub 2@\fP
@cw Da ~dh sub 1~ dv sub 1 ~ dh sub 2 ~dv sub 2@	draw arc from current position to @dh sub 1 +dh sub 2 ,~ dv sub 1 +dv sub 2@,
		   center at @dh sub 1 ,~ dv sub 1@ from current position
@cw "D~" ~dh sub 1 ~dv sub 1 ~dh sub 2 ~dv sub 2@ ...	draw B-spline from current position to @dh sub 1 ,~ dv sub 1@,
		   then to @dh sub 2 , ~dv sub 2@, then to ...
@cw "D"z ~dh sub 1 ~dv sub 1 ~dh sub 2 ~dv sub 2@ ...	for any other @z@ is uninterpreted
.LP
In all of these, @dh, ~dv@ is an increment on the current horizontal and
vertical position,
with down and right positive.
All distances and dimensions are in units.
.PP
The device control functions begin with
.CW x ,
then a command, then other parameters.
.IP
.ta .8i 1.2i
.nf
.ft CW
x T \f2s\fP	\f1name of typesetter is @s@\f(CW
x r \f2n h v\fP	\f1resolution is @n@ units/inch;\f(CW
		\f1@h@ = minimum horizontal motion, @v@ = minimum vertical\f(CW
x i	\f1initialize\fP
x f \f2n s\fP	\f1mount font @s@ on font position @n@\f(CW
x p	\f1pause\(emcan restart\f(CW
x s	\f1stop\(emdone forever\f(CW
x t	\f1generate trailer information, if any\f(CW
x H \f2n\fP	\f1set character height to @n@\f(CW
x S \f2n\fP	\f1set slant to @n@\f(CW
x X \f2any\fP	\f1generated by the \&\f(CW\eX\fP function\f(CW
x \f2any\fP	\f1to be ignored if not recognized\f(CW
.LP
Subcommands like
.CW i '' ``
may be spelled out like
.CW init ''. ``
.PP
The commands
.CW "x T" ,
.CW "x r " ...,
and
.CW "x i"
must occur first;
fonts must be mounted before they can be used;
.CW "x s
comes last.
There are no other order requirements.
.PP
The following is the output from
.CW hello, "" ``
.CW world ''
for a typical printer,
as described in §23:
.P1
x T utf
x res 720 1 1
x init
V0
p1
.P2
.P1
x font 1 R
x font 2 I
x font 3 B
x font 4 BI
x font 5 CW
x font 6 H
x font 7 HB
x font 8 HX
x font 9 S1
x font 10 S
.P2
.P1
s10
f1
H0
s10
f1
V0
H720
V120
ch
50e44l28l28o50,w58w72o50r33l28dn120 0
x trailer
V7920
x stop
.P2
.PP
\*(Tr output is normally not redundant;
size and font changes and position information are not included
unless needed.
Nevertheless, each page is self-contained, for the benefit of postprocessors
that re-order pages or process only a subset.
.NH
Device and Font Description Files
.PP
The parameters that describe a output device
.I name
are read
from the directory
.CW /sys/lib/troff/font/dev@name@ ,
each time
\*(TR
is invoked.
The device name is provided by default,
by the environment variable
.CW TYPESETTER ,
or by a command-line argument
.CW -T@name@ .
The default device name is
.CW utf ,
for \s-1UTF\s0-encoded Unicode characters.
The pre-defined string
.CW .T
contains the name of the device.
The
.CW -F
command-line option may be used to change the default directory.
.......
.sc "Device description file.
General parameters of the device are stored, one per line, in
the file 
.CW /sys/lib/troff/font/dev@name@/DESC ,
as a sequence of names and values.
\*(Tr recognizes these parameters, and ignores any
others that may be present for specific drivers:
.IP
.nf
.ta 1i
@cw fonts ~ n ~ F sub 1  ~F sub 2  ~. . .~ F sub n@
@cw sizes ~ s sub 1 ~ s sub 2 ~ . . . cw 0@
@cw res ~n@
@cw hor ~n@
@cw vert ~n@
@cw unitwidth ~n@
@cw charset@
\f2list of multi-character character names (optional)\fP
.LP
The @F sub i@ are font names
to be initially mounted.
The list of sizes is a set of integers representing
some or all of the legal sizes the device can produce,
terminated by a zero.
The 
.CW res
parameter gives the resolution of the machine in units per inch;
.CW hor
and
.CW ver
give the minimum number of units that can be moved
horizontally and vertically.
.PP
Character widths for each font are assumed to be given in machine units
at point size
.CW unitwidth .
(In other words, a character with a width of
@n@ is @n@ units wide at size
.CW unitwidth .)
All widths are integers at all sizes.
.PP
A list of valid character names may be introduced by
.CW charset ;
the list of names is optional.
.PP
A line whose first non-blank character is
.CW #
is a comment.
Except that
.CW charset
must occur last, parameters may appear in any order.
.PP
Here is a subset of the
.CW DESC
file for a typical Postscript printer:
.P1
# Description file for Postscript printers.

fonts 10 R I B BI CW H HB HX S1 S
sizes 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23
   24 25 26 27 28 29 30 31 32 33 34 35 36 38 40 44 48 54 60 72 0
res 720
hor 1
vert 1
unitwidth 10
charset
hy ct fi fl ff Fi Fl dg em 14 34 12 en aa
ga ru sc dd -> br Sl ps cs cy as os =. ld
rd le ge pp -+ ob vr
sq bx ci fa te ** pl mi eq ~= *A *B *X *D
*E *F *G *Y *I *K *L *M *N *O *P *R *H *S *T *U *W
*C *Q *Z ul rn *a *b *x *d *e *f *g *y *i *k
*l *m *n *o *p *h *r *s *t *u *w *c *q *z
.P2
.sc "Font description files.
Each font is described by an analogous description file,
which begins with parameters of the font, one per line, followed by a
list of characters and widths.
The file for font
.I f
is
.CW /sys/lib/troff/font/dev@name@/@f@ .
.IP
.ta 1.7i
.nf
@cw name ~str@	name of font is @str@
@cw ligatures ~ ". . ." ~ cw "0"@	list of ligatures
@cw spacewidth ~n@	width of a space on this font
@cw special@	this is a special font
@cw charset@
\f2list of character name, width, ascender/descender, code\fP, tab separated
.LP
The
.CW name
and
.CW charset
fields are mandatory;
.CW charset
must be last.
Comments are permitted,
as are other unrecognized parameters.
.PP
Each line following
.CW charset 
describes one character: its name, its width in units as described above,
ascender/descender information, and a decimal, octal or hexadecimal value
by which the output device knows it
(the
.CW \eN
``number'' of the character).
The character name is arbitrary, except that
.CW ---
signifies an unnamed character.
If the width field contains
.CW \&" ,
the name is a synonym for the previous character.
The ascender/descender field is 1 if
the character has a descender (hangs below the baseline, like
.CW y ),
is 2 if it has an ascender (is tall, like
.CW Y ),
is 3 if both,
and is 0 if neither.
The value is returned
in the 
.CW ct
register, as computed by the
.CW \ew
function (§11.2).
.PP
Here are excerpts from a typical font description file
for the same Postscript printer.
.P1
hy	33	0	45	hyphen \e(hy
-	"			- is a synonym for \e(hy
.sp .3
Q	72	3	81
.sp .3
a	44	0	97
b	50	2	98
c	44	0	99
d	50	2	100
y	50	1	121
.sp .3
em	100	0	208
---	44	2	220	Pound symbol £, \eN'220'
---	36	0	221	centered dot \eN'221'
.P2
This says, for example, that the width of the letter
.CW a
is 44 units at point size 10,
the value of 
.CW unitwidth .
Point sizes are scaled linearly and rounded, so the width of
.CW a
will be 44 at size 10, 40 at size 9, 35 at size 8,
and so on.
.sp 100
.BP
.fp 8 C CW
.tr &.
.tr |
.tr ~|
.TL
Tutorial Examples
.SP
....2C
.sp .25i
.SH
Introduction
.PP
It is almost always necessary to
prepare at least a small set of macro definitions
to describe a document.
Such common formatting needs
as page margins and footnotes
are deliberately not built into \*(NR and \*(TR.
Instead,
the macro and string definition, number register, diversion,
environment switching, page-position trap, and conditional input mechanisms
provide the basis for user-defined implementations.
.PP
For most uses, a standard package like
.CW -ms
or
.CW -mm
is the right choice.
The next stage is to augment that,
or to selectively replace macros from the standard package.
The last stage, much harder,
is to write one's own from scratch.
This is not a task for the novice.
.PP
The examples discussed here are intended to be useful and somewhat realistic,
but will not necessarily cover all relevant contingencies.
Explicit numerical parameters are used
in the examples
to make them easier to read and to
illustrate typical values.
In many cases, number registers would be used
to reduce the number of places where numerical
information is kept,
and to concentrate conditional parameter initialization
like that which depends on whether \*(TR or \*(NR is being used.
.SH
Page Margins
.PP
As discussed in §3,
header and footer macros are usually defined
to describe the top and bottom page margin areas respectively.
A trap is planted at page position 0 for the header, and at
\fI-N\fR (\fIN\fR from the page bottom) for the footer.
The simplest such definitions might be
.P1 .1i
&de hd	\e"define header
\&'sp 1i
&&	\e"end definition
&de fo	\e"define footer
\&'bp
&&	\e"end definition
&wh 0 hd
&wh -1i fo
.P2
which provide blank 1 inch top and bottom margins.
The header will occur on the \fIfirst\fR page
only if the definition and trap exist prior to
the initial pseudo-page transition (§3).
In fill mode, the output line that springs the footer trap
was typically forced out because some part or whole word didn't fit on it.
If anything in the footer and header that follows causes a break,
that word or part word will be forced out.
In this and other examples,
requests like \&\f(CWbp\fR and \&\f(CWsp\fR that normally cause breaks are invoked using
the no-break control character \&\f(CW'\fR
to avoid this.
When the header/footer design contains material
requiring independent text processing, the
environment may be switched, avoiding
most interaction with the running text.
.PP
A more realistic example would be
.P1 .1i
&de hd	\e"header
&if \e\en%>1 \e{\e
\&'sp ~0.5i-1	\e"tl base at 0.5i
&tl ''- % -''	\e"centered page number
&ps	\e"restore size
&ft	\e"restore font
&vs  \e}	\e"restore vs
\&'sp ~1.0i  	\e"space to 1.0i
&ns	\e"turn on no-space mode
&&
&de fo	\e"footer
&ps 10	\e"set footer/header size
&ft R	\e"set font
&vs 12p	\e"set baseline spacing
&if \e\en%=1 \e{\e
\&'sp ~\e\en(.pu-0.5i-1  \e"tl base 0.5i up
&tl ''- % -'' \e}  \e"first page number
\&'bp
&&
&wh 0 hd
&wh -1i fo
.P2
which sets the size, font, and baseline spacing for the
header/footer material, and ultimately restores them.
The material in this case is a page number at the bottom of the
first page and at the top of the remaining pages.
The \&\f(CWsp\fR's refer to absolute positions to avoid
dependence on the baseline spacing.
Another reason for doing this in the footer
is that the footer is invoked by printing a line whose
vertical spacing swept past the trap position by possibly
as much as the baseline spacing.
No-space mode is turned on at the end of \&\f(CWhd\fR
to render ineffective
accidental occurrences of \&\f(CWsp\fR at the top of the running text.
.PP
This method of restoring size, font, etc., presupposes
that such requests (that set \fIprevious\fR value) are \fInot\fR
used in the running text.
A better scheme is to save and restore both the current \fIand\fR
previous values as shown for size in the following:
.P1 .1i
&de fo
&nr s1 \e\en(.s	\e"current size
&ps
&nr s2 \e\en(.s	\e"previous size
&  ---	\e"rest of footer
&&
&de hd
&  ---	\e"header stuff
&ps \e\en(s2  \e"restore previous size
&ps \e\en(s1  \e"restore current size
&&
.P2
Page numbers may be printed in the bottom margin
by a separate macro triggered during the footer's
page ejection:
.P1 .1i
&de bn	\e"bottom number
&tl ''- % -''	\e"centered page number
&&
&wh -0.5i-1v bn	 \e"tl base 0.5i up
.P2
.SH
Paragraphs and Headings
.PP
The housekeeping
associated with starting a new paragraph should be collected
in a paragraph macro
that, for example,
does the desired preparagraph spacing,
forces the correct font, size, baseline spacing, and indent,
checks that enough space remains for \fImore than one\fR line,
and
requests a temporary indent.
.P1 .1i
&de pg    \e"paragraph
&br       \e"break
&ft R     \e"force font,
&ps 10    \e"size,
&vs 12p   \e"spacing,
&in 0     \e"and indent
&sp 0.4   \e"prespace
&ne 1+\e\en(.Vu  \e"want more than 1 line
&ti 0.2i         \e"temp indent
&&
.P2
The first break in \&\f(CWpg\fR
will force out any previous partial lines,
and must occur before the \&\f(CWvs\fR.
The forcing of font, etc., is
partly a defense against prior error and
partly to permit
things like section heading macros to
set parameters only once.
The prespacing parameter is suitable for \*(TR;
a larger space, at least as big as the output device vertical resolution, would be
more suitable in \*(NR.
The choice of remaining space to test for in the \&\f(CWne\fR
is the smallest amount greater than one line
(the \&\f(CW.V\fR is the available vertical resolution).
.PP
A macro to automatically number section headings
might look like:
.P1 .1i
&de sc	\e"section
&  ---	\e"force font, etc.
&sp 0.4	\e"prespace
&ne 2.4+\e\en(.Vu \e"want 2.4+ lines
.lg 0
&fi
.lg
\e\en+S.
&&
&nr S 0 1	\e"init S
.P2
The usage is \&\f(CW.sc\fR,
followed by the section heading text,
followed by \&\f(CW.pg\fR.
The \&\f(CWne\fR test value includes one line of heading,
0.4 line in the following \&\f(CWpg\fR, and
one line of the paragraph text.
A word consisting of the next section number and a period is
produced to begin the heading line.
The format of the number may be set by \&\f(CWaf\fR (§8).
.PP
Another common form is the labeled, indented paragraph,
where the label protrudes left into the indent space.
.P1 .1i
&de lp	\e"labeled paragraph
&pg
&in 0.5i	\e"paragraph indent
&ta 0.2i 0.5i	\e"label, paragraph
&ti 0
\et\e\e$1\et\ec	\e"flow into paragraph
&&
.P2
The intended usage is ``\&\f(CW.lp\fR \fIlabel\fR\|'';
\fIlabel\fR will begin at 0.2 inch, and
cannot exceed a length of 0.3 inch without intruding into
the paragraph.
The label could be right adjusted against 0.4 inch by
setting the tabs instead with \&\f(CW.ta|0.4iR|0.5i\fR.
The last line of \&\f(CWlp\fR ends with \&\f(CW\ec\fR so that
it will become a part of the first line of the text
that follows.
.SH
Multiple Column Output
.PP
The production of multiple column pages requires
the footer macro to decide whether it was
invoked by other than the last column,
so that it will begin a new column rather than
produce the bottom margin.
The header can initialize a column register that
the footer will increment and test.
The following is arranged for two columns, but
is easily modified for more.
.P1 .1i
&de hd	\e"header
&  ---
&nr cl 0 1	\e"init column count
&mk	\e"mark top of text
&&
.P2
.P1 .1i
&de fo	\e"footer
&ie \e\en+(cl<2 \e{\e
&po +3.4i	\e"next column; 3.1+0.3
&rt	\e"back to mark
&ns \e}	\e"no-space mode
&el \e{\e
&po \e\enMu	\e"restore left margin
&  ---
\&'bp \e}
&&
&ll 3.1i	\e"column width
&nr M \e\en(.o	\e"save left margin
.P2
Typically a portion of the top of the first page
contains full width text;
the request for the narrower line length,
as well as another \&\f(CW.mk\fR would
be made where the two column output was to begin.
.SH
Footnotes
.PP
The footnote mechanism to be described is used by
embedding the footnotes in the input text at the
point of reference,
demarcated by an initial \&\f(CW.fn\fR and a terminal \&\f(CW.ef\fR:
.P1 .1i
&fn
\fIFootnote text and control lines...\fP
&ef
.P2
In the following,
footnotes are processed in a separate environment and diverted
for later printing in the space immediately prior to the bottom
margin.
There is provision for the case where the last collected
footnote doesn't completely fit in the available space.
.P1 .1i
&de hd	\e"header
&  ---
&nr x 0 1	\e"init footnote count
&nr y 0-\e\enb	\e"current footer place
&ch fo -\e\enbu	\e"reset footer trap
&if \e\en(dn .fz	\e"leftover footnote
&&
.P2
.P1 .1i
&de fo	\e"footer
&nr dn 0  \e"zero last diversion size
&if \e\enx \e{\e
&ev 1	\e"expand footnotes in ev1
&nf	\e"retain vertical size
&FN	\e"footnotes
&rm FN	\e"delete it
.P2
.P1 .1i
&if "\e\en(.z"fy" .di  \e"end overflow di
&nr x 0	\e"disable fx
&ev  \e}	\e"pop environment
&  ---
\&'bp
&&
.P2
.P1 .1i
&de fx	\e"process footnote overflow
&if \e\enx .di fy	\e"divert overflow
&&
.P2
.P1 .1i
&de fn	\e"start footnote
&da FN	\e"divert (append) footnote
&ev 1	\e"in environment 1
&if \e\en+x=1 .fs   \e"if 1st, separator
&fi	\e"fill mode
&&
.P2
.P1 .1i
&de ef	\e"end footnote
&br	\e"finish output
&nr z \e\en(.v	\e"save spacing
&ev	\e"pop ev
&di	\e"end diversion
&nr y -\e\en(dn	\e"new footer position,
&if \e\enx=1 .nr y -(\e\en(.v-\e\enz) \e
	\e"uncertainty correction
&ch fo \e\enyu	\e"y is negative
&if (\e\en(nl+1v)>(\e\en(.p+\e\eny) \e
&ch fo \e\en(nlu+1v	 \e"didn't fit
&&
.P2
.P1 .1i
&de fs	\e"separator
\el'1i'	\e"1 inch rule
&br
&&
.P2
.P1 .1i
&de fz	\e"get leftover footnote
&fn
&nf	\e"retain vertical size
&fy	\e"where fx put it
&ef
&&
.P2
.P1 .1i
&nr b 1.0i  \e"bottom margin size
&wh 0 hd    \e"header trap
&wh 12i fo  \e"footer trap->temp pos
&wh -\e\enbu fx	\e"fx at footer position
&ch fo -\e\enbu	\e"conceal fx with fo
.P2
.PP
The header \&\f(CWhd\fR initializes a footnote count register \&\f(CWx\fR,
and sets both the current footer trap position register \&\f(CWy\fR and
the footer trap itself to a nominal position specified in
register \&\f(CWb\fR.
In addition, if the register \&\f(CWdn\fR indicates a leftover footnote,
\&\f(CWfz\fR is invoked to reprocess it.
The footnote start macro \&\f(CWfn\fR begins a diversion (append) in environment 1,
and increments the count \&\f(CWx\fR; if the count is one, the footnote separator \&\f(CWfs\fR
is interpolated.
The separator is kept in a separate macro to permit user redefinition.
.PP
The footnote end macro \&\f(CWef\fR restores
the previous environment and ends the diversion after saving the spacing size in register \&\f(CWz\fR.
\&\f(CWy\fR is then decremented by the size of the footnote, available in \&\f(CWdn\fR;
then on the first footnote, \&\f(CWy\fR is further decremented by the difference
in vertical baseline spacings of the two environments, to
prevent the late triggering of the footer trap from causing the last
line of the combined footnotes to overflow.
The footer trap is then set to the lower (on the page) of \&\f(CWy\fR or the current page position (\&\f(CWnl\fR)
plus one line, to allow for printing the reference line.
.PP
If indicated by \&\f(CWx\fR, the footer \&\f(CWfo\fR rereads the footnotes from \&\f(CWFN\fR in nofill mode
in environment 1,
and deletes \&\f(CWFN\fR.
If the footnotes were too large to fit, the macro \&\f(CWfx\fR will be trap-invoked to redivert
the overflow into \&\f(CWfy\fR,
and the register \&\f(CWdn\fR will later indicate to the header whether \&\f(CWfy\fR is empty.
.PP
Both \&\f(CWfo\fR and \&\f(CWfx\fR are planted in the nominal footer trap position in an order
that causes \&\f(CWfx\fR to be concealed unless the \&\f(CWfo\fR trap is moved.
The footer then terminates the overflow diversion, if necessary, and
zeros \&\f(CWx\fR to disable \&\f(CWfx\fR,
because the uncertainty correction
together with a not-too-late triggering of the footer can result
in the footnote rereading finishing before reaching the \&\f(CWfx\fR trap.
.PP
A good exercise for the student is to combine the multiple-column and footnote mechanisms.
.SH
The Last Page
.PP
After the last input file has ended, \*(NR and \*(TR
invoke the \fIend macro\fR (§7), if any,
and when it finishes, eject the remainder of the page.
During the eject, any traps encountered are processed normally.
At the end of this last page, processing terminates
unless a partial line, word, or partial word remains.
If it is desired that another page be started, the end-macro
.P1 .1i
&de en	\e"end-macro
\ec
\&'bp
&&
&em en
.P2
will deposit a null partial word,
and produce another last page.
....1C
.sp 100
.BP
........
.TL
Special Character Names
.SP
.PP
The following table lists names for a set of characters,
most of which have traditionally been provided by \*(TR using
the `special' or `symbol' font.
Many of these sequences are old ways to get what are now Unicode
characters;
Lucida Sans, for example, has glyphs corresponding to many of these
but does not have the special sequences.
Therefore
the \*(TR sequence
.CW \e(*F
gives the character \(*F from the Times font instead of the
character Φ from the current font, in this case Lucida Sans.
Not all sequences print on any particular device, including this one; Peter
faces appear in their place.
.TS
center;
l l20fCW l l20fCW l l20fCW.
\&\'	\e'	\(*m	\e(*m	\(~=	\e(~=
\`	\e`	\(*n	\e(*n	\(ap	\e(ap
\(em	\e(em	\(*c	\e(*c	\(!=	\e(!=
\(en	\e(en	\(*o	\e(*o	\(->	\e(->
\(hy	\e(hy	\(*p	\e(*p	\(<-	\e(<-
\-	\e-	\(*r	\e(*r	\(ua	\e(ua
\(bu	\e(bu	\(*s	\e(*s	\(da	\e(da
\(sq	\e(sq	\(ts	\e(ts	\(mu	\e(mu
\(ru	\e(ru	\(*t	\e(*t	\(di	\e(di
\(14	\e(14	\(*u	\e(*u	\(+-	\e(+-
\(12	\e(12	\(*f	\e(*f	\(cu	\e(cu
\(34	\e(34	\(*x	\e(*x	\(ca	\e(ca
\(fi	\e(fi	\(*q	\e(*q	\(sb	\e(sb
\(fl	\e(fl	\(*w	\e(*w	\(sp	\e(sp
\(ff	\e(ff	\(*A	\e(*A	\(ib	\e(ib
\(Fi	\e(Fi	\(*B	\e(*B	\(ip	\e(ip
\(Fl	\e(Fl	\(*G	\e(*G	\(if	\e(if
\(de	\e(de	\(*D	\e(*D	\(pd	\e(pd
\(dg	\e(dg	\(*E	\e(*E	\(gr	\e(gr
\(fm	\e(fm	\(*Z	\e(*Z	\(no	\e(no
\(ct	\e(ct	\(*Y	\e(*Y	\(is	\e(is
\(rg	\e(rg	\(*H	\e(*H	\(pt	\e(pt
\(co	\e(co	\(*I	\e(*I	\(es	\e(es
\(pl	\e(pl	\(*K	\e(*K	\(mo	\e(mo
\(mi	\e(mi	\(*L	\e(*L	\(br	\e(br
\(eq	\e(eq	\(*M	\e(*M	\(dd	\e(dd
\(**	\e(**	\(*N	\e(*N	\(rh	\e(rh
\(sc	\e(sc	\(*C	\e(*C	\(lh	\e(lh
\(aa	\e(aa	\(*O	\e(*O	\(L1	\e(bs
\(ga	\e(ga	\(*P	\e(*P	\(or	\e(or
\(ul	\e(ul	\(*R	\e(*R	\(ci	\e(ci
\(sl	\e(sl	\(*S	\e(*S	\(lt	\e(lt
\(*a	\e(*a	\(*T	\e(*T	\(lb	\e(lb
\(*b	\e(*b	\(*U	\e(*U	\(rt	\e(rt
\(*g	\e(*g	\(*F	\e(*F	\(rb	\e(rb
\(*d	\e(*d	\(*X	\e(*X	\(lk	\e(lk
\(*e	\e(*e	\(*Q	\e(*Q	\(rk	\e(rk
\(*z	\e(*z	\(*W	\e(*W	\(bv	\e(bv
\(*y	\e(*y	\(sr	\e(sr	\(lf	\e(lf
\(*h	\e(*h	\(rn	\e(rn	\(rf	\e(rf
\(*i	\e(*i	\(>=	\e(>=	\(lc	\e(lc
\(*k	\e(*k	\(<=	\e(<=	\(rc	\e(rc
\(*l	\e(*l	\(==	\e(==	\d\h'-5m'\(LH\u	\e(LH
.TE
