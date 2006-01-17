#    Copyright (C) 1989, 2000-2004 artofcode LLC. All rights reserved.
# 
# This software is provided AS-IS with no warranty, either express or
# implied.
# 
# This software is distributed under license and may not be copied,
# modified or distributed except as expressly authorized under the terms
# of the license contained in the file LICENSE in this distribution.
# 
# For more information about licensing, please refer to
# http://www.ghostscript.com/licensing/. For information on
# commercial licensing, go to http://www.artifex.com/licensing/ or
# contact Artifex Software, Inc., 101 Lucas Valley Road #110,
# San Rafael, CA  94903, U.S.A., +1(415)492-9861.

# $Id: devs.mak,v 1.140 2005/10/18 09:05:58 leonardo Exp $
# makefile for Aladdin's device drivers.

# Define the name of this makefile.
DEVS_MAK=$(GLSRC)devs.mak

# All device drivers depend on the following:
GDEVH=$(gserrors_h) $(gx_h) $(gxdevice_h)
GDEV=$(AK) $(ECHOGS_XE) $(GDEVH)

###### --------------------------- Overview -------------------------- ######

# It is possible to build Ghostscript with an arbitrary collection of device
# drivers, although some drivers are supported only on a subset of the
# target platforms.

# The catalog in this file, devs.mak, lists all the drivers that were
# written by Aladdin, or by people working closely with Aladdin, and for
# which Aladdin is willing to take problem reports (although since
# Ghostscript is provided with NO WARRANTY and NO SUPPORT, we can't promise
# that we'll solve your problem).  Another file, contrib.mak, lists all the
# drivers contributed by other people that are distributed by Aladdin with
# Ghostscript.  Note in particular that all drivers for color inkjets and
# other non-PostScript-capable color printers are in contrib.mak.

# If you haven't configured Ghostscript before, or if you want to add a
# driver that that isn't included in the catalogs (for which you have the
# source code), we suggest you skip to the "End of catalog" below and read
# the documentation there before continuing.

###### --------------------------- Catalog -------------------------- ######

# MS-DOS displays (note: not usable with Desqview/X):
#   MS-DOS EGA and VGA:
#	ega	EGA (640x350, 16-color)
#	vga	VGA (640x480, 16-color)
#   MS-DOS SuperVGA:
# *	ali	SuperVGA using Avance Logic Inc. chipset, 256-color modes
# *	atiw	ATI Wonder SuperVGA, 256-color modes
# *	cirr	SuperVGA using Cirrus Logic CL-GD54XX chips, 256-color modes
# *	s3vga	SuperVGA using S3 86C911 chip (e.g., Diamond Stealth board)
#	svga16	Generic SuperVGA in 800x600, 16-color mode
# *	tseng	SuperVGA using Tseng Labs ET3000/4000 chips, 256-color modes
# *	tvga	SuperVGA using Trident chipset, 256-color modes
#   ****** NOTE: The vesa device does not work with the Watcom (32-bit MS-DOS)
#   ****** compiler or executable.
#	vesa	SuperVGA with VESA standard API driver
# Other displays:
#	display   For use on any platform that supports DLLs
#   MS Windows:
#	mswindll  Microsoft Windows 3.1 DLL  [MS Windows only]
#	mswinprn  Microsoft Windows 3.0, 3.1 DDB printer  [MS Windows only]
#	mswinpr2  Microsoft Windows 3.0, 3.1 DIB printer  [MS Windows only]
#   OS/2:
# *	os2pm	OS/2 Presentation Manager   [OS/2 only]
# *	os2dll	OS/2 DLL bitmap             [OS/2 only]
# *	os2prn	OS/2 printer                [OS/2 only]
#   Unix and VMS:
#   ****** NOTE: For direct frame buffer addressing under SCO Unix or Xenix,
#   ****** edit the definition of EGAVGA below.
# *	lvga256  Linux vgalib, 256-color VGA modes  [Linux only]
# +	vgalib	Linux vgalib, 16-color VGA modes   [Linux only]
#	x11	X Windows version 11, release >=4   [Unix and VMS only]
#	x11alpha  X Windows masquerading as a device with alpha capability
#	x11cmyk  X Windows masquerading as a 1-bit-per-plane CMYK device
#	x11cmyk2  X Windows as a 2-bit-per-plane CMYK device
#	x11cmyk4  X Windows as a 4-bit-per-plane CMYK device
#	x11cmyk8  X Windows as an 8-bit-per-plane CMYK device
#	x11gray2  X Windows as a 2-bit gray-scale device
#	x11gray4  X Windows as a 4-bit gray-scale device
#	x11mono  X Windows masquerading as a black-and-white device
#	x11rg16x  X Windows with G5/B5/R6 pixel layout for testing.
#	x11rg32x  X Windows with G11/B10/R11 pixel layout for testing.
# Printers:
# +	atx23	Practical Automation ATX-23 label printer
# +	atx24	Practical Automation ATX-24 label printer
# +	atx38	Practical Automation ATX-38 label printer
# +	deskjet  H-P DeskJet and DeskJet Plus
#	djet500  H-P DeskJet 500; use -r600 for DJ 600 series
# +	fs600	Kyocera FS-600 (600 dpi)
# +	laserjet  H-P LaserJet
# +	ljet2p	H-P LaserJet IId/IIp/III* with TIFF compression
# +	ljet3	H-P LaserJet III* with Delta Row compression
# +	ljet3d	H-P LaserJet IIID with duplex capability
# +	ljet4	H-P LaserJet 4 (defaults to 600 dpi)
# +	ljet4d	H-P LaserJet 4 (defaults to 600 dpi) with duplex
# +	ljetplus  H-P LaserJet Plus
#	lj5mono  H-P LaserJet 5 & 6 family (PCL XL), bitmap:
#		see below for restrictions & advice
#	lj5gray  H-P LaserJet 5 & 6 family, gray-scale bitmap;
#		see below for restrictions & advice
# *	lp2563	H-P 2563B line printer
# *	oce9050  OCE 9050 printer
#	(pxlmono) H-P black-and-white PCL XL printers (LaserJet 5 and 6 family)
#	(pxlcolor) H-P color PCL XL printers (e.g. Color LaserJet 4500)
# Fax file format:
#   ****** NOTE: all of these drivers normally adjust the page size to match
#   ****** one of the three CCITT standard sizes (U.S. letter with A4 width,
#   ****** A4, or B4).  To suppress this, use -dAdjustWidth=0.
#	faxg3	Group 3 fax, with EOLs but no header or EOD
#	faxg32d  Group 3 2-D fax, with EOLs but no header or EOD
#	faxg4	Group 4 fax, with EOLs but no header or EOD
#	tiffcrle  TIFF "CCITT RLE 1-dim" (= Group 3 fax with no EOLs)
#	tiffg3	TIFF Group 3 fax (with EOLs)
#	tiffg32d  TIFF Group 3 2-D fax
#	tiffg4	TIFF Group 4 fax
# High-level file formats:
#	epswrite  EPS output (like PostScript Distillery)
#	pdfwrite  PDF output (like Adobe Acrobat Distiller)
#	pswrite  PostScript output (like PostScript Distillery)
#	pxlmono  Black-and-white PCL XL
#	pxlcolor  Color PCL XL
# Other raster file formats and devices:
#	bit	Plain bits, monochrome
#	bitrgb	Plain bits, RGB
#	bitcmyk  Plain bits, CMYK
#	bmpmono	Monochrome MS Windows .BMP file format
#	bmpgray	8-bit gray .BMP file format
#	bmpsep1	Separated 1-bit CMYK .BMP file format, primarily for testing
#	bmpsep8	Separated 8-bit CMYK .BMP file format, primarily for testing
#	bmp16	4-bit (EGA/VGA) .BMP file format
#	bmp256	8-bit (256-color) .BMP file format
#	bmp16m	24-bit .BMP file format
#	bmp32b  32-bit pseudo-.BMP file format
#	cgmmono  Monochrome (black-and-white) CGM -- LOW LEVEL OUTPUT ONLY
#	cgm8	8-bit (256-color) CGM -- DITTO
#	cgm24	24-bit color CGM -- DITTO
#	jpeg	JPEG format, RGB output
#	jpeggray  JPEG format, gray output
#	jpegcmyk  JPEG format, cmyk output
#	miff24	ImageMagick MIFF format, 24-bit direct color, RLE compressed
#	pam	Portable Arbitrary Map file format
#	pcxmono	PCX file format, monochrome (1-bit black and white)
#	pcxgray	PCX file format, 8-bit gray scale
#	pcx16	PCX file format, 4-bit planar (EGA/VGA) color
#	pcx256	PCX file format, 8-bit chunky color
#	pcx24b	PCX file format, 24-bit color (3 8-bit planes)
#	pcxcmyk PCX file format, 4-bit chunky CMYK color
#	pbm	Portable Bitmap (plain format)
#	pbmraw	Portable Bitmap (raw format)
#	pgm	Portable Graymap (plain format)
#	pgmraw	Portable Graymap (raw format)
#	pgnm	Portable Graymap (plain format), optimizing to PBM if possible
#	pgnmraw	Portable Graymap (raw format), optimizing to PBM if possible
#	pnm	Portable Pixmap (plain format) (RGB), optimizing to PGM or PBM
#		 if possible
#	pnmraw	Portable Pixmap (raw format) (RGB), optimizing to PGM or PBM
#		 if possible
#	ppm	Portable Pixmap (plain format) (RGB)
#	ppmraw	Portable Pixmap (raw format) (RGB)
#	pkm	Portable inKmap (plain format) (4-bit CMYK => RGB)
#	pkmraw	Portable inKmap (raw format) (4-bit CMYK => RGB)
#	pksm	Portable Separated map (plain format) (4-bit CMYK => 4 pages)
#	pksmraw	Portable Separated map (raw format) (4-bit CMYK => 4 pages)
# *	plan9bm  Plan 9 bitmap format
#	pngmono	Monochrome Portable Network Graphics (PNG)
#	pnggray	8-bit gray Portable Network Graphics (PNG)
#	png16	4-bit color Portable Network Graphics (PNG)
#	png256	8-bit color Portable Network Graphics (PNG)
#	png16m	24-bit color Portable Network Graphics (PNG)
#	pngalpha 32-bit RGBA color Portable Network Graphics (PNG)
#	psmono	PostScript (Level 1) monochrome image
#	psgray	PostScript (Level 1) 8-bit gray image
#	psrgb	PostScript (Level 2) 24-bit color image
#	tiffgray  TIFF 8-bit gray, no compression
#	tiff12nc  TIFF 12-bit RGB, no compression
#	tiff24nc  TIFF 24-bit RGB, no compression (NeXT standard format)
#	tiff32nc  TIFF 32-bit CMYK
#	tiffsep   Creates tiffgray for each colorant plus a CMYK composite
#	tifflzw  TIFF LZW (tag = 5) (monochrome)
#	tiffpack  TIFF PackBits (tag = 32773) (monochrome)

# Note that MS Windows-specific drivers are defined in pcwin.mak, not here,
# because they have special compilation requirements that require defining
# parameter macros not relevant to other platforms; the OS/2-specific
# drivers are there too, because they share some definitions.

# User-contributed drivers marked with * require hardware or software
# that is not available to Aladdin Enterprises.  Please contact the
# original contributors, not Aladdin Enterprises, if you have questions.
# Contact information appears in the driver entry below.
#
# Drivers marked with a + are maintained by Aladdin Enterprises with
# the assistance of users, since Aladdin Enterprises doesn't have access to
# the hardware for these either.

# If you add drivers, it would be nice if you kept each list
# in alphabetical order.

###### ----------------------- End of catalog ----------------------- ######

# As noted in gs.mak, DEVICE_DEVS and DEVICE_DEVS1..15 select the devices
# that should be included in a given configuration.  By convention, these
# are used as follows.  Each of these must be limited to about 6 devices
# so as not to overflow the 120 character limit on MS-DOS command lines.
#	DEVICE_DEVS - the default device, and any display devices.
#	DEVICE_DEVS1 - additional display devices if needed.
#	DEVICE_DEVS2 - dot matrix printers.
#	DEVICE_DEVS3 - H-P monochrome printers.
#	DEVICE_DEVS4 - H-P color printers.
#	DEVICE_DEVS5 - additional inkjet printers if needed.
#	DEVICE_DEVS6 - other ink-jet and laser printers.
#	DEVICE_DEVS7 - fax file formats.
#	DEVICE_DEVS8 - PCX file formats.
#	DEVICE_DEVS9 - PBM/PGM/PPM file formats.
#	DEVICE_DEVS10 - black-and-white TIFF file formats.
#	DEVICE_DEVS11 - BMP and color TIFF file formats.
#	DEVICE_DEVS12 - PostScript image and 'bit' file formats.
#	DEVICE_DEVS13 - PNG file formats.
#	DEVICE_DEVS14 - CGM, JPEG, and MIFF file formats.
#	DEVICE_DEVS15 - high-level (PostScript and PDF) file formats.
#	DEVICE_DEVS16 - additional high-level and utility drivers
#	DEVICE_DEVS17 - (overflow for PC platforms)
#	DEVICE_DEVS18 - (ditto)
#	DEVICE_DEVS19 - (ditto)
#	DEVICE_DEVS20 - (ditto)
# Feel free to disregard this convention if it gets in your way.

