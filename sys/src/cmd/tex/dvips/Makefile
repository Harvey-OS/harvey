# (This is only used on our Unix systems;  Plan 9 uses mkfile)
#   Makefile for dvips.  Edit this first part of the file.
#
#   First, the things that absolutely must be edited for your system.
#   Please, don't forget to edit MakeTeXPK in the same way!

# where TeX is `rooted'.  Sometimes /usr/local/lib/tex.
# TEXDIR = /usr/lib/tex
TEXDIR = /usr/lib/tex

# another place pk, tfm, and vf files might be found.
# LOCALDIR = /LocalLibrary/Fonts/TeXFonts
LOCALDIR = /LocalLibrary/Fonts/TeXFonts

# the default path to search for TFM files 
# (this usually is identical to TeX's defaultfontpath, which omits `.')
# (private fonts are given an explicit directory, which overrides the path)
#   overridden by the environment variable TEXFONTS
# TFMPATH = $(LOCALDIR)/tfm:$(TEXDIR)/fonts/tfm
TFMPATH = .:$(TEXDIR)/fonts/tfm

# the default path to search for PK files (usually omits `.')
# Don't forget to add the directory that
# MakeTeXPK puts the files!  (In this case, /LocalLibrary/Fonts...)
#   overridden by the environment variable TEXPKS or TEXPACKED or PKFONTS
# PKPATH = $(LOCALDIR)/pk:$(TEXDIR)/fonts/pk
PKPATH = .:$(TEXDIR)/fonts/canonpk

# the default path to search for VF files (usually omits `.')
#   overridden by the environment variable VFFONTS
# VFPATH = $(LOCALDIR)/vf:$(TEXDIR)/fonts/vf
VFPATH = .:$(TEXDIR)/fonts/psvf

# additional directories in which to search for subdirectories to find
# both tfm and pk files
FONTSUBDIRPATH = 

# where the config files go
# CONFIGDIR = $(TEXDIR)/ps
CONFIGDIR = $(TEXDIR)/ps

# the default path to search for config files
#   overridden by the environment variable TEXCONFIG
# CONFIGPATH = .:$(CONFIGDIR)
CONFIGPATH = .:$(CONFIGDIR)

# the name of your config file
# CONFIGFILE = config.ps
CONFIGFILE = config.ps

# where the header PS files go
# HEADERDIR = $(TEXDIR)/ps
HEADERDIR = $(TEXDIR)/ps

# the default path to search for header files
# HEADERPATH = .:$(HEADERDIR)
HEADERPATH = .:$(HEADERDIR)

# where epsf.tex and rotate.tex go (usually the TeX macros directory)
# TEXMACRODIR = $(TEXDIR)/inputs
TEXMACRODIR = $(TEXDIR)/macros

# the default path to search for epsf and psfiles
# (usually the same as TeX's defaultinputpath)
# FIGPATH = .:..:$(TEXDIR)/inputs
FIGPATH = .:..:$(TEXDIR)/macros

# the place man pages are located.
MANDIR = /usr/man/man1

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
# (RESEARCH) -DRESEARCH for local hacks
DEFS= -DTPIC -DDEBUG -DDEFRES=300 -DRESEARCH
#
#   If you define FONTLIB, make sure to also give definitions to
#   FLIPATH and FLINAME.
#
#   If your compiler doesn't like void*, then add
#      -DVOID=char
#
DEFS= -DTPIC -DDEBUG -DDEFRES=400 -DCREATIONDATE

# either use
# OPT = -O -funsigned-char
# or
OPT = -g -Wall -funsigned-char

# libraries to include (-lm -lc on most systems)
#FLIBS= -lNeXT_s -lsys_s
FLIBS=

# If you are compiling dvips for suid or other privileges, you will
# definitely want to define the following symbol; else don't.
# SECURE = -DSECURE

# If you want EMTEX specials, define the following.
EMTEX = -DEMTEX

# for SYSV (and friends which use <string.h> instead of <strings.h>)
# define the c-compiler flag
# add -D_POSIX_SOURCE if you are POSIX (also define SYSV) (only relevant
# for subdir searching)
SYS = -DSYSV

# Define this to be whatever you use for installation.  If you don't have
# install, use a script that invokes copy and chmod's the files
# appropriately.
# INSTALL = install
INSTALL = install

# where the installed binary goes
# BINDIR = /usr/bin
BINDIR = /v/bin

PATHS = -DTFMPATH=\"$(TFMPATH)\" \
	-DPKPATH=\"$(PKPATH)\" \
	-DVFPATH=\"$(VFPATH)\" \
	-DHEADERPATH=\"$(HEADERPATH)\" \
	-DCONFIGPATH=\"$(CONFIGPATH)\" \
        -DCONFIGFILE=\"$(CONFIGFILE)\" \
	-DFONTSUBDIRPATH=\"$(FONTSUBDIRPATH)\" \
	-DFIGPATH=\"$(FIGPATH)\"

CFLAGS = $(DEFS) $(OPT) $(SYS) $(SECURE) $(EMTEX) $(DEFPFMT)

