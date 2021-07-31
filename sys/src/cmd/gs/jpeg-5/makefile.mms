# Makefile for Independent JPEG Group's software

# This makefile is for use with MMS on Digital VMS systems.
# Thanks to Rick Dyson (dyson@iowasp.physics.uiowa.edu)
# and Tim Bell (tbell@netcom.com) for their help.

# Read installation instructions before saying "MMS" !!

# You may need to adjust these cc options:
CFLAGS= $(CFLAGS) /NoDebug /Optimize
# Generally, we recommend defining any configuration symbols in jconfig.h,
# NOT via /Define switches here.
.ifdef ALPHA
OPT=
.else
OPT= ,Sys$Disk:[]MAKVMS.OPT/Option
.endif

# Put here the object file name for the correct system-dependent memory
# manager file.  For Unix this is usually jmemnobs.o, but you may want
# to use jmemansi.o or jmemname.o if you have limited swap space.
SYSDEPMEM= jmemnobs.obj

# End of configurable options.


# source files: JPEG library proper
LIBSOURCES= jcapi.c jccoefct.c jccolor.c jcdctmgr.c jchuff.c jcmainct.c \
        jcmarker.c jcmaster.c jcomapi.c jcparam.c jcprepct.c jcsample.c \
        jdapi.c jdatasrc.c jdatadst.c jdcoefct.c jdcolor.c jddctmgr.c \
        jdhuff.c jdmainct.c jdmarker.c jdmaster.c jdpostct.c jdsample.c \
        jerror.c jutils.c jfdctfst.c jfdctflt.c jfdctint.c jidctfst.c \
        jidctflt.c jidctint.c jidctred.c jquant1.c jquant2.c jdmerge.c \
        jmemmgr.c jmemansi.c jmemname.c jmemnobs.c jmemdos.c
# source files: cjpeg/djpeg applications, also rdjpgcom/wrjpgcom
APPSOURCES= cjpeg.c djpeg.c rdcolmap.c rdppm.c wrppm.c rdgif.c wrgif.c \
        rdtarga.c wrtarga.c rdbmp.c wrbmp.c rdrle.c wrrle.c rdjpgcom.c \
        wrjpgcom.c
SOURCES= $(LIBSOURCES) $(APPSOURCES)
# files included by source files
INCLUDES= jdct.h jerror.h jinclude.h jmemsys.h jmorecfg.h jpegint.h \
        jpeglib.h jversion.h cdjpeg.h cderror.h
# documentation, test, and support files
DOCS= README install.doc usage.doc cjpeg.1 djpeg.1 rdjpgcom.1 wrjpgcom.1 \
        example.c libjpeg.doc structure.doc coderules.doc filelist.doc \
        change.log
MKFILES= configure makefile.auto makefile.ansi makefile.unix makefile.manx \
        makefile.sas makcjpeg.st makdjpeg.st makljpeg.st makefile.bcc \
        makefile.mc6 makefile.dj makefile.mms makefile.vms makvms.opt
CONFIGFILES= jconfig.auto jconfig.manx jconfig.sas jconfig.st jconfig.bcc \
        jconfig.mc6 jconfig.dj jconfig.vms
OTHERFILES= jconfig.doc ckconfig.c ansi2knr.c ansi2knr.1 jmemdosa.asm
TESTFILES= testorig.jpg testimg.ppm testimg.gif testimg.jpg
DISTFILES= $(DOCS) $(MKFILES) $(CONFIGFILES) $(SOURCES) $(INCLUDES) \
        $(OTHERFILES) $(TESTFILES)
# library object files common to compression and decompression
COMOBJECTS= jcomapi.obj jutils.obj jerror.obj jmemmgr.obj $(SYSDEPMEM)
# compression library object files
CLIBOBJECTS= jcapi.obj jcparam.obj jdatadst.obj jcmaster.obj jcmarker.obj \
        jcmainct.obj jcprepct.obj jccoefct.obj jccolor.obj jcsample.obj \
        jchuff.obj jcdctmgr.obj jfdctfst.obj jfdctflt.obj jfdctint.obj
# decompression library object files
DLIBOBJECTS= jdapi.obj jdatasrc.obj jdmaster.obj jdmarker.obj jdmainct.obj \
        jdcoefct.obj jdpostct.obj jddctmgr.obj jidctfst.obj jidctflt.obj \
        jidctint.obj jidctred.obj jdhuff.obj jdsample.obj jdcolor.obj \
        jquant1.obj jquant2.obj jdmerge.obj
# These objectfiles are included in libjpeg.olb
LIBOBJECTS= $(CLIBOBJECTS) $(DLIBOBJECTS) $(COMOBJECTS)
# object files for cjpeg and djpeg applications (excluding library files)
COBJECTS= cjpeg.obj rdppm.obj rdgif.obj rdtarga.obj rdrle.obj rdbmp.obj
DOBJECTS= djpeg.obj wrppm.obj wrgif.obj wrtarga.obj wrrle.obj wrbmp.obj \
        rdcolmap.obj
# objectfile lists with commas --- what a crock
COBJLIST= cjpeg.obj,rdppm.obj,rdgif.obj,rdtarga.obj,rdrle.obj,rdbmp.obj
DOBJLIST= djpeg.obj,wrppm.obj,wrgif.obj,wrtarga.obj,wrrle.obj,wrbmp.obj,\
          rdcolmap.obj