# If you want to add a new device driver, the examples below should be
# enough of a guide to the correct form for the makefile rules.
# Note that all drivers other than displays must include page.dev in their
# dependencies and use $(SETPDEV) rather than $(SETDEV) in their rule bodies.

# "Printer" drivers depend on the following:
PDEVH=$(AK) $(gdevprn_h)

# Define the header files for device drivers.  Every header file used by
# more than one device driver family must be listed here.
gdev8bcm_h=$(GLSRC)gdev8bcm.h
gdevcbjc_h=$(GLSRC)gdevcbjc.h $(stream_h)
gdevdcrd_h=$(GLSRC)gdevdcrd.h
gsequivc_h=$(GLSRC)gsequivc.h
gdevdevn_h=$(GLSRC)gdevdevn.h $(gsequivc_h)
gdevpccm_h=$(GLSRC)gdevpccm.h
gdevpcfb_h=$(GLSRC)gdevpcfb.h $(dos__h)
gdevpcl_h=$(GLSRC)gdevpcl.h
gdevpsu_h=$(GLSRC)gdevpsu.h
gdevsvga_h=$(GLSRC)gdevsvga.h
# Out of order
gdevdljm_h=$(GLSRC)gdevdljm.h $(gdevpcl_h)

###### ----------------------- Device support ----------------------- ######

# Implement dynamic color management for 8-bit mapped color displays.
$(GLOBJ)gdev8bcm.$(OBJ) : $(GLSRC)gdev8bcm.c $(AK)\
 $(gx_h) $(gxdevice_h) $(gdev8bcm_h)
	$(GLCC) $(GLO_)gdev8bcm.$(OBJ) $(C_) $(GLSRC)gdev8bcm.c

# PC display color mapping
$(GLOBJ)gdevpccm.$(OBJ) : $(GLSRC)gdevpccm.c $(AK)\
 $(gx_h) $(gsmatrix_h) $(gxdevice_h) $(gdevpccm_h)
	$(GLCC) $(GLO_)gdevpccm.$(OBJ) $(C_) $(GLSRC)gdevpccm.c

# Generate Canon BJC command sequences.
$(GLOBJ)gdevcbjc.$(OBJ) : $(GLSRC)gdevcbjc.c $(AK)\
 $(std_h) $(stream_h) $(gdevcbjc_h)
	$(GLCC) $(GLO_)gdevcbjc.$(OBJ) $(C_) $(GLSRC)gdevcbjc.c

# Provide a sample device CRD.
$(GLOBJ)gdevdcrd.$(OBJ) : $(GLSRC)gdevdcrd.c $(AK)\
 $(math__h) $(memory__h) $(string__h)\
 $(gscrd_h) $(gscrdp_h) $(gserrors_h) $(gsparam_h) $(gscspace_h)\
 $(gx_h) $(gxdevcli_h) $(gdevdcrd_h)
	$(GLCC) $(GLO_)gdevdcrd.$(OBJ) $(C_) $(GLSRC)gdevdcrd.c

# Support for writing PostScript (high- or low-level).
$(GLOBJ)gdevpsu.$(OBJ) : $(GLSRC)gdevpsu.c $(GX) $(GDEV) $(math__h) $(time__h)\
 $(stat__h) $(unistd__h)\
 $(gdevpsu_h) $(gscdefs_h) $(gxdevice_h)\
 $(spprint_h) $(stream_h)
	$(GLCC) $(GLO_)gdevpsu.$(OBJ) $(C_) $(GLSRC)gdevpsu.c

###### ------------------- MS-DOS display devices ------------------- ######

# There are really only three drivers: an EGA/VGA driver (4 bit-planes,
# plane-addressed), a SuperVGA driver (8 bit-planes, byte addressed),
# and a special driver for the S3 chip.

### ----------------------- EGA and VGA displays ----------------------- ###

# The shared MS-DOS makefile defines PCFBASM as either gdevegaa.$(OBJ)
# or an empty string.

$(GLOBJ)gdevegaa.$(OBJ) : $(GLSRC)gdevegaa.asm
	$(GLCC) $(GLO_)gdevegaa.$(OBJ) $(C_) $(GLSRC)gdevegaa.c

EGAVGA_DOS=$(GLOBJ)gdevevga.$(OBJ) $(GLOBJ)gdevpcfb.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ) $(PCFBASM)
EGAVGA_SCO=$(GLOBJ)gdevsco.$(OBJ) $(GLOBJ)gdevpcfb.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ) $(PCFBASM)
# NOTE: for direct frame buffer addressing under SCO Unix or Xenix,
# change DOS to SCO in the following line.  Also, since SCO's /bin/as
# does not support the "out" instructions, you must build the GNU
# assembler and have it on your path as "as".
EGAVGA=$(EGAVGA_DOS)

#**************** $(CCD) gdevevga.c
$(GLOBJ)gdevevga.$(OBJ) : $(GLSRC)gdevevga.c $(GDEV) $(memory__h) $(gdevpcfb_h)
	$(GLCC) $(GLO_)gdevevga.$(OBJ) $(C_) $(GLSRC)gdevevga.c

$(GLOBJ)gdevsco.$(OBJ) : $(GLSRC)gdevsco.c $(GDEV) $(memory__h) $(gdevpcfb_h)
	$(GLCC) $(GLO_)gdevsco.$(OBJ) $(C_) $(GLSRC)gdevsco.c

# Common code for MS-DOS and SCO.
#**************** $(CCD) gdevpcfb.c
$(GLOBJ)gdevpcfb.$(OBJ) : $(GLSRC)gdevpcfb.c $(GDEV) $(memory__h) $(gconfigv_h)\
 $(gdevpccm_h) $(gdevpcfb_h) $(gsparam_h)
	$(GLCC) $(GLO_)gdevpcfb.$(OBJ) $(C_) $(GLSRC)gdevpcfb.c

# The EGA/VGA family includes EGA and VGA.  Many SuperVGAs in 800x600,
# 16-color mode can share the same code; see the next section below.
$(DD)ega.dev : $(DEVS_MAK) $(EGAVGA)
	$(SETDEV) $(DD)ega $(EGAVGA)

$(DD)vga.dev : $(DEVS_MAK) $(EGAVGA)
	$(SETDEV) $(DD)vga $(EGAVGA)

### ------------------------- SuperVGA displays ------------------------ ###

# SuperVGA displays in 16-color, 800x600 mode are really just slightly
# glorified VGA's, so we can handle them all with a single driver.
# The way to select them on the command line is with
#	-sDEVICE=svga16 -dDisplayMode=NNN
# where NNN is the display mode in decimal.  See Use.htm for the modes
# for some popular display chipsets.

$(DD)svga16.dev : $(DEVS_MAK) $(EGAVGA)
	$(SETDEV) $(DD)svga16 $(EGAVGA)

# More capable SuperVGAs have a wide variety of slightly differing
# interfaces, so we need a separate driver for each one.

SVGA=$(GLOBJ)gdevsvga.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ) $(PCFBASM)

#**************** $(CCD) gdevsvga.c
$(GLOBJ)gdevsvga.$(OBJ) : $(GLSRC)gdevsvga.c $(GDEV) $(memory__h) $(gconfigv_h)\
 $(gsparam_h) $(gxarith_h) $(gdevpccm_h) $(gdevpcfb_h) $(gdevsvga_h)
	$(GLCC) $(GLO_)gdevsvga.$(OBJ) $(C_) $(GLSRC)gdevsvga.c

# The SuperVGA family includes: Avance Logic Inc., ATI Wonder, S3,
# Trident, Tseng ET3000/4000, and VESA.

$(DD)ali.dev : $(DEVS_MAK) $(SVGA)
	$(SETDEV) $(DD)ali $(SVGA)

$(DD)atiw.dev : $(DEVS_MAK) $(SVGA)
	$(SETDEV) $(DD)atiw $(SVGA)

$(DD)cirr.dev : $(DEVS_MAK) $(SVGA)
	$(SETDEV) $(DD)cirr $(SVGA)

$(DD)tseng.dev : $(DEVS_MAK) $(SVGA)
	$(SETDEV) $(DD)tseng $(SVGA)

$(DD)tvga.dev : $(DEVS_MAK) $(SVGA)
	$(SETDEV) $(DD)tvga $(SVGA)

$(DD)vesa.dev : $(DEVS_MAK) $(SVGA)
	$(SETDEV) $(DD)vesa $(SVGA)

# The S3 driver doesn't share much code with the others.

s3vga_=$(GLOBJ)gdevs3ga.$(OBJ) $(GLOBJ)gdevsvga.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ)
$(DD)s3vga.dev : $(DEVS_MAK) $(SVGA) $(s3vga_)
	$(SETDEV) $(DD)s3vga $(SVGA)
	$(ADDMOD) $(DD)s3vga -obj $(s3vga_)

#**************** $(CCD) gdevs3ga.c
$(GLOBJ)gdevs3ga.$(OBJ) : $(GLSRC)gdevs3ga.c $(GDEV) $(gdevpcfb_h) $(gdevsvga_h)
	$(GLCC) $(GLO_)gdevs3ga.$(OBJ) $(C_) $(GLSRC)gdevs3ga.c

###### ----------------------- Other displays ------------------------ ######

### ------------------ Display device for DLL platforms ----------------- ###

display_=$(GLOBJ)gdevdsp.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ) $(GLOBJ)gdevdevn.$(OBJ) \
	 $(GLOBJ)gsequivc.$(OBJ) $(GLOBJ)gdevdcrd.$(OBJ)
$(DD)display.dev : $(display_)
	$(SETDEV) $(DD)display $(display_)

$(GLOBJ)gdevdsp.$(OBJ) : $(GLSRC)gdevdsp.c $(string__h)\
 $(gp_h) $(gpcheck_h) $(gdevpccm_h) $(gsparam_h) $(gsdevice_h)\
 $(GDEVH) $(gxdevmem_h) $(gdevdevn_h) $(gsequivc_h) $(gdevdsp_h) $(gdevdsp2_h)
	$(GLCC) $(GLO_)gdevdsp.$(OBJ) $(C_) $(GLSRC)gdevdsp.c


### ---------------------- Linux PC with vgalib ------------------------- ###
### Note: these drivers were contributed by users.                        ###
### For questions about the lvga256 driver, please contact                ###
###       Ludger Kunz (ludger.kunz@fernuni-hagen.de).                     ###
### For questions about the vgalib driver, please contact                 ###
###       Erik Talvola (talvola@gnu.ai.mit.edu).                          ###
### Note that the vgalib device supports only 16-color VGA modes.         ###

lvga256_=$(GLOBJ)gdevl256.$(OBJ)
$(DD)lvga256.dev : $(DEVS_MAK) $(lvga256_)
	$(SETDEV) $(DD)lvga256 $(lvga256_)
	$(ADDMOD) $(DD)lvga256 -lib vga vgagl

$(GLOBJ)gdevl256.$(OBJ) : $(GLSRC)gdevl256.c $(GDEV) $(memory__h)
	$(GLCC) $(GLO_)gdevl256.$(OBJ) $(C_) $(GLSRC)gdevl256.c

vgalib_=$(GLOBJ)gdevvglb.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ)
$(DD)vgalib.dev : $(DEVS_MAK) $(vgalib_)
	$(SETDEV2) $(DD)vgalib $(vgalib_)
	$(ADDMOD) $(DD)vgalib -lib vga

$(GLOBJ)gdevvglb.$(OBJ) : $(GLSRC)gdevvglb.c $(GDEV) $(gdevpccm_h) $(gsparam_h)
	$(GLCC) $(GLO_)gdevvglb.$(OBJ) $(C_) $(GLSRC)gdevvglb.c

### -------------------------- The X11 device -------------------------- ###

# Please note that Aladdin Enterprises does not support Ghostview.
# For more information about Ghostview, please contact Tim Theisen
# (ghostview@cs.wisc.edu).

x__h=$(GLSRC)x_.h
gdevxcmp_h=$(GLSRC)gdevxcmp.h
gdevx_h=$(GLSRC)gdevx.h $(gdevbbox_h) $(gdevxcmp_h)

# See the main makefile for the definition of XLIBDIRS and XLIBS.
x11_=$(GLOBJ)gdevx.$(OBJ) $(GLOBJ)gdevxcmp.$(OBJ) $(GLOBJ)gdevxini.$(OBJ)\
 $(GLOBJ)gdevxres.$(OBJ) $(GLOBJ)gdevxxf.$(OBJ)\
 $(GLOBJ)gdevemap.$(OBJ) $(GLOBJ)gsparamx.$(OBJ)
$(DD)x11_.dev : $(DEVS_MAK) $(x11_) $(GLD)bboxutil.dev
	$(SETMOD) $(DD)x11_ $(x11_)
	$(ADDMOD) $(DD)x11_ -link $(XLIBDIRS)
	$(ADDMOD) $(DD)x11_ -lib $(XLIBS)
	$(ADDMOD) $(DD)x11_ -include $(GLD)bboxutil

