#    Copyright (C) 1989, 1992, 1993, 1994, 1995 Aladdin Enterprises.  All rights reserved.
# 
# This file is part of Aladdin Ghostscript.
# 
# Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
# or distributor accepts any responsibility for the consequences of using it,
# or for whether it serves any particular purpose or works at all, unless he
# or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
# License (the "License") for full details.
# 
# Every copy of Aladdin Ghostscript must include a copy of the License,
# normally in a plain ASCII text file named PUBLIC.  The License grants you
# the right to copy, modify and redistribute Aladdin Ghostscript, but only
# under certain conditions described in the License.  Among other things, the
# License requires that the copyright notice and this notice be preserved on
# all copies.

# makefile for device drivers.

# -------------------------------- Catalog ------------------------------- #

# It is possible to build configurations with an arbitrary collection of
# device drivers, although some drivers are supported only on a subset
# of the target platforms.  The currently available drivers are:

# MS-DOS displays (note: not usable with Desqview/X):
#   MS-DOS EGA and VGA:
#	ega	EGA (640x350, 16-color)
#	vga	VGA (640x480, 16-color)
#   MS-DOS SuperVGA:
# +	atiw	ATI Wonder SuperVGA, 256-color modes
# +	s3vga	SuperVGA with S3 86C911 chip (e.g., Diamond Stealth board)
#	svga16	Generic SuperVGA in 800x600, 16-color mode
#		  (replaces atiw16, tseng16, and tvga16 -- see below)
#	tseng	SuperVGA using Tseng Labs ET3000/4000 chips, 256-color modes
#	tseng8	Tseng Labs SuperVGA using only 8 colors (for halftone testing)
# +	tvga	Trident SuperVGA, 256-color modes
#   ****** NOTE: The vesa device does not work with the Watcom (32-bit MS-DOS)
#   ****** compiler or executable.
#	vesa	SuperVGA with VESA standard API driver
#   MS-DOS other:
#	bgi	Borland Graphics Interface (CGA)  [MS-DOS only]
# *	herc	Hercules Graphics display   [MS-DOS only]
# *	pe	Private Eye display
# Other displays:
#   MS Windows:
#	mswin	Microsoft Windows 3.0, 3.1   [MS Windows only]
#	mswindll  Microsoft Windows 3.1 DLL    [MS Windows only]
#	mswinprn  Microsoft Windows 3.0, 3.1 printer   [MS Windows only]
#   OS/2:
# *	os2pm	OS/2 Presentation Manager   [OS/2 only]
# *	os2dll	OS/2 DLL bitmap             [OS/2 only]
#   Unix and VMS:
#   ****** NOTE: For direct frame buffer addressing under SCO Unix or Xenix,
#   ****** edit the definition of EGAVGA below.
# *	att3b1	AT&T 3b1/Unixpc monochrome display   [3b1 only]
# *	lvga256  Linux vgalib, 256-color VGA modes  [Linux only]
# *	sonyfb	Sony Microsystems monochrome display   [Sony only]
# *	sunview  SunView window system   [SunOS only]
# +	vgalib	Linux PC with VGALIB   [Linux only]
#	x11	X Windows version 11, release >=4   [Unix and VMS only]
#	x11alpha  X Windows masquerading as a device with alpha capability
#	x11cmyk  X Windows masquerading as a 1-bit-per-plane CMYK device
#	x11mono  X Windows masquerading as a black-and-white device
#   Platform-independent:
# *	sxlcrt	CRT sixels, e.g. for VT240-like terminals
# Printers:
# *	ap3250	Epson AP3250 printer
# *	appledmp  Apple Dot Matrix Printer (should also work with Imagewriter)
#	bj10e	Canon BubbleJet BJ10e
# *	bj200	Canon BubbleJet BJ200
# *	cdeskjet  H-P DeskJet 500C with 1 bit/pixel color
# *	cdjcolor  H-P DeskJet 500C with 24 bit/pixel color and
#		high-quality color (Floyd-Steinberg) dithering;
#		also good for DeskJet 540C
# *	cdjmono  H-P DeskJet 500C printing black only;
#		also good for DeskJet 510, 520, and 540C (black only)
# *	cdj500	H-P DeskJet 500C (same as cdjcolor)
# *	cdj550	H-P DeskJet 550C;
#		also good for DeskJet 560C
# *	cp50	Mitsubishi CP50 color printer
# *	declj250  alternate DEC LJ250 driver
# +	deskjet  H-P DeskJet and DeskJet Plus
#	djet500  H-P DeskJet 500
# *	djet500c  H-P DeskJet 500C alternate driver (does not work on 550C)
# *	dnj650c  H-P DesignJet 650C
#	epson	Epson-compatible dot matrix printers (9- or 24-pin)
# *	eps9mid  Epson-compatible 9-pin, interleaved lines
#		(intermediate resolution)
# *	eps9high  Epson-compatible 9-pin, interleaved lines
#		(triple resolution)
# *	epsonc	Epson LQ-2550 and Fujitsu 3400/2400/1200 color printers
# *	ibmpro  IBM 9-pin Proprinter
# *	imagen	Imagen ImPress printers
# *	iwhi	Apple Imagewriter in high-resolution mode
# *	iwlo	Apple Imagewriter in low-resolution mode
# *	iwlq	Apple Imagewriter LQ in 320 x 216 dpi mode
# *	jetp3852  IBM Jetprinter ink-jet color printer (Model #3852)
# +	laserjet  H-P LaserJet
# *	la50	DEC LA50 printer
# *	la70	DEC LA70 printer
# *	la70t	DEC LA70 printer with low-resolution text enhancement
# *	la75	DEC LA75 printer
# *	la75plus DEC LA75plus printer
# *	lbp8	Canon LBP-8II laser printer
# *	lips3	Canon LIPS III laser printer in English (CaPSL) mode
# *	ln03	DEC LN03 printer
# *	lj250	DEC LJ250 Companion color printer
# +	ljet2p	H-P LaserJet IId/IIp/III* with TIFF compression
# +	ljet3	H-P LaserJet III* with Delta Row compression
# +	ljet4	H-P LaserJet 4 (defaults to 600 dpi)
# +	lj4dith  H-P LaserJet 4 with Floyd-Steinberg dithering
# +	ljetplus  H-P LaserJet Plus
# *	lp2563	H-P 2563B line printer
# *	m8510	C.Itoh M8510 printer
# *	necp6	NEC P6/P6+/P60 printers at 360 x 360 DPI resolution
# *	nwp533  Sony Microsystems NWP533 laser printer   [Sony only]
# *	oki182	Okidata MicroLine 182
# *	paintjet  alternate H-P PaintJet color printer
# *	pj	H-P PaintJet XL driver 
# *	pjetxl	alternate H-P PaintJet XL driver
# *	pjxl	H-P PaintJet XL color printer
# *	pjxl300  H-P PaintJet XL300 color printer;
#		also good for PaintJet 1200C
# *	r4081	Ricoh 4081 laser printer
# *	sj48	StarJet 48 inkjet printer
# *	sparc	SPARCprinter
# *	st800	Epson Stylus 800 printer
# *	t4693d2  Tektronix 4693d color printer, 2 bits per R/G/B component
# *	t4693d4  Tektronix 4693d color printer, 4 bits per R/G/B component
# *	t4693d8  Tektronix 4693d color printer, 8 bits per R/G/B component
# *	tek4696  Tektronix 4695/4696 inkjet plotter
# *	xes	Xerox XES printers (2700, 3700, 4045, etc.)
# Fax systems:
# *	dfaxhigh  DigiBoard, Inc.'s DigiFAX software format (high resolution)
# *	dfaxlow  DigiFAX low (normal) resolution
# Fax file format:
#   ****** NOTE: all of these drivers adjust the page size to match
#   ****** one of the three CCITT standard sizes (U.S. letter with A4 width,
#   ****** A4, or B4).
#	faxg3	Group 3 fax, with EOLs but no header or EOD
#	faxg32d  Group 3 2-D fax, with EOLs but no header or EOD
#	faxg4	Group 4 fax, with EOLs but no header or EOD
#	tiffg3	Group 3 fax TIFF
#	tiffg32d  Group 3 2-D fax TIFF
#	tiffg4	Group 4 fax TIFF
# File formats and others:
#	bit	Plain bits, monochrome
#	bitrgb	Plain bits, RGB
#	bitcmyk  Plain bits, CMYK
#	bmpmono	Monochrome MS Windows .BMP file format
#	bmp16	4-bit (EGA/VGA) .BMP file format
#	bmp256	8-bit (256-color) .BMP file format
#	bmp16m	24-bit .BMP file format
# *	cif	CIF file format for VLSI
#	gifmono	Monochrome GIF file format
#	gif8	8-bit color GIF file format
# *	mgrmono  1-bit monochrome MGR devices
# *	mgrgray2  2-bit gray scale MGR devices
# *	mgrgray4  4-bit gray scale MGR devices
# *	mgrgray8  8-bit gray scale MGR devices
# *	mgr4	4-bit (VGA) color MGR devices
# *	mgr8	8-bit color MGR devices
#	pcxmono	Monochrome PCX file format
#	pcxgray	8-bit gray scale PCX file format
#	pcx16	4-bit planar (EGA/VGA) color PCX file format
#	pcx256	8-bit chunky color PCX file format
#	pcx24b	24-bit color PCX file format, 3 8-bit planes
#	psmono	Monochrome PostScript (Level 1) image
#	pbm	Portable Bitmap (plain format)
#	pbmraw	Portable Bitmap (raw format)
#	pgm	Portable Graymap (plain format)
#	pgmraw	Portable Graymap (raw format)
#	ppm	Portable Pixmap (plain format) (RGB)
#	ppmraw	Portable Pixmap (raw format) (RGB)

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

