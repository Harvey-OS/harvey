#
#   Makefile for dvips.  Edit this first part of the file.
#
#   First, the things that absolutely must be edited for your system.

# modified for Borland MAKE 3.0 and Watcom 386 9.01 on MS-DOS
# by js@rphnw3.ngate.uni-regensburg.de
# 18th November 1992
# This version of dvips HAS NOT BEEN THOROUGHLY TESTED

# All DIRs must use \\ as a directory separator.
# All PATHs must use \\\\ as a directory separator

# the default path to search for TFM files
# (this usually is identical to TeX's defaultfontpath, which omits `.')
# (private fonts are given an explicit directory, which overrides the path)
#   overridden by the environment variable TEXFONTS
# TFMPATH = c:\\emtex\\tfm
TFMPATH = c:\\\\emtex\\\\tfm

# the default path to search for PK files (usually omits `.')
# Don't forget to add the directory that
# MakeTeXPK puts the files!  (In this case, /LocalLibrary/Fonts...)
#   overridden by the environment variable TEXPKS or TEXPACKED or PKFONTS
# PKPATH = c:\\texfonts\\pixel.lj\\%ddpi\\%f.%p
PKPATH = c:\\\\texfonts\\\\pixel.lj\\\\%ddpi\\\\%f.%p

# the default path to search for VF files (usually omits `.')
#   overridden by the environment variable VFFONTS
# VFPATH = c:\\texfonts\\vf
VFPATH = c:\\\\emtex\\\\vf

# additional directories in which to search for subdirectories to find
# both tfm and pk files
FONTSUBDIRPATH =

# where the config files go
# CONFIGDIR = c:\emtex\ps
CONFIGDIR = c:\\emtex\\ps

# the default path to search for config files
#   overridden by the environment variable TEXCONFIG
# CONFIGPATH = .;c:\\emtex\\ps
CONFIGPATH = .;c:\\\\emtex\\\\ps

# the name of your config file
# CONFIGFILE = config.ps
CONFIGFILE = config.ps

# where the header PS files go
# HEADERDIR = c:\emtex\ps
HEADERDIR = c:\\emtex\\ps

# the default path to search for header files
# HEADERPATH = .;c:\\emtex\\ps
HEADERPATH = .;c:\\\\emtex\\\\ps

# where epsf.tex and rotate.tex go (usually the TeX macros directory)
# TEXMACRODIR = c:\emtex\texinput
TEXMACROPARENTDIR = c:\\emtex\\texinput
TEXMACRODIR = c:\\emtex\\texinput\\dvips

# the default path to search for epsf and psfiles
# (usually the same as TeX's defaultinputpath)
# FIGPATH = .;..;c:\\emtex\\texinput
FIGPATH = .;..;c:\\\\emtex\\\\texinput

# the default path to search for emTeX font libraries
# FLIPATH = c:\\texfonts
FLIPATH = c:\\\\texfonts

# the names of emTeX font libraries
# FLINAME = lj_0;lj_h;lj_1;lj_2;lj_3;lj_4;lj_5a;lj_5b;lj_sli
FLINAME = lj_0;lj_h;lj_1;lj_2;lj_3;lj_4;lj_5;lj_sli

# change -DDEFRES=300 or whatever is required
#    if the default resolution is not 300 dpi,
# add -DDEBUG to turn on debugging capability
# add -DTPIC for tpic support
# add -DFONTLIB to search font libraries
# add -DSEARCH_SUBDIRECTORIES to search the FONTSUBDIRPATH.
# add -DHAVE_GETCWD if you have getcwd (relevant only for subdir searching)
# add -DIBM6000 for compiling on IBM 6000 systems
# add -DCREATIONDATE if your system has a working time() and you want dated files
# (for VM/CMS, see the MKDVIPS.EXEC file in the vmcms subdirectory).
#
#   If you define FONTLIB, make sure to also give definitions to
#   FLIPATH and FLINAME.
#
DEFS= -DTPIC -DDEBUG -DDEFRES=300 -DFONTLIB -DHAVE_GETCWD

# your compiler
CC = wcl386

# either use
# OPT = -g -Wall -funsigned-char
# or
# OPT = -O -s -Wall -funsigned-char
OPT = /fpi/om/ox/p

# libraries to include (-lm -lc on most systems)
#FLIBS= -lNeXT_s -lsys_s
#FLIBS= -lm -lc
FLIBS=

