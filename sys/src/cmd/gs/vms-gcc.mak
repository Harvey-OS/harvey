$ !    Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
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
$ ! "makefile" for Ghostscript, VMS / GNU C / DECwindows (X11) configuration.
$ !
$ !
$ ! Execute this command file while in the Ghostscript directory!
$ !
$ !
$ ! To build a debugging configuration, issue the command:
$ !
$ !          $ @VMS-GCC.MAK DEBUG
$ !
$ ! Do not  abbreviate "DEBUG".  To specify an alternate directory for
$ ! GS_LIB_DEFAULT, issue the command:
$ !
$ !          $ @VMS-GCC.MAK directory
$ !
$ ! with "directory" a fully qualified directory name.  "directory" cannot
$ ! be DEBUG (otherwise it will be confused with the DEBUG switch).  Both
$ ! the DEBUG switch and a directory name may be specified on the same
$ ! command line and in either order.
$ !
$ !
$ ! Declare local DCL symbols used by VMS.MAK
$ !
$ CC_QUAL    = ""                      ! Qualifiers for the compile comand
$ L_QUAL     = ""                      ! Qualifiers for the link command
$ CC_COMMAND = "GCC"                   ! Compile command
$ X_INCLUDE  = F$TRNLNM("SYS$COMMON")  ! Used to find path to X11 include files
$ !
$ GS_DOCDIR      = F$ENVIRONMENT("DEFAULT")
$ GS_INIT        = "GS_INIT.PS"
$ GS_LIB_DEFAULT = F$ENVIRONMENT("DEFAULT")
$ FEATURE_DEVS   = "level2.dev"
$ DEVICE_DEVS    = "x11.dev"
$ !
$ !
$ ! Is GNU C about and defined?
$ !
$ IF F$TRNLNM("GNU_CC") .NES. "" THEN GOTO GNU_CC_OKAY
$ WRITE SYS$OUTPUT "GNU_CC logical is undefined.  You must execute the command"
$ WRITE SYS$OUTPUT "file GCC_INSTALL.COM in the GNU CC directory before using"
$ WRITE SYS$OUTPUT "this command file."
$ EXIT
$ !
$ GNU_CC_OKAY:
$ !
$ ! Option file to use when linking genarch.c
$ !
$ SET COMMAND GNU_CC:[000000]GCC.CLD
$ CREATE RTL.OPT
GNU_CC:[000000]GCCLIB.OLB/LIBRARY
SYS$SHARE:VAXCRTL.EXE/SHARE
$ !
$ !
$ ! Now build everything
$ !
$ @VMS.MAK 'P1 'P2 'P3
$ !
$ !
$ ! Cleanup
$ !
$ IF F$SEARCH("RTL.OPT") .NES. "" THEN DELETE RTL.OPT;*
$ !
$ ! ALL DONE
