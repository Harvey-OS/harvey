#
#   Makefile for dvips.  Edit this first part of the file.
#
#   This version is for Vax/VMS running the Todd Aven MAKE program.
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
TFMPATH = ""TEX_FONTS:""	# TEX_FONTS is a searchlist.

# the default path to search for PK files (usually omits `.')
# Don't forget to add the directory that
# MakeTeXPK puts the files!  (In this case, /LocalLibrary/Fonts...)
#   overridden by the environment variable TEXPKS or TEXPACKED or PKFONTS
# PKPATH = $(LOCALDIR)/pk:$(TEXDIR)/fonts/pk
PKPATH = ""VC$3:[PHS595F.FONTS.%d]%f.PK,TEX_DISK:[TEX.FONTS.%d]%f.PK,""

# the default path to search for VF files (usually omits `.')
#   overridden by the environment variable VFFONTS
# VFPATH = $(LOCALDIR)/vf:$(TEXDIR)/fonts/vf
VFPATH = ""TEX_VF:""		# TEX_VF may be a searchlist.

# additional directories in which to search for subdirectories to find
# both tfm and pk files
FONTSUBDIRPATH = """"

# where the config files go
# CONFIGDIR = $(TEXDIR)/ps
CONFIGDIR = TEX$POSTSCRIPT:		# The Physics Dept. DVIPS directory

# the default path to search for config files
#   overridden by the environment variable TEXCONFIG
# CONFIGPATH = .:$(CONFIGDIR)
CONFIGPATH = "" ,SYS$LOGIN:,$(CONFIGDIR),""

# the name of your config file
# CONFIGFILE = config.ps
CONFIGFILE = config.ps

# where the header PS files go
# HEADERDIR = $(TEXDIR)/ps
HEADERDIR = TEX$POSTSCRIPT:		# The Physics Dept. DVIPS directory

# the default path to search for header files
# HEADERPATH = .:$(HEADERDIR)
HEADERPATH = "" ,SYS$LOGIN:,$(HEADERDIR),""

# where epsf.tex and rotate.tex go (usually the TeX macros directory)
# TEXMACRODIR = $(TEXDIR)/inputs
TEXMACRODIR = TEX_INPUTS:

# the default path to search for epsf and psfiles
# (usually the same as TeX's defaultinputpath)
# FIGPATH = .:..:$(TEXDIR)/inputs
FIGPATH = "" ,TEX_INPUTS:,""  # Include blank so it looks on default dir.

# the place man pages are located.
# MANDIR = /usr/man/man1
MANDIR = TEX_INPUTS:

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
# DEFS= -DTPIC -DDEBUG -DDEFRES=400
DEFS= TPIC,DEBUG,DEFRES=300,HAVE_GETCWD,ANSI,CREATIONDATE

# either use
# OPT = -g -Wall -funsigned-char
# or
# OPT = -O -s -Wall -funsigned-char
OPT = 

# libraries to include (-lm -lc on most systems)
#FLIBS= -lNeXT_s -lsys_s
FLIBS=,sys$disk:[.vms]vaxcrtl/opt
# LIBS= 				# VAXC
LIBS= ,gnu_cc:[000000]GCCLIB/LIB	# GNUC

# If you are compiling dvips for suid or other privileges, you will
# definitely want to define the following symbol; else don't.
# SECURE = -DSECURE
SECURE =

# If you want EMTEX specials, define the following.
EMTEX =,EMTEX

# for SYSV (and friends which use <string.h> instead of <strings.h>)
# define the c-compiler flag
# add -D_POSIX_SOURCE if you are POSIX (also define SYSV) (only relevant
# for subdir searching)
# SYS = -DSYSV
SYS =

# Define this to be whatever you use for installation.  If you don't have
# install, use a script that invokes copy and chmod's the files
# appropriately.
# INSTALL = install
INSTALL = copy

# where the installed binary goes
# BINDIR = /usr/bin
BINDIR = TEX$POSTSCRIPT:	# The Physics Dept. DVIPS directory

# Define the default paper format (Actually no longer used)
DEFPFMT =

# PATHS =\					# VAXC
#  ,TFMPATH="$(TFMPATH)",\
#  PKPATH="$(PKPATH)",\
#  VFPATH="$(VFPATH)",\
#  HEADERPATH="$(HEADERPATH)",\
#  CONFIGPATH="$(CONFIGPATH)",\
#  CONFIGFILE="""$(CONFIGFILE)""",\
#  FONTSUBDIRPATH="$(FONTSUBDIRPATH)",\
#  FIGPATH="$(FIGPATH)"
PATHS =						# GNUC

INC = /include="SYS$DISK:[],SYS$SHARE:"
# CC      = CC					# VAXC
CC      = GCC/NoCase_Hack			# GNUC
CFLAGS  = /NoList $(INC)-
 /Define=($(DEFS) $(OPT) $(SYS) $(SECURE) $(EMTEX) $(DEFPFMT))
CFLAGS2 = /NoList $(INC)-
 /Define=($(DEFS) $(OPT) $(SYS) $(SECURE) $(EMTEX) $(DEFPFMT) $(PATHS))
LFLAGS  = /NoMap

SRC = dospecial.c dviinput.c fontdef.c loadfont.c dvips.c tfmload.c \
	download.c prescan.c scanpage.c skippage.c output.c scalewidth.c \
	dosection.c dopage.c resident.c search.c unpack.c drawPS.c \
	header.c makefont.c repack.c virtualfont.c dpicheck.c finclude.c \
	pprescan.c papersiz.c flib.c color.c bbox.c emspecial.c

OBJ = dospecial.obj, dviinput.obj, fontdef.obj, loadfont.obj, dvips.obj,\
	tfmload.obj, download.obj, prescan.obj, scanpage.obj, skippage.obj,\
	output.obj, scalewidth.obj, dosection.obj, dopage.obj, resident.obj,\
	search.obj, unpack.obj, drawPS.obj, header.obj, makefont.obj,\
	repack.obj, virtualfont.obj, dpicheck.obj, finclude.obj, \
	pprescan.obj, papersiz.obj, flib.obj, color.obj, bbox.obj, \
	emspecial.obj, vaxvms.obj

all : afm2tfm.exe dvips.exe tex.pro texps.pro texc.pro special.pro \
	finclude.pro color.pro crop.pro dvips.hlb

dvips.exe : $(OBJ)
	Link /Exe=DviPS.exe $(LFLAGS) $(OBJ) $(LIBS) $(FLIBS)

# dvips.obj : dvips.c vmscli.c			# VAXC
dvips.obj : dvips.c vmscli.c vms_gcc_paths.h	# GNUC
	$(CC) $(CFLAGS2) dvips.c

afm2tfm.exe : afm2tfm.obj
	Link $(LFLAGS) afm2tfm.obj $(LIBS) $(FLIBS)

afm2tfm.obj : afm2tfm.c
	$(CC) $(CFLAGS) afm2tfm.c

$(OBJ) : dvips.h debug.h vms.h

flib.obj resident.obj dvips.obj loadfont.obj tfmload.obj : paths.h

vaxvms.obj : [.vms]vaxvms.c
! 	$(CC) $(CFLAGS) [.vms]vaxvms.c /object=[]vaxvms.obj

vmscli.c : [.vms]vmscli.c
	Copy /Log [.vms]vmscli.c []

vms.h : [.vms]vms.h
	Copy /Log [.vms]vms.h []

vms_gcc_paths.h : [.vms]vms_gcc_paths.h		# GNUC
	Copy /Log [.vms]vms_gcc_paths.h []	# GNUC

*.obj : *.c
	$(CC) $(CFLAGS) $<

squeeze.exe : squeeze.obj
	Link $(LFLAGS) squeeze.obj $(LIBS) $(FLIBS)

squeeze.obj : squeeze.c
	$(CC) $(CFLAGS) squeeze.c

tex.pro : tex.lpro squeeze.exe
	MCR sys$disk:[]squeeze tex.lpro tex.pro

texc.pro: texc.lpro squeeze.exe
	MCR sys$disk:[]squeeze texc.lpro texc.pro

texc.lpro: tex.lpro
	@[.vms]texc_scr.com # tex.lpro texc.lpro

texps.pro : texps.lpro squeeze.exe
	MCR sys$disk:[]squeeze texps.lpro texps.pro

special.pro : special.lpro squeeze.exe
	MCR sys$disk:[]squeeze special.lpro special.pro

finclude.pro : finclude.lpro squeeze.exe
	MCR sys$disk:[]squeeze finclude.lpro finclude.pro

color.pro : color.lpro squeeze.exe
	MCR sys$disk:[]squeeze color.lpro color.pro

crop.pro : crop.lpro squeeze.exe
	MCR sys$disk:[]squeeze crop.lpro crop.pro

dvips.hlb : dvips.hlp
	Library/Create=Blocks:40/Help dvips.hlb dvips.hlp

dvips.hlp : [.vms]dvips.rnh
	Runoff [.vms]dvips.rnh

install : afm2tfm.exe dvips.exe dvips.hlb [.vms]MakeTeXPK.com \
	tex.pro texc.pro texps.pro special.pro finclude.pro color.pro \
	crop.pro config.ps psfonts.map epsf.tex epsf.sty rotate.tex \
	rotate.sty colordvi.tex colordvi.sty blackdvi.tex blackdvi.sty
# 	- mkdir $(BINDIR)
# 	- mkdir $(HEADERDIR)
# 	- mkdir $(CONFIGDIR)
# 	- mkdir $(MANDIR)
# 	- mkdir $(TEXMACRODIR)
	$(INSTALL) afm2tfm.exe $(BINDIR)afm2tfm.exe
	$(INSTALL) dvips.exe $(BINDIR)dvips.exe
	$(INSTALL) dvips.hlb $(BINDIR)dvips.hlb
#	$(INSTALL) [.VMS]MakeTeXPK.COM $(BINDIR)MakeTeXPK.COM
	$(INSTALL) tex.pro $(HEADERDIR)
	$(INSTALL) texc.pro $(HEADERDIR)
	$(INSTALL) texps.pro $(HEADERDIR)
	$(INSTALL) special.pro $(HEADERDIR)
	$(INSTALL) finclude.pro $(HEADERDIR)
	$(INSTALL) color.pro $(HEADERDIR)
	$(INSTALL) crop.pro $(HEADERDIR)
	$(INSTALL) config.ps $(CONFIGDIR)$(CONFIGFILE)
	$(INSTALL) psfonts.map $(CONFIGDIR)
	$(INSTALL) epsf.tex $(TEXMACRODIR)
	$(INSTALL) epsf.sty $(TEXMACRODIR)
	$(INSTALL) rotate.tex $(TEXMACRODIR)
	$(INSTALL) rotate.sty $(TEXMACRODIR)
	$(INSTALL) colordvi.sty $(TEXMACRODIR)
	$(INSTALL) colordvi.tex $(TEXMACRODIR)
	$(INSTALL) blackdvi.sty $(TEXMACRODIR)
	$(INSTALL) blackdvi.tex $(TEXMACRODIR)
	$(INSTALL) dvips.tex $(TEXMACRODIR)
	$(INSTALL) dvipsmac.tex $(TEXMACRODIR)
	$(INSTALL) dvips.1 $(MANDIR)
	$(INSTALL) afm2tfm.1 $(MANDIR)

veryclean :
	Delete *.obj;*, dvips.exe;*, squeeze.exe;*, afm2tfm.exe;*,\
	texc.lpro;*, *.pro;*, *.log;*, *.dvi;*

clean :
	Delete *.obj;*, squeeze.exe;*, afm2tfm.exe;*, *.log;*, *.dvi;*

# lint :
# 	lint $(DEFS) $(PATHS) $(SRC)
# 	lint $(DEFS) squeeze.c
# 	lint $(DEFS) afm2tfm.c