# ---------------------------- End of catalog ---------------------------- #

# As noted in gs.mak, DEVICE_DEVS and DEVICE_DEVS1..9 select the devices
# that should be included in a given configuration.  By convention,
# these are used as follows:
#	DEVICE_DEVS - the default device, and any display devices.
#	DEVICE_DEVS1 - additional display devices if needed.
#	DEVICE_DEVS2 - dot matrix printers.
#	DEVICE_DEVS3 - H-P monochrome printers.
#	DEVICE_DEVS4 - H-P color printers.
#	DEVICE_DEVS5 - additional H-P printers if needed.
#	DEVICE_DEVS6 - other ink-jet and laser printers.
#	DEVICE_DEVS7 - GIF, TIFF, and fax file formats.
#	DEVICE_DEVS8 - BMP and PCX file formats.
#	DEVICE_DEVS9 - PBM/PGM/PPM and bit file formats.
# Feel free to disregard this convention if it gets in your way.

# If you want to add a new device driver, the examples below should be
# enough of a guide to the correct form for the makefile rules.

# All device drivers depend on the following:
GDEV=$(AK) echogs$(XE) $(gserrors_h) $(gx_h) $(gxdevice_h)

# Define the header files for device drivers.  Every header file used by
# more than one device driver must be listed here.
gdev8bcm_h=gdev8bcm.h
gdevpccm_h=gdevpccm.h
gdevpcfb_h=gdevpcfb.h $(dos__h)
gdevpcl_h=gdevpcl.h
gdevsvga_h=gdevsvga.h
gdevx_h=gdevx.h

###### ----------------------- Device support ----------------------- ######

# Provide a mapping between StandardEncoding and ISOLatin1Encoding.
gdevemap.$(OBJ): gdevemap.c $(AK) $(std_h)

# Implement dynamic color management for 8-bit mapped color displays.
gdev8bcm.$(OBJ): gdev8bcm.c $(AK) \
  $(gx_h) $(gxdevice_h) $(gdev8bcm_h)

###### ------------------- MS-DOS display devices ------------------- ######

# There are really only three drivers: an EGA/VGA driver (4 bit-planes,
# plane-addressed), a SuperVGA driver (8 bit-planes, byte addressed),
# and a special driver for the S3 chip.

# PC display color mapping
gdevpccm.$(OBJ): gdevpccm.c $(AK) \
  $(gx_h) $(gsmatrix_h) $(gxdevice_h) $(gdevpccm_h)

### ----------------------- EGA and VGA displays ----------------------- ###

gdevegaa.$(OBJ): gdevegaa.asm

ETEST=ega.$(OBJ) $(ega_) gdevpcfb.$(OBJ) gdevpccm.$(OBJ) gdevegaa.$(OBJ)
ega.exe: $(ETEST) libc$(MM).tr
	$(COMPDIR)\tlink $(LCT) $(LO) $(LIBDIR)\c0$(MM) @ega.tr @libc$(MM).tr

ega.$(OBJ): ega.c $(GDEV)
	$(CCC) -v ega.c

# The shared MS-DOS makefile defines PCFBASM as either gdevegaa.$(OBJ)
# or an empty string.

