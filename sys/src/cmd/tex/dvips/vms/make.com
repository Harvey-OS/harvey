$ !
$ !     dvips for VAX/VMS
$ !
$ !
$ !     Use to compile and link dvips with VAXC under VMS.  Before using
$ !     this command file you must set the following definitions according
$ !     to your own environment;
$ !
$ !     TFMPATH         - The directory where TFM files live ( needed for
$ !                       the compilation of dvips.c ).  Be sure and include
$ !                       the needed directory separator in the path ( i.e.
$ !                       TEX_FONTS: )
$ !     PKPATH          - The directory where PK files live ( needed for the
$ !                       compilation of dvips.c ).  You should also decided
$ !                       whether you need VMS_ROOTED ( needed for the
$ !                       compilation of loadfont.c ) defined so that
$ !                       PKPATH will be interpreted as a rooted directory
$ !                       under VMS.  Be sure and include the needed directory
$ !                       separator in the path ( i.e. TEX_FONTS: ).
$ !     HEADERPATH      - The directory where the PostScript prologue file
$ !                       live ( needed for the compilation of output.c &
$ !                       dvips.c).  Be sure and include a trailing comma in
$ !                       your definition of the header path as dvips uses this
$ !                       definition as a path for both PostScript prologue
$ !                       files and files which are included with \special
$ !                       options.  This definition should be a comma
$ !                       separated list of directories where dvips will
$ !                       look for a specified file.  As an example;
$ !
$ !                       "HEADERPATH=""TEX_POSTSCRIPT:,SYS$LOGIN:,"""
$ !
$ !                       to look first in TEX_POSTSCRIPT:, then in SYS$LOGIN:,
$ !                       and finally in the current default directory.
$ !     CONFIGPATH      - The directory where the configuration file lives
$ !                       ( needed for the compilation of resident.c ).  Be
$ !                       sure and include the needed directory separator
$ !                       in the path ( i.e. TEX_POSTSCRIPT: ).
$ !
$ !     FIGPATH         - Where the .ps and .eps files are
$ !
$ !     After dvips has been compiled and linked the user will be given the
$ !     option of compressing the PostScript prologue files which are used
$ !     by dvips and copying the resultant prologue files to the specifed
$ !     HEADERPATH.  The user will also be given the option of copying the
$ !     dvips image file to the area where TeX related images are stored.
$ !     Answering "yes" to either of these options should be done only if
$ !     the user has write priviledges to the directories which are specified.
$ !
$ !     Command file by: Robert Schneider
$ !                      Department of Petroleum Engineering
$ !                      The University of Texas at Austin
$ !
$ !                      October 1989
$ !
$ !     Updates by:      Earle Ake
$ !                      Science Applications International Corporation
$ !                      Dayton, Ohio
$ !                      ake@dayton.saic.com
$ !
$ !                      Updated for DVIPS 5.34
$ !                      August 1990
$ !
$ !                      Ted Nieland
$ !                      Control Data Corporation
$ !                      DECUS TeX Collection Editor
$ !                      ted@nieland.dayton.oh.us
$ !
$ !                      Updated for DVIPS 5.47, February 1991
$ !
$ !                      Updated for DVIPS 5.474, March 1992
$ !                      Earle F. Ake
$ !
$!                       Updated for DVIPS 5.493, Sept 1992
$!                       Added HAVE_GETCWD,ANSI in CC for
$!                       dvips.c and resident.c
$!                       Max Calvani
$!                       calvani@astrpd.astro.it
$!
$!                       Updated for DVIPS 5.495, Oct. 1992
$!                       calvani@astrpd.astro.it
$!
$!                       Updated for DVIPS 5.499, Jan. 1993
$!                       Added FONTLIB support
$!                       calvani@astrpd.astro.it
$!
$ on error then goto bad_exit
$ on severe_error then goto bad_exit
$ !
$ TFMPATH = "TEX_FONTS:"
$ PKPATH = "TEX_PKDIR:"
$ VFPATH = "TEX_VFDIR:"
$ HEADERPATH = "DVI_INPUTS:,TEX_INPUTS:,SYS$LOGIN:,SYS$DISK:[],"
$ PLACEHEADER_DIR = "TEX_DISK:[TEX.INPUTS.DVIDRIVERS]"
$ CONFIGPATH = "TEX_INPUTS:"
$ FIGPATH = "TEX_INPUTS:,SYS$DISK:"
$ TPIC = ",TPIC"
$ EMTEX = ",EMTEX"
$ FONTLIB = ",FONTLIB"
$ VMS_ROOTED = ",VMS_ROOTED"
$ TEXEXEPATH = "TEX_DISK:[TEX.EXE]"
$ DEBUG = ",DEBUG"
$ !
$ write sys$output " "
$ inquire/nop ANSWER "Have you read the file VMS_INSTALL.TXT [no]?  "
$ if ANSWER .eqs. "" then ANSWER = 0
$ if ANSWER then goto read_instructions
$   write sys$output " "
$   write sys$output "Please read the VMS_INSTALL.TXT file,  edit this command"
$   write sys$output "file if necessary, and then execute this file again."
$   write sys$output " "
$   exit
$ read_instructions:
$ write sys$output " "
$ inquire/nop ANSWER "Compile sources [no]?  "
$ if ANSWER .eqs. "" then ANSWER = 0
$ if .not. ANSWER then goto linkstep
$ !
$ get_definitions:
$ write sys$output " "
$ !
$ inquire/nop ANSWER "TFM path [''TFMPATH']?  "
$ if ANSWER .nes. "" then TFMPATH = ANSWER
$ inquire/nop ANSWER "PK path [''PKPATH']?  "
$ if ANSWER .nes. "" then PKPATH = ANSWER
$ inquire/nop ANSWER "VF path [''VFPATH']?  "
$ if ANSWER .nes. "" then VFPATH = ANSWER
$ inquire/nop ANSWER "PostScript HEADER path [''HEADERPATH']?  "
$ if ANSWER .nes. "" then HEADERPATH = ANSWER
$ inquire/nop ANSWER "PostScript CONFIG path [''CONFIGPATH'])?  "
$ if ANSWER .nes. "" then CONFIGPATH = ANSWER
$ inquire/nop ANSWER "FIG path [''FIGPATH'])?  "
$ if ANSWER .nes. "" then FIGPATH = ANSWER
$ inquire/nop ANSWER "Do you want TPIC support [yes]?  "
$ if ANSWER .eqs. "" then ANSWER = 1
$ if .not. ANSWER then TPIC = ""
$ inquire/nop ANSWER "Do you want EMTEX support [yes]?  "
$ if ANSWER .eqs. "" then ANSWER = 1
$ if .not. ANSWER then EMTEX = ""
$ inquire/nop ANSWER "Do you want FONTLIB support [yes]?  "
$ if ANSWER .eqs. "" then ANSWER = 1
$ if .not. ANSWER then FONTLIB = ""
$ inquire/nop ANSWER "Do you want DEBUG support [yes]?  "
$ if ANSWER .eqs. "" then ANSWER = 1
$ if .not. ANSWER then DEBUG = ""
$ inquire/nop ANSWER "Is ''PKPATH' a rooted directory [yes]?   "
$ if ANSWER .eqs. "" then ANSWER = 1
$ if .not. ANSWER then VMS_ROOTED = ""
$ write sys$output " "
$ write sys$output "dvips will be compiled with the following definitions;"
$ write sys$output " "
$ write sys$output "TFMPATH = ",TFMPATH
$ write sys$output "PKPATH = ",PKPATH
$ write sys$output "VFPATH = ",VFPATH
$ write sys$output "HEADERPATH = ",HEADERPATH
$ write sys$output "CONFIGPATH = ",CONFIGPATH
$ write sys$output "FIGPATH = ",FIGPATH
$ if TPIC .eqs. "" then -
  write sys$output "TPIC support is not enabled."
