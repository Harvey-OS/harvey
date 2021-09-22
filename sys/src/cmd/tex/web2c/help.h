/* help.h: help messages for web2c programs.

   This is included by everyone, from cpascal.h.  This is better than
   putting the help messages directly in the change files because (1)
   multiline strings aren't supported by tangle, and it would be a pain
   to make up a new syntax for them in web2c, and (2) when a help msg
   changes, we need only recompile, not retangle or reconvert.  The
   downside is that everything gets recompiled when any msg changes, but
   that's better than having umpteen separate tiny files.  (For one
   thing, the messages have a lot in common, so it's nice to have them
   in one place.)

Copyright (C) 1995, 96 Karl Berry.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef HELP_H
#define HELP_H

#ifdef BIBTEX
#define BIBTEXHELP \
"Usage: bibtex [OPTION]... AUXFILE[.aux]\n\
  Write bibliography for entries in AUXFILE to AUXFILE.bbl.\n\
\n\
-min-crossrefs=NUMBER  include item after NUMBER cross-refs; default 2\n\
-terse                 do not print progress reports\n\
-help                  display this help and exit\n\
-version               output version information and exit\n"
#endif /* BIBTEX */

#ifdef DVICOPY
#define DVICOPYHELP \
"Usage: dvicopy [OPTION]... [INDVI[.dvi] [OUTDVI[.dvi]]]\n\
  Expand virtual font references in INDVI to OUTDVI.\n\
  Defaults are standard input and standard output, respectively.\n\
\n\
-magnification=NUMBER  override existing magnification with NUMBER\n\
-max-pages=NUMBER      process NUMBER pages; default one million\n\
-page-start=PAGE-SPEC  start at PAGE-SPEC, for example `2' or `5.*.-2'\n\
-help                  display this help and exit\n\
-version               output version information and exit\n"
#endif /* DVICOPY */

#ifdef DVITOMP
#define DVITOMPHELP \
"Usage: dvitomp [OPTION]... DVIFILE[.dvi] [MPXFILE[.mpx]]\n\
  Translate DVIFILE to the MetaPost MPXFILE.\n\
  Default MPXFILE is basename of DVIFILE extended with `.mpx'.\n\
\n\
-help                  display this help and exit\n\
-version               output version information and exit\n"
#endif /* DVITOMP */

#ifdef DVITYPE
#define DVITYPEHELP \
"Usage: dvitype [OPTION]... DVIFILE[.dvi]\n\
  Verify and translate DVIFILE to human-readable form,\n\
  written to standard output.\n\
\n\
-dpi=REAL              set resolution to REAL pixels per inch; default 300.0\n\
-magnification=NUMBER  override existing magnification with NUMBER\n\
-max-pages=NUMBER      process NUMBER pages; default one million\n\
-output-level=NUMBER   verbosity level, from 0 to 4; default 4\n\
-page-start=PAGE-SPEC  start at PAGE-SPEC, for example `2' or `5.*.-2'\n\
-show-opcodes          show numeric opcodes (in decimal)\n\
-help                  display this help and exit\n\
-version               output version information and exit\n"
#endif /* DVITYPE */

#ifdef eTeX
#define ETEXHELP \
"Usage: etex [OPTION]... [TEXNAME[.tex]] [COMMANDS]\n\
   or: etex [OPTION]... \\FIRST-LINE\n\
   or: etex [OPTION]... &FMT ARGS\n\
  Run e-TeX on TEXNAME, usually creating TEXNAME.dvi.\n\
  Any remaining COMMANDS are processed as e-TeX input, after TEXNAME is read.\n\
  If the first line of TEXNAME is %&FMT, and FMT is an existing .efmt file,\n\
  use it; %&ini means einitex.  Else use `NAME.efmt', where NAME is the program\n\
  invocation name, most commonly `etex'.\n\
\n\
  Alternatively, if the first non-option argument begins with a backslash,\n\
  interpret all non-option arguments as a line of e-TeX input.\n\
\n\
  Alternatively, if the first non-option argument begins with a &, the\n\
  next word is taken as the FMT to read, overriding all else.  Any\n\
  remaining arguments are processed as above.\n\
\n\
  If no arguments or options are specified, prompt for input.\n\
\n\
-efmt=FMTNAME            use FMTNAME instead of program name or a %& line\n\
-ini                     be einitex, for dumping formats; this is implicitly\n\
                          true if the program name is `einitex'\n\
-interaction=STRING      set interaction mode (STRING=batchmode/nonstopmode/\n\
                          scrollmode/errorstopmode)\n\
-kpathsea-debug=NUMBER   set path searching debugging flags according to\n\
                          the bits of NUMBER\n\
[-no]-mktex=FMT          disable/enable mktexFMT generation (FMT=tex/tfm)\n\
-mltex                   enable MLTeX extensions such as \\charsubdef\n\
-output-comment=STRING   use STRING for DVI file comment instead of date\n\
-progname=STRING         set program (and fmt) name to STRING\n\
-shell-escape            enable \\write18{SHELL COMMAND}\n\
-help                    display this help and exit\n\
-version                 output version information and exit\n"
#endif /* eTeX */