# NOTE: for direct frame buffer addressing under SCO Unix or Xenix,
# change gdevevga to gdevsco in the following line.  Also, since
# SCO's /bin/as does not support the "out" instructions, you must build
# the gnu assembler and have it on your path as "as".
EGAVGA=gdevevga.$(OBJ) gdevpcfb.$(OBJ) gdevpccm.$(OBJ) $(PCFBASM)
#EGAVGA=gdevsco.$(OBJ) gdevpcfb.$(OBJ) gdevpccm.$(OBJ) $(PCFBASM)

gdevevga.$(OBJ): gdevevga.c $(GDEV) $(gdevpcfb_h)
	$(CCD) gdevevga.c

gdevsco.$(OBJ): gdevsco.c $(GDEV) $(gdevpcfb_h)

# Common code for MS-DOS and SCO.
gdevpcfb.$(OBJ): gdevpcfb.c $(GDEV) $(MAKEFILE) $(gconfigv_h) \
  $(gdevpccm_h) $(gdevpcfb_h) $(gsparam_h)
	$(CCD) gdevpcfb.c

# The EGA/VGA family includes EGA and VGA.  Many SuperVGAs in 800x600,
# 16-color mode can share the same code; see the next section below.

ega.dev: $(EGAVGA)
	$(SETDEV) ega $(EGAVGA)

vga.dev: $(EGAVGA)
	$(SETDEV) vga $(EGAVGA)

### ------------------------- SuperVGA displays ------------------------ ###

# SuperVGA displays in 16-color, 800x600 mode are really just slightly
# glorified VGA's, so we can handle them all with a single driver.
# The way to select them on the command line is with
#	-sDEVICE=svga16 -dDisplayMode=NNN
# where NNN is the display mode in decimal.  See use.doc for the modes
# for some popular display chipsets.

svga16.dev: $(EGAVGA)
	$(SETDEV) svga16 $(EGAVGA)

# More capable SuperVGAs have a wide variety of slightly differing
# interfaces, so we need a separate driver for each one.

SVGA=gdevsvga.$(OBJ) gdevpccm.$(OBJ) $(PCFBASM)

gdevsvga.$(OBJ): gdevsvga.c $(GDEV) $(MAKEFILE) $(gconfigv_h) \
  $(gxarith_h) $(gdevpccm_h) $(gdevpcfb_h) $(gdevsvga_h)
	$(CCD) gdevsvga.c

# The SuperVGA family includes: ATI Wonder, S3, Trident, Tseng ET3000/4000,
# and VESA.

atiw.dev: $(SVGA)
	$(SETDEV) atiw $(SVGA)

tseng.dev: $(SVGA)
	$(SETDEV) tseng $(SVGA)

tseng8.dev: $(SVGA)
	$(SETDEV) tseng8 $(SVGA)

tvga.dev: $(SVGA)
	$(SETDEV) tvga $(SVGA)

vesa.dev: $(SVGA)
	$(SETDEV) vesa $(SVGA)

# The S3 driver doesn't share much code with the others.

s3vga_=$(SVGA) gdevs3ga.$(OBJ) gdevsvga.$(OBJ) gdevpccm.$(OBJ)
s3vga.dev: $(s3vga_)
	$(SETDEV) s3vga $(s3vga_)

gdevs3ga.$(OBJ): gdevs3ga.c $(GDEV) $(MAKEFILE) $(gdevpcfb_h) $(gdevsvga_h)
	$(CCD) gdevs3ga.c

### ------------ The BGI (Borland Graphics Interface) device ----------- ###

cgaf.$(OBJ): $(BGIDIR)\cga.bgi
	$(BGIDIR)\bgiobj /F $(BGIDIR)\cga

egavgaf.$(OBJ): $(BGIDIR)\egavga.bgi
	$(BGIDIR)\bgiobj /F $(BGIDIR)\egavga

# Include egavgaf.$(OBJ) for debugging only.
bgi_=gdevbgi.$(OBJ) cgaf.$(OBJ)
bgi.dev: $(bgi_)
	$(SETDEV) bgi $(bgi_)
	$(ADDMOD) bgi -lib $(LIBDIR)\graphics

gdevbgi.$(OBJ): gdevbgi.c $(GDEV) $(MAKEFILE) $(gxxfont_h)
	$(CCC) -DBGI_LIB="$(BGIDIRSTR)" gdevbgi.c

### ------------------- The Hercules Graphics display ------------------- ###

herc_=gdevherc.$(OBJ)
herc.dev: $(herc_)
	$(SETDEV) herc $(herc_)

gdevherc.$(OBJ): gdevherc.c $(GDEV)
	$(CCC) gdevherc.c

###### ----------------------- Other displays ------------------------ ######

### ---------------------- The Private Eye display ---------------------- ###
### Note: this driver was contributed by a user:                          ###
###   please contact narf@media-lab.media.mit.edu if you have questions.  ###

pe_=gdevpe.$(OBJ)
pe.dev: $(pe_)
	$(SETDEV) pe $(pe_)

gdevpe.$(OBJ): gdevpe.c $(GDEV)

### -------------------- The MS-Windows 3.n display --------------------- ###

gdevmswn_h=gdevmswn.h $(GDEV) gp_mswin.h

# Choose one of gdevwddb or gdevwdib here.
mswin_=gdevmswn.$(OBJ) gdevmsxf.$(OBJ) gdevwdib.$(OBJ) \
  gdevemap.$(OBJ) gdevpccm.$(OBJ)
mswin.dev: $(mswin_)
	$(SETDEV) mswin $(mswin_)

gdevmswn.$(OBJ): gdevmswn.c $(gdevmswn_h) $(gp_h) $(gpcheck_h) \
  $(gsparam_h) $(gdevpccm_h)

gdevmsxf.$(OBJ): gdevmsxf.c $(ctype__h) $(math__h) $(memory__h) \
  $(gdevmswn_h) $(gsstruct_h) $(gsutil_h) $(gxxfont_h)

# An implementation using a device-dependent bitmap.
gdevwddb.$(OBJ): gdevwddb.c $(gdevmswn_h)

# An implementation using a DIB filled by an image device.
gdevwdib.$(OBJ): gdevwdib.c $(dos__h) $(gdevmswn_h)

### -------------------- The MS-Windows 3.n DLL ------------------------- ###

# Must use gdevwdib.  mswin device must use the same.
mswindll_=gdevmswn.$(OBJ) gdevmsxf.$(OBJ) gdevwdib.$(OBJ) \
  gdevemap.$(OBJ) gdevpccm.$(OBJ)