# If you are compiling dvips for suid or other privileges, you will
# definitely want to define the following symbol; else don't.
# SECURE = -DSECURE
SECURE =

# If you want EMTEX specials, define the following.
EMTEX = -DEMTEX

# for SYSV (and friends which use <string.h> instead of <strings.h>)
# define the c-compiler flag
# add -D_POSIX_SOURCE if you are POSIX (also define SYSV) (only relevant
# for subdir searching)
# add -DMSDOS for MS-DOS systems
# add -DDJGPP for MS-DOS GNU's
SYS = -DMSDOS -DDJGPP


# where the installed binary goes
# BINDIR = c:\emtex
BINDIR = c:\emtex

# where the manual page goes
# MANDIR = c:\emtex\doc
MANDIR = c:\emtex\doc

PATHS = -DTFMPATH=\"$(TFMPATH)\" \
        -DPKPATH=\"$(PKPATH)\" \
        -DVFPATH=\"$(VFPATH)\" \
        -DHEADERPATH=\"$(HEADERPATH)\" \
        -DCONFIGPATH=\"$(CONFIGPATH)\" \
        -DCONFIGFILE=\"$(CONFIGFILE)\" \
        -DFONTSUBDIRPATH=\"$(FONTSUBDIRPATH)\" \
        -DFIGPATH=\"$(FIGPATH)\" \
        -DFLIPATH=\"$(FLIPATH)\" \
        -DFLINAME=\"$(FLINAME)\"

CFLAGS = $(DEFS) $(OPT) $(SYS) $(SECURE) $(EMTEX)

# default rules
.c.obj:
        $(CC) -c $(CFLAGS) $<

SRC = dospecia.c dviinput.c fontdef.c loadfont.c dvips.c tfmload.c \
        download.c prescan.c scanpage.c skippage.c output.c scalewid.c \
        dosectio.c dopage.c resident.c search.c unpack.c drawPS.c \
        header.c makefont.c repack.c virtualf.c dpicheck.c finclude.c \
        pprescan.c papersiz.c flib.c color.c bbox.c emspecial.c

OBJ = dospecia.obj dviinput.obj fontdef.obj loadfont.obj dvips.obj \
        tfmload.obj download.obj prescan.obj scanpage.obj skippage.obj \
        output.obj scalewid.obj dosectio.obj dopage.obj resident.obj \
        search.obj unpack.obj drawPS.obj header.obj makefont.obj repack.obj \
        virtualf.obj dpicheck.obj finclude.obj \
        pprescan.obj papersiz.obj flib.obj color.obj bbox.obj emspecial.obj

all : afm2tfm.exe dvips4gl.exe dvipsos2.exe tex.pro texps.pro texc.pro \
   special.pro finclude.pro color.pro crop.pro

dvips4gl.exe: $(OBJ) wat.lnk
        $(CC) /fe=$&.exe /l=dos4g @wat.lnk

dvipsos2.exe: $(OBJ) wat.lnk
        $(CC) /fe=$&.exe /l=os2v2 @wat.lnk

wat.lnk: pc\makefile.wat
        copy &&|
FILE dospecia.obj
FILE dviinput.obj
FILE fontdef.obj
FILE loadfont.obj
FILE dvips.obj
FILE tfmload.obj
FILE download.obj
FILE prescan.obj
FILE scanpage.obj
FILE skippage.obj
FILE output.obj
FILE scalewid.obj
FILE dosectio.obj
FILE dopage.obj
FILE resident.obj
FILE search.obj
FILE unpack.obj
FILE drawPS.obj
FILE header.obj
FILE makefont.obj
FILE repack.obj
FILE virtualf.obj
FILE dpicheck.obj
FILE finclude.obj
FILE pprescan.obj
FILE papersiz.obj
FILE flib.obj
FILE color.obj
FILE bbox.obj
FILE emspecial.obj
| wat.lnk

# you have to add the hash sign by hand !
win\wat.h:
        echo define TFMPATH        \"$(TFMPATH)\"        >>win\wat.h
        echo define PKPATH         \"$(PKPATH)\"         >>win\wat.h
        echo define VFPATH         \"$(VFPATH)\"         >>win\wat.h
        echo define HEADERPATH     \"$(HEADERPATH)\"     >>win\wat.h
        echo define CONFIGPATH     \"$(CONFIGPATH)\"     >>win\wat.h
        echo define CONFIGFILE     \"$(CONFIGFILE)\"     >>win\wat.h
        echo define FONTSUBDIRPATH \"$(FONTSUBDIRPATH)\" >>win\wat.h
        echo define FIGPATH        \"$(FIGPATH)\"        >>win\wat.h
        echo define FLIPATH        \"$(FLIPATH)\"        >>win\wat.h
        echo define FLINAME        \"$(FLINAME)\"        >>win\wat.h