#ifdef GFTODVI
#define GFTODVIHELP \
"Usage: gftodvi [OPTION]... GFNAME\n\
  Translate each character in GFNAME to a page in a DVI file,\n\
  which is named with the basename of GFNAME extended with `.dvi'.\n\
\n\
-overflow-label-offset=REAL  override 2.1in offset for overflow labels\n\
-help                        display this help and exit\n\
-verbose                     display progress reports\n\
-version                     output version information and exit\n"
#endif /* GFTODVI */

#ifdef GFTOPK
#define GFTOPKHELP \
"Usage: gftopk [OPTION]... GFNAME [PKFILE]\n\
  Translate the bitmap font GFNAME to PKFILE.\n\
  Default PKFILE is basename of GFNAME extended with `pk'.\n\
\n\
-help       display this help and exit\n\
-verbose    display progress reports\n\
-version    output version information and exit\n"
#endif /* GFTOPK */

#ifdef GFTYPE
#define GFTYPEHELP \
"Usage: gftype [OPTION]... GFNAME\n\
  Verify and translate the bitmap font GFNAME to human-readable form,\n\
  written to standard output.\n\
\n\
-images       show characters as pixels\n\
-mnemonics    translate all GF commands\n\
-help         display this help and exit\n\
-version      output version information and exit\n"
#endif /* GFTYPE */

#ifdef MF
#define MFHELP \
"Usage: mf [OPTION]... [MFNAME[.mf]] [COMMANDS]\n\
   or: mf [OPTION]... \\FIRST-LINE\n\
   or: mf [OPTION]... &BASE ARGS\n\
  Run Metafont on MFNAME, usually creating MFNAME.tfm and MFNAME.NNNNgf,\n\
  where NNNN is the resolution of the specified mode (2602 by default).\n\
  Any following COMMANDS are processed as Metafont input,\n\
  after MFNAME is read.\n\
  If the first line of MFNAME is %&BASE, and BASE is an existing .base file,\n\
  use it; %&ini means inimf.  Else use `NAME.base', where NAME is the program\n\
  invocation name, most commonly `mf'.\n\
\n\
  Alternatively, if the first non-option argument begins with a backslash,\n\
  interpret all non-option arguments as a line of Metafont input.\n\
\n\
  Alternatively, if the first non-option argument begins with a &, the\n\
  next word is taken as the BASE to read, overriding all else. Any\n\
  remaining arguments are processed as above.\n\
\n\
  If no arguments or options are specified, prompt for input.\n\
\n\
-base=BASENAME           use BASENAME instead of program name or a %& line\n\
-ini                     be inimf, for dumping bases; this is implicitly\n\
                          true if the program name is `inimf'\n\
-interaction=STRING      set interaction mode (STRING=batchmode/nonstopmode/\n\
                          scrollmode/errorstopmode)\n\
-kpathsea-debug=NUMBER   set path searching debugging flags according to\n\
                          the bits of NUMBER\n\
[-no]-mktex=FMT          disable/enable mktexFMT generation (FMT=mf)\n\
-progname=STRING         set program (and base) name to STRING\n\
-help                    display this help and exit\n\
-version                 output version information and exit\n"
#endif /* MF */

#ifdef MFT
#define MFTHELP \
"Usage: mft [OPTION]... MFNAME[.mf]\n\
  Translate MFNAME to TeX for printing, using the mftmac.tex macros.\n\
  Output goes to basename of MFNAME extended with `.tex'.\n\
\n\
-change=CHFILE  apply the change file CHFILE as with tangle and weave\n\
-style=MFTNAME  use MFTNAME instead of plain.mft\n\
-help           display this help and exit\n\
-version        output version information and exit\n"
#endif /* MFT */