$(DD)x11.dev : $(DEVS_MAK) $(DD)x11_.dev
	$(SETDEV2) $(DD)x11 -include $(DD)x11_

# See the main makefile for the definition of XINCLUDE.
GDEVX=$(GDEV) $(x__h) $(gdevx_h) $(TOP_MAKEFILES)
$(GLOBJ)gdevx.$(OBJ) : $(GLSRC)gdevx.c $(GDEVX) $(math__h) $(memory__h)\
 $(gscoord_h) $(gsdevice_h) $(gsiparm2_h) $(gsmatrix_h) $(gsparam_h)\
 $(gxdevmem_h) $(gxgetbit_h) $(gxiparam_h) $(gxpath_h)
	$(GLCC) $(XINCLUDE) $(GLO_)gdevx.$(OBJ) $(C_) $(GLSRC)gdevx.c

$(GLOBJ)gdevxcmp.$(OBJ) : $(GLSRC)gdevxcmp.c $(GDEVX) $(math__h)
	$(GLCC) $(XINCLUDE) $(GLO_)gdevxcmp.$(OBJ) $(C_) $(GLSRC)gdevxcmp.c

$(GLOBJ)gdevxini.$(OBJ) : $(GLSRC)gdevxini.c $(GDEVX) $(memory__h)\
 $(gserrors_h) $(gsparamx_h) $(gxdevmem_h) $(gdevbbox_h)
	$(GLCC) $(XINCLUDE) $(GLO_)gdevxini.$(OBJ) $(C_) $(GLSRC)gdevxini.c

# We have to compile gdevxres without warnings, because there is a
# const/non-const cast required by the X headers that we can't work around.
$(GLOBJ)gdevxres.$(OBJ) : $(GLSRC)gdevxres.c $(std_h) $(x__h)\
 $(gsmemory_h) $(gstypes_h) $(gxdevice_h) $(gdevx_h)
	$(CC_NO_WARN) $(GLCCFLAGS) $(XINCLUDE) $(GLO_)gdevxres.$(OBJ) $(C_) $(GLSRC)gdevxres.c

$(GLOBJ)gdevxxf.$(OBJ) : $(GLSRC)gdevxxf.c $(GDEVX) $(math__h) $(memory__h)\
 $(gsstruct_h) $(gsutil_h) $(gxxfont_h)
	$(GLCC) $(XINCLUDE) $(GLO_)gdevxxf.$(OBJ) $(C_) $(GLSRC)gdevxxf.c

# Alternate X11-based devices to help debug other drivers.
# x11alpha pretends to have 4 bits of alpha channel.
# x11cmyk pretends to be a CMYK device with 1 bit each of C,M,Y,K.
# x11cmyk2 pretends to be a CMYK device with 2 bits each of C,M,Y,K.
# x11cmyk4 pretends to be a CMYK device with 4 bits each of C,M,Y,K.
# x11cmyk8 pretends to be a CMYK device with 8 bits each of C,M,Y,K.
# x11gray2 pretends to be a 2-bit gray-scale device.
# x11gray4 pretends to be a 4-bit gray-scale device.
# x11mono pretends to be a black-and-white device.
# x11rg16x pretends to be a G5/B5/R6 color device.
# x11rg16x pretends to be a G11/B10/R11 color device.
x11alt_=$(GLOBJ)gdevxalt.$(OBJ)
$(DD)x11alt_.dev : $(DEVS_MAK) $(x11alt_) $(DD)x11_.dev
	$(SETMOD) $(DD)x11alt_ $(x11alt_)
	$(ADDMOD) $(DD)x11alt_ -include $(DD)x11_

$(DD)x11alpha.dev : $(DEVS_MAK) $(DD)x11alt_.dev
	$(SETDEV2) $(DD)x11alpha -include $(DD)x11alt_

$(DD)x11cmyk.dev : $(DEVS_MAK) $(DD)x11alt_.dev
	$(SETDEV2) $(DD)x11cmyk -include $(DD)x11alt_

$(DD)x11cmyk2.dev : $(DEVS_MAK) $(DD)x11alt_.dev
	$(SETDEV2) $(DD)x11cmyk2 -include $(DD)x11alt_

$(DD)x11cmyk4.dev : $(DEVS_MAK) $(DD)x11alt_.dev
	$(SETDEV2) $(DD)x11cmyk4 -include $(DD)x11alt_

$(DD)x11cmyk8.dev : $(DEVS_MAK) $(DD)x11alt_.dev
	$(SETDEV2) $(DD)x11cmyk8 -include $(DD)x11alt_

$(DD)x11gray2.dev : $(DEVS_MAK) $(DD)x11alt_.dev
	$(SETDEV2) $(DD)x11gray2 -include $(DD)x11alt_

$(DD)x11gray4.dev : $(DEVS_MAK) $(DD)x11alt_.dev
	$(SETDEV2) $(DD)x11gray4 -include $(DD)x11alt_

$(DD)x11mono.dev : $(DEVS_MAK) $(DD)x11alt_.dev
	$(SETDEV2) $(DD)x11mono -include $(DD)x11alt_

$(DD)x11rg16x.dev : $(DEVS_MAK) $(DD)x11alt_.dev
	$(SETDEV2) $(DD)x11rg16x -include $(DD)x11alt_

$(DD)x11rg32x.dev : $(DEVS_MAK) $(DD)x11alt_.dev
	$(SETDEV2) $(DD)x11rg32x -include $(DD)x11alt_

$(GLOBJ)gdevxalt.$(OBJ) : $(GLSRC)gdevxalt.c $(GDEVX) $(math__h) $(memory__h)\
 $(gsdevice_h) $(gsparam_h) $(gsstruct_h)
	$(GLCC) $(XINCLUDE) $(GLO_)gdevxalt.$(OBJ) $(C_) $(GLSRC)gdevxalt.c

###### --------------- Memory-buffered printer devices --------------- ######

### ---------------- Practical Automation label printers ---------------- ###

atx_=$(GLOBJ)gdevatx.$(OBJ)

$(DD)atx23.dev : $(DEVS_MAK) $(atx_) $(GLD)page.dev
	$(SETPDEV2) $(DD)atx23 $(atx_)

$(DD)atx24.dev : $(DEVS_MAK) $(atx_) $(GLD)page.dev
	$(SETPDEV2) $(DD)atx24 $(atx_)

$(DD)atx38.dev : $(DEVS_MAK) $(atx_) $(GLD)page.dev
	$(SETPDEV2) $(DD)atx38 $(atx_)

$(GLOBJ)gdevatx.$(OBJ) : $(GLSRC)gdevatx.c $(PDEVH) $(math__h)
	$(GLCC) $(GLO_)gdevatx.$(OBJ) $(C_) $(GLSRC)gdevatx.c

### ----------- The H-P DeskJet and LaserJet printer devices ----------- ###

### These are essentially the same device.
### NOTE: printing at full resolution (300 DPI) requires a printer
###   with at least 1.5 Mb of memory.  150 DPI only requires .5 Mb.
### Note that the lj4dith driver is included with the H-P color printer
###   drivers below.
### For questions about the fs600 device, please contact                  ###
### Peter Schildmann (peter.schildmann@etechnik.uni-rostock.de).          ###

HPPCL=$(GLOBJ)gdevpcl.$(OBJ)
HPDLJM=$(GLOBJ)gdevdljm.$(OBJ) $(HPPCL)
HPMONO=$(GLOBJ)gdevdjet.$(OBJ) $(HPDLJM)

$(GLOBJ)gdevpcl.$(OBJ) : $(GLSRC)gdevpcl.c $(PDEVH) $(math__h) $(gdevpcl_h)
	$(GLCC) $(GLO_)gdevpcl.$(OBJ) $(C_) $(GLSRC)gdevpcl.c

$(GLOBJ)gdevdljm.$(OBJ) : $(GLSRC)gdevdljm.c $(PDEVH) $(gdevdljm_h)
	$(GLCC) $(GLO_)gdevdljm.$(OBJ) $(C_) $(GLSRC)gdevdljm.c

$(GLOBJ)gdevdjet.$(OBJ) : $(GLSRC)gdevdjet.c $(PDEVH) $(gdevdljm_h)
	$(GLCC) $(GLO_)gdevdjet.$(OBJ) $(C_) $(GLSRC)gdevdjet.c

$(DD)deskjet.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)deskjet $(HPMONO)

$(DD)djet500.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)djet500 $(HPMONO)

$(DD)fs600.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)fs600 $(HPMONO)

$(DD)laserjet.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)laserjet $(HPMONO)

$(DD)ljetplus.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)ljetplus $(HPMONO)

### Selecting ljet2p provides TIFF (mode 2) compression on LaserJet III,
### IIIp, IIId, IIIsi, IId, and IIp. 

$(DD)ljet2p.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)ljet2p $(HPMONO)

### Selecting ljet3 provides Delta Row (mode 3) compression on LaserJet III,
### IIIp, IIId, IIIsi.

$(DD)ljet3.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)ljet3 $(HPMONO)

### Selecting ljet3d also provides duplex printing capability.

$(DD)ljet3d.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)ljet3d $(HPMONO)

### Selecting ljet4 or ljet4d also provides Delta Row compression on
### LaserJet IV series.

$(DD)ljet4.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)ljet4 $(HPMONO)

$(DD)ljet4d.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)ljet4d $(HPMONO)

$(DD)lp2563.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)lp2563 $(HPMONO)

$(DD)oce9050.dev : $(DEVS_MAK) $(HPMONO) $(GLD)page.dev
	$(SETPDEV2) $(DD)oce9050 $(HPMONO)

### ------------------ The H-P LaserJet 5 and 6 devices ----------------- ###

### These drivers use H-P's new PCL XL printer language, like H-P's
### LaserJet 5 Enhanced driver for MS Windows.  We don't recommend using
### them:
###	- If you have a LJ 5L or 5P, which isn't a "real" LaserJet 5,
###	use the ljet4 driver instead.  (The lj5 drivers won't work.)
###	- If you have any other model of LJ 5 or 6, use the pxlmono
###	driver, which often produces much more compact output.

gdevpxat_h=$(GLSRC)gdevpxat.h
gdevpxen_h=$(GLSRC)gdevpxen.h
gdevpxop_h=$(GLSRC)gdevpxop.h
gdevpxut_h=$(GLSRC)gdevpxut.h


$(GLOBJ)gdevpxut.$(OBJ) : $(GLSRC)gdevpxut.c $(math__h) $(string__h)\
 $(gx_h) $(gxdevcli_h) $(stream_h)\
 $(gdevpxat_h) $(gdevpxen_h) $(gdevpxop_h) $(gdevpxut_h)
	$(GLCC) $(GLO_)gdevpxut.$(OBJ) $(C_) $(GLSRC)gdevpxut.c

ljet5_=$(GLOBJ)gdevlj56.$(OBJ) $(GLOBJ)gdevpxut.$(OBJ) $(HPPCL)
$(DD)lj5mono.dev : $(DEVS_MAK) $(ljet5_) $(GLD)page.dev
	$(SETPDEV) $(DD)lj5mono $(ljet5_)

$(DD)lj5gray.dev : $(DEVS_MAK) $(ljet5_) $(GLD)page.dev
	$(SETPDEV) $(DD)lj5gray $(ljet5_)

$(GLOBJ)gdevlj56.$(OBJ) : $(GLSRC)gdevlj56.c $(PDEVH) $(gdevpcl_h)\
 $(gdevpxat_h) $(gdevpxen_h) $(gdevpxop_h) $(gdevpxut_h) $(stream_h)
	$(GLCC) $(GLO_)gdevlj56.$(OBJ) $(C_) $(GLSRC)gdevlj56.c

### -------------------- The ijs client ----------------- ###

ijs_=$(GLOBJ)gdevijs.$(OBJ) $(IJSOBJ)ijs.$(OBJ) $(IJSOBJ)ijs_client.$(OBJ) \
 $(IJSOBJ)ijs_exec_$(IJSEXECTYPE).$(OBJ)

$(DD)ijs.dev : $(ijs_) $(GLD)page.dev $(DD)ijslib.dev
	$(SETPDEV) $(DD)ijs $(ijs_)

$(GLOBJ)gdevijs.$(OBJ) : $(GLSRC)gdevijs.c $(PDEVH) $(unistd__h) $(gp_h)\
 $(ijs_h) $(ijs_client_h)
	$(CC_) $(I_)$(GLI_) $(II)$(IJSI_)$(_I) $(GLF_) $(GLO_)gdevijs.$(OBJ) $(C_) $(GLSRC)gdevijs.c

# Please see ijs.mak for the Makefile fragment which builds the IJS
# library.


### -------------------------- The rinkj device ------------------------ ###

RINKJ_SRCDIR=$(GLSRC)rinkj
RINKJ_SRC=$(RINKJ_SRCDIR)$(D)
RINKJ_OBJ=$(GLOBJ)
RINKJ_O_=$(O_)$(RINKJ_OBJ)