mswindll.dev: $(mswindll_)
	$(SETDEV) mswindll $(mswindll_)

### -------------------- The MS-Windows 3.n printer --------------------- ###

mswinprn_=gdevwprn.$(OBJ) gdevmsxf.$(OBJ)
mswinprn.dev: $(mswinprn_)
	$(SETDEV) mswinprn $(mswinprn_)

gdevwprn.$(OBJ): gdevwprn.c $(gdevmswn_h) $(gp_h)

### ------------------ OS/2 Presentation Manager device ----------------- ###

os2pm_=gdevpm.$(OBJ) gdevpccm.$(OBJ)
os2pm.dev: $(os2pm_)
	$(SETDEV) os2pm $(os2pm_)

os2dll_=gdevpm.$(OBJ) gdevpccm.$(OBJ)
os2dll.dev: $(os2dll_)
	$(SETDEV) os2dll $(os2dll_)

gdevpm.$(OBJ): gdevpm.c $(gp_h) $(gpcheck_h) \
  $(gsexit_h) $(gsparam_h) $(gdevpccm_h)

### -------------- The AT&T 3b1 Unixpc monochrome display --------------- ###
### Note: this driver was contributed by a user: please contact           ###
###       Andy Fyfe (andy@cs.caltech.edu) if you have questions.          ###

att3b1_=gdev3b1.$(OBJ)
att3b1.dev: $(att3b1_)
	$(SETDEV) att3b1 $(att3b1_)

gdev3b1.$(OBJ): gdev3b1.c

### ------------------------- DEC sixel displays ------------------------ ###
### Note: this driver was contributed by a user: please contact           ###
###   Phil Keegstra (keegstra@tonga.gsfc.nasa.gov) if you have questions. ###

# This is a "printer" device, but it probably shouldn't be.
# I don't know why the implementor chose to do it this way.
sxlcrt_=gdevln03.$(OBJ) gdevprn.$(OBJ)
sxlcrt.dev: $(sxlcrt_)
	$(SETDEV) sxlcrt $(sxlcrt_)

###### --------------- Memory-buffered printer devices --------------- ######

PDEVH=$(GDEV) $(gdevprn_h)

gdevprn.$(OBJ): gdevprn.c $(PDEVH) $(gp_h) $(gsparam_h)

### --------------------- The Apple printer devices --------------------- ###
### Note: these drivers were contributed by users.                        ###
###   If you have questions about the DMP driver, please contact          ###
###	Mark Wedel (master@cats.ucsc.edu).                                ###
###   If you have questions about the Imagewriter drivers, please contact ###
###	Jonathan Luckey (luckey@rtfm.mlb.fl.us).                          ###
###   If you have questions about the Imagewriter LQ driver, please       ###
###	contact Scott Barker (barkers@cuug.ab.ca).                        ###

appledmp_=gdevadmp.$(OBJ) gdevprn.$(OBJ)

gdevadmp.$(OBJ): gdevadmp.c $(GDEV) $(gdevprn_h)

appledmp.dev: $(appledmp_)
	$(SETDEV) appledmp $(appledmp_)

iwhi.dev: $(appledmp_)
	$(SETDEV) iwhi $(appledmp_)

iwlo.dev: $(appledmp_)
	$(SETDEV) iwlo $(appledmp_)

iwlq.dev: $(appledmp_)
	$(SETDEV) iwlq $(appledmp_)

### ------------ The Canon BubbleJet BJ10e and BJ200 devices ------------ ###

bj10e_=gdevbj10.$(OBJ) gdevprn.$(OBJ)

bj10e.dev: $(bj10e_)
	$(SETDEV) bj10e $(bj10e_)

bj200.dev: $(bj10e_)
	$(SETDEV) bj200 $(bj10e_)

gdevbj10.$(OBJ): gdevbj10.c $(PDEVH)

### ----------- The H-P DeskJet and LaserJet printer devices ----------- ###

### These are essentially the same device.
### NOTE: printing at full resolution (300 DPI) requires a printer
###   with at least 1.5 Mb of memory.  150 DPI only requires .5 Mb.
### Note that the lj4dith driver is included with the H-P color printer
###   drivers below.

HPPCL=gdevprn.$(OBJ) gdevpcl.$(OBJ)
HPMONO=gdevdjet.$(OBJ) $(HPPCL)

gdevpcl.$(OBJ): gdevpcl.c $(PDEVH) $(gdevpcl_h)

gdevdjet.$(OBJ): gdevdjet.c $(PDEVH) $(gdevpcl_h)

deskjet.dev: $(HPMONO)
	$(SETDEV) deskjet $(HPMONO)

djet500.dev: $(HPMONO)
	$(SETDEV) djet500 $(HPMONO)

laserjet.dev: $(HPMONO)
	$(SETDEV) laserjet $(HPMONO)

ljetplus.dev: $(HPMONO)
	$(SETDEV) ljetplus $(HPMONO)

### Selecting ljet2p provides TIFF (mode 2) compression on LaserJet III,
### IIIp, IIId, IIIsi, IId, and IIp. 

ljet2p.dev: $(HPMONO)
	$(SETDEV) ljet2p $(HPMONO)

### Selecting ljet3 provides Delta Row (mode 3) compression on LaserJet III,
### IIIp, IIId, IIIsi.

ljet3.dev: $(HPMONO)
	$(SETDEV) ljet3 $(HPMONO)

### Selecting ljet4 also provides Delta Row compression on LaserJet IV series.

ljet4.dev: $(HPMONO)
	$(SETDEV) ljet4 $(HPMONO)

lp2563.dev: $(HPMONO)
	$(SETDEV) lp2563 $(HPMONO)

### The H-P DeskJet, PaintJet, and DesignJet family color printer devices.###
### Note: there are two different 500C drivers, both contributed by users.###
###   If you have questions about the djet500c driver,                    ###
###       please contact AKayser@et.tudelft.nl.                           ###
###   If you have questions about the cdj* drivers,                       ###
###       please contact g.cameron@biomed.abdn.ac.uk.                     ###
###   If you have questions about the dnj560c driver,                     ###
###       please contact koert@zen.cais.com.                              ###
###   If you have questions about the lj4dith driver,                     ###
###       please contact Eckhard.Rueggeberg@ts.go.dlr.de.                 ###

cdeskjet_=gdevcdj.$(OBJ) $(HPPCL)