#ifdef MP
#define MPHELP \
"Usage: mpost [OPTION]... [MPNAME[.mp]] [COMMANDS]\n\
   or: mpost [OPTION]... \\FIRST-LINE\n\
   or: mpost [OPTION]... &MEM ARGS\n\
  Run MetaPost on MPNAME, usually creating MPNAME.NNN (and perhaps\n\
  MPNAME.tfm), where NNN are the character numbers generated.\n\
  Any remaining COMMANDS are processed as MetaPost input,\n\
  after MPNAME is read.\n\
  If the first line of MPNAME is %&MEM, and MEM is an existing .mem file,\n\
  use it; %&ini means inimp.  Else use `NAME.mem', where NAME is the program\n\
  invocation name, most commonly `mp'.\n\
\n\
  Alternatively, if the first non-option argument begins with a backslash,\n\
  interpret all non-option arguments as a line of MetaPost input.\n\
\n\
  Alternatively, if the first non-option argument begins with a &, the\n\
  next word is taken as the MEM to read, overriding all else.  Any\n\
  remaining arguments are processed as above.\n\
\n\
  If no arguments or options are specified, prompt for input.\n\
\n\
-ini                     be inimpost, for dumping mems; this is implicitly\n\
                          true if the program name is `inimpost'\n\
-interaction=STRING      set interaction mode (STRING=batchmode/nonstopmode/\n\
                          scrollmode/errorstopmode)\n\
-kpathsea-debug=NUMBER   set path searching debugging flags according to\n\
                          the bits of NUMBER\n\
-mem=MEMNAME             use MEMNAME instead of program name or a %& line\n\
-progname=STRING         set program (and mem) name to STRING\n\
-T, -troff               set the prologues variable, use `makempx -troff'\n\
-help                    display this help and exit\n\
-version                 output version information and exit\n"
#endif /* MP */

#ifdef ODVICOPY
#define ODVICOPYHELP \
"Usage: odvicopy [OPTION]... [INDVI[.dvi] [OUTDVI[.dvi]]]\n\
  Expand virtual font references in INDVI to OUTDVI.\n\
  Defaults are standard input and standard output, respectively.\n\
\n\
-magnification=NUMBER  override existing magnification with NUMBER\n\
-max-pages=NUMBER      process NUMBER pages; default one million\n\
-page-start=PAGE-SPEC  start at PAGE-SPEC, for example `2' or `5.*.-2'\n\
-help                  display this help and exit\n\
-version               output version information and exit\n"
#endif /* ODVICOPY */

#ifdef ODVITYPE
#define ODVITYPEHELP \
"Usage: odvitype [OPTION]... DVIFILE[.dvi]\n\
  Verify and translate DVIFILE to human-readable form,\n\
  written to standard output.\n\
\n\
-dpi=REAL              set resolution to REAL pixels per inch; default 300.0\n\
-magnification=NUMBER  override existing magnification with NUMBER\n\
-max-pages=NUMBER      process NUMBER pages; default one million\n\
-output-level=NUMBER   verbosity level, from 0 to 4; default 4\n\
-page-start=PAGE-SPEC  start at PAGE-SPEC, for example `2' or `5.*.-2'\n\
-show-opcodes          show numeric opcodes (in decimal)\n\
-help                  display this help and exit\n\
-version               output version information and exit\n"
#endif /* ODVITYPE */

#ifdef OFM2OPL
#define OFM2OPLHELP \
"Usage: ofm2opl [OPTION]... OFMNAME[.ofm] [OPLFILE[.opl]]\n\
  Translate the font metrics OFMNAME to human-readable property list file\n\
  OPLFILE or standard output.\n\
\n\
-charcode-format=TYPE  output character codes according to TYPE,\n\
                        either `hex' or `ascii'; default is hex,\n\
                        ascii = ascii letters and digits, hex for all else\n\
-help                  display this help and exit\n\
-verbose               display progress reports\n\
-version               output version information and exit\n"
#endif /* OFM2OPL */