$ if TPIC .nes. "" then -
  write sys$output "TPIC support is enabled."
$ if FONTLIB .eqs. "" then -
  write sys$output "FONTLIB support is not enabled."
$ if FONTLIB .nes. "" then -
  write sys$output "FONTLIB support is enabled."
$ if EMTEX .eqs. "" then -
  write sys$output "EMTEX support is not enabled."
$ if EMTEX .nes. "" then -
  write sys$output "EMTEX support is enabled."
$ if DEBUG .eqs. "" then -
  write sys$output "DEBUG support is not enabled."
$ if DEBUG .nes. "" then -
  write sys$output "DEBUG support is enabled."
$ if VMS_ROOTED .eqs. "" then -
  write sys$output PKPATH," is not a rooted directory."
$ if VMS_ROOTED .nes. "" then -
  write sys$output PKPATH," is a rooted directory."
$ write sys$output " "
$ inquire/nop ANSWER "Is this correct [yes]?  "
$ if ANSWER .eqs. "" then ANSWER = 1
$ if .not. ANSWER then goto get_definitions
$ !
$ if VMS_ROOTED .nes. "" then PKPATH = "''PKPATH'[%d]%f.PK"
$ write sys$output " "
$ write sys$output "Compiling sources ..."
$ write sys$output " "
$ write sys$output "afm2tfm.c ..."
$ cc /define=(VMS'DEBUG') AFM2TFM.C
$ write sys$output "color.c ..."
$ cc /define=(VMS'DEBUG') COLOR.C
$ write sys$output "dopage.c ..."
$ cc /define=(VMS'DEBUG''EMTEX') DOPAGE.C
$ write sys$output "dosection.c ..."
$ cc /define=(VMS'DEBUG') DOSECTION.C
$ write sys$output "dospecial.c ..."
$ cc /define=(VMS'DEBUG''TPIC') DOSPECIAL.C
$ write sys$output "download.c ..."
$ cc /define=(VMS'DEBUG') DOWNLOAD.C
$ write sys$output "dpicheck.c ..."
$ cc /define=(VMS'DEBUG') DPICHECK.C
$ write sys$output "drawps.c ..."
$ cc /define=(VMS'DEBUG''TPIC') DRAWPS.C
$ write sys$output "dviinput.c ..."
$ cc /define=(VMS'DEBUG') DVIINPUT.C
$ write sys$output "dvips.c ..."
$ cc /define=(VMS'DEBUG',"TFMPATH=""''TFMPATH'""","PKPATH=""''PKPATH'""", -
      "VFPATH=""''VFPATH'""","FIGPATH=""''FIGPATH'""",HAVE_GETCWD,ANSI, -
      "CONFIGPATH=""''CONFIGPATH'""","HEADERPATH=""''HEADERPATH'""") DVIPS.C
