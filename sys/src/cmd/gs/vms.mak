$ !    Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
$ !
$ ! This file is part of Aladdin Ghostscript.
$ !
$ ! Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
$ ! or distributor accepts any responsibility for the consequences of using it,
$ ! or for whether it serves any particular purpose or works at all, unless he
$ ! or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
$ ! License (the "License") for full details.
$ !
$ ! Every copy of Aladdin Ghostscript must include a copy of the License,
$ ! normally in a plain ASCII text file named PUBLIC.  The License grants you
$ ! the right to copy, modify and redistribute Aladdin Ghostscript, but only
$ ! under certain conditions described in the License.  Among other things, the
$ ! License requires that the copyright notice and this notice be preserved on
$ ! all copies.
$ !
$ !
$ ! VMS "makefile" for Ghostscript.
$ !
$ WSO = "WRITE SYS$OUTPUT"
$ ON ERROR THEN GOTO DONE
$ ON CONTROL_Y THEN GOTO DONE
$ !
$ ! Check input parameters
$ !
$ IF P1 .NES. "" .AND. P1 .NES. "DEBUG" .AND. P1 .NES. "LINK" .AND. -
   P1 .NES. "BUILD" THEN GS_LIB_DEFAULT = P1
$ IF P2 .NES. "" .AND. P2 .NES. "DEBUG" .AND. P2 .NES. "LINK" .AND. -
   P2 .NES. "BUILD" THEN GS_LIB_DEFAULT = P2