#ifdef Omega
#define OMEGAHELP \
"Usage: omega [OPTION]... [TEXNAME[.tex]] [COMMANDS]\n\
   or: omega [OPTION]... \\FIRST-LINE\n\
   or: omega [OPTION]... &FMT ARGS\n\
  Run Omega on TEXNAME, usually creating TEXNAME.dvi.\n\
  Any remaining COMMANDS are processed as Omega input, after TEXNAME is read.\n\
  If the first line of TEXNAME is %&FMT, and FMT is an existing .fmt file,\n\
  use it; %&ini means iniomega.  Else use `NAME.fmt', where NAME is the\n\
  program invocation name, most commonly `omega'.\n\
\n\
  Alternatively, if the first non-option argument begins with a backslash,\n\
  interpret all non-option arguments as a line of Omega input.\n\
\n\
  Alternatively, if the first non-option argument begins with a &, the\n\
  next word is taken as the FMT to read, overriding all else.  Any\n\
  remaining arguments are processed as above.\n\
\n\
  If no arguments or options are specified, prompt for input.\n\
\n\
-fmt=FMTNAME             use FMTNAME instead of program name or a %& line\n\
-ini                     be iniomega, for dumping formats; this is implicitly\n\
                          true if the program name is `iniomega'\n\
-interaction=STRING      set interaction mode (STRING=batchmode/nonstopmode/\n\
                          scrollmode/errorstopmode)\n\
-kpathsea-debug=NUMBER   set path searching debugging flags according to\n\
                          the bits of NUMBER\n\
[-no]-mktex=FMT          disable/enable mktexFMT generation (FMT=tex/tfm)\n\
-output-comment=STRING   use STRING for DVI file comment instead of date\n\
-progname=STRING         set program (and fmt) name to STRING\n\
-shell-escape            enable \\write18{SHELL COMMAND}\n\
-help                    display this help and exit\n\
-version                 output version information and exit\n"
#endif /* Omega */

#ifdef OPL2OFM
#define OPL2OFMHELP \
"Usage: opl2ofm [OPTION]... OPLFILE[.opl] [OFMFILE[.ofm]]\n\
  Translate the property list OPLFILE to OFMFILE.\n\
  Default OFMFILE is basename of OPLFILE extended with `.ofm'.\n\
\n\
-help       display this help and exit\n\
-verbose    display progress reports\n\
-version    output version information and exit\n"
#endif /* OPL2OFM */

#if defined (OTANGLE) || defined (OTANGLEBOOT)
#define OTANGLEHELP \
"Usage: otangle [OPTION]... WEBFILE[.web] [CHANGEFILE[.ch]]\n\
  Tangle WEBFILE with CHANGEFILE into a Pascal program.\n\
  Default CHANGEFILE is /dev/null;\n\
  Pascal output goes to the basename of WEBFILE extended with `.p',\n\
  and a string pool file, if necessary, to the same extended with `.pool'.\n\
\n\
-help       display this help and exit\n\
-version    output version information and exit\n"
#endif /* OTANGLE */

#ifdef OVF2OVP
#define OVF2OVPHELP \
"Usage: ovf2ovp [OPTION]... OVFNAME[.ovf] [OFMNAME[.ofm] [OVPFILE[.ovp]]]\n\
  Translate OVFNAME and companion OFMNAME to human-readable\n\
  virtual property list file OVPFILE or standard output.\n\
  If OFMNAME is not specified, OVFNAME (with `.ovf' removed) is used.\n\
\n\
-charcode-format=TYPE  output character codes according to TYPE,\n\
                        either `hex' or `ascii'; default is hex,\n\
                        ascii = ascii letters and digits, hex for all else\n\
-help                  display this help and exit\n\
-verbose               display progress reports\n\
-version               output version information and exit\n"
#endif /* OVF2OVP */

#ifdef OVP2OVF
#define OVP2OVFHELP \
"Usage: ovp2ovf [OPTION]... OVPFILE[.ovp] [OVFFILE[.ovf] [OFMFILE[.ofm]]]\n\
  Translate OVPFILE to OVFFILE and companion OFMFILE.\n\
  Default OVFFILE is basename of OVPFILE extended with `.ovf'.\n\
  Default OFMFILE is OVFFILE extended with `.ofm'.\n\
\n\
-help                  display this help and exit\n\
-verbose               display progress reports\n\
-version               output version information and exit\n"
#endif /* OVP2OVF */

#ifdef PATGEN
#define PATGENHELP \
"Usage: patgen [OPTION]... DICTIONARY PATTERNS OUTPUT TRANSLATE\n\
  Generate the OUTPUT hyphenation file for use with TeX\n\
  from the DICTIONARY, PATTERNS, and TRANSLATE files.\n\
\n\
-help           display this help and exit\n\
-version        output version information and exit\n"
#endif /* PATGEN */