RINKJ_INCL=$(I_)$(RINKJ_SRCDIR)$(_I)
RINKJ_CC=$(CC_) $(RINKJ_INCL)

rinkj_core=$(RINKJ_OBJ)evenbetter-rll.$(OBJ) \
 $(RINKJ_OBJ)rinkj-byte-stream.$(OBJ) $(RINKJ_OBJ)rinkj-device.$(OBJ) \
 $(RINKJ_OBJ)rinkj-config.$(OBJ) $(RINKJ_OBJ)rinkj-dither.$(OBJ) \
 $(RINKJ_OBJ)rinkj-epson870.$(OBJ) $(RINKJ_OBJ)rinkj-screen-eb.$(OBJ)

$(RINKJ_OBJ)evenbetter-rll.$(OBJ): $(RINKJ_SRC)evenbetter-rll.c
	$(RINKJ_CC) $(RINKJ_O_)evenbetter-rll.$(OBJ) $(C_) $(RINKJ_SRC)evenbetter-rll.c

$(RINKJ_OBJ)rinkj-byte-stream.$(OBJ): $(RINKJ_SRC)rinkj-byte-stream.c
	$(RINKJ_CC) $(RINKJ_O_)rinkj-byte-stream.$(OBJ) $(C_) $(RINKJ_SRC)rinkj-byte-stream.c

$(RINKJ_OBJ)rinkj-device.$(OBJ): $(RINKJ_SRC)rinkj-device.c
	$(RINKJ_CC) $(RINKJ_O_)rinkj-device.$(OBJ) $(C_) $(RINKJ_SRC)rinkj-device.c

$(RINKJ_OBJ)rinkj-config.$(OBJ): $(RINKJ_SRC)rinkj-config.c
	$(RINKJ_CC) $(RINKJ_O_)rinkj-config.$(OBJ) $(C_) $(RINKJ_SRC)rinkj-config.c

$(RINKJ_OBJ)rinkj-dither.$(OBJ): $(RINKJ_SRC)rinkj-dither.c
	$(RINKJ_CC) $(RINKJ_O_)rinkj-dither.$(OBJ) $(C_) $(RINKJ_SRC)rinkj-dither.c

$(RINKJ_OBJ)rinkj-epson870.$(OBJ): $(RINKJ_SRC)rinkj-epson870.c
	$(RINKJ_CC) $(RINKJ_O_)rinkj-epson870.$(OBJ) $(C_) $(RINKJ_SRC)rinkj-epson870.c

$(RINKJ_OBJ)rinkj-screen-eb.$(OBJ): $(RINKJ_SRC)rinkj-screen-eb.c
	$(RINKJ_CC) $(RINKJ_O_)rinkj-screen-eb.$(OBJ) $(C_) $(RINKJ_SRC)rinkj-screen-eb.c

rinkj_=$(GLOBJ)gdevrinkj.$(OBJ) $(rinkj_core)

$(DD)rinkj.dev : $(DEVS_MAK) $(rinkj_) $(GLD)page.dev
	$(SETDEV) $(DD)rinkj $(rinkj_)

$(GLOBJ)gdevrinkj.$(OBJ) : $(GLSRC)gdevrinkj.c $(PDEVH) $(math__h)\
 $(gdevdcrd_h) $(gscrd_h) $(gscrdp_h) $(gsparam_h) $(gxlum_h) $(icc_h)\
 $(gxdcconv_h)
	$(GLICCCC) $(GLO_)gdevrinkj.$(OBJ) $(C_) $(GLSRC)gdevrinkj.c


###### ------------------- High-level file formats ------------------- ######

# Support for PostScript and PDF

gdevpsdf_h=$(GLSRC)gdevpsdf.h $(gdevvec_h) $(gsparam_h)\
 $(sa85x_h) $(scfx_h) $(spsdf_h) $(strimpl_h)
gdevpsds_h=$(GLSRC)gdevpsds.h $(strimpl_h) $(gsiparam_h)

psdf_1=$(GLOBJ)gdevpsdi.$(OBJ) $(GLOBJ)gdevpsdp.$(OBJ)
psdf_2=$(GLOBJ)gdevpsds.$(OBJ) $(GLOBJ)gdevpsdu.$(OBJ)
psdf_3=$(GLOBJ)scfparam.$(OBJ) $(GLOBJ)sdcparam.$(OBJ) $(GLOBJ)sdeparam.$(OBJ)
psdf_4=$(GLOBJ)spprint.$(OBJ) $(GLOBJ)spsdf.$(OBJ) $(GLOBJ)sstring.$(OBJ)
psdf_5=$(GLOBJ)gsparamx.$(OBJ)
psdf_=$(psdf_1) $(psdf_2) $(psdf_3) $(psdf_4) $(psdf_5)
psdf_inc1=$(GLD)vector.dev $(GLD)pngp.dev $(GLD)seexec.dev
psdf_inc2=$(GLD)sdcte.dev $(GLD)slzwe.dev $(GLD)szlibe.dev
psdf_inc=$(psdf_inc1) $(psdf_inc2)
$(DD)psdf.dev : $(DEVS_MAK) $(ECHOGS_XE) $(psdf_) $(psdf_inc)
	$(SETMOD) $(DD)psdf $(psdf_1)
	$(ADDMOD) $(DD)psdf -obj $(psdf_2)
	$(ADDMOD) $(DD)psdf -obj $(psdf_3)
	$(ADDMOD) $(DD)psdf -obj $(psdf_4)
	$(ADDMOD) $(DD)psdf -obj $(psdf_5)
	$(ADDMOD) $(DD)psdf -include $(psdf_inc1)
	$(ADDMOD) $(DD)psdf -include $(psdf_inc2)

$(GLOBJ)gdevpsdi.$(OBJ) : $(GLSRC)gdevpsdi.c $(GXERR)\
 $(jpeglib__h) $(math__h) $(stdio__h)\
 $(gscspace_h)\
 $(scfx_h) $(sdct_h) $(sjpeg_h) $(slzwx_h) $(srlx_h) $(spngpx_h)\
 $(strimpl_h) $(szlibx_h)\
 $(gdevpsdf_h) $(gdevpsds_h) $(gxdevmem_h) $(gxcspace_h) $(gxparamx_h)
	$(GLJCC) $(GLO_)gdevpsdi.$(OBJ) $(C_) $(GLSRC)gdevpsdi.c

$(GLOBJ)gdevpsdp.$(OBJ) : $(GLSRC)gdevpsdp.c $(GDEVH)\
 $(string__h) $(jpeglib__h)\
 $(scfx_h) $(sdct_h) $(slzwx_h) $(srlx_h) $(strimpl_h) $(szlibx_h)\
 $(gsparamx_h) $(gsutil_h) $(gdevpsdf_h) $(spprint_h)
	$(GLJCC) $(GLO_)gdevpsdp.$(OBJ) $(C_) $(GLSRC)gdevpsdp.c

$(GLOBJ)gdevpsds.$(OBJ) : $(GLSRC)gdevpsds.c $(GX) $(memory__h)\
 $(gserrors_h) $(gxdcconv_h) $(gdevpsds_h) $(gxbitmap_h)\
 $(gxcspace_h) $(gsdcolor_h) $(gscspace_h) $(gxdevcli_h)
	$(GLCC) $(GLO_)gdevpsds.$(OBJ) $(C_) $(GLSRC)gdevpsds.c

$(GLOBJ)gdevpsdu.$(OBJ) : $(GLSRC)gdevpsdu.c $(GXERR)\
 $(jpeglib__h) $(memory__h) $(stdio__h)\
 $(sa85x_h) $(scfx_h) $(sdct_h) $(sjpeg_h) $(sstring_h) $(strimpl_h)\
 $(gdevpsdf_h) $(spprint_h) $(gsovrc_h)
	$(GLJCC) $(GLO_)gdevpsdu.$(OBJ) $(C_) $(GLSRC)gdevpsdu.c

# PostScript and EPS writers

pswrite_=$(GLOBJ)gdevps.$(OBJ) $(GLOBJ)gdevpsu.$(OBJ) $(GLOBJ)scantab.$(OBJ) $(GLOBJ)sfilter2.$(OBJ)
$(DD)epswrite.dev : $(DEVS_MAK) $(ECHOGS_XE) $(pswrite_) $(GLD)psdf.dev
	$(SETDEV2) $(DD)epswrite $(pswrite_)
	$(ADDMOD) $(DD)epswrite -include $(GLD)psdf

$(DD)pswrite.dev : $(DEVS_MAK) $(ECHOGS_XE) $(pswrite_) $(GLD)psdf.dev
	$(SETDEV2) $(DD)pswrite $(pswrite_)
	$(ADDMOD) $(DD)pswrite -include $(GLD)psdf

$(GLOBJ)gdevps.$(OBJ) : $(GLSRC)gdevps.c $(GDEV)\
 $(math__h) $(memory__h) $(string__h) $(time__h)\
 $(gscdefs_h) $(gscspace_h) $(gsline_h) $(gsparam_h) $(gsiparam_h) $(gsmatrix_h)\
 $(gxdcolor_h) $(gxpath_h)\
 $(sa85x_h) $(sstring_h) $(strimpl_h)\
 $(gdevpsdf_h) $(gdevpsu_h) $(spprint_h)
	$(GLCC) $(GLO_)gdevps.$(OBJ) $(C_) $(GLSRC)gdevps.c

################ BEGIN PDF WRITER ################

# PDF writer
# Note that gs_pdfwr.ps will only actually be loaded if the configuration
# includes a PostScript interpreter.

# We reserve slots here for gdevpdfa...z, just in case we need them.
pdfwrite1_=$(GLOBJ)gdevpdf.$(OBJ) $(GLOBJ)gdevpdfb.$(OBJ)
pdfwrite2_=$(GLOBJ)gdevpdfc.$(OBJ) $(GLOBJ)gdevpdfd.$(OBJ)
pdfwrite3_=$(GLOBJ)gdevpdfg.$(OBJ)
pdfwrite4_=$(GLOBJ)gdevpdfi.$(OBJ) $(GLOBJ)gdevpdfj.$(OBJ) $(GLOBJ)gdevpdfk.$(OBJ)
pdfwrite5_=$(GLOBJ)gdevpdfm.$(OBJ)
pdfwrite6_=$(GLOBJ)gdevpdfo.$(OBJ) $(GLOBJ)gdevpdfp.$(OBJ) $(GLOBJ)gdevpdft.$(OBJ)
pdfwrite7_=$(GLOBJ)gdevpdfr.$(OBJ)
pdfwrite8_=$(GLOBJ)gdevpdfu.$(OBJ) $(GLOBJ)gdevpdfv.$(OBJ)
pdfwrite9_=
pdfwrite10_=$(GLOBJ)gsflip.$(OBJ)
pdfwrite11_=$(GLOBJ)scantab.$(OBJ) $(GLOBJ)sfilter2.$(OBJ)
pdfwrite_=$(pdfwrite1_) $(pdfwrite2_) $(pdfwrite3_) $(pdfwrite4_)\
 $(pdfwrite5_) $(pdfwrite6_) $(pdfwrite7_) $(pdfwrite8_) $(pdfwrite9_)\
 $(pdfwrite10_) $(pdfwrite11_)

# Since ps2write actually is a clone of pdfwrite,
# we don't define a full module for it, 
# but generate both devices within pdfwrite.dev .

# Note that for ps2pdf operation, we need to parse DSC comments to set
# the Orientation (Page dict /Rotate value). This is not part of the
# pdfwrite device, but part of the PS interpreter so that the pdfwrite
# device can be used with other top level interpreters (such as PCL).
$(DD)pdfwrite.dev : $(DEVS_MAK) $(ECHOGS_XE) $(pdfwrite_)\
 $(GLD)cmyklib.dev $(GLD)cfe.dev $(GLD)lzwe.dev\
 $(GLD)rle.dev $(GLD)sdcte.dev $(GLD)sdeparam.dev $(GLD)smd5.dev\
 $(GLD)szlibe.dev $(GLD)psdf.dev \
 $(DD)pdtext.dev
	$(SETDEV2) $(DD)ps2write $(pdfwrite1_)
	$(SETDEV2) $(DD)pdfwrite $(pdfwrite1_)
	$(ADDMOD) $(DD)pdfwrite $(pdfwrite2_)
	$(ADDMOD) $(DD)pdfwrite $(pdfwrite3_)
	$(ADDMOD) $(DD)pdfwrite $(pdfwrite4_)
	$(ADDMOD) $(DD)pdfwrite $(pdfwrite5_)
	$(ADDMOD) $(DD)pdfwrite $(pdfwrite6_)
	$(ADDMOD) $(DD)pdfwrite $(pdfwrite7_)
	$(ADDMOD) $(DD)pdfwrite $(pdfwrite8_)
	$(ADDMOD) $(DD)pdfwrite $(pdfwrite9_)
	$(ADDMOD) $(DD)pdfwrite $(pdfwrite10_)
	$(ADDMOD) $(DD)pdfwrite $(pdfwrite11_)
	$(ADDMOD) $(DD)pdfwrite -ps gs_pdfwr
	$(ADDMOD) $(DD)pdfwrite -include $(GLD)cmyklib $(GLD)cfe $(GLD)lzwe
	$(ADDMOD) $(DD)pdfwrite -include $(GLD)rle $(GLD)sdcte $(GLD)sdeparam
	$(ADDMOD) $(DD)pdfwrite -include $(GLD)smd5 $(GLD)szlibe
	$(ADDMOD) $(DD)pdfwrite -include $(GLD)psdf
	$(ADDMOD) $(DD)pdfwrite -include $(DD)pdtext