$ write sys$output "finclude.c ..."
$ cc /DEF=(VMS'DEBUG') FINCLUDE.C
$ write sys$output "flib.c ..."
$ cc /DEF=(VMS'DEBUG''FONTLIB') FLIB.C
$ write sys$output "fontdef.c ..."
$ cc /define=(VMS'DEBUG') FONTDEF.C
$ write sys$output "header.c ..."
$ cc /define=(VMS'DEBUG') HEADER.C
$ write sys$output "loadfont.c ..."
$ cc /define=(VMS'DEBUG') LOADFONT.C
$ write sys$output "makefont.c ..."
$ cc /define=(VMS'DEBUG') MAKEFONT.C
$ write sys$output "output.c ..."
$ cc /define=(VMS'DEBUG',"HEADERPATH=""''HEADERPATH'""") OUTPUT.C
$ write sys$output "papersiz.c  ..."
$ cc /define=(VMS'DEBUG') PAPERSIZ.C
$ write sys$output "pprescan.c  ..."
$ cc /define=(VMS'DEBUG') PPRESCAN.C
$ write sys$output "prescan.c  ..."
$ cc /define=(VMS'DEBUG') PRESCAN.C
$ write sys$output "repack.c ..."
$ cc /define=(VMS'DEBUG') REPACK.C
$ write sys$output "resident.c ..."
$ cc /define=(VMS'DEBUG',"CONFIGPATH=""''CONFIGPATH'""",HAVE_GETCWD,ANSI) -
      RESIDENT.C
$ write sys$output "scalewidth.c ..."
$ cc /define=(VMS'DEBUG') SCALEWIDTH.C
$ write sys$output "scanpage.c ..."
$ cc /define=(VMS'DEBUG') SCANPAGE.C
$ write sys$output "search.c ..."
$ cc /define=(VMS'DEBUG') SEARCH.C
$ write sys$output "skippage.c ..."
$ cc /define=(VMS'DEBUG') SKIPPAGE.C
$ write sys$output "squeeze.c ..."
$ cc /define=(VMS'DEBUG') SQUEEZE.C
$ write sys$output "tfmload.c ..."
$ cc /define=(VMS'DEBUG') TFMLOAD.C
$ write sys$output "unpack.c ..."
$ cc /define=(VMS'DEBUG') UNPACK.C
$ write sys$output "virtualfont.c ..."
$ cc /define=(VMS'DEBUG') VIRTUALFONT.C
$ write sys$output "bbox.c ..."
$ cc /define=(VMS'DEBUG') BBOX.C
$ write sys$output "emspecial.c ..."
$ cc /define=(VMS'DEBUG''EMTEX') EMSPECIAL.C
$ !
$ !     vaxvms fixes some irritating problems with VAXC ( particulary
$ !     fseek and ftell ).  Thanks to Nelson Beebee at Utah.
$ !
$ write sys$output "vaxvms.c ..."
$ cc /define=(VMS'DEBUG') [.VMS]VAXVMS.C
$ !
$ linkstep:
$ write sys$output " "
$ write sys$output "Linking dvips ..."
$ link /exe=dvips dvips,dopage,dosection,dospecial,download,dpicheck,drawps, -
         dviinput,header,finclude,flib,fontdef,loadfont,tfmload,prescan, -
                  scanpage,skippage,output,scalewidth,resident,search,unpack, -
                  makefont,repack,virtualfont,vaxvms,color,papersiz,pprescan, -
                  bbox,emspecial,[.vms]vaxcrtl.opt/opt