SRC = dospecial.c dviinput.c fontdef.c loadfont.c dvips.c tfmload.c \
	download.c prescan.c scanpage.c skippage.c output.c scalewidth.c \
	dosection.c dopage.c resident.c search.c unpack.c drawPS.c \
	header.c makefont.c repack.c virtualfont.c dpicheck.c finclude.c \
	pprescan.c papersiz.c flib.c color.c bbox.c emspecial.c

OBJ = dospecial.o dviinput.o fontdef.o loadfont.o dvips.o tfmload.o \
	download.o prescan.o scanpage.o skippage.o output.o scalewidth.o \
	dosection.o dopage.o resident.o search.o unpack.o drawPS.o \
	header.o makefont.o repack.o virtualfont.o dpicheck.o finclude.o \
	pprescan.o papersiz.o flib.o color.o bbox.o emspecial.o

all : afm2tfm dvips tex.pro texps.pro texc.pro special.pro finclude.pro \
      color.pro crop.pro

dvips : $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) $(LIBS) $(FLIBS) -o dvips

dvips.o: dvips.c
	$(CC) $(CFLAGS) $(PATHS) -c dvips.c

afm2tfm: afm2tfm.c
	$(CC) $(CFLAGS) -o afm2tfm afm2tfm.c $(LIBS) $(FLIBS)

$(OBJ) : dvips.h debug.h
flib.o resident.o dvips.o loadfont.o tfmload.o : paths.h

squeeze : squeeze.o
	$(CC) $(CFLAGS) squeeze.o -o squeeze $(LIBS) $(FLIBS)

tex.pro : tex.lpro squeeze
	./squeeze <tex.lpro > tex.pro

texc.pro: texc.lpro squeeze
	./squeeze <texc.lpro >texc.pro

texc.lpro: texc.script tex.lpro
	sh texc.script tex.lpro texc.lpro

texps.pro : texps.lpro squeeze
	./squeeze <texps.lpro >texps.pro

special.pro : special.lpro squeeze
	./squeeze <special.lpro >special.pro

finclude.pro : finclude.lpro squeeze
	./squeeze <finclude.lpro >finclude.pro

color.pro : color.lpro squeeze
	./squeeze <color.lpro >color.pro

crop.pro : crop.lpro squeeze
	./squeeze <crop.lpro >crop.pro

install : afm2tfm dvips MakeTeXPK \
	tex.pro texc.pro texps.pro special.pro finclude.pro color.pro \
	crop.pro config.ps psfonts.map epsf.tex epsf.sty rotate.tex \
	rotate.sty colordvi.tex colordvi.sty blackdvi.tex blackdvi.sty \
	dvips.tex dvipsmac.tex dvips.1 afm2tfm.1
	- mkdir $(BINDIR)
	- mkdir $(CONFIGDIR)
	- mkdir $(HEADERDIR)
	- mkdir $(MANDIR)
	- mkdir $(TEXMACRODIR)
	$(INSTALL) -c -m 755 afm2tfm $(BINDIR)/afm2tfm
	$(INSTALL) -c -m 755 dvips $(BINDIR)/dvips
	$(INSTALL) -c -m 755 MakeTeXPK $(BINDIR)/MakeTeXPK
	$(INSTALL) -c -m 644 tex.pro $(HEADERDIR)
	$(INSTALL) -c -m 644 texc.pro $(HEADERDIR)
	$(INSTALL) -c -m 644 texps.pro $(HEADERDIR)
	$(INSTALL) -c -m 644 special.pro $(HEADERDIR)
	$(INSTALL) -c -m 644 finclude.pro $(HEADERDIR)
	$(INSTALL) -c -m 644 color.pro $(HEADERDIR)
	$(INSTALL) -c -m 644 crop.pro $(HEADERDIR)
	$(INSTALL) -c -m 644 config.ps $(CONFIGDIR)/$(CONFIGFILE)
	$(INSTALL) -c -m 644 psfonts.map $(CONFIGDIR)
	$(INSTALL) -c -m 644 epsf.tex $(TEXMACRODIR)
	$(INSTALL) -c -m 644 epsf.sty $(TEXMACRODIR)
	$(INSTALL) -c -m 644 rotate.tex $(TEXMACRODIR)
	$(INSTALL) -c -m 644 rotate.sty $(TEXMACRODIR)
	$(INSTALL) -c -m 644 colordvi.sty $(TEXMACRODIR)
	$(INSTALL) -c -m 644 colordvi.tex $(TEXMACRODIR)
	$(INSTALL) -c -m 644 blackdvi.sty $(TEXMACRODIR)
	$(INSTALL) -c -m 644 blackdvi.tex $(TEXMACRODIR)
	$(INSTALL) -c -m 644 dvips.tex $(TEXMACRODIR)
	$(INSTALL) -c -m 644 dvipsmac.tex $(TEXMACRODIR)
	- $(INSTALL) -c -m 644 dvips.1 $(MANDIR)
	- $(INSTALL) -c -m 644 afm2tfm.1 $(MANDIR)

veryclean :
	rm -f *.o dvips squeeze afm2tfm texc.lpro *.pro *~ *.log *.dvi

clean :
	rm -f *.o squeeze afm2tfm *~ *.log *.dvi

lint :
	lint $(DEFS) $(PATHS) $(SRC)
	lint $(DEFS) squeeze.c
	lint $(DEFS) afm2tfm.c