gdevpdfb_h=$(GLSRC)gdevpdfb.h
gdevpdfc_h=$(GLSRC)gdevpdfc.h
gdevpdfg_h=$(GLSRC)gdevpdfg.h $(gscspace_h)
gdevpdfo_h=$(GLSRC)gdevpdfo.h $(gsparam_h)
gdevpdfx_h=$(GLSRC)gdevpdfx.h\
 $(gsparam_h) $(gsuid_h) $(gxdevice_h) $(gxfont_h) $(gxline_h)\
 $(spprint_h) $(stream_h) $(gdevpsdf_h)

$(GLOBJ)gdevpdf.$(OBJ) : $(GLSRC)gdevpdf.c $(GDEVH)\
 $(fcntl__h) $(memory__h) $(string__h) $(time__h) $(unistd__h) $(gp_h)\
 $(gdevpdfg_h) $(gdevpdfo_h) $(gdevpdfx_h) $(gdevpdt_h) $(smd5_h) $(sarc4_h)\
 $(gdevpdfb_h)
	$(GLCC) $(GLO_)gdevpdf.$(OBJ) $(C_) $(GLSRC)gdevpdf.c

$(GLOBJ)gdevpdfb.$(OBJ) : $(GLSRC)gdevpdfb.c\
 $(string__h) $(gx_h)\
 $(gdevpdfg_h) $(gdevpdfo_h) $(gdevpdfx_h)\
 $(gserrors_h) $(gxcspace_h) $(gxdcolor_h) $(gxpcolor_h) $(gxhldevc_h)
	$(GLCC) $(GLO_)gdevpdfb.$(OBJ) $(C_) $(GLSRC)gdevpdfb.c

$(GLOBJ)gdevpdfc.$(OBJ) : $(GLSRC)gdevpdfc.c $(GXERR) $(math__h) $(memory__h)\
 $(gdevpdfc_h) $(gdevpdfg_h) $(gdevpdfo_h) $(gdevpdfx_h)\
 $(gscie_h) $(gscindex_h) $(gscspace_h) $(gscdevn_h) $(gscsepr_h) $(gsicc_h)\
 $(sstring_h) $(stream_h) $(strimpl_h) $(gxcspace_h)
	$(GLCC) $(GLO_)gdevpdfc.$(OBJ) $(C_) $(GLSRC)gdevpdfc.c

$(GLOBJ)gdevpdfd.$(OBJ) : $(GLSRC)gdevpdfd.c $(math__h) $(memory__h)\
 $(gx_h) $(gxdevice_h) $(gxfixed_h) $(gxistate_h) $(gxpaint_h)\
 $(gxcoord_h) $(gxdevmem_h) $(gxcolor2_h) $(gxhldevc_h)\
 $(gsstate_h) $(gserrors_h) $(gsptype2_h)\
 $(gzpath_h) $(gzcpath_h) $(gdevpdfx_h) $(gdevpdfg_h) $(gdevpdfo_h) $(gsutil_h)
	$(GLCC) $(GLO_)gdevpdfd.$(OBJ) $(C_) $(GLSRC)gdevpdfd.c

$(GLOBJ)gdevpdfg.$(OBJ) : $(GLSRC)gdevpdfg.c $(GXERR) $(math__h) $(string__h)\
 $(memory__h) $(gdevpdfg_h) $(gdevpdfo_h) $(gdevpdfx_h)\
 $(gsfunc0_h) $(gsstate_h) $(gxdcolor_h) $(gxpcolor_h) $(gxcolor2_h) $(gsptype2_h)\
 $(gxbitmap_h) $(gxdht_h) $(gxfarith_h) $(gxfmap_h) $(gxht_h) $(gxistate_h)\
 $(gzht_h)\
 $(szlibx_h)
	$(GLCC) $(GLO_)gdevpdfg.$(OBJ) $(C_) $(GLSRC)gdevpdfg.c

$(GLOBJ)gdevpdfi.$(OBJ) : $(GLSRC)gdevpdfi.c $(memory__h) $(math__h)\
 $(gx_h)\
 $(gserrors_h) $(gsdevice_h) $(gsflip_h) $(gsiparm4_h) $(gsstate_h) $(gscolor2_h)\
 $(gdevpdfx_h) $(gdevpdfg_h) $(gdevpdfo_h)\
 $(gxcspace_h) $(gximage3_h) $(gximag3x_h) $(gxdcolor_h) $(gxpcolor_h)\
 $(gxhldevc_h)
	$(GLCC) $(GLO_)gdevpdfi.$(OBJ) $(C_) $(GLSRC)gdevpdfi.c

$(GLOBJ)gdevpdfj.$(OBJ) : $(GLSRC)gdevpdfj.c\
 $(memory__h) $(string__h) $(gx_h) $(gserrors_h)\
 $(gdevpdfx_h) $(gdevpdfg_h) $(gdevpdfo_h) $(gxcspace_h)\
 $(gsiparm4_h) $(gdevpsds_h) $(spngpx_h)
	$(GLJCC) $(GLO_)gdevpdfj.$(OBJ) $(C_) $(GLSRC)gdevpdfj.c

$(GLOBJ)gdevpdfk.$(OBJ) : $(GLSRC)gdevpdfk.c $(GXERR) $(math__h) $(memory__h)\
 $(gdevpdfc_h) $(gdevpdfg_h) $(gdevpdfo_h) $(gdevpdfx_h)\
 $(gsicc_h) $(gxcie_h) $(gxcspace_h)\
 $(stream_h) $(strimpl_h)
	$(GLCC) $(GLO_)gdevpdfk.$(OBJ) $(C_) $(GLSRC)gdevpdfk.c

$(GLOBJ)gdevpdfm.$(OBJ) : $(GLSRC)gdevpdfm.c\
 $(math__h) $(memory__h) $(string__h) $(gx_h)\
 $(gdevpdfo_h) $(gdevpdfx_h) $(gserrors_h) $(gsutil_h) $(scanchar_h)\
 $(szlibx_h) $(slzwx_h)
	$(GLCC) $(GLO_)gdevpdfm.$(OBJ) $(C_) $(GLSRC)gdevpdfm.c

$(GLOBJ)gdevpdfo.$(OBJ) : $(GLSRC)gdevpdfo.c $(memory__h) $(string__h)\
 $(gx_h)\
 $(gdevpdfo_h) $(gdevpdfx_h) $(gserrors_h) $(gsparam_h) $(gsutil_h)\
 $(sa85x_h) $(slzwx_h) $(sarc4_h) $(sstring_h) $(strimpl_h) $(szlibx_h)
	$(GLCC) $(GLO_)gdevpdfo.$(OBJ) $(C_) $(GLSRC)gdevpdfo.c

$(GLOBJ)gdevpdfp.$(OBJ) : $(GLSRC)gdevpdfp.c $(memory__h) $(string__h) $(gx_h)\
 $(gdevpdfo_h) $(gdevpdfg_h) $(gdevpdfx_h) $(gserrors_h) $(gsparamx_h)
	$(GLCC) $(GLO_)gdevpdfp.$(OBJ) $(C_) $(GLSRC)gdevpdfp.c

$(GLOBJ)gdevpdfr.$(OBJ) : $(GLSRC)gdevpdfr.c $(memory__h) $(string__h)\
 $(gx_h)\
 $(gdevpdfo_h) $(gdevpdfx_h) $(gserrors_h) $(gsutil_h)\
 $(scanchar_h) $(sstring_h) $(strimpl_h)
	$(GLCC) $(GLO_)gdevpdfr.$(OBJ) $(C_) $(GLSRC)gdevpdfr.c

$(GLOBJ)gdevpdft.$(OBJ) : $(GLSRC)gdevpdft.c $(string__h)\
 $(gx_h) $(gserrors_h) $(gstrans_h) $(gscolor2_h) $(gzstate_h)\
 $(gdevpdfx_h) $(gdevpdfg_h) $(gdevpdfo_h)
	$(GLCC) $(GLO_)gdevpdft.$(OBJ) $(C_) $(GLSRC)gdevpdft.c

$(GLOBJ)gdevpdfu.$(OBJ) : $(GLSRC)gdevpdfu.c $(GXERR)\
 $(jpeglib__h) $(memory__h) $(string__h)\
 $(gdevpdfo_h) $(gdevpdfx_h) $(gdevpdfg_h) $(gdevpdtd_h) $(gscdefs_h)\
 $(gsdsrc_h) $(gsfunc_h) $(gsfunc3_h)\
 $(sa85x_h) $(scanchar_h) $(scfx_h) $(sdct_h) $(slzwx_h) $(spngpx_h)\
 $(srlx_h) $(sarc4_h) $(smd5_h) $(sstring_h) $(strimpl_h) $(szlibx_h)
	$(GLCC) $(GLO_)gdevpdfu.$(OBJ) $(C_) $(GLSRC)gdevpdfu.c

$(GLOBJ)gdevpdfv.$(OBJ) : $(GLSRC)gdevpdfv.c $(GXERR) $(math__h) $(string__h)\
 $(gdevpdfg_h) $(gdevpdfo_h) $(gdevpdfx_h)\
 $(gscindex_h) $(gscoord_h) $(gsiparm3_h) $(gsmatrix_h) $(gsptype2_h)\
 $(gxcolor2_h) $(gxdcolor_h) $(gxpcolor_h) $(gxshade_h)\
 $(szlibx_h)
	$(GLCC) $(GLO_)gdevpdfv.$(OBJ) $(C_) $(GLSRC)gdevpdfv.c

######## pdfwrite text

# The text facilities for the PDF writer are so large and complex that
# we give them their own module name and (for the new code) file name prefix.
# However, logically they are part of pdfwrite and cannot be used separately.

$(DD)pdtext.dev : $(DEVS_MAK) $(ECHOGS_XE) $(DD)pdxtext.dev
	$(SETMOD) $(DD)pdtext -include $(DD)pdxtext

# For a code roadmap, see gdevpdtx.h.

gdevpdt_h=$(GLSRC)gdevpdt.h
gdevpdtx_h=$(GLSRC)gdevpdtx.h $(gdevpdt_h)
gdevpdtb_h=$(GLSRC)gdevpdtb.h $(gdevpdtx_h)
gdevpdtd_h=$(GLSRC)gdevpdtd.h $(gdevpdtb_h) $(gdevpdtx_h)
gdevpdtf_h=$(GLSRC)gdevpdtf.h $(gdevpdtx_h)
gdevpdti_h=$(GLSRC)gdevpdti.h $(gdevpdt_h)
gdevpdts_h=$(GLSRC)gdevpdts.h $(gsmatrix_h)
gdevpdtt_h=$(GLSRC)gdevpdtt.h
gdevpdtv_h=$(GLSRC)gdevpdtv.h
gdevpdtw_h=$(GLSRC)gdevpdtw.h

# We reserve space for all of a..z, just in case.
pdxtext_ab=$(GLOBJ)gdevpdt.$(OBJ) $(GLOBJ)gdevpdtb.$(OBJ)
pdxtext_cde=$(GLOBJ)gdevpdtc.$(OBJ) $(GLOBJ)gdevpdtd.$(OBJ) $(GLOBJ)gdevpdte.$(OBJ)
pdxtext_fgh=$(GLOBJ)gdevpdtf.$(OBJ)
pdxtext_ijk=$(GLOBJ)gdevpdti.$(OBJ)
pdxtext_lmn=
pdxtext_opq=
pdxtext_rst=$(GLOBJ)gdevpdts.$(OBJ) $(GLOBJ)gdevpdtt.$(OBJ)
pdxtext_uvw=$(GLOBJ)gdevpdtv.$(OBJ) $(GLOBJ)gdevpdtw.$(OBJ)
pdxtext_xyz=
pdxtext_=$(pdxtext_ab) $(pdxtext_cde) $(pdxtext_fgh) $(pdxtext_ijk)\
 $(pdxtext_lmn) $(pdxtext_opq) $(pdxtext_rst) $(pdxtext_uvw) $(pdxtext_xyz)\
 $(GLOBJ)gsfont0c.$(OBJ)
