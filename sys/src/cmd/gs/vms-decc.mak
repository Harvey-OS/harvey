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
$ VERIFY = 'F$VERIFY(F$INTEGER(F$LOGICAL("GS_VERIFY")))'
$ !
$ ! "makefile" for Ghostscript, VMS / DEC C / DECwindows (X11) configuration,
$ !	on AXP or VAX
$ !
$ !
$ ! Execute this command file while in the Ghostscript directory!
$ !
$ !
$ ! To build a debugging configuration, issue the command:
$ !
$ !          $ @VMS-DECC.MAK DEBUG
$ !
$ ! Do not  abbreviate "DEBUG".  To specify an alternate file for
$ ! GS_LIB_DEFAULT, issue the command:
$ !
$ !          $ @VMS-DECC.MAK file
$ !
$ ! with "file" a fully qualified filename.  "file" cannot
$ ! be DEBUG (otherwise it will be confused with the DEBUG switch).  Both
$ ! the DEBUG switch and a directory name may be specified on the same
$ ! command line and in either order.
$ !
$ !
$ ! Declare local DCL symbols used by VMS.MAK
$ !
$ CC_QUAL    = "/PREFIX=ALL/NESTED_INCLUDE=PRIMARY"     ! Quals. for compile, US
$ !CC_QUAL   = "/PREFIX=ALL/NESTED_INCLUDE=PRIMARY/DEFINE=(A4)"    ! Europe
$ IF F$GETSYI("HW_MODEL") .GE. 1024 THEN CC_QUAL = CC_QUAL + "/TIE"
$ L_QUAL     = ""         ! Qualifiers for the link command
$ CC_COMMAND = "CC"       ! Compile command
$ X_INCLUDE  = ""         ! Used with Gnu C to find path to X11 include files
$ !
$ GS_DOCDIR      = F$ENVIRONMENT("DEFAULT")
$ GS_INIT        = "GS_Init.PS"
$ GS_LIB_DEFAULT = F$ENVIRONMENT("DEFAULT")
$ FEATURE_DEVS   = "level2.dev"
$ DEVICE_DEVS    = "x11.dev"
$ DEVICE_DEVS3 = "deskjet.dev djet500.dev laserjet.dev ljetplus.dev ljet2p.dev ljet3.dev"
$ DEVICE_DEVS4 = "cdeskjet.dev cdjcolor.dev cdjmono.dev cdj550.dev pj.dev pjxl.dev pjxl300.dev"
$ DEVICE_DEVS6 = "bj10e.dev bj200.dev"
$ DEVICE_DEVS8 = "gifmono.dev gif8.dev pcxmono.dev pcxgray.dev pcx16.dev pcx256.dev pcx24b.dev"
$ DEVICE_DEVS9 = "pbm.dev pbmraw.dev pgm.dev pgmraw.dev ppm.dev ppmraw.dev bit.dev faxg3.dev tiffg3.dev"
$ !
$ !
$ ! Give ourself a healthy search list for C include files
$ !
$ if f$trnlnm("DECC$LIBRARY_INCLUDE").nes.""
$  then
$   DEFINE DECC$USER_INCLUDE 'F$ENVIRONMENT("DEFAULT"),DECW$INCLUDE,DECC$LIBRARY_INCLUDE,SYS$LIBRARY
$   DEFINE DECC$SYSTEM_INCLUDE 'F$ENVIRONMENT("DEFAULT"),DECW$INCLUDE,DECC$LIBRARY_INCLUDE,SYS$LIBRARY
$   DEFINE SYS DECC$LIBRARY_INCLUDE,SYS$LIBRARY
$  else
$   DEFINE DECC$USER_INCLUDE 'F$ENVIRONMENT("DEFAULT"),DECW$INCLUDE,SYS$LIBRARY
$   DEFINE DECC$SYSTEM_INCLUDE 'F$ENVIRONMENT("DEFAULT"),DECW$INCLUDE,SYS$LIBRARY
$   DEFINE SYS SYS$LIBRARY
$ endif
$ !
$ !
$ ! Empty option file to use when linking genarch.c
$ !
$ CREATE RTL.OPT
$ !
$ !
$ ! Now build everything
$ !
$ @VMS.MAK 'P1 'P2 'P3 'P4 'P5 'P6 'P7 'P8
$ !
$ !
$ ! Cleanup
$ !
$ IF F$LOGICAL("DECC$USER_INCLUDE")   .NES. "" THEN DEASSIGN DECC$USER_INCLUDE
$ IF F$LOGICAL("DECC$SYSTEM_INCLUDE") .NES. "" THEN DEASSIGN DECC$SYSTEM_INCLUDE
$ IF F$LOGICAL("SYS")          .NES. "" THEN DEASSIGN SYS
$ IF F$SEARCH("RTL.OPT")       .NES. "" THEN DELETE RTL.OPT;*
$ !
$ ! ALL DONE
$ !
$ VERIFY = F$VERIFY(VERIFY)