#ifdef PDFTeX
#define PDFTEXHELP \
"Usage: pdftex [OPTION]... [TEXNAME[.tex]] [COMMANDS]\n\
   or: pdftex [OPTION]... \\FIRST-LINE\n\
   or: pdftex [OPTION]... &FMT ARGS\n\
  Run PDFTeX on TEXNAME, usually creating TEXNAME.pdf.\n\
  Any remaining COMMANDS are processed as PDFTeX input, after TEXNAME is read.\n\
  If the first line of TEXNAME is %&FMT, and FMT is an existing .fmt file,\n\
  use it; %&ini means inipdftex.  Else use `NAME.fmt', where NAME is the\n\
  program invocation name, most commonly `pdftex'.\n\
\n\
  Alternatively, if the first non-option argument begins with a backslash,\n\
  interpret all non-option arguments as a line of PDFTeX input.\n\
\n\
  Alternatively, if the first non-option argument begins with a &, the\n\
  next word is taken as the FMT to read, overriding all else.  Any\n\
  remaining arguments are processed as above.\n\
\n\
  If no arguments or options are specified, prompt for input.\n\
\n\
-fmt=FMTNAME             use FMTNAME instead of program name or a %& line\n\
-ini                     be pdfinitex, for dumping formats; this is implicitly\n\
                          true if the program name is `pdfinitex'\n\
-interaction=STRING      set interaction mode (STRING=batchmode/nonstopmode/\n\
                          scrollmode/errorstopmode)\n\
-kpathsea-debug=NUMBER   set path searching debugging flags according to\n\
                          the bits of NUMBER\n\
[-no]-mktex=FMT          disable/enable mktexFMT generation (FMT=tex/tfm)\n\
-output-comment=STRING   use STRING for PDF file comment instead of date\n\
-progname=STRING         set program (and fmt) name to STRING\n\
-shell-escape            enable \\write18{SHELL COMMAND}\n\
-help                    display this help and exit\n\
-version                 output version information and exit\n"
#endif /* PDFTeX */

#ifdef PKTOGF
#define PKTOGFHELP \
"Usage: pktogf [OPTION]... PKNAME [GFFILE]\n\
  Translate the bitmap font PKNAME to GFFILE.\n\
  Default GFFILE is basename of PKNAME extended with `gf'.\n\
\n\
-help       display this help and exit\n\
-verbose    display progress reports\n\
-version    output version information and exit\n"
#endif /* PKTOGF */

#ifdef PKTYPE
#define PKTYPEHELP \
"Usage: pktype [OPTION]... PKNAME\n\
  Verify and translate the bitmap font PKNAME to human-readable form,\n\
  written to standard output.\n\
\n\
-help       display this help and exit\n\
-version    output version information and exit\n"
#endif /* PKTYPE */

#ifdef PLTOTF
#define PLTOTFHELP \
"Usage: pltotf [OPTION]... PLFILE[.pl] [TFMFILE[.tfm]]\n\
  Translate the property list PLFILE to TFMFILE.\n\
  Default TFMFILE is basename of PLFILE extended with `.tfm'.\n\
\n\
-help       display this help and exit\n\
-verbose    display progress reports\n\
-version    output version information and exit\n"
#endif /* PLTOTF */

#ifdef POOLTYPE
#define POOLTYPEHELP \
"Usage: pooltype [OPTION]... POOLFILE[.pool]\n\
  Display the string number of each string in POOLFILE.\n\
\n\
-help       display this help and exit\n\
-version    output version information and exit\n"
#endif /* POOLTYPE */

#if defined (TANGLE) || defined (TANGLEBOOT)
#define TANGLEHELP \
"Usage: tangle [OPTION]... WEBFILE[.web] [CHANGEFILE[.ch]]\n\
  Tangle WEBFILE with CHANGEFILE into a Pascal program.\n\
  Default CHANGEFILE is /dev/null;\n\
  Pascal output goes to the basename of WEBFILE extended with `.p',\n\
  and a string pool file, if necessary, to the same extended with `.pool'.\n\
\n\
-help       display this help and exit\n\
-version    output version information and exit\n"
#endif /* TANGLE */