$(DD)pdxtext.dev : $(DEVS_MAK) $(ECHOGS_XE) $(pdxtext_)\
 $(GLD)fcopy.dev $(GLD)psf.dev
	$(SETMOD) $(DD)pdxtext $(pdxtext_ab)
	$(ADDMOD) $(DD)pdxtext $(pdxtext_cde)
	$(ADDMOD) $(DD)pdxtext $(pdxtext_fgh)
	$(ADDMOD) $(DD)pdxtext $(pdxtext_ijk)
	$(ADDMOD) $(DD)pdxtext $(pdxtext_lmn)
	$(ADDMOD) $(DD)pdxtext $(pdxtext_opq)
	$(ADDMOD) $(DD)pdxtext $(pdxtext_rst)
	$(ADDMOD) $(DD)pdxtext $(pdxtext_uvw)
	$(ADDMOD) $(DD)pdxtext $(pdxtext_xyz)
	$(ADDMOD) $(DD)pdxtext $(GLOBJ)gsfont0c.$(OBJ)
	$(ADDMOD) $(DD)pdxtext -include $(GLD)fcopy $(GLD)psf

$(GLOBJ)gdevpdt.$(OBJ) : $(GLSRC)gdevpdt.c $(gx_h) $(gxpath_h) $(memory__h)\
 $(gdevpdfx_h) $(gdevpdfg_h) $(gdevpdtf_h) $(gdevpdti_h) $(gdevpdts_h) $(gdevpdtx_h) $(gdevpdt_h)
	$(GLCC) $(GLO_)gdevpdt.$(OBJ) $(C_) $(GLSRC)gdevpdt.c

$(GLOBJ)gdevpdtb.$(OBJ) : $(GLSRC)gdevpdtb.c $(memory__h) $(ctype__h) $(string__h)\
 $(gx_h) $(gserrors_h) $(gsutil_h)\
 $(gxfcid_h) $(gxfcopy_h) $(gxfont_h) $(gxfont42_h)\
 $(gdevpsf_h) $(gdevpdfx_h) $(gdevpdtb_h)
	$(GLCC) $(GLO_)gdevpdtb.$(OBJ) $(C_) $(GLSRC)gdevpdtb.c

$(GLOBJ)gdevpdtc.$(OBJ) : $(GLSRC)gdevpdtc.c $(gx_h) $(memory__h) $(string__h)\
 $(gserrors_h) $(gxfcmap_h) $(gxfont_h) $(gxfont0_h) $(gxfont0c_h)\
 $(gzpath_h) $(gxchar_h) $(gdevpsf_h) $(gdevpdfx_h) $(gdevpdtx_h)\
 $(gdevpdtd_h) $(gdevpdtf_h) $(gdevpdts_h) $(gdevpdtt_h)
	$(GLCC) $(GLO_)gdevpdtc.$(OBJ) $(C_) $(GLSRC)gdevpdtc.c

$(GLOBJ)gdevpdte.$(OBJ) : $(GLSRC)gdevpdte.c $(gx_h) $(math__h) $(memory__h)\
 $(gserrors_h) $(gsutil_h) $(gxfcmap_h) $(gxfcopy_h) $(gxfont_h) \
 $(gxfont0_h) $(gxfont0c_h) $(gxpath_h) $(gdevpsf_h) $(gdevpdfx_h) \
 $(gdevpdfg_h) $(gdevpdtx_h) $(gdevpdtd_h) $(gdevpdtf_h) $(gdevpdts_h) \
 $(gdevpdtt_h)
	$(GLCC) $(GLO_)gdevpdte.$(OBJ) $(C_) $(GLSRC)gdevpdte.c

$(GLOBJ)gdevpdtd.$(OBJ) : $(GLSRC)gdevpdtd.c $(math__h) $(memory__h) $(gx_h)\
 $(gserrors_h) $(gsrect_h)\
 $(gdevpdfo_h) $(gdevpdfx_h)\
 $(gdevpdtb_h) $(gdevpdtd_h)
	$(GLCC) $(GLO_)gdevpdtd.$(OBJ) $(C_) $(GLSRC)gdevpdtd.c

$(GLOBJ)gdevpdtf.$(OBJ) : $(GLSRC)gdevpdtf.c $(gx_h) $(memory__h)\
 $(string__h) $(gserrors_h) $(gsutil_h)\
 $(gxfcache_h) $(gxfcid_h) $(gxfcmap_h) $(gxfcopy_h) $(gxfont_h) $(gxfont1_h)\
 $(gdevpsf_h) $(gdevpdfx_h) $(gdevpdtb_h) $(gdevpdtd_h) $(gdevpdtf_h) $(gdevpdtw_h)
	$(GLCC) $(GLO_)gdevpdtf.$(OBJ) $(C_) $(GLSRC)gdevpdtf.c

$(GLOBJ)gdevpdti.$(OBJ) : $(GLSRC)gdevpdti.c $(memory__h) $(string__h) $(gx_h)\
 $(gserrors_h) $(gsutil_h)\
 $(gdevpdfx_h) $(gdevpdfg_h)\
 $(gdevpdtf_h) $(gdevpdti_h) $(gdevpdts_h) $(gdevpdtw_h) $(gdevpdtt_h) $(gdevpdfo_h)
	$(GLCC) $(GLO_)gdevpdti.$(OBJ) $(C_) $(GLSRC)gdevpdti.c

$(GLOBJ)gdevpdts.$(OBJ) : $(GLSRC)gdevpdts.c $(gx_h) $(math__h) $(memory__h)\
 $(gdevpdfx_h) $(gdevpdtf_h) $(gdevpdts_h) $(gdevpdtx_h)
	$(GLCC) $(GLO_)gdevpdts.$(OBJ) $(C_) $(GLSRC)gdevpdts.c

$(GLOBJ)gdevpdtt.$(OBJ) : $(GLSRC)gdevpdtt.c $(gx_h) $(math__h) $(string__h)\
 $(gserrors_h) $(gsencs_h) $(gscedata_h) $(gsmatrix_h) $(gzstate_h)\
 $(gxfcache_h) $(gxfont_h) $(gxfont0_h) $(gxfcid_h) $(gxfcopy_h)\
 $(gxfcmap_h) $(gxpath_h) $(gxchar_h) $(gxstate_h) $(gdevpdfx_h) $(gdevpdfg_h)\
 $(gdevpdtx_h) $(gdevpdtd_h) $(gdevpdtf_h) $(gdevpdts_h) $(gdevpdtt_h)\
 $(gdevpdti_h) $(gxhldevc_h)
	$(GLCC) $(GLO_)gdevpdtt.$(OBJ) $(C_) $(GLSRC)gdevpdtt.c

$(GLOBJ)gdevpdtv.$(OBJ) : $(GLSRC)gdevpdtv.c $(gx_h) $(gdevpdtv_h)
	$(GLCC) $(GLO_)gdevpdtv.$(OBJ) $(C_) $(GLSRC)gdevpdtv.c

$(GLOBJ)gdevpdtw.$(OBJ) : $(GLSRC)gdevpdtw.c $(gx_h) $(gserrors_h) $(memory__h)\
 $(gxfcmap_h) $(gxfont_h) $(gscencs_h)\
 $(gdevpsf_h) $(gdevpdfx_h) $(gdevpdfo_h)\
 $(gdevpdtd_h) $(gdevpdtf_h) $(gdevpdti_h) $(gdevpdtw_h) $(gdevpdtv_h) $(sarc4_h)
	$(GLCC) $(GLO_)gdevpdtw.$(OBJ) $(C_) $(GLSRC)gdevpdtw.c

################ END PDF WRITER ################

# High-level PCL XL writer

pxl_=$(GLOBJ)gdevpx.$(OBJ) $(GLOBJ)gdevpxut.$(OBJ)
$(DD)pxlmono.dev : $(DEVS_MAK) $(pxl_) $(GDEV) $(GLD)vector.dev
	$(SETDEV2) $(DD)pxlmono $(pxl_)
	$(ADDMOD) $(DD)pxlmono -include $(GLD)vector

$(DD)pxlcolor.dev : $(DEVS_MAK) $(pxl_) $(GDEV) $(GLD)vector.dev
	$(SETDEV2) $(DD)pxlcolor $(pxl_)
	$(ADDMOD) $(DD)pxlcolor -include $(GLD)vector

$(GLOBJ)gdevpx.$(OBJ) : $(GLSRC)gdevpx.c\
 $(math__h) $(memory__h) $(string__h)\
 $(gx_h) $(gsccolor_h) $(gsdcolor_h) $(gserrors_h)\
 $(gxcspace_h) $(gxdevice_h) $(gxpath_h)\
 $(gdevpxat_h) $(gdevpxen_h) $(gdevpxop_h) $(gdevpxut_h) $(gdevvec_h)\
 $(srlx_h) $(strimpl_h)
	$(GLCC) $(GLO_)gdevpx.$(OBJ) $(C_) $(GLSRC)gdevpx.c

###### --------------------- Raster file formats --------------------- ######

### --------------------- The "plain bits" devices ---------------------- ###

# This device also exercises the driver CRD facilities, which is why it
# needs some additional files.

bit_=$(GLOBJ)gdevbit.$(OBJ) $(GLOBJ)gdevdcrd.$(OBJ)

$(DD)bit.dev : $(DEVS_MAK) $(bit_) $(GLD)page.dev $(GLD)cielib.dev
	$(SETPDEV2) $(DD)bit $(bit_)
	$(ADDMOD) $(DD)bit -include $(GLD)cielib

$(DD)bitrgb.dev : $(DEVS_MAK) $(bit_) $(GLD)page.dev $(GLD)cielib.dev
	$(SETPDEV2) $(DD)bitrgb $(bit_)
	$(ADDMOD) $(DD)bitrgb -include $(GLD)cielib

$(DD)bitcmyk.dev : $(DEVS_MAK) $(bit_) $(GLD)page.dev $(GLD)cielib.dev
	$(SETPDEV2) $(DD)bitcmyk $(bit_)
	$(ADDMOD) $(DD)bitcmyk -include $(GLD)cielib

$(DD)bitrgbtags.dev : $(DEVS_MAK) $(bit_) $(GLD)page.dev $(GLD)cielib.dev
	$(SETPDEV2) $(DD)bitrgbtags $(bit_)
	$(ADDMOD) $(DD)bitrgbtags -include $(GLD)cielib

$(GLOBJ)gdevbit.$(OBJ) : $(GLSRC)gdevbit.c $(PDEVH) $(math__h)\
 $(gdevdcrd_h) $(gscrd_h) $(gscrdp_h) $(gsparam_h) $(gxlum_h)
	$(GLCC) $(GLO_)gdevbit.$(OBJ) $(C_) $(GLSRC)gdevbit.c

### ------------------------- .BMP file formats ------------------------- ###

gdevbmp_h=$(GLSRC)gdevbmp.h

bmp_=$(GLOBJ)gdevbmp.$(OBJ) $(GLOBJ)gdevbmpc.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ)

$(GLOBJ)gdevbmp.$(OBJ) : $(GLSRC)gdevbmp.c $(PDEVH) $(gdevbmp_h) $(gdevpccm_h)
	$(GLCC) $(GLO_)gdevbmp.$(OBJ) $(C_) $(GLSRC)gdevbmp.c

$(GLOBJ)gdevbmpc.$(OBJ) : $(GLSRC)gdevbmpc.c $(PDEVH) $(gdevbmp_h)
	$(GLCC) $(GLO_)gdevbmpc.$(OBJ) $(C_) $(GLSRC)gdevbmpc.c

$(DD)bmpmono.dev : $(DEVS_MAK) $(bmp_) $(GLD)page.dev
	$(SETPDEV2) $(DD)bmpmono $(bmp_)

$(DD)bmpgray.dev : $(DEVS_MAK) $(bmp_) $(GLD)page.dev
	$(SETPDEV2) $(DD)bmpgray $(bmp_)

$(DD)bmpsep1.dev : $(DEVS_MAK) $(bmp_) $(GLD)page.dev
	$(SETPDEV2) $(DD)bmpsep1 $(bmp_)

$(DD)bmpsep8.dev : $(DEVS_MAK) $(bmp_) $(GLD)page.dev
	$(SETPDEV2) $(DD)bmpsep8 $(bmp_)

$(DD)bmp16.dev : $(DEVS_MAK) $(bmp_) $(GLD)page.dev
	$(SETPDEV2) $(DD)bmp16 $(bmp_)

$(DD)bmp256.dev : $(DEVS_MAK) $(bmp_) $(GLD)page.dev
	$(SETPDEV2) $(DD)bmp256 $(bmp_)

$(DD)bmp16m.dev : $(DEVS_MAK) $(bmp_) $(GLD)page.dev
	$(SETPDEV2) $(DD)bmp16m $(bmp_)

$(DD)bmp32b.dev : $(DEVS_MAK) $(bmp_) $(GLD)page.dev
	$(SETPDEV2) $(DD)bmp32b $(bmp_)

### ------------- BMP driver that serves as demo of async rendering ---- ###

bmpa_=$(GLOBJ)gdevbmpa.$(OBJ) $(GLOBJ)gdevbmpc.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ) $(GLOBJ)gdevppla.$(OBJ)

$(GLOBJ)gdevbmpa.$(OBJ) : $(GLSRC)gdevbmpa.c $(AK) $(stdio__h)\
 $(gdevbmp_h) $(gdevprna_h) $(gdevpccm_h) $(gdevppla_h)\
 $(gserrors_h) $(gpsync_h)
	$(GLCC) $(GLO_)gdevbmpa.$(OBJ) $(C_) $(GLSRC)gdevbmpa.c