cdeskjet.dev: $(cdeskjet_)
	$(SETDEV) cdeskjet $(cdeskjet_)

cdjcolor.dev: $(cdeskjet_)
	$(SETDEV) cdjcolor $(cdeskjet_)

cdjmono.dev: $(cdeskjet_)
	$(SETDEV) cdjmono $(cdeskjet_)

cdj500.dev: $(cdeskjet_)
	$(SETDEV) cdj500 $(cdeskjet_)

cdj550.dev: $(cdeskjet_)
	$(SETDEV) cdj550 $(cdeskjet_)

declj250.dev: $(cdeskjet_)
	$(SETDEV) declj250 $(cdeskjet_)

dnj650c.dev: $(cdeskjet_)
	$(SETDEV) dnj650c $(cdeskjet_)

lj4dith.dev: $(cdeskjet_)
	$(SETDEV) lj4dith $(cdeskjet_)

pj.dev: $(cdeskjet_)
	$(SETDEV) pj $(cdeskjet_)

pjxl.dev: $(cdeskjet_)
	$(SETDEV) pjxl $(cdeskjet_)

pjxl300.dev: $(cdeskjet_)
	$(SETDEV) pjxl300 $(cdeskjet_)

# NB: you can also customise the build if required, using
# -DBitsPerPixel=<number> if you wish the default to be other than 24
# for the generic drivers (cdj500, cdj550, pjxl300, pjtest, pjxltest).
gdevcdj.$(OBJ): gdevcdj.c $(PDEVH) $(gdevpcl_h)
	$(CCC) gdevcdj.c

djet500c_=gdevdjtc.$(OBJ) $(HPPCL)
djet500c.dev: $(djet500c_)
	$(SETDEV) djet500c $(djet500c_)

gdevdjtc.$(OBJ): gdevdjtc.c $(PDEVH) $(gdevpcl_h)

### -------------------- The Mitsubishi CP50 printer -------------------- ###
### Note: this driver was contributed by a user: please contact           ###
###       Michael Hu (michael@ximage.com) if you have questions.          ###

cp50_=gdevcp50.$(OBJ) gdevprn.$(OBJ)
cp50.dev: $(cp50_)
	$(SETDEV) cp50 $(cp50_)

gdevcp50.$(OBJ): gdevcp50.c $(PDEVH)

### ----------------- The generic Epson printer device ----------------- ###
### Note: most of this code was contributed by users.  Please contact    ###
###       the following people if you have questions:                    ###
###   eps9mid - Guenther Thomsen (thomsen@cs.tu-berlin.de)               ###
###   eps9high - David Wexelblat (dwex@mtgzfs3.att.com)                  ###
###   ibmpro - James W. Birdsall (jwbirdsa@picarefy.picarefy.com)        ###

epson_=gdevepsn.$(OBJ) gdevprn.$(OBJ)

epson.dev: $(epson_)
	$(SETDEV) epson $(epson_)

eps9mid.dev: $(epson_)
	$(SETDEV) eps9mid $(epson_)

eps9high.dev: $(epson_)
	$(SETDEV) eps9high $(epson_)

gdevepsn.$(OBJ): gdevepsn.c $(PDEVH)

### ----------------- The IBM Proprinter printer device ---------------- ###

ibmpro.dev: $(epson_)
	$(SETDEV) ibmpro $(epson_)

### -------------- The Epson LQ-2550 color printer device -------------- ###
### Note: this driver was contributed by users: please contact           ###
###       Dave St. Clair (dave@exlog.com) if you have questions.         ###

epsonc_=gdevepsc.$(OBJ) gdevprn.$(OBJ)
epsonc.dev: $(epsonc_)
	$(SETDEV) epsonc $(epsonc_)

gdevepsc.$(OBJ): gdevepsc.c $(PDEVH)

### ------------ The Epson ESC/P 2 language printer devices ------------ ###
### Note: these drivers were contributed by a user: please contact       ###
###       Richard Brown (rab@tauon.ph.unimelb.edu.au) with any questions.###
### This driver supports the Stylus 800 and AP3250 printers.             ###

ESCP2=gdevescp.$(OBJ) gdevprn.$(OBJ)

gdevescp.$(OBJ): gdevescp.c $(PDEVH)

ap3250.dev: $(ESCP2)
	$(SETDEV) ap3250 $(ESCP2)

st800.dev: $(ESCP2)
	$(SETDEV) st800 $(ESCP2)

### ------------ The H-P PaintJet color printer device ----------------- ###
### Note: this driver also supports the DEC LJ250 color printer, which   ###
###       has a PaintJet-compatible mode, and the PaintJet XL.           ###
### If you have questions about the XL, please contact Rob Reiss         ###
###       (rob@moray.berkeley.edu).                                      ###

PJET=gdevpjet.$(OBJ) $(HPPCL)

gdevpjet.$(OBJ): gdevpjet.c $(PDEVH) $(gdevpcl_h)

lj250.dev: $(PJET)
	$(SETDEV) lj250 $(PJET)

paintjet.dev: $(PJET)
	$(SETDEV) paintjet $(PJET)

pjetxl.dev: $(PJET)
	$(SETDEV) pjetxl $(PJET)

### -------------- Imagen ImPress Laser Printer device ----------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Alan Millar (AMillar@bolis.sf-bay.org) if you have questions.  ###
### Set USE_BYTE_STREAM if using parallel interface;                     ###
### Don't set it if using 'ipr' spooler (default).                       ###
### You may also add -DA4 if needed for A4 paper.			 ###

imagen_=gdevimgn.$(OBJ) gdevprn.$(OBJ)
imagen.dev: $(imagen_)
	$(SHP)gssetdev imagen $(imagen_)

gdevimgn.$(OBJ): gdevimgn.c $(PDEVH)
	$(CCC) gdevimgn.c			# for ipr spooler
#	$(CCC) -DUSE_BYTE_STREAM gdevimgn.c	# for parallel

### ------- The IBM 3852 JetPrinter color inkjet printer device -------- ###
### Note: this driver was contributed by users: please contact           ###
###       Kevin Gift (kgift@draper.com) if you have questions.           ###
### Note that the paper size that can be addressed by the graphics mode  ###
###   used in this driver is fixed at 7-1/2 inches wide (the printable   ###
###   width of the jetprinter itself.)                                   ###

jetp3852_=gdev3852.$(OBJ) gdevprn.$(OBJ)
jetp3852.dev: $(jetp3852_)
	$(SETDEV) jetp3852 $(jetp3852_)

