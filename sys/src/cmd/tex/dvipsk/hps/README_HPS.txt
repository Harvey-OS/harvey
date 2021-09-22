README_HPS, version 0.3

last modified Wed Jul 19 11:34:52 MDT 1995

This is an alpha release of a hacked version of dvips v5.58 for
converting HyperTeX into HyperPostScript that in turn can be run
through the Adobe distiller to produce a PDF file with chapter and
section numbers, references, equation numbers, etc. all links.
This program is intended for use with the TeX and LaTex macros
modified to create HyperTeX (see http://xxx.lanl.gov/hypertex/).
With these macros, TeX and LaTex can be converted into HyperTeX
without any additional work by the user.  The linking information
is stamped into the dvi file via a set of html: \special commands.
Normally conversion to PostScript involves a loss of this information.
This program preserves all of the information in the Hyperdvi file
and makes it available to all PostScript interpreters, as well as
the Adobe PDF distiller. In addition, the HyperPostScript produced will
work with ordinary PostScript interpreters as well.
The software itself is availabe at http://nqcd.lanl.gov/people/doyle/
or at ftp://gita.lanl.gov/people/doyle .

INSTALLATION:

To compile this program, just follow the usual instructions. Since
this version stems from the latest version of dvips, you should
install the up-to-date versions of config.* and the *.pro files.
You can edit the Makefile to put these in some other place besides
the usual one.  Compiling with the -DHPS option (enabled by default
in the Makefile) will produce the hacked version of dvips. Disabling
this option will produce the version of dvips that you already know
and love.  I have made an effort to keep my changes to the dvips
source code minimal. Most of the important stuff is in the source
file hps.c.  Almost all of the changes to the dvips source code
are mandated by the need for putting information at the beginning
of the PostScript file that isn't normally available until the
entire dvi file has been completely processed. Thus, two temporary
files are created when running the program to create HyperPostScript,
head.tmp and body.tmp. One can freely delete these after running
the program.

The files modified from the 5.58 dvips distribution are: 
dvips.c, dvips.h, paths.h, dosection.c, dopage.c, dospecial.c, output.c, 
header.c, Makefile, and a very minor (irrelevant) change to afm2tfm.c.

Files added are README_HPS.txt, CHANGES_HPS.txt, hps.c, and hps.lpro.

Anyone interested in going all the way to PDF is encouraged to install
PostScript versions of the TeX fonts on their system. The Adobe PDF
readers are extremely slow when dealing with bitmapped fonts. There
is, however, a bug in the Adobe distillers that requires a workaround
for certain characters.  See below for more info.  Commercial versions
of the fonts are available from BlueSky and public domain versions are
available at ftp://ftp.shsu.edu/pub/tex/fonts/postscript/bakoma/ and
ftp://ftp.shsu.edu/pub/tex/fonts/postscript/paradissa/ .
MS-DOS/Windows user beware: The latest Adobe Type Manager doesn't like
the BlueSky fonts. I haven't tried the public domain ones. (On a new
and different PC I was able to get the Blue Sky fonts installed with
ATM and it works fine. Your mileage may vary. July 19, 1995.)

USAGE:

After compiling with -DHPS, a user can invoke the Hyperdvi to
HyperPostScript conversion with the -z flag. If the flag is
omitted, dvips simply will revert back to the default behavior and
skip over the html: \special commands.

There appears to be a bug in the Adobe distillers. For optimization
purposes the distillers drop trailing blanks (character code 32)
from strings. Unfortunately, the PostScript fonts use this character
code for certain characters (notably the Greek letter psi), and
so these characters are dropped. This bug still appears in the 2.0
distiller.

                         -----HACK ALERT----- 

One way around this is to change all the trailing blanks of strings to
a character code that isn't in the font. The default behavior is to
substitute a blank for a missing character, i.e. the distiller is
fooled into substituting the right character. For instance, with the
BlueSky fonts, one can globally replace " )" with "\200)" and get the
desired result. With the public domain fonts, one will probably have
to use a character code in the range 128 to 191 since these fonts
duplicate the first 32 characters starting at 192 to avoid MS-DOS
problems. Fortunately, a simple sed will accomplish the replacements.

OTHER NOTES:

The program is rather straightforward and operates by converting
the dvi html: href \specials into pdfmark operators in the PostScript
that specify the links. The html: name \specials are combined into
a PostScript dictionary with all of the target information. The
hps.pro header tries to gracefully handle various contingencies.
If the resulting HyperPostScript file is handed off to an ordinary
PostScript interpreter, the pdfm operators are automatically defined
to pop the irrelevant information off the stack. On the other hand,
if it is handed to a distiller, the PostScript program in the header
tries to determine of what vintage the distiller is so that various
enhancements of the pdfmark operator can be used or ignored (color,
border dashing, etc.). Thus the resulting files can be distilled
using either the latest PC versions or the older Unix versions.
The hps.pro header was written by Tanmoy Bhattacharya
(http://nqcd.lanl.gov/people/tanmoy/tanmoy.html) with some additions
by me. Other PostScript interpreters are free to pick up the pdfm
operators as well. For instance, Tanmoy has hacked Ghostview to
pick up the links and make them active. This can be found at
ftp://gita.lanl.gov/people/tanmoy/hypertex/ .

THINGS TO DO:

1) More robust error checking and avoidance.  
2) Escaping html correctly for external URL's.  
3) More support for external URL's 
4) More flexibility in the appearance of links on the page 
5) Automatically get the papersize and margins correct (hardcoded at the
   moment).  
6) Make C source more platform independent (which T. Rokicki has tried
   very hard to maintain). Code has only been tested on NeXT
   3.3 (Motorola and HP) and Sun/SPARC (SunOS 4.1.4), and HP-UX 9.0.5. No
   effort has been made to make the code portable for MS-DOS, OS/2, VMS,
   or any other of the platforms that appear in the #ifdef statements.
   Code improvements for MS-DOS and OS/2 are forthcoming.

DISCLAIMER:

This software is made available without any implied warranties. See 
http://xxx.lanl.gov/disclaimer.html for a more complete disclaimer. 

COPYRIGHT:

The HyperPostScript additions to dvips are Copyright (C) 1994, 1995 by
Mark Doyle and the University of California. You may modify and
use this program to your heart's content, so long as you send
modifications to Mark Doyle and abide by the rest of the dvips
copyrights.

The hps.lpro file is Copyright (C) 1994, 1995 by Tanmoy Bhattacharya, Mark
Doyle, and  the University of California. You may modify and use
it to your heart's content, so long as you send modifications
to Tanmoy Bhattacharya or Mark Doyle and abide by the rest of the
dvips copyrights.

Mark Doyle
doyle@mmm.lanl.gov
http://nqcd.lanl.gov/people/doyle/