$(DD)bmpamono.dev : $(DEVS_MAK) $(bmpa_) $(GLD)page.dev $(GLD)async.dev
	$(SETPDEV2) $(DD)bmpamono $(bmpa_)
	$(ADDMOD) $(DD)bmpamono -include $(GLD)async

$(DD)bmpasep1.dev : $(DEVS_MAK) $(bmpa_) $(GLD)page.dev $(GLD)async.dev
	$(SETPDEV2) $(DD)bmpasep1 $(bmpa_)
	$(ADDMOD) $(DD)bmpasep1 -include $(GLD)async

$(DD)bmpasep8.dev : $(DEVS_MAK) $(bmpa_) $(GLD)page.dev $(GLD)async.dev
	$(SETPDEV2) $(DD)bmpasep8 $(bmpa_)
	$(ADDMOD) $(DD)bmpasep8 -include $(GLD)async

$(DD)bmpa16.dev : $(DEVS_MAK) $(bmpa_) $(GLD)page.dev $(GLD)async.dev
	$(SETPDEV2) $(DD)bmpa16 $(bmpa_)
	$(ADDMOD) $(DD)bmpa16 -include $(GLD)async

$(DD)bmpa256.dev : $(DEVS_MAK) $(bmpa_) $(GLD)page.dev $(GLD)async.dev
	$(SETPDEV2) $(DD)bmpa256 $(bmpa_)
	$(ADDMOD) $(DD)bmpa256 -include $(GLD)async

$(DD)bmpa16m.dev : $(DEVS_MAK) $(bmpa_) $(GLD)page.dev $(GLD)async.dev
	$(SETPDEV2) $(DD)bmpa16m $(bmpa_)
	$(ADDMOD) $(DD)bmpa16m -include $(GLD)async

$(DD)bmpa32b.dev : $(DEVS_MAK) $(bmpa_) $(GLD)page.dev $(GLD)async.dev
	$(SETPDEV2) $(DD)bmpa32b $(bmpa_)
	$(ADDMOD) $(DD)bmpa32b -include $(GLD)async

### -------------------------- CGM file format ------------------------- ###
### This driver is under development.  Use at your own risk.             ###
### The output is very low-level, consisting only of rectangles and      ###
### cell arrays.                                                         ###

cgm_=$(GLOBJ)gdevcgm.$(OBJ) $(GLOBJ)gdevcgml.$(OBJ)

gdevcgml_h=$(GLSRC)gdevcgml.h
gdevcgmx_h=$(GLSRC)gdevcgmx.h $(gdevcgml_h)

$(GLOBJ)gdevcgm.$(OBJ) : $(GLSRC)gdevcgm.c $(GDEV) $(memory__h)\
 $(gp_h) $(gsparam_h) $(gdevpccm_h) $(gdevcgml_h)
	$(GLCC) $(GLO_)gdevcgm.$(OBJ) $(C_) $(GLSRC)gdevcgm.c

$(GLOBJ)gdevcgml.$(OBJ) : $(GLSRC)gdevcgml.c $(memory__h) $(stdio__h)\
 $(gdevcgmx_h)
	$(GLCC) $(GLO_)gdevcgml.$(OBJ) $(C_) $(GLSRC)gdevcgml.c

$(DD)cgmmono.dev : $(DEVS_MAK) $(cgm_)
	$(SETDEV) $(DD)cgmmono $(cgm_)

$(DD)cgm8.dev : $(DEVS_MAK) $(cgm_)
	$(SETDEV) $(DD)cgm8 $(cgm_)

$(DD)cgm24.dev : $(DEVS_MAK) $(cgm_)
	$(SETDEV) $(DD)cgm24 $(cgm_)

### ------------------------ The DeviceN device ------------------------ ###

devn_=$(GLOBJ)gdevdevn.$(OBJ)

$(DD)spotcmyk.dev : $(DEVS_MAK) $(devn_) $(GLD)page.dev
	$(SETDEV) $(DD)spotcmyk $(devn_)

$(DD)devicen.dev : $(DEVS_MAK) $(devn_) $(GLD)page.dev
	$(SETDEV) $(DD)devicen $(devn_)

$(GLOBJ)gdevdevn.$(OBJ) : $(GLSRC)gdevdevn.c $(PDEVH) $(math__h) $(string__h)\
 $(gdevprn_h) $(gsparam_h) $(gscrd_h) $(gscrdp_h) $(gxlum_h) $(gdevdcrd_h)\
 $(gstypes_h) $(gxdcconv_h) $(gdevdevn_h) $(gsequivc_h)
	$(GLCC) $(GLO_)gdevdevn.$(OBJ) $(C_) $(GLSRC)gdevdevn.c

### --------------------------- The XCF device ------------------------- ###

xcf_=$(GLOBJ)gdevxcf.$(OBJ)

$(DD)xcf.dev : $(DEVS_MAK) $(xcf_) $(GLD)page.dev
	$(SETDEV) $(DD)xcf $(xcf_)

$(DD)xcfcmyk.dev : $(DEVS_MAK) $(xcf_) $(GLD)page.dev
	$(SETDEV) $(DD)xcfcmyk $(xcf_)

$(GLOBJ)gdevxcf.$(OBJ) : $(GLSRC)gdevxcf.c $(PDEVH) $(math__h)\
 $(gdevdcrd_h) $(gscrd_h) $(gscrdp_h) $(gsparam_h) $(gxlum_h) $(icc_h)\
 $(gxdcconv_h)
	$(GLICCCC) $(GLO_)gdevxcf.$(OBJ) $(C_) $(GLSRC)gdevxcf.c

### --------------------------- The PSD device ------------------------- ###

psd_=$(GLOBJ)gdevpsd.$(OBJ) $(GLOBJ)gdevdevn.$(OBJ) $(GLOBJ)gsequivc.$(OBJ)

$(DD)psdrgb.dev : $(DEVS_MAK) $(psd_) $(GLD)page.dev
	$(SETDEV) $(DD)psdrgb $(psd_)

$(DD)psdcmyk.dev : $(DEVS_MAK) $(psd_) $(GLD)page.dev
	$(SETDEV) $(DD)psdcmyk $(psd_)

$(GLOBJ)gdevpsd.$(OBJ) : $(GLSRC)gdevpsd.c $(PDEVH) $(math__h)\
 $(gdevdcrd_h) $(gscrd_h) $(gscrdp_h) $(gsparam_h) $(gxlum_h) $(icc_h)\
 $(gstypes_h) $(gxdcconv_h) $(gdevdevn_h) $(gsequivc_h)
	$(GLICCCC) $(GLO_)gdevpsd.$(OBJ) $(C_) $(GLSRC)gdevpsd.c

$(GLOBJ)gsequivc.$(OBJ) : $(GLSRC)gsequivc.c $(math__h)\
 $(PDEVH) $(gsparam_h) $(gstypes_h) $(gxdconv_h) $(gdevdevn_h)\
 $(gsequivc_h) $(gzstate_h) $(gsstate_h) $(gscspace_h) $(gxcspace_h)
	$(GLICCCC) $(GLO_)gsequivc.$(OBJ) $(C_) $(GLSRC)gsequivc.c

### ----------------------- The permutation device --------------------- ###

perm_=$(GLOBJ)gdevperm.$(OBJ)

$(DD)perm.dev : $(DEVS_MAK) $(perm_) $(GLD)page.dev
	$(SETDEV) $(DD)perm $(perm_)

$(GLOBJ)gdevperm.$(OBJ) : $(GLSRC)gdevperm.c $(PDEVH) $(math__h)\
 $(gdevdcrd_h) $(gscrd_h) $(gscrdp_h) $(gsparam_h) $(gxlum_h) $(icc_h)
	$(GLICCCC) $(GLO_)gdevperm.$(OBJ) $(C_) $(GLSRC)gdevperm.c

### ------------------------- JPEG file format ------------------------- ###

jpeg_=$(GLOBJ)gdevjpeg.$(OBJ)

# RGB output
$(DD)jpeg.dev : $(DEVS_MAK) $(jpeg_) $(GLD)sdcte.dev $(GLD)page.dev
	$(SETPDEV2) $(DD)jpeg $(jpeg_)
	$(ADDMOD) $(DD)jpeg -include $(GLD)sdcte

# Gray output
$(DD)jpeggray.dev : $(DEVS_MAK) $(jpeg_) $(GLD)sdcte.dev $(GLD)page.dev
	$(SETPDEV2) $(DD)jpeggray $(jpeg_)
	$(ADDMOD) $(DD)jpeggray -include $(GLD)sdcte

# CMYK output
$(DD)jpegcmyk.dev : $(DEVS_MAK) $(jpeg_) $(GLD)sdcte.dev $(GLD)page.dev
	$(SETPDEV2) $(DD)jpegcmyk $(jpeg_)
	$(ADDMOD) $(DD)jpegcmyk -include $(GLD)sdcte

$(GLOBJ)gdevjpeg.$(OBJ) : $(GLSRC)gdevjpeg.c $(PDEVH)\
 $(stdio__h) $(jpeglib__h)\
 $(sdct_h) $(sjpeg_h) $(stream_h) $(strimpl_h)
	$(GLCC) $(GLO_)gdevjpeg.$(OBJ) $(C_) $(GLSRC)gdevjpeg.c

### ------------------------- MIFF file format ------------------------- ###
### Right now we support only 24-bit direct color, but we might add more ###
### formats in the future.                                               ###

miff_=$(GLOBJ)gdevmiff.$(OBJ)

$(DD)miff24.dev : $(DEVS_MAK) $(miff_) $(GLD)page.dev
	$(SETPDEV) $(DD)miff24 $(miff_)

$(GLOBJ)gdevmiff.$(OBJ) : $(GLSRC)gdevmiff.c $(PDEVH)
	$(GLCC) $(GLO_)gdevmiff.$(OBJ) $(C_) $(GLSRC)gdevmiff.c

### ------------------------- PCX file formats ------------------------- ###

pcx_=$(GLOBJ)gdevpcx.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ)

$(GLOBJ)gdevpcx.$(OBJ) : $(GLSRC)gdevpcx.c $(PDEVH) $(gdevpccm_h) $(gxlum_h)
	$(GLCC) $(GLO_)gdevpcx.$(OBJ) $(C_) $(GLSRC)gdevpcx.c