gdev3852.$(OBJ): gdev3852.c $(PDEVH) $(gdevpcl_h)

### ---------- The Canon LBP-8II and LIPS III printer devices ---------- ###
### Note: these drivers were contributed by users.                       ###
### For questions about the LBP8 driver, please contact                  ###
###       Tom Quinn (trq@prg.oxford.ac.uk).                              ###
### For questions about the LIPS III driver, please contact              ###
###       Kenji Okamoto (okamoto@okamoto.cias.osakafu-u.ac.jp).          ###

lbp8_=gdevlbp8.$(OBJ) gdevprn.$(OBJ)
lbp8.dev: $(lbp8_)
	$(SETDEV) lbp8 $(lbp8_)

lips3.dev: $(lbp8_)
	$(SETDEV) lips3 $(lips3_)

gdevlbp8.$(OBJ): gdevlbp8.c $(PDEVH)

### ----------- The DEC LN03/LA50/LA70/LA75 printer devices ------------ ###
### Note: this driver was contributed by users: please contact           ###
###       Ulrich Mueller (ulm@vsnhd1.cern.ch) if you have questions.     ###
### For questions about LA50 and LA75, please contact                    ###
###       Ian MacPhedran (macphed@dvinci.USask.CA).                      ###
### For questions about the LA70, please contact                         ###
###       Bruce Lowekamp (lowekamp@csugrad.cs.vt.edu).                   ###
### For questions about the LA75plus, please contact                     ###
###       Andre' Beck (Andre_Beck@IRS.Inf.TU-Dresden.de).                ###

ln03_=gdevln03.$(OBJ) gdevprn.$(OBJ)
ln03.dev: $(ln03_)
	$(SETDEV) ln03 $(ln03_)

la50.dev: $(ln03_)
	$(SETDEV) la50 $(ln03_)

la70.dev: $(ln03_)
	$(SETDEV) la70 $(ln03_)

la75.dev: $(ln03_)
	$(SETDEV) la75 $(ln03_)

la75plus.dev: $(ln03_)
	$(SETDEV) la75plus $(ln03_)

gdevln03.$(OBJ): gdevln03.c $(PDEVH)

# LA70 driver with low-resolution text enhancement.

la70t_=gdevla7t.$(OBJ) gdevprn.$(OBJ)
la70t.dev: $(la70t_)
	$(SETDEV) la70t $(la70t_)

gdevla7t.$(OBJ): gdevla7t.c $(PDEVH)

### -------------- The C.Itoh M8510 printer device --------------------- ###
### Note: this driver was contributed by a user: please contact Bob      ###
###       Smith <bob@snuffy.penfield.ny.us> if you have questions.       ###

m8510_=gdev8510.$(OBJ) gdevprn.$(OBJ)
m8510.dev: $(m8510_)
	$(SETDEV) m8510 $(m8510_)

gdev8510.$(OBJ): gdev8510.c $(PDEVH)

### --------------------- The NEC P6 family devices -------------------- ###

necp6_=gdevnp6.$(OBJ) gdevprn.$(OBJ)
necp6.dev: $(necp6_)
	$(SETDEV) necp6 $(necp6_)

gdevnp6.$(OBJ): gdevnp6.c $(PDEVH)

### ----------------- The Okidata MicroLine 182 device ----------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Maarten Koning (smeg@bnr.ca) if you have questions.            ###

oki182_=gdevo182.$(OBJ) gdevprn.$(OBJ)
oki182.dev: $(oki182_)
	$(SETDEV) oki182 $(oki182_)

gdevo182.$(OBJ): gdevo182.c $(PDEVH)

### ------------- The Ricoh 4081 laser printer device ------------------ ###
### Note: this driver was contributed by users:                          ###
###       please contact kdw@oasis.icl.co.uk if you have questions.      ###

r4081_=gdev4081.$(OBJ) gdevprn.$(OBJ)
r4081.dev: $(r4081_)
	$(SETDEV) r4081 $(r4081_)

gdev4081.$(OBJ): gdev4081.c $(PDEVH)

###### ------------------------ Sony devices ------------------------ ######
### Note: these drivers were contributed by users: please contact        ###
###       Mike Smolenski (mike@intertech.com) if you have questions.     ###

### ------------------- Sony NeWS frame buffer device ------------------ ###

sonyfb_=gdevsnfb.$(OBJ) gdevprn.$(OBJ)
sonyfb.dev: $(sonyfb_)
	$(SETDEV) sonyfb $(sonyfb_)

gdevsnfb.$(OBJ): gdevsnfb.c $(PDEVH)

### -------------------- Sony NWP533 printer device -------------------- ###
### Note: this driver was contributed by a user: please contact Tero     ###
###       Kivinen (kivinen@joker.cs.hut.fi) if you have questions.       ###

nwp533_=gdevn533.$(OBJ) gdevprn.$(OBJ)
nwp533.dev: $(nwp533_)
	$(SETDEV) nwp533 $(nwp533_)

gdevn533.$(OBJ): gdevn533.c $(PDEVH)

### ------------------------- The SPARCprinter ------------------------- ###
### Note: this driver was contributed by users: please contact Martin    ###
###       Schulte (schulte@thp.uni-koeln.de) if you have questions.      ###
###       He would also like to hear from anyone using the driver.       ###
### Please consult the source code for additional documentation.         ###

sparc_=gdevsppr.$(OBJ) gdevprn.$(OBJ)
sparc.dev: $(sparc_)
	$(SETDEV) sparc $(sparc_)

gdevsppr.$(OBJ): gdevsppr.c $(PDEVH)

### ----------------- The StarJet SJ48 device -------------------------- ###
### Note: this driver was contributed by a user: if you have questions,  ###
###	                      .                                          ###
###       please contact Mats Akerblom (f86ma@dd.chalmers.se).           ###

sj48_=gdevsj48.$(OBJ) gdevprn.$(OBJ)
sj48.dev: $(sj48_)
	$(SETDEV) sj48 $(sj48_)

gdevsj48.$(OBJ): gdevsj48.c $(PDEVH)

###### --------------------- The SunView device --------------------- ######
### Note: this driver is maintained by a user: if you have questions,    ###
###       please contact Andreas Stolcke (stolcke@icsi.berkeley.edu).    ###

sunview_=gdevsun.$(OBJ)
sunview.dev: $(sunview_)
	$(SETDEV) sunview $(sunview_)
	$(ADDMOD) sunview -lib suntool sunwindow pixrect