$ !
$ IF P1 .NES. "DEBUG" .AND. P2 .NES. "DEBUG" THEN GOTO NODEBUG
$ CC_QUAL = CC_QUAL + "/DEFINE=(""DEBUG"")/NOOPTIMIZE/DEBUG"     ! US
$ !CC_QUAL = CC_QUAL + "/DEFINE=(""A4"",""DEBUG"")/NOOPTIMIZE/DEBUG" ! Europe
$ L_QUAL  = L_QUAL + "/DEBUG"
$ !
$ NODEBUG:
$ If P1 .EQS. "LINK" .OR. P2 .EQS. "LINK" Then GoTo LINK_ONLY
$ If P1 .EQS. "BUILD" .OR. P2 .EQS. "BUILD" Then GoTo BUILD_EXES
$ !
$ !
$ ! Compile and link genarch.c and then run it to create the arch.h header file
$ !
$ WSO "''CC_COMMAND'''CC_QUALI'/NOLIST/OBJECT=GENARCH.OBJ GENARCH.C"
$ 'CC_COMMAND/NOLIST/OBJECT=GENARCH.OBJ GENARCH.C
$ LINK/NOMAP/EXE=GENARCH.EXE GENARCH.OBJ,RTL.OPT/OPT
$ GENARCH = "$" + F$ENVIRONMENT("DEFAULT") + "GENARCH.EXE"
$ GENARCH ARCH.H
$ DELETE GENARCH.EXE.*,GENARCH.OBJ.*
$ PURGE ARCH.H
$ !
$ !
$ ! Compile and link echogs.c; define ECHOGS as a command
$ !
$ WSO "''CC_COMMAND'''CC_QUALI'/NOLIST/OBJECT=ECHOGS.OBJ ECHOGS.C"
$ 'CC_COMMAND/NOLIST/OBJECT=ECHOGS.OBJ ECHOGS.C
$ LINK/NOMAP/EXE=ECHOGS.EXE ECHOGS.OBJ,RTL.OPT/OPT
$ ECHOGS = "$" + F$ENVIRONMENT("DEFAULT") + "ECHOGS.EXE"
$ DELETE ECHOGS.OBJ;*
$ PURGE ECHOGS.EXE
$ !
$ !
$ ! Compile and link genconf.c; define GENCONF as a command
$ !
$ WSO "''CC_COMMAND'''CC_QUALI/NOLIST/OBJECT=GENCONF.OBJ GENCONF.C"
$ 'CC_COMMAND/NOLIST/OBJECT=GENCONF.OBJ GENCONF.C
$ LINK/NOMAP/EXE=GENCONF.EXE GENCONF.OBJ,RTL.OPT/OPT
$ GENCONF = "$" + F$ENVIRONMENT("DEFAULT") + "GENCONF.EXE"
$ DELETE GENCONF.OBJ;*
$ PURGE GENCONF.EXE
$ !
$ !
$ ! Create GSSETDEV.COM, GSSETMOD.COM, and GSADDMOD.COM.
$ ! (These aren't distributed with the fileset because the .COM suffix
$ ! causes certain development tools to treat them incorrectly.)
$ !
$ !
$ OPEN/WRITE SETDEV GSSETDEV.COM
$   WRITE SETDEV "$ echogs -w 'p1'.dev - -dev 'p1' -obj 'p2 'p3 'p4 'p5 'p6 'p7 'p8"
$ CLOSE SETDEV
$ !
$ OPEN/WRITE SETMOD GSSETMOD.COM
$   WRITE SETMOD "$ if p2 .nes. """""
$   WRITE SETMOD "$ then"
$   WRITE SETMOD "$   echogs -w 'p1'.dev - 'p2 'p3 'p4 'p5 'p6 'p7 'p8"
$   WRITE SETMOD "$ else"
$   WRITE SETMOD "$   echogs -w 'p1'.dev"
$   WRITE SETMOD "$ endif"
$ CLOSE SETMOD
$ !
$ OPEN/WRITE ADDMOD GSADDMOD.COM
$   WRITE ADDMOD "$ if (p2 .nes. """") then echogs -a 'p1'.dev - 'p2 'p3 'p4 'p5 'p6 'p7 'p8"
$ CLOSE ADDMOD
$ !
$ !
$ ! Define SETDEV, SETMOD, ADDMOD as commands to execute GSSETDEV.COM,
$ ! GSSETMOD.COM and GSADDMOD.COM respectively.  Those three command
$ ! procedures make use of ECHOGS.EXE and the ECHOGS verb.
$ !
$ SETDEV = "@GSSETDEV.COM"
$ SETMOD = "@GSSETMOD.COM"
$ ADDMOD = "@GSADDMOD.COM"
$ !
$ !
$ ! Build GCONFIG_.H
$ !
$ ECHOGS -w gconfig_.h #define SYSTIME_H
$ !
$ ! Build GCONFIGV.H
$ !
$ ECHOGS -w gconfigv.h #define USE_ASM 0
$ ECHOGS -a gconfigv.h #define USE_FPU 1
$ !
$ ! Now generate *.dev files
$ !
$ IF F$TYPE(DEVICE_DEVS1) .EQS. "" THEN DEVICE_DEVS1 = ""
$ IF F$TYPE(DEVICE_DEVS2) .EQS. "" THEN DEVICE_DEVS2 = ""
$ IF F$TYPE(DEVICE_DEVS3) .EQS. "" THEN DEVICE_DEVS3 = ""
$ IF F$TYPE(DEVICE_DEVS4) .EQS. "" THEN DEVICE_DEVS4 = ""
$ IF F$TYPE(DEVICE_DEVS5) .EQS. "" THEN DEVICE_DEVS5 = ""
$ IF F$TYPE(DEVICE_DEVS6) .EQS. "" THEN DEVICE_DEVS6 = ""
$ IF F$TYPE(DEVICE_DEVS7) .EQS. "" THEN DEVICE_DEVS7 = ""
$ IF F$TYPE(DEVICE_DEVS8) .EQS. "" THEN DEVICE_DEVS8 = ""
$ IF F$TYPE(DEVICE_DEVS9) .EQS. "" THEN DEVICE_DEVS9 = ""
$ DEV_LIST_NAMES = "FEATURE_DEVS DEVICE_DEVS DEVICE_DEVS1 DEVICE_DEVS2 DEVICE_DEVS3 DEVICE_DEVS4 DEVICE_DEVS5 DEVICE_DEVS6 DEVICE_DEVS7 DEVICE_DEVS8 DEVICE_DEVS9"
$ DEV_MODULES = ""
$ I = 0
$ DEVS_OUTER:
$   DEV_LIST = F$ELEMENT(I," ",DEV_LIST_NAMES)
$   IF DEV_LIST .EQS. " " THEN GOTO DEVS_DONE
$   I = I+1
$   IF F$TYPE(DEV_LIST) .EQS. "" THEN GOTO DEVS_OUTER
$   J = 0
$   DEVS_INNER:
$     ACTION = F$ELEMENT(J," ",'DEV_LIST')
$     IF ACTION .EQS. " " THEN GOTO DEVS_OUTER
$     J = J+1
$     ! Replace "." with "_"
$     IF F$LOCATE(".",ACTION) .NE. F$LENGTH(ACTION) THEN -
$	ACTION = F$ELEMENT(0,".",ACTION) + "_" + F$ELEMENT(1,".",ACTION)
$     GOSUB 'ACTION
$   GOTO DEVS_INNER
$ !
$ DEVS_DONE:
$ !
$ DEV_MODULES = F$EDIT(DEV_MODULES,"TRIM")
$ !
$ !
$ ! And now build gconfig.h and gconfigf.h
$ !
$ GOSUB GCONFIG_H
$ GOSUB GCONFIGF_H
$ !
$ !
$ ! Create an empty object library
$ !
$ LIBRARY/CREATE GS.OLB
$ !
$ ! NOW COMPILE AWAY!
$ !
$ OPEN/READ/ERROR=NO_MODULES MODULE_LIST MODULES.LIS
$ OPEN/WRITE OPT_FILE GS.OPT
$ OPT_LINE = "GS.OLB/LIBRARY/INCLUDE=("
$ COMMA = ""
$ !
$ DEFDIR = F$PARSE(F$ENVIRONMENT("DEFAULT"),,,"DIRECTORY","SYNTAX_ONLY")
$ COMPILE_LOOP:
$   READ/END=END_COMPILE MODULE_LIST MODULE
$   NAME = F$PARSE(MODULE,,,"NAME","SYNTAX_ONLY")
$   DIR  = F$PARSE(MODULE,,,"DIRECTORY","SYNTAX_ONLY") - DEFDIR
$   IF DIR .NES. ""
$   THEN
$     COPY 'MODULE'.C []
$     INC = "/INCLUDE_DIRECTORY=(''DEFDIR',''DIR')"
$   ELSE
$     INC = ""
$   ENDIF
$   WSO "''CC_COMMAND'''CC_QUAL'''INC'/NOLIST/OBJECT=''MODULE'.OBJ ''MODULE'.C"
$   'CC_COMMAND''CC_QUAL''INC'/NOLIST/OBJECT='NAME'.OBJ 'NAME'.C
$   LIBRARY/INSERT GS.OLB 'NAME'.OBJ
$   DELETE 'NAME'.OBJ.*
$   IF DIR .NES. "" THEN DELETE 'NAME'.C;
$   IF F$LENGTH(OPT_LINE) .GE. 70
$   THEN
$     OPT_LINE = OPT_LINE + COMMA + "-"
$     WRITE OPT_FILE OPT_LINE
$     OPT_LINE = NAME
$   ELSE
$     OPT_LINE = OPT_LINE + COMMA + NAME
$   ENDIF
$   COMMA = ","
$ GOTO COMPILE_LOOP
$ !
$ END_COMPILE:
$ !
$ ! Now compile device modules found in symbol DEV_MODULES
$ !
$ I = 0
$ COMPILE_DEV_LOOP:
$   MODULE = F$ELEMENT(I," ",DEV_MODULES)
$   IF MODULE .EQS. " " THEN GOTO END_COMPILE_DEV
$   I = I + 1
$   NAME = F$PARSE(MODULE,,,"NAME","SYNTAX_ONLY")
$   DIR  = F$PARSE(MODULE,,,"DIRECTORY","SYNTAX_ONLY") - DEFDIR
$   IF DIR .NES. ""
$   THEN
$     COPY 'MODULE'.C []
$     INC = "/INCLUDE_DIRECTORY=(''DEFDIR',''DIR')"
$   ELSE
$     INC = ""
$   ENDIF
$   WSO "''CC_COMMAND'''CC_QUAL'''INC'/NOLIST/OBJECT=''MODULE'.OBJ ''MODULE'.C"
$   'CC_COMMAND''CC_QUAL''INC'/NOLIST/OBJECT='NAME'.OBJ 'NAME'.C
$   LIBRARY/INSERT GS.OLB 'NAME'.OBJ
$   DELETE 'NAME'.OBJ.*
$   IF DIR .NES. "" THEN DELETE 'NAME'.C;
$   IF F$LENGTH(OPT_LINE) .GE. 70
$   THEN
$     OPT_LINE = OPT_LINE + COMMA + "-"
$     WRITE OPT_FILE OPT_LINE
$     OPT_LINE = NAME
$   ELSE
$     OPT_LINE = OPT_LINE + COMMA + NAME
$   ENDIF
$   COMMA = ","
$ GOTO COMPILE_DEV_LOOP
$ !
$ END_COMPILE_DEV:
$ !
$ OPT_LINE = OPT_LINE + ")"
$ WRITE OPT_FILE OPT_LINE
$ IF F$SEARCH("SYS$SHARE:DECW$XMLIBSHR12.EXE") .NES. ""
$ THEN
$   WRITE OPT_FILE "SYS$SHARE:DECW$XMLIBSHR12.EXE/SHARE"
$   WRITE OPT_FILE "SYS$SHARE:DECW$XTLIBSHRR5.EXE/SHARE"
$   WRITE OPT_FILE "SYS$SHARE:DECW$XLIBSHR.EXE/SHARE"
$ ELSE
$   WRITE OPT_FILE "SYS$SHARE:DECW$XMLIBSHR.EXE/SHARE"
$   WRITE OPT_FILE "SYS$SHARE:DECW$XTSHR.EXE/SHARE"
$   WRITE OPT_FILE "SYS$SHARE:DECW$XLIBSHR.EXE/SHARE"
$ ENDIF
$ WRITE OPT_FILE "Ident=""gs 3.23"""
$ CLOSE MODULE_LIST
$ CLOSE OPT_FILE
$ !
$ !
$ ! Is the DECwindows environment about?  Must be installed in order to
$ ! build the executable program gs.exe.
$ !
$ IF F$SEARCH("SYS$SHARE:DECW$XLIBSHR.EXE") .NES. "" THEN GOTO CHECK2
$ WSO "DECwindows user environment not installed;"
$ WSO "unable to build executable programs."
$ GOTO DONE
$ !
$ CHECK2:
$ IF F$TRNLNM("DECW$INCLUDE") .NES. "" THEN GOTO BUILD_EXES
$ WSO "You must invoke @DECW$STARTUP before using this"
$ WSO "command procedure to build the executable programs."
$ GOTO DONE
$ !
$ ! Build the executables
$ !
$ BUILD_EXES:
$ !
$ DEFINE X11 DECW$INCLUDE
$ !
$ WSO "''CC_COMMAND'''CC_QUAL'/NOLIST/OBJECT=GCONFIG.OBJ GCONFIG.C"
$ 'CC_COMMAND''CC_QUAL/NOLIST/OBJECT=GCONFIG.OBJ GCONFIG.C
$ !
$ WSO "''CC_COMMAND'''CC_QUAL'/NOLIST/OBJECT=GSMAIN.OBJ GSMAIN.C"
$ 'CC_COMMAND''CC_QUAL/NOLIST/OBJECT=GSMAIN.OBJ GSMAIN.C
$ !
$ WSO "''CC_COMMAND'''CC_QUAL'/NOLIST/OBJECT=GS.OBJ GS.C"
$ 'CC_COMMAND''CC_QUAL/NOLIST/OBJECT=GS.OBJ GS.C
$ !
$LINK_ONLY:
$ WSO "Linking ... "
$ WSO "LINK''L_QUAL'/NOMAP/EXE=GS.EXE GS,GSMAIN,GCONFIG,GS.OPT/OPT"
$ LINK'L_QUAL/NOMAP/EXE=GS.EXE GS,GSMAIN,GCONFIG,GS.OPT/OPT
$ !
$ DELETE GSMAIN.OBJ.*,GS.OBJ.*,GCONFIG.OBJ.*
$ !
$ GOTO DONE
$ !
$ !
$ BTOKEN_DEV:
$   SETMOD btoken 'btoken_
$   ADDMOD btoken -oper zbseq_l2
$   ADDMOD btoken -ps gs_btokn
$   RETURN
$ !
$ CMYKCORE_DEV:
$   SETMOD cmykcore 'cmykcore_
$   RETURN
$ !
$ CMYKREAD_DEV:
$   SETMOD cmykread 'cmykread_
$   ADDMOD cmykread -oper zcolor1
$   RETURN
$ !
$ COLOR_DEV:
$   GOSUB CMYKCORE_DEV
$   GOSUB CMYKREAD_DEV
$   SETMOD color
$   ADDMOD color -include cmykcore cmykread
$   RETURN
$ !
$ PSF1CORE_DEV:
$   SETMOD psf1core 'psf1core_
$   RETURN
$ !
$ PSF1READ_DEV:
$   SETMOD psf1read 'psf1read_
$   ADDMOD psf1read -oper zchar1 zfont1 zmisc1
$   ADDMOD psf1read -ps gs_type1
$   RETURN
$ !
$ TYPE1_DEV:
$   GOSUB PSF1CORE_DEV
$   GOSUB PSF1READ_DEV
$   SETMOD type1
$   ADDMOD type1 -include psf1core psf1read
$   RETURN
$ !
$ LEVEL1_DEV:
$   GOSUB TYPE1_DEV
$   SETMOD level1
$   ADDMOD level1 -include type1
$   ADDMOD level1 -oper zarith zarray zcontrol zdict
$   ADDMOD level1 -oper zfile zfileio zfilter zfproc
$   ADDMOD level1 -oper zgeneric ziodev zmath zmisc zpacked
$   ADDMOD level1 -oper zrelbit zstack zstring zsysvm
$   ADDMOD level1 -oper ztoken ztype zvmem
$   ADDMOD level1 -oper zchar zcolor zdevice zfont zfont2
$   ADDMOD level1 -oper zgstate zht zmatrix zpaint zpath zpath2
$   RETURN
$ !
$ DPSAND2_DEV:
$   GOSUB BTOKEN_DEV
$   GOSUB COLOR_DEV
$   SETMOD dpsand2 'dpsand2a_
$   ADDMOD dpsand2 -include btoken color
$   ADDMOD dpsand2 -obj 'dpsand2a_
$   ADDMOD dpsand2 -obj 'dpsand2b_
$   ADDMOD dpsand2 -oper zvmem2
$   ADDMOD dpsand2 -oper igc_l2 zchar2_l2 zdps1_l2 zupath_l2
$   ADDMOD dpsand2 -ps gs_dps1
$   RETURN
$ !
$ DPS_DEV:
$   GOSUB DPSAND2_DEV
$   SETMOD dps 'dps_
$   ADDMOD dps -include dpsand2
$   ADDMOD dps -obj 'dps_
$   RETURN
$ !
$ PSF0CORE_DEV:
$   SETMOD psf0core 'psf0core_
$   RETURN
$ !
$ PSF0READ_DEV:
$   SETMOD psf0read 'psf0read_
$   ADDMOD psf0read -oper zfont0 zchar2
$   ADDMOD psf0read -ps gs_type0
$   RETURN
$ !
$ COMPFONT_DEV:
$   GOSUB PSF0CORE_DEV
$   GOSUB PSF0READ_DEV
$   SETMOD compfont
$   ADDMOD compfont -include psf0core psf0read
$   RETURN
$ !
$ LEV2ONLY_DEV:
$   SETMOD lev2only 'level2a_
$   ADDMOD lev2only -obj 'level2b_
$   ADDMOD lev2only -obj 'level2c_
$   ADDMOD lev2only -obj 'level2d_
$   ADDMOD lev2only -oper zmisc2
$   ADDMOD lev2only -oper zcie_l2 zcolor2_l2 zcsindex_l2 zcssepr_l2
$   ADDMOD lev2only -oper zdevice2_l2 zht2_l2 zimage2_l2 ziodev2_l2
$   ADDMOD lev2only -iodev null ram
$   ADDMOD lev2only -ps gs_lev2 gs_res
$   RETURN
$ !
$ PSL2CORE_DEV:
$   SETMOD psl2core 'psl2core_
$   RETURN
$ !
$ PSL2READ_DEV:
$   SETMOD psl2read 'psl2read1_
$   ADDMOD psl2read -obj 'psl2read2_
$   ADDMOD psl2read -oper zmisc2
$   ADDMOD psl2read -oper zcie_l2 zcolor2_l2 zcsindex_l2
$   ADDMOD psl2read -oper zdevice2_l2 zht2_l2 zimage2_l2 ziodev2_l2
$   ADDMOD psl2read -iodev null ram
$   ADDMOD psl2read -ps gs_lev2
$   RETURN
$ !
$ LEVEL2_DEV:
$   GOSUB COMPFONT_DEV
$   GOSUB DCT_DEV
$   GOSUB DPSAND2_DEV
$   GOSUB FILTER_DEV
$   GOSUB LEVEL1_DEV
$   GOSUB PSL2CORE_DEV
$   GOSUB PSL2READ_DEV
$   SETMOD level2
$   ADDMOD level2 -include compfont dct dpsand2 filter
$   ADDMOD level2 -include level1 psl2core psl2read
$   RETURN
$ !
$ FILTER_DEV:
$   SETMOD filter 'filter_1
$   ADDMOD filter -obj 'filter_2
$   ADDMOD filter -obj 'filter_3
$   ADDMOD filter -oper zfilter2
$   RETURN
$ !
$ JPEG_DEV:
$   COPY GSJCONF.H JCONFIG.H
$   COPY GSJMOREC.H JMORECFG.H
$   COPY [.JPEG-5]JMORECFG.H []JMCORIG.H
$   COPY [.JPEG-5]JERROR.H   []JERROR.H
$   COPY [.JPEG-5]JINCLUDE.H []JINCLUDE.H
$   COPY [.JPEG-5]JPEGLIB.H  []JPEGLIB.H
$   SETMOD jpeg 'DCT1
$   ADDMOD jpeg -obj 'DCT2
$   ADDMOD jpeg -obj 'DCT3
$   ADDMOD jpeg -obj 'DCTE1
$   ADDMOD jpeg -obj 'DCTE2
$   ADDMOD jpeg -obj 'DCTE3
$   ADDMOD jpeg -obj 'DCTD1
$   ADDMOD jpeg -obj 'DCTD2
$   ADDMOD jpeg -obj 'DCTD3
$   RETURN
$ !
$ DCT_DEV:
$   GOSUB JPEG_DEV
$   SETMOD dct
$   ADDMOD dct -include jpeg
$   ADDMOD dct -include jpeg
$   ADDMOD dct -obj sdctc sdcte scdtd sjpegc sjpege sjpegd
$   ADDMOD dct -oper zfdcte zfdctd
$   RETURN
$ !
$ CCFONTS_DEV:
$   ADDMOD ccfonts -include type1
$   SETMOD ccfonts iccfont.obj
$   ADDMOD ccfonts -obj 'ccfonts1_
$   ADDMOD ccfonts -obj 'ccfonts2_
$   ADDMOD ccfonts -obj 'ccfonts3_
$   ADDMOD ccfonts -obj 'ccfonts4_
$   ADDMOD ccfonts -obj 'ccfonts5_
$   ADDMOD ccfonts -obj 'ccfonts6_
$   ADDMOD ccfonts -obj 'ccfonts7_
$   ADDMOD ccfonts -obj 'ccfonts8_
$   ADDMOD ccfonts -obj 'ccfonts9_
$   ADDMOD ccfonts -oper ccfonts
$   ADDMOD ccfonts -ps gs_ccfnt
$   RETURN
$ !
$ GCONFIGF_H:
$   SETMOD ccfonts_
$   ADDMOD ccfonts_ -font "''ccfonts1'"
$   ADDMOD ccfonts_ -font "''ccfonts2'"
$   ADDMOD ccfonts_ -font "''ccfonts3'"
$   ADDMOD ccfonts_ -font "''ccfonts4'"
$   ADDMOD ccfonts_ -font "''ccfonts5'"
$   ADDMOD ccfonts_ -font "''ccfonts6'"
$   ADDMOD ccfonts_ -font "''ccfonts7'"
$   ADDMOD ccfonts_ -font "''ccfonts8'"
$   ADDMOD ccfonts_ -font "''ccfonts9'"
$   GENCONF ccfonts_.dev -f gconfigf.h
$   RETURN
$ !
$ CCINIT_DEV:
$   SETMOD ccinit iccinit.obj gs_init.obj
$   ADDMOD ccinit -oper ccinit
$   RETURN
$ !
$ X11_DEV:
$   SETDEV x11 'x11_
$   ADDMOD x11 -lib Xt Xext X11
$   ADD_DEV_MODULES = "GDEVX GDEVXINI GDEVXXF GDEVEMAP"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ GS_DEV:
$   ECHOGS -w gs.dev - -obj gsmain.obj
$   ECHOGS -a gs.dev 'INT1'
$   ECHOGS -a gs.dev 'INT2'
$   ECHOGS -a gs.dev 'INT3'
$   ECHOGS -a gs.dev 'INT4'
$   ECHOGS -a gs.dev 'INT5'
$   ECHOGS -a gs.dev 'INT6'
$   ECHOGS -a gs.dev 'INT7'
$   ECHOGS -a gs.dev 'INT8'
$   ECHOGS -a gs.dev 'INT9'
$   ECHOGS -a gs.dev 'INT10'
$   ECHOGS -a gs.dev 'INT11'
$   ECHOGS -a gs.dev - -iodev stdin stdout stderr lineedit statementedit
$   RETURN
$ !
$ DEVS_TR:
$ ! quote the dashes so that they are not interpreted as continuation
$ ! marks when the following DCL symbol is not defined!
$   ECHOGS -w devs.tr "-"  gs.dev
$   ECHOGS -a devs.tr "-" 'FEATURE_DEVS
$   ECHOGS -a devs.tr "-" 'DEVICE_DEVS
$   ECHOGS -a devs.tr "-" 'DEVICE_DEVS1'
$   ECHOGS -a devs.tr "-" 'DEVICE_DEVS2'
$   ECHOGS -a devs.tr "-" 'DEVICE_DEVS3'
$   ECHOGS -a devs.tr "-" 'DEVICE_DEVS4'
$   ECHOGS -a devs.tr "-" 'DEVICE_DEVS5'
$   ECHOGS -a devs.tr "-" 'DEVICE_DEVS6'
$   ECHOGS -a devs.tr "-" 'DEVICE_DEVS7'
$   ECHOGS -a devs.tr "-" 'DEVICE_DEVS8'
$   ECHOGS -a devs.tr "-" 'DEVICE_DEVS9'
$   RETURN
$ !
$ GCONFIG_H:
$   GOSUB GS_DEV
$   GOSUB DEVS_TR
$   GENCONF "@devs.tr" -h gconfig.h
$   ECHOGS -a gconfig.h "#define GS_LIB_DEFAULT ""''GS_LIB_DEFAULT'"""
$   ECHOGS -a gconfig.h "#define GS_DOCDIR ""''GS_DOCDIR'"""
$   ECHOGS -a gconfig.h "#define GS_INIT ""''GS_INIT'"""
$   RETURN
$ !  
$ !  
$ ! Next devices added by BSN
$ !
$ BJ10E_DEV:
$   SETDEV bj10e 'bj10e_
$   ADD_DEV_MODULES = "GDEVBJ10 GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ BJ200_DEV:
$   SETDEV bj200 'bj10e_
$   ADD_DEV_MODULES = "GDEVBJ10 GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ DESKJET_DEV:
$   SETDEV deskjet 'HPMONO
$   ADD_DEV_MODULES = "GDEVDJET GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ DJET500_DEV:
$   SETDEV djet500 'HPMONO
$   ADD_DEV_MODULES = "GDEVDJET GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ LASERJET_DEV:
$   SETDEV laserjet 'HPMONO
$   ADD_DEV_MODULES = "GDEVDJET GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ LJETPLUS_DEV:
$   SETDEV ljetplus 'HPMONO
$   ADD_DEV_MODULES = "GDEVDJET GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ LJET2P_DEV:
$   SETDEV ljet2p 'HPMONO
$   ADD_DEV_MODULES = "GDEVDJET GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ LJET3_DEV:
$   SETDEV ljet3 'HPMONO
$   ADD_DEV_MODULES = "GDEVDJET GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ LJET4_DEV:
$   SETDEV ljet4 'HPMONO
$   ADD_DEV_MODULES = "GDEVDJET GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ CDESKJET_DEV:
$   SETDEV cdeskjet 'cdeskjet_
$   ADD_DEV_MODULES = "GDEVCDJ GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ CDJCOLOR_DEV:
$   SETDEV cdjcolor 'cdeskjet_
$   ADD_DEV_MODULES = "GDEVCDJ GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ CDJMONO_DEV:
$   SETDEV cdjmono 'cdeskjet_
$   ADD_DEV_MODULES = "GDEVCDJ GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ CDJ550_DEV:
$   SETDEV cdj550 'cdeskjet_
$   ADD_DEV_MODULES = "GDEVCDJ GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PAINTJET_DEV:
$   SETDEV paintjet 'PJET
$   ADD_DEV_MODULES = "GDEVPJET GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PJETXL_DEV:
$   SETDEV pjetxl 'PJET
$   ADD_DEV_MODULES = "GDEVPJET GDEVPRN GDEVPCL"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ BIT_DEV:
$   SETDEV bit 'bit_
$   ADD_DEV_MODULES = "GDEVBIT GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ GIFMONO_DEV:
$   SETDEV gifmono 'GIF
$   ADD_DEV_MODULES = "GDEVGIF GDEVPCCM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ GIF8_DEV:
$   SETDEV gif8 'GIF
$   ADD_DEV_MODULES = "GDEVGIF GDEVPCCM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PCXMONO_DEV:
$   SETDEV pcxmono 'pcx_
$   ADD_DEV_MODULES = "GDEVPCX GDEVPCCM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PCXGRAY_DEV:
$   SETDEV pcxgray 'pcx_
$   ADD_DEV_MODULES = "GDEVPCX GDEVPCCM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PCX16_DEV:
$   SETDEV pcx16 'pcx_
$   ADD_DEV_MODULES = "GDEVPCX GDEVPCCM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PCX256_DEV:
$   SETDEV pcx256 'pcx_
$   ADD_DEV_MODULES = "GDEVPCX GDEVPCCM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PCX24B_DEV:
$   SETDEV pcx24b 'pcx_
$   ADD_DEV_MODULES = "GDEVPCX GDEVPCCM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PBM_DEV:
$   SETDEV pbm 'pxm_
$   ADD_DEV_MODULES = "GDEVPBM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PBMRAW_DEV:
$   SETDEV pbmraw 'pxm_
$   ADD_DEV_MODULES = "GDEVPBM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PGM_DEV:
$   SETDEV pgm 'pxm_
$   ADD_DEV_MODULES = "GDEVPBM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PGMRAW_DEV:
$   SETDEV pgmraw 'pxm_
$   ADD_DEV_MODULES = "GDEVPBM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PPM_DEV:
$   SETDEV ppm 'pxm_
$   ADD_DEV_MODULES = "GDEVPBM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ PPMRAW_DEV:
$   SETDEV ppmraw 'pxm_
$   ADD_DEV_MODULES = "GDEVPBM GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ FAXG3_DEV:
$   SETDEV faxg3 'tfax_
$   ADD_DEV_MODULES = "GDEVTFAX GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ FAXG4_DEV:
$   SETDEV faxg4 'tfax_
$   ADD_DEV_MODULES = "GDEVTFAX GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ TIFFG3_DEV:
$   SETDEV tiffg3 'tfax_
$   ADD_DEV_MODULES = "GDEVTFAX GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ TIFFG4_DEV:
$   SETDEV tiffg4 'tfax_
$   ADD_DEV_MODULES = "GDEVTFAX GDEVPRN"
$   GOSUB ADD_DEV_MOD
$   RETURN
$ !
$ ADD_DEV_MOD:
$   II = 0
$   ADD_MORE:
$     DEV_NOW = F$ELEMENT(II, " ", ADD_DEV_MODULES)
$     IF DEV_NOW .EQS. " " THEN RETURN
$     II = II + 1
$     IF F$LOCATE(DEV_NOW, DEV_MODULES) .NE. F$LENGTH(DEV_MODULES) THEN -
       GOTO ADD_MORE
$     DEV_MODULES = DEV_MODULES + DEV_NOW + " "
$     GOTO ADD_MORE
$ !
$ DONE:
$ !
$ DELETE *.DEV;*
$ IF F$SEARCH("DEVS.TR")       .NES. "" THEN DELETE DEVS.TR;*
$ IF F$SEARCH("ECHOGS.EXE")    .NES. "" THEN DELETE ECHOGS.EXE;*
$ IF F$SEARCH("GENCONF.EXE")   .NES. "" THEN DELETE GENCONF.EXE;*
$ IF F$LOGICAL("MODULE_LIST")  .NES. "" THEN CLOSE MODULE_LIST
$ IF F$LOGICAL("OPT_FILE")     .NES. "" THEN CLOSE OPT_FILE
$ IF F$LOGICAL("X11")          .NES. "" THEN DEASSIGN X11
$ !
$ ! ALL DONE
$ EXIT
$ !
$ NO_MODULES:
$ !
$ WSO "Error opening MODULES.LIS. Check this file!"
$ GOTO DONE