$(DD)pcxmono.dev : $(DEVS_MAK) $(pcx_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pcxmono $(pcx_)

$(DD)pcxgray.dev : $(DEVS_MAK) $(pcx_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pcxgray $(pcx_)

$(DD)pcx16.dev : $(DEVS_MAK) $(pcx_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pcx16 $(pcx_)

$(DD)pcx256.dev : $(DEVS_MAK) $(pcx_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pcx256 $(pcx_)

$(DD)pcx24b.dev : $(DEVS_MAK) $(pcx_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pcx24b $(pcx_)

$(DD)pcxcmyk.dev : $(DEVS_MAK) $(pcx_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pcxcmyk $(pcx_)

# The 2-up PCX device is here only as an example, and for testing.

$(DD)pcx2up.dev : $(DEVS_MAK) $(LIB_MAK) $(ECHOGS_XE) $(GLOBJ)gdevp2up.$(OBJ) $(GLD)page.dev $(DD)pcx256.dev
	$(SETPDEV) $(DD)pcx2up $(GLOBJ)gdevp2up.$(OBJ)
	$(ADDMOD) $(DD)pcx2up -include $(DD)pcx256

$(GLOBJ)gdevp2up.$(OBJ) : $(GLSRC)gdevp2up.c $(AK)\
 $(gdevpccm_h) $(gdevprn_h) $(gxclpage_h)
	$(GLCC) $(GLO_)gdevp2up.$(OBJ) $(C_) $(GLSRC)gdevp2up.c

### ------------------- Portable Bitmap file formats ------------------- ###
### For more information, see the pam(5), pbm(5), pgm(5), and ppm(5)     ###
### man pages.                                                           ###

pxm_=$(GLOBJ)gdevpbm.$(OBJ) $(GLOBJ)gdevppla.$(OBJ) $(GLOBJ)gdevmpla.$(OBJ)

$(GLOBJ)gdevpbm.$(OBJ) : $(GLSRC)gdevpbm.c $(PDEVH)\
 $(gdevmpla_h) $(gdevplnx_h) $(gdevppla_h)\
 $(gscdefs_h) $(gscspace_h) $(gxgetbit_h) $(gxiparam_h) $(gxlum_h)
	$(GLCC) $(GLO_)gdevpbm.$(OBJ) $(C_) $(GLSRC)gdevpbm.c

### Portable Bitmap (PBM, plain or raw format, magic numbers "P1" or "P4")

$(DD)pbm.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pbm $(pxm_)

$(DD)pbmraw.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pbmraw $(pxm_)

### Portable Graymap (PGM, plain or raw format, magic numbers "P2" or "P5")

$(DD)pgm.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pgm $(pxm_)

$(DD)pgmraw.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pgmraw $(pxm_)

# PGM with automatic optimization to PBM if this is possible.

$(DD)pgnm.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pgnm $(pxm_)

$(DD)pgnmraw.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pgnmraw $(pxm_)

### Portable Pixmap (PPM, plain or raw format, magic numbers "P3" or "P6")

$(DD)ppm.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)ppm $(pxm_)

$(DD)ppmraw.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)ppmraw $(pxm_)

# PPM with automatic optimization to PGM or PBM if possible.

$(DD)pnm.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pnm $(pxm_)

$(DD)pnmraw.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pnmraw $(pxm_)

### Portable inKmap (CMYK internally, converted to PPM=RGB at output time)

$(DD)pkm.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pkm $(pxm_)

$(DD)pkmraw.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pkmraw $(pxm_)

### Portable Separated map (CMYK internally, produces 4 monobit pages)

$(DD)pksm.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pksm $(pxm_)

$(DD)pksmraw.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pksmraw $(pxm_)

### Plan 9 bitmap format

$(DD)plan9bm.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)plan9bm $(pxm_)

### Portable Arbitrary Map (PAM, magic number "P7", CMYK)

$(DD)pam.dev : $(DEVS_MAK) $(pxm_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pam $(pxm_)

### --------------- Portable Network Graphics file format --------------- ###
### Requires libpng 0.81 and zlib 0.95 (or more recent versions).         ###
### See libpng.mak and zlib.mak for more details.                         ###

png__h=$(GLSRC)png_.h $(MAKEFILE)

png_=$(GLOBJ)gdevpng.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ)
libpng_dev=$(PNGGENDIR)$(D)libpng.dev
png_i_=-include $(PNGGENDIR)$(D)libpng

$(GLOBJ)gdevpng.$(OBJ) : $(GLSRC)gdevpng.c\
 $(gdevprn_h) $(gdevpccm_h) $(gscdefs_h) $(png__h)
	$(CC_) $(I_)$(GLI_) $(II)$(PI_)$(_I) $(PCF_) $(GLF_) $(GLO_)gdevpng.$(OBJ) $(C_) $(GLSRC)gdevpng.c

$(DD)pngmono.dev : $(DEVS_MAK) $(libpng_dev) $(png_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pngmono $(png_)
	$(ADDMOD) $(DD)pngmono $(png_i_)

$(DD)pnggray.dev : $(DEVS_MAK) $(libpng_dev) $(png_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pnggray $(png_)
	$(ADDMOD) $(DD)pnggray $(png_i_)

$(DD)png16.dev : $(DEVS_MAK) $(libpng_dev) $(png_) $(GLD)page.dev
	$(SETPDEV2) $(DD)png16 $(png_)
	$(ADDMOD) $(DD)png16 $(png_i_)

$(DD)png256.dev : $(DEVS_MAK) $(libpng_dev) $(png_) $(GLD)page.dev
	$(SETPDEV2) $(DD)png256 $(png_)
	$(ADDMOD) $(DD)png256 $(png_i_)

$(DD)png16m.dev : $(DEVS_MAK) $(libpng_dev) $(png_) $(GLD)page.dev
	$(SETPDEV2) $(DD)png16m $(png_)
	$(ADDMOD) $(DD)png16m $(png_i_)

$(DD)png48.dev : $(DEVS_MAK) $(libpng_dev) $(png_) $(GLD)page.dev
	$(SETPDEV2) $(DD)png48 $(png_)
	$(ADDMOD) $(DD)png48 $(png_i_)

$(DD)pngalpha.dev : $(DEVS_MAK) $(libpng_dev) $(png_) $(GLD)page.dev
	$(SETPDEV2) $(DD)pngalpha $(png_)
	$(ADDMOD) $(DD)pngalpha $(png_i_)

### -------------------- PNG with transparency -------------------- ###

pnga_=$(GLOBJ)gdevpnga.$(OBJ)
$(DD)pnga.dev :	$(pnga_)
	$(SETDEV) $(DD)pnga $(pnga_)

$(GLOBJ)gdevpnga.$(OBJ) : $(GLSRC)gdevpnga.c $(png__h)\
 $(gscdefs_h) $(gsdevice_h) $(gxblend_h) $(gxtext_h)\
 $(gdevmem_h) $(gdevpccm_h) $(gdevprn_h)
	$(CC_) $(I_)$(GLI_) $(II)$(PI_)$(_I) $(PCF_) $(GLF_) $(GLO_)gdevpnga.$(OBJ) $(C_) $(GLSRC)gdevpnga.c

### ---------------------- PostScript image format ---------------------- ###
### These devices make it possible to print monochrome Level 2 files on a ###
###   Level 1 printer, by converting them to a bitmap in PostScript       ###
###   format.  They also can convert big, complex color PostScript files  ###
###   to (often) smaller and more easily printed bitmaps.                 ###

psim_=$(GLOBJ)gdevpsim.$(OBJ) $(GLOBJ)gdevpsu.$(OBJ)

$(GLOBJ)gdevpsim.$(OBJ) : $(GLSRC)gdevpsim.c $(PDEVH)\
 $(gdevpsu_h)\
 $(sa85x_h) $(srlx_h) $(stream_h) $(strimpl_h)
	$(GLCC) $(GLO_)gdevpsim.$(OBJ) $(C_) $(GLSRC)gdevpsim.c

# Monochrome, Level 1 output

$(DD)psmono.dev : $(DEVS_MAK) $(psim_) $(GLD)page.dev
	$(SETPDEV2) $(DD)psmono $(psim_)

$(DD)psgray.dev : $(DEVS_MAK) $(psim_) $(GLD)page.dev
	$(SETPDEV2) $(DD)psgray $(psim_)

# RGB, Level 2 output

$(DD)psrgb.dev : $(DEVS_MAK) $(psim_) $(GLD)page.dev
	$(SETPDEV2) $(DD)psrgb $(psim_)

### ---------------- Fax encoding ---------------- ###

# By default, these drivers recognize 3 page sizes -- (U.S.) letter, A4, and
# B4 -- and adjust the page width to the nearest legal value for real fax
# systems (1728 or 2048 pixels).  To suppress this, set the device parameter
# AdjustWidth to 0 (e.g., -dAdjustWidth=0 on the command line).

gdevfax_h=$(GLSRC)gdevfax.h

fax_=$(GLOBJ)gdevfax.$(OBJ)
$(DD)fax.dev : $(DEVS_MAK) $(fax_) $(GLD)cfe.dev
	$(SETMOD) $(DD)fax $(fax_)
	$(ADDMOD) $(DD)fax -include $(GLD)cfe

$(GLOBJ)gdevfax.$(OBJ) : $(GLSRC)gdevfax.c $(PDEVH)\
 $(gdevfax_h) $(scfx_h) $(strimpl_h)
	$(GLCC) $(GLO_)gdevfax.$(OBJ) $(C_) $(GLSRC)gdevfax.c

$(DD)faxg3.dev : $(DEVS_MAK) $(DD)fax.dev
	$(SETDEV2) $(DD)faxg3 -include $(DD)fax

$(DD)faxg32d.dev : $(DEVS_MAK) $(DD)fax.dev
	$(SETDEV2) $(DD)faxg32d -include $(DD)fax

$(DD)faxg4.dev : $(DEVS_MAK) $(DD)fax.dev
	$(SETDEV2) $(DD)faxg4 -include $(DD)fax

### -------------------- Plain or TIFF fax encoding --------------------- ###
###    Use -sDEVICE=tiffg3 or tiffg4 and				  ###
###	  -r204x98 for low resolution output, or			  ###
###	  -r204x196 for high resolution output				  ###

gdevtifs_h=$(GLSRC)gdevtifs.h
gdevtfax_h=$(GLSRC)gdevtfax.h

tfax_=$(GLOBJ)gdevtfax.$(OBJ)
$(DD)tfax.dev : $(DEVS_MAK) $(tfax_) $(GLD)cfe.dev $(GLD)lzwe.dev $(GLD)rle.dev $(DD)fax.dev $(DD)tiffs.dev
	$(SETMOD) $(DD)tfax $(tfax_)
	$(ADDMOD) $(DD)tfax -include $(GLD)cfe $(GLD)lzwe $(GLD)rle
	$(ADDMOD) $(DD)tfax -include $(DD)fax $(DD)tiffs

$(GLOBJ)gdevtfax.$(OBJ) : $(GLSRC)gdevtfax.c $(PDEVH)\
 $(gdevfax_h) $(gdevtfax_h) $(gdevtifs_h)\
 $(scfx_h) $(slzwx_h) $(srlx_h) $(strimpl_h)
	$(GLCC) $(GLO_)gdevtfax.$(OBJ) $(C_) $(GLSRC)gdevtfax.c

### ---------------------------- TIFF formats --------------------------- ###

tiffs_=$(GLOBJ)gdevtifs.$(OBJ)
$(DD)tiffs.dev : $(DEVS_MAK) $(tiffs_) $(GLD)page.dev
	$(SETMOD) $(DD)tiffs $(tiffs_)
	$(ADDMOD) $(DD)tiffs -include $(GLD)page

$(GLOBJ)gdevtifs.$(OBJ) : $(GLSRC)gdevtifs.c $(PDEVH) $(stdio__h) $(time__h)\
 $(gdevtifs_h) $(gscdefs_h) $(gstypes_h)
	$(GLCC) $(GLO_)gdevtifs.$(OBJ) $(C_) $(GLSRC)gdevtifs.c

# Black & white, G3/G4 fax
# NOTE: see under faxg* above regarding page width adjustment.

$(DD)tiffcrle.dev : $(DEVS_MAK) $(DD)tfax.dev
	$(SETDEV2) $(DD)tiffcrle -include $(DD)tfax

$(DD)tiffg3.dev : $(DEVS_MAK) $(DD)tfax.dev
	$(SETDEV2) $(DD)tiffg3 -include $(DD)tfax

$(DD)tiffg32d.dev : $(DEVS_MAK) $(DD)tfax.dev
	$(SETDEV2) $(DD)tiffg32d -include $(DD)tfax

$(DD)tiffg4.dev : $(DEVS_MAK) $(DD)tfax.dev
	$(SETDEV2) $(DD)tiffg4 -include $(DD)tfax

# Black & white, LZW compression

$(DD)tifflzw.dev : $(DEVS_MAK) $(DD)tfax.dev
	$(SETDEV2) $(DD)tifflzw -include $(DD)tfax

# Black & white, PackBits compression

$(DD)tiffpack.dev : $(DEVS_MAK) $(DD)tfax.dev
	$(SETDEV2) $(DD)tiffpack -include $(DD)tfax

# TIFF Gray, no compression

tiffgray_=$(GLOBJ)gdevtsep.$(OBJ)

$(DD)tiffgray.dev : $(DEVS_MAK) $(tiffgray_) $(DD)tiffs.dev
	$(SETPDEV2) $(DD)tiffgray $(tiffgray_)
	$(ADDMOD) $(DD)tiffgray -include $(DD)tiffs

$(GLOBJ)gdevtsep.$(OBJ) : $(GLSRC)gdevtsep.c $(PDEVH) $(gdevtifs_h)\
	$(gdevdevn_h) $(gsequivc_h)
	$(GLCC) $(GLO_)gdevtsep.$(OBJ) $(C_) $(GLSRC)gdevtsep.c

# TIFF RGB, no compression

tiffrgb_=$(GLOBJ)gdevtfnx.$(OBJ)

$(DD)tiff12nc.dev : $(DEVS_MAK) $(tiffrgb_) $(DD)tiffs.dev
	$(SETPDEV2) $(DD)tiff12nc $(tiffrgb_)
	$(ADDMOD) $(DD)tiff12nc -include $(DD)tiffs

$(DD)tiff24nc.dev : $(DEVS_MAK) $(tiffrgb_) $(DD)tiffs.dev
	$(SETPDEV2) $(DD)tiff24nc $(tiffrgb_)
	$(ADDMOD) $(DD)tiff24nc -include $(DD)tiffs

$(GLOBJ)gdevtfnx.$(OBJ) : $(GLSRC)gdevtfnx.c $(PDEVH) $(gdevtifs_h)
	$(GLCC) $(GLO_)gdevtfnx.$(OBJ) $(C_) $(GLSRC)gdevtfnx.c

# TIFF CMYK, no compression

$(DD)tiff32nc.dev : $(DEVS_MAK) $(tiffgray_) $(DD)tiffs.dev
	$(SETPDEV2) $(DD)tiff32nc $(tiffgray_)
	$(ADDMOD) $(DD)tiff32nc -include $(DD)tiffs
#
# Create separation files (tiffgray) plus CMYK composite (tiff32nc)

tiffsep_=$(tiffgray_) $(GLOBJ)gdevdevn.$(OBJ) $(GLOBJ)gsequivc.$(OBJ)

$(DD)tiffsep.dev : $(DEVS_MAK) $(tiffgray_) $(DD)tiffs.dev
	$(SETPDEV2) $(DD)tiffsep $(tiffsep_)
	$(ADDMOD) $(DD)tiffsep -include $(DD)tiffs