#ifdef TeX
#define TEXHELP \
"Usage: tex [OPTION]... [TEXNAME[.tex]] [COMMANDS]\n\
   or: tex [OPTION]... \\FIRST-LINE\n\
   or: tex [OPTION]... &FMT ARGS\n\
  Run TeX on TEXNAME, usually creating TEXNAME.dvi.\n\
  Any remaining COMMANDS are processed as TeX input, after TEXNAME is read.\n\
  If the first line of TEXNAME is %&FMT, and FMT is an existing .fmt file,\n\
  use it; %&ini means initex.  Else use `NAME.fmt', where NAME is the program\n\
  invocation name, most commonly `tex'.\n\
\n\
  Alternatively, if the first non-option argument begins with a backslash,\n\
  interpret all non-option arguments as a line of TeX input.\n\
\n\
  Alternatively, if the first non-option argument begins with a &, the\n\
  next word is taken as the FMT to read, overriding all else.  Any\n\
  remaining arguments are processed as above.\n\
\n\
  If no arguments or options are specified, prompt for input.\n\
\n\
-fmt=FMTNAME             use FMTNAME instead of program name or a %& line\n\
-ini                     be initex, for dumping formats; this is implicitly\n\
                          true if the program name is `initex'\n\
-interaction=STRING      set interaction mode (STRING=batchmode/nonstopmode/\n\
                          scrollmode/errorstopmode)\n\
-kpathsea-debug=NUMBER   set path searching debugging flags according to\n\
                          the bits of NUMBER\n\
[-no]-mktex=FMT          disable/enable mktexFMT generation (FMT=tex/tfm)\n\
-mltex                   enable MLTeX extensions such as \\charsubdef\n\
-output-comment=STRING   use STRING for DVI file comment instead of date\n\
-progname=STRING         set program (and fmt) name to STRING\n\
-shell-escape            enable \\write18{SHELL COMMAND}\n\
-help                    display this help and exit\n\
-version                 output version information and exit\n"
#ifdef IPC
#define TEX_IPC_HELP \
"-ipc                     send DVI output to a socket as well as the usual\n\
                          output file\n\
-ipc-start               as -ipc, and also start the server at the other end\n"
#endif /* IPC */
#endif /* TeX */
/* TCX files are probably a bad idea.
-translate-file=TCXFILE  use TCXFILE for printable chars and translations\n\
*/

#ifdef TFTOPL
#define TFTOPLHELP \
"Usage: tftopl [OPTION]... TFMNAME[.tfm] [PLFILE[.pl]]\n\
  Translate the font metrics TFMNAME to human-readable property list file\n\
  PLFILE or standard output.\n\
\n\
-charcode-format=TYPE  output character codes according to TYPE,\n\
                        either `octal' or `ascii'; default is ascii for\n\
                        letters and digits, octal for all else\n\
-help                  display this help and exit\n\
-verbose               display progress reports\n\
-version               output version information and exit\n"
#endif /* TFTOPL */

#ifdef VFTOVP
#define VFTOVPHELP \
"Usage: vftovp [OPTION]... VFNAME[.vf] [TFMNAME[.tfm] [VPLFILE[.vpl]]]\n\
  Translate VFNAME and companion TFMNAME to human-readable\n\
  virtual property list file VPLFILE or standard output.\n\
  If TFMNAME is not specified, VFNAME (with `.vf' removed) is used.\n\
\n\
-charcode-format=TYPE  output character codes according to TYPE,\n\
                       either `octal' or `ascii'; default is ascii for\n\
                        letters and digits, octal for all else\n\
-help                   display this help and exit\n\
-verbose               display progress reports\n\
-version               output version information and exit\n"
#endif /* VFTOVP */

#ifdef VPTOVF
#define VPTOVFHELP \
"Usage: vptovf [OPTION]... VPLFILE[.vpl] [VFFILE[.vf] [TFMFILE[.tfm]]]\n\
  Translate VPLFILE to VFFILE and companion TFMFILE.\n\
  Default VFFILE is basename of VPLFILE extended with `.vf'.\n\
  Default TFMFILE is VFFILE extended with `.tfm'.\n\
\n\
-help                  display this help and exit\n\
-verbose               display progress reports\n\
-version               output version information and exit\n"
#endif /* VPTOVF */

#ifdef WEAVE
#define WEAVEHELP \
"Usage: weave [OPTION]... WEBFILE[.web] [CHANGEFILE[.ch]]\n\
  Weave WEBFILE with CHANGEFILE into a TeX document.\n\
  Default CHANGEFILE is /dev/null;\n\
  TeX output goes to the basename of WEBFILE extended with `.tex'.\n\
\n\
-x          omit cross-reference information\n\
-help       display this help and exit\n\
-version    output version information and exit\n"
#endif /* WEAVE */

#endif /* not HELP_H */