gdevsun.$(OBJ): gdevsun.c $(GDEV) $(malloc__h) $(gscdefs_h) $(gsmatrix_h)

### ----------------- Tektronix 4396d color printer -------------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Karl Hakimian (hakimian@haney.eecs.wsu.edu)                    ###
###       if you have questions.                                         ###

t4693d_=gdev4693.$(OBJ) gdevprn.$(OBJ)
t4693d2.dev: $(t4693d_)
	$(SETDEV) t4693d2 $(t4693d_)

t4693d4.dev: $(t4693d_)
	$(SETDEV) t4693d4 $(t4693d_)

t4693d8.dev: $(t4693d_)
	$(SETDEV) t4693d8 $(t4693d_)

gdev4693.$(OBJ): gdev4693.c $(GDEV)

### -------------------- Tektronix ink-jet printers -------------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Karsten Spang (spang@nbivax.nbi.dk) if you have questions.     ###

tek4696_=gdevtknk.$(OBJ) gdevprn.$(OBJ)
tek4696.dev: $(tek4696_)
	$(SETDEV) tek4696 $(tek4696_)

gdevtknk.$(OBJ): gdevtknk.c $(PDEVH)

###### ------------------------- Fax devices ------------------------- ######

### --------------- Generic PostScript system compatible fax ------------ ###

# This code doesn't work yet.  Don't even think about using it.

PSFAX=gdevpfax.$(OBJ) gdevprn.$(OBJ)

psfax_=$(PSFAX)
psfax.dev: $(psfax_)
	$(SETDEV) psfax $(psfax_)
	$(ADDMOD) psfax -iodev Fax

gdevpfax.$(OBJ): gdevpfax.c $(PDEVH) $(gxiodev_h)

### ------------------------- The DigiFAX device ------------------------ ###
###    This driver outputs images in a format suitable for use with       ###
###    DigiBoard, Inc.'s DigiFAX software.  Use -sDEVICE=dfaxhigh for     ###
###    high resolution output, -sDEVICE=dfaxlow for normal output.        ###
### Note: this driver was contributed by a user: please contact           ###
###       Rick Richardson (rick@digibd.com) if you have questions.        ###

dfax_=gdevdfax.$(OBJ) gdevtfax.$(OBJ) gdevprn.$(OBJ) $(scfe_)

dfaxlow.dev: $(dfax_)
	$(SETDEV) dfaxlow $(dfax_)

dfaxhigh.dev: $(dfax_)
	$(SETDEV) dfaxhigh $(dfax_)

gdevdfax.$(OBJ): gdevdfax.c $(GDEV) $(gdevprn_h)

###### ------------------- Linux PC with vgalib ---------------------- ######
### Note: these drivers were contributed by users.                        ###
### For questions about the lvga256 driver, please contact                ###
###       Ludger Kunz (ludger.kunz@fernuni-hagen.de).                     ###
### For questions about the vgalib driver, please contact                 ###
###       Erik Talvola (talvola@gnu.ai.mit.edu).                          ###

lvga256_=gdevl256.$(OBJ)
lvga256.dev: $(lvga256_)
	$(SETDEV) lvga256 $(lvga256_)
	$(ADDMOD) lvga256 -lib vga vgagl

gdevl256.$(OBJ): gdevl256.c $(GDEV) $(MAKEFILE) $(gxxfont_h)

vgalib_=gdevvglb.$(OBJ)
vgalib.dev: $(vgalib_)
	$(SETDEV) vgalib $(vgalib_)
	$(ADDMOD) vgalib -lib vga

gdevvglb.$(OBJ): gdevvglb.c $(GDEV)

###### ----------------------- The X11 device ----------------------- ######

# Aladdin Enterprises does not support Ghostview.  For more information
# about Ghostview, please contact Tim Theisen (ghostview@cs.wisc.edu).

# See the main makefile for the definition of XLIBS.
x11_=gdevx.$(OBJ) gdevxini.$(OBJ) gdevxxf.$(OBJ) gdevemap.$(OBJ)
x11.dev: $(x11_)
	$(SETDEV) x11 $(x11_)
	$(ADDMOD) x11 -lib $(XLIBS)

# See the main makefile for the definition of XINCLUDE.
GDEVX=$(GDEV) x_.h gdevx.h $(MAKEFILE)
gdevx.$(OBJ): gdevx.c $(GDEVX) $(gsparam_h)
	$(CCC) $(XINCLUDE) gdevx.c

gdevxini.$(OBJ): gdevxini.c $(GDEVX) $(ctype__h)
	$(CCC) $(XINCLUDE) gdevxini.c

gdevxxf.$(OBJ): gdevxxf.c $(GDEVX) $(gsstruct_h) $(gsutil_h) $(gxxfont_h)
	$(CCC) $(XINCLUDE) gdevxxf.c

# Alternate X11-based devices to help debug other drivers.
# x11alpha pretends to have 4 bits of alpha channel.
# x11cmyk pretends to be a CMYK device with 1 bit each of C,M,Y,K.
# x11mono pretends to be a black-and-white device.
x11alt_=$(x11_) gdevxalt.$(OBJ)
x11alpha.dev: $(x11alt_)
	$(SETDEV) x11alpha $(x11alt_)
	$(ADDMOD) x11alpha -lib $(XLIBS)

x11cmyk.dev: $(x11alt_)
	$(SETDEV) x11cmyk $(x11alt_)
	$(ADDMOD) x11cmyk -lib $(XLIBS)

x11mono.dev: $(x11alt_)
	$(SETDEV) x11mono $(x11alt_)
	$(ADDMOD) x11mono -lib $(XLIBS)

gdevxalt.$(OBJ): gdevxalt.c $(GDEVX) $(PDEVH)
	$(CCC) $(XINCLUDE) gdevxalt.c

### ----------------- The Xerox XES printer device --------------------- ###
### Note: this driver was contributed by users: please contact           ###
###       Peter Flass (flass@lbdrscs.bitnet) if you have questions.      ###

xes_=gdevxes.$(OBJ) gdevprn.$(OBJ)
xes.dev: $(xes_)
	$(SETDEV) xes $(xes_)

gdevxes.$(OBJ): gdevxes.c $(GDEV)

### --------------------- The "plain bits" devices ---------------------- ###

bit_=gdevbit.$(OBJ) gdevprn.$(OBJ)

bit.dev: $(bit_)
	$(SETDEV) bit $(bit_)

bitrgb.dev: $(bit_)
	$(SETDEV) bitrgb $(bit_)