$ write sys$output " "
$ write sys$output "Linking squeeze ..."
$ link /exe=squeeze squeeze,vaxvms,[.vms]vaxcrtl.opt/opt
$ write sys$output " "
$ write sys$output "Linking afm2tfm ..."
$ write sys$output " "
$ link /exe=afm2tfm afm2tfm,vaxvms,[.vms]vaxcrtl.opt/opt
$ !
$ inquire/nop ANSWER -
  "Do you wish to compress the PostScript prologue files [no]?   "
$ if ANSWER .eqs. "" then ANSWER = 0
$ if .not. ANSWER then goto copy_prologue
$ squeeze :== $'f$environment("DEFAULT")'squeeze.exe
$ write sys$output " "
$ set verify
$ squeeze COLOR.LPRO COLOR.PRO
$ squeeze FINCLUDE.LPRO FINCLUDE.PRO
$ squeeze TEX.LPRO TEX.PRO
$ squeeze TEXPS.LPRO TEXPS.PRO
$ squeeze SPECIAL.LPRO SPECIAL.PRO
$ verify = 'f$verify(0)
$ write sys$output " "
$ copy_prologue:
$ FIRSTPATH =  PLACEHEADER_DIR
$ inquire/nop ANSWER -
  "Do you wish to copy the prologue files to ''FIRSTPATH' [yes]?   "
$ if ANSWER .eqs. "" then ANSWER = 1
$ if .not. ANSWER then goto copy_config
$ write sys$output " "
$ set verify
$ copy COLOR.PRO 'FIRSTPATH'COLOR.PRO
$ copy FINCLUDE.PRO 'FIRSTPATH'FINCLUDE.PRO
$ copy TEX.PRO 'FIRSTPATH'TEX.PRO
$ copy TEXPS.PRO 'FIRSTPATH'TEXPS.PRO
$ copy SPECIAL.PRO 'FIRSTPATH'SPECIAL.PRO
$ set protection=(s:rwed,o:rwed,g:re,w:re) 'FIRSTPATH'COLOR.PRO
$ set protection=(s:rwed,o:rwed,g:re,w:re) 'FIRSTPATH'FINCLUDE.PRO
$ set protection=(s:rwed,o:rwed,g:re,w:re) 'FIRSTPATH'TEX.PRO
$ set protection=(s:rwed,o:rwed,g:re,w:re) 'FIRSTPATH'SPECIAL.PRO
$ set protection=(s:rwed,o:rwed,g:re,w:re) 'FIRSTPATH'TEXPS.PRO
$ verify = 'f$verify(0)
$ write sys$output " "
$ copy_config:
$ FIRSTPATH =  PLACEHEADER_DIR
$ inquire/nop ANSWER -
  "Do you wish to copy the CONFIG.PS file to ''FIRSTPATH' [yes]?   "
$ if ANSWER .eqs. "" then ANSWER = 1
$ if .not. ANSWER then goto copy_image
$ write sys$output " "
$ set verify
$ copy CONFIG.PS 'FIRSTPATH'CONFIG.PS
$ set protection=(s:rwed,o:rwed,g:re,w:re) 'FIRSTPATH'CONFIG.PS
$ verify = 'f$verify(0)
$ write sys$output " "
$ copy_image:
$ inquire/nop ANSWER "Do you wish to copy DVIPS.EXE to the TeX area [yes]?   "
$ if ANSWER .eqs. "" then ANSWER = 1
$ if .not. ANSWER then goto done
$ inquire/nop ANSWER "TeX image area [''TEXEXEPATH']?  "
$ if ANSWER .nes. "" THEN TEXEXEPATH = ANSWER
$ write sys$output " "
$ set verify
$ copy dvips.exe 'TEXEXEPATH'
$ set protect=(s:rwed,o:rwed,g:re,w:re) 'TEXEXEPATH'dvips.exe
$ verify = 'f$verify(0)
$ write sys$output " "
$ done:
$ write sys$output " "
$ write sys$output "Be sure to add dvips to your CLI by using the SET "
$ write sys$output "COMMAND syntax and do the other steps which are"
$ write sys$output "necessary to finish up the installation of dvips."
$ write sys$output " "
$ goto good_exit
$ bad_exit:
$ write sys$output " "
$ write sys$output "Something's wrong here.  You might want to take a look"
$ write sys$output "at the offending code or command and start over."
$ write sys$output " "
$ exit
$ good_exit:
$ write sys$output "Done."
$ write sys$output " "
$ exit
$! --------------------- EOF -------------------------------------