LIBOBJLIST= jcapi.obj,jcparam.obj,jdatadst.obj,jcmaster.obj,jcmarker.obj,\
          jcmainct.obj,jcprepct.obj,jccoefct.obj,jccolor.obj,jcsample.obj,\
          jchuff.obj,jcdctmgr.obj,jfdctfst.obj,jfdctflt.obj,jfdctint.obj,\
          jdapi.obj,jdatasrc.obj,jdmaster.obj,jdmarker.obj,jdmainct.obj,\
          jdcoefct.obj,jdpostct.obj,jddctmgr.obj,jidctfst.obj,jidctflt.obj,\
          jidctint.obj,jidctred.obj,jdhuff.obj,jdsample.obj,jdcolor.obj,\
          jquant1.obj,jquant2.obj,jdmerge.obj,jcomapi.obj,jutils.obj,\
          jerror.obj,jmemmgr.obj,$(SYSDEPMEM)


.first
	@- Define /NoLog Sys Sys$Library

ALL : libjpeg.olb cjpeg.exe djpeg.exe rdjpgcom.exe wrjpgcom.exe
	@ Continue

libjpeg.olb : $(LIBOBJECTS)
	Library /Create libjpeg.olb $(LIBOBJLIST)

cjpeg.exe : $(COBJECTS) libjpeg.olb
	$(LINK) $(LFLAGS) /Executable = cjpeg.exe $(COBJLIST),libjpeg.olb/Library$(OPT)

djpeg.exe : $(DOBJECTS) libjpeg.olb
	$(LINK) $(LFLAGS) /Executable = djpeg.exe $(DOBJLIST),libjpeg.olb/Library$(OPT)

rdjpgcom.exe : rdjpgcom.obj
	$(LINK) $(LFLAGS) /Executable = rdjpgcom.exe rdjpgcom.obj$(OPT)

wrjpgcom.exe : wrjpgcom.obj
	$(LINK) $(LFLAGS) /Executable = wrjpgcom.exe wrjpgcom.obj$(OPT)

jconfig.h : jconfig.vms
	@- Copy jconfig.vms jconfig.h

clean :
	@- Set Protection = Owner:RWED *.*;-1
	@- Set Protection = Owner:RWED *.OBJ
	- Purge /NoLog /NoConfirm *.*
	- Delete /NoLog /NoConfirm *.OBJ;

test : cjpeg.exe djpeg.exe
	mcr sys$disk:[]djpeg -dct int -ppm -outfile testout.ppm testorig.jpg
	mcr sys$disk:[]djpeg -dct int -gif -outfile testout.gif testorig.jpg
	mcr sys$disk:[]cjpeg -dct int      -outfile testout.jpg testimg.ppm
	- Backup /Compare/Log	  testimg.ppm testout.ppm
	- Backup /Compare/Log	  testimg.gif testout.gif
	- Backup /Compare/Log	  testimg.jpg testout.jpg


jcapi.obj : jcapi.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jccoefct.obj : jccoefct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jccolor.obj : jccolor.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcdctmgr.obj : jcdctmgr.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jchuff.obj : jchuff.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcmainct.obj : jcmainct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcmarker.obj : jcmarker.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcmaster.obj : jcmaster.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcomapi.obj : jcomapi.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcparam.obj : jcparam.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcprepct.obj : jcprepct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jcsample.obj : jcsample.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdapi.obj : jdapi.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdatasrc.obj : jdatasrc.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h
jdatadst.obj : jdatadst.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h
jdcoefct.obj : jdcoefct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdcolor.obj : jdcolor.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jddctmgr.obj : jddctmgr.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jdhuff.obj : jdhuff.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdmainct.obj : jdmainct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdmarker.obj : jdmarker.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdmaster.obj : jdmaster.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdpostct.obj : jdpostct.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdsample.obj : jdsample.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jerror.obj : jerror.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jversion.h jerror.h
jutils.obj : jutils.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jfdctfst.obj : jfdctfst.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jfdctflt.obj : jfdctflt.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jfdctint.obj : jfdctint.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jidctfst.obj : jidctfst.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jidctflt.obj : jidctflt.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jidctint.obj : jidctint.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jidctred.obj : jidctred.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jdct.h
jquant1.obj : jquant1.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jquant2.obj : jquant2.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jdmerge.obj : jdmerge.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h
jmemmgr.obj : jmemmgr.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
jmemansi.obj : jmemansi.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
jmemname.obj : jmemname.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
jmemnobs.obj : jmemnobs.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
jmemdos.obj : jmemdos.c jinclude.h jconfig.h jpeglib.h jmorecfg.h jpegint.h jerror.h jmemsys.h
cjpeg.obj : cjpeg.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h jversion.h
djpeg.obj : djpeg.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h jversion.h
rdcolmap.obj : rdcolmap.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdppm.obj : rdppm.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
wrppm.obj : wrppm.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdgif.obj : rdgif.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
wrgif.obj : wrgif.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdtarga.obj : rdtarga.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
wrtarga.obj : wrtarga.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdbmp.obj : rdbmp.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
wrbmp.obj : wrbmp.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdrle.obj : rdrle.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
wrrle.obj : wrrle.c cdjpeg.h jinclude.h jconfig.h jpeglib.h jmorecfg.h jerror.h cderror.h
rdjpgcom.obj : rdjpgcom.c jinclude.h jconfig.h
wrjpgcom.obj : wrjpgcom.c jinclude.h jconfig.h