bitcmyk.dev: $(bit_)
	$(SETDEV) bitcmyk $(bit_)

gdevbit.$(OBJ): gdevbit.c $(PDEVH) $(gsparam_h) $(gxlum_h)

###### ----------------------- PC file formats ----------------------- ######

### ------------------------- .BMP file formats ------------------------- ###

bmp_=gdevbmp.$(OBJ) gdevpccm.$(OBJ) gdevprn.$(OBJ)

gdevbmp.$(OBJ): gdevbmp.c $(PDEVH) $(gdevpccm_h)

bmpmono.dev: $(bmp_)
	$(SETDEV) bmpmono $(bmp_)

bmp16.dev: $(bmp_)
	$(SETDEV) bmp16 $(bmp_)

bmp256.dev: $(bmp_)
	$(SETDEV) bmp256 $(bmp_)

bmp16m.dev: $(bmp_)
	$(SETDEV) bmp16m $(bmp_)

### -------------------- The CIF file format for VLSI ------------------ ###
### Note: this driver was contributed by a user: please contact          ###
###       Frederic Petrot (petrot@masi.ibp.fr) if you have questions.    ###

cif_=gdevcif.$(OBJ) gdevprn.$(OBJ)
cif.dev: $(cif_)
	$(SETDEV) cif $(cif_)

gdevcif.$(OBJ): gdevcif.c $(PDEVH)

### ------------------------- GIF file formats ------------------------- ###

GIF=gdevgif.$(OBJ) gdevpccm.$(OBJ) gdevprn.$(OBJ)

gdevgif.$(OBJ): gdevgif.c $(PDEVH) $(gdevpccm_h)

gifmono.dev: $(GIF)
	$(SETDEV) gifmono $(GIF)

gif8.dev: $(GIF)
	$(SETDEV) gif8 $(GIF)

### --------------------------- MGR devices ---------------------------- ###
### Note: these drivers were contributed by a user: please contact       ###
###       Carsten Emde (carsten@ce.pr.net.ch) if you have questions.     ###

MGR=gdevmgr.$(OBJ) gdevpccm.$(OBJ) gdevprn.$(OBJ)

gdevmgr.$(OBJ): gdevmgr.c $(PDEVH) $(gdevpccm_h) gdevmgr.h

mgrmono.dev: $(MGR)
	$(SETDEV) mgrmono $(MGR)

mgrgray2.dev: $(MGR)
	$(SETDEV) mgrgray2 $(MGR)

mgrgray4.dev: $(MGR)
	$(SETDEV) mgrgray4 $(MGR)

mgrgray8.dev: $(MGR)
	$(SETDEV) mgrgray8 $(MGR)

mgr4.dev: $(MGR)
	$(SETDEV) mgr4 $(MGR)

mgr8.dev: $(MGR)
	$(SETDEV) mgr8 $(MGR)

### ------------------------- PCX file formats ------------------------- ###

pcx_=gdevpcx.$(OBJ) gdevpccm.$(OBJ) gdevprn.$(OBJ)

gdevpcx.$(OBJ): gdevpcx.c $(PDEVH) $(gdevpccm_h) $(gxlum_h)

pcxmono.dev: $(pcx_)
	$(SETDEV) pcxmono $(pcx_)

pcxgray.dev: $(pcx_)
	$(SETDEV) pcxgray $(pcx_)

pcx16.dev: $(pcx_)
	$(SETDEV) pcx16 $(pcx_)

pcx256.dev: $(pcx_)
	$(SETDEV) pcx256 $(pcx_)

pcx24b.dev: $(pcx_)
	$(SETDEV) pcx24b $(pcx_)

###### ---------------- Portable Bitmap file formats ---------------- ######
### For more information, see the pbm(5), pgm(5), and ppm(5) man pages.  ###

pxm_=gdevpbm.$(OBJ) gdevprn.$(OBJ)

gdevpbm.$(OBJ): gdevpbm.c $(PDEVH) $(gscdefs_h) $(gxlum_h)

### Portable Bitmap (PBM, plain or raw format, magic numbers "P1" or "P4")

pbm.dev: $(pxm_)
	$(SETDEV) pbm $(pxm_)

pbmraw.dev: $(pxm_)
	$(SETDEV) pbmraw $(pxm_)

### Portable Graymap (PGM, plain or raw format, magic numbers "P2" or "P5")

pgm.dev: $(pxm_)
	$(SETDEV) pgm $(pxm_)

pgmraw.dev: $(pxm_)
	$(SETDEV) pgmraw $(pxm_)

### Portable Pixmap (PPM, plain or raw format, magic numbers "P3" or "P6")

ppm.dev: $(pxm_)
	$(SETDEV) ppm $(pxm_)

ppmraw.dev: $(pxm_)
	$(SETDEV) ppmraw $(pxm_)

### ---------------------- PostScript image format ---------------------- ###
### These devices make it possible to print Level 2 files on a Level 1    ###
###   printer, by converting them to a bitmap in PostScript format.       ###

ps_=gdevpsim.$(OBJ) gdevprn.$(OBJ)

gdevpsim.$(OBJ): gdevpsim.c $(GDEV) $(gdevprn_h)

psmono.dev: $(ps_)
	$(SETDEV) psmono $(ps_)

# Someday there will be RGB and CMYK variants....

### -------------------- Plain or TIFF fax encoding --------------------- ###
###    Use -sDEVICE=tiffg3 or tiffg4 and				  ###
###	  -r204x98 for low resolution output, or			  ###
###	  -r204x196 for high resolution output				  ###
###    These drivers recognize 3 page sizes: letter, A4, and B4.	  ###

tfax_=gdevtfax.$(OBJ) gdevprn.$(OBJ) $(scfe_)

gdevtfax.$(OBJ): gdevtfax.c $(GDEV) $(gdevprn_h) \
  $(time__h) $(gscdefs_h) gdevtifs.h

### Plain G3/G4 fax with no header

faxg3.dev: $(tfax_)
	$(SETDEV) faxg3 $(tfax_)

faxg32d.dev: $(tfax_)
	$(SETDEV) faxg32d $(tfax_)

faxg4.dev: $(tfax_)
	$(SETDEV) faxg4 $(tfax_)

### G3/G4 fax TIFF

tiffg3.dev: $(tfax_)
	$(SETDEV) tiffg3 $(tfax_)

tiffg32d.dev: $(tfax_)
	$(SETDEV) tiffg32d $(tfax_)

tiffg4.dev: $(tfax_)
	$(SETDEV) tiffg4 $(tfax_)