dvips.obj: dvips.c
        $(CC) $(CFLAGS) -c dvips.c /fi=win\wat.h

afm2tfm.exe: afm2tfm.c
        $(CC) $(CFLAGS) afm2tfm.c $(LIBS) $(FLIBS)

#$(OBJ) : dvips.h debug.h paths.h

squeeze.exe : squeeze.c
        $(CC) $(CFLAGS) squeeze.c $(LIBS) $(FLIBS)

tex.pro : tex.lpr squeeze.exe
        squeeze tex.lpro  tex.pro

texc.pro: texc.lpr squeeze.exe
        squeeze texc.lpro texc.pro

texc.lpr: tex.lpr
        @echo This does "texc.scr tex.lpr texc.lpr" on Unix
        @echo For MSDOS, copy tex.lpr to texc.lpr
        @echo then edit texc.lpr to remove the code
        @echo for uncompressed fonts, and uncomment the
        @echo code for unpacking compressed fonts

texps.pro : texps.lpr squeeze.exe
        squeeze texps.lpro texps.pro

special.pro : special.lpr squeeze.exe
        squeeze special.lpr special.pro

finclude.pro: finclude.lpr squeeze.exe
        squeeze finclude.lpr finclude.pro

color.pro: color.lpr squeeze.exe
        squeeze color.lpr color.pro

crop.pro: crop.lpr squeeze.exe
        squeeze crop.lpr crop.pro

install : afm2tfm.exe dvips$(DEST).exe pc/maketexp.bat \
        tex.pro texc.pro texps.pro special.pro finclude.pro color.pro crop.pro \
        pc/config.ps psfonts.map epsf.tex epsf.sty rotate.tex rotate.sty \
        dvips.tex dvipsmac.tex colordvi.sty colordvi.tex blackdvi.sty \
        blackdvi.tex pc/dvips.doc pc/afm2tfm.doc
        mkdir $(BINDIR)
        mkdir $(HEADERDIR)
        mkdir $(CONFIGDIR)
        mkdir $(MANDIR)
	mkdir $(TEXMACROPARENTDIR)
        mkdir $(TEXMACRODIR)
        copy afm2tfm.exe $(BINDIR)\afm2tfm.exe
        copy dvips4gl.exe $(BINDIR)\dvips4gl.exe
        copy dvipsos2.exe $(BINDIR)\dvipsos2.exe
        copy pc\maketexp.bat $(BINDIR)\maketexp.bat
        copy tex.pro $(HEADERDIR)
        copy texc.pro $(HEADERDIR)
        copy texps.pro $(HEADERDIR)
        copy texps.pro $(HEADERDIR)
        copy special.pro $(HEADERDIR)
        copy finclude.pro $(HEADERDIR)
        copy color.pro $(HEADERDIR)
        copy crop.pro $(HEADERDIR)
        copy pc\config.ps $(CONFIGDIR)\$(CONFIGFILE)
        copy psfonts.map $(CONFIGDIR)
        copy epsf.tex $(TEXMACRODIR)
        copy epsf.sty $(TEXMACRODIR)
        copy rotate.tex $(TEXMACRODIR)
        copy rotate.sty $(TEXMACRODIR)
        copy colordvi.sty $(TEXMACRODIR)
        copy colordvi.tex $(TEXMACRODIR)
        copy blackdvi.sty $(TEXMACRODIR)
        copy blackdvi.tex $(TEXMACRODIR)
        copy dvips.tex $(TEXMACRODIR)
        copy dvipsmac.tex $(TEXMACRODIR)
        copy pc\dvips.doc $(MANDIR)
        copy pc\afm2tfm.doc $(MANDIR)

veryclean :
        del *.obj
        del dvips4gl.exe
        del dvipsos2.exe
        del squeeze.exe
        del afm2tfm.exe
        del *.pro
        del *.bak
        del *.log
        del wat.lnk

clean :
        del *.obj
        del squeeze.exe
        del *.bak
        del *.log
        del wat.lnk
