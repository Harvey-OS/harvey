#    Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.
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

# $Id: contrib.mak,v 1.28 2002/10/16 23:26:33 giles Exp $
# makefile for contributed device drivers.

# Define the name of this makefile.
CONTRIB_MAK=$(GLSRC)contrib.mak

###### --------------------------- Catalog -------------------------- ######

# The following drivers are user-contributed, and maintained (if at all) by
# users.  Please report problems in these drivers to their authors, whose
# e-mail addresses appear below: do not report them to mailing lists or
# mailboxes for general Ghostscript problems.

# Displays:
#   MS-DOS (note: not usable with Desqview/X):
#	herc	Hercules Graphics display   [MS-DOS only]
#	pe	Private Eye display
#   Unix and VMS:
#	att3b1	AT&T 3b1/Unixpc monochrome display   [3b1 only]
#	sonyfb	Sony Microsystems monochrome display   [Sony only]
#	sunview  SunView window system   [SunOS only]
# Printers:
#	ap3250	Epson AP3250 printer
#	appledmp  Apple Dot Matrix Printer (should also work with Imagewriter)
#	bj10e	Canon BubbleJet BJ10e
#	bj200	Canon BubbleJet BJ200; also good for BJ300 in ProPrinter mode
#		(see comments in source code)
#	bjc600   Canon Color BubbleJet BJC-600, BJC-4000 and BJC-70
#               also good for Apple printers like the StyleWriter 2x00
#	bjc800   Canon Color BubbleJet BJC-800
#	ccr     CalComp Raster format
#	cdeskjet  H-P DeskJet 500C with 1 bit/pixel color
#	cdjcolor  H-P DeskJet 500C with 24 bit/pixel color and
#		high-quality color (Floyd-Steinberg) dithering;
#		also good for DeskJet 540C and Citizen Projet IIc (-r200x300)
#	cdjmono  H-P DeskJet 500C printing black only;
#		also good for DeskJet 510, 520, and 540C (black only)
#	cdj500	H-P DeskJet 500C (same as cdjcolor)
#	cdj550	H-P DeskJet 550C/560C/660C/660Cse
#	cljet5	H-P Color LaserJet 5/5M (see below for some notes)
#	cljet5c  H-P Color LaserJet 5/5M (see below for some notes)
#	coslw2p  CoStar LabelWriter II II/Plus
#	coslwxl  CoStar LabelWriter XL
#	cp50	Mitsubishi CP50 color printer
#	declj250  alternate DEC LJ250 driver
#	djet500c  H-P DeskJet 500C alternate driver
#		(does not work on 550C or 560C)
#	dnj650c  H-P DesignJet 650C
#	epson	Epson-compatible dot matrix printers (9- or 24-pin)
#	eps9mid  Epson-compatible 9-pin, interleaved lines
#		(intermediate resolution)
#	eps9high  Epson-compatible 9-pin, interleaved lines
#		(triple resolution)
#	epsonc	Epson LQ-2550 and Fujitsu 3400/2400/1200 color printers
#	hl7x0   Brother HL 720 and HL 730 (HL 760 is PCL compliant);
#		also usable with the MFC6550MC Fax Machine.
#	ibmpro  IBM 9-pin Proprinter
#	imagen	Imagen ImPress printers
#	iwhi	Apple Imagewriter in high-resolution mode
#	iwlo	Apple Imagewriter in low-resolution mode
#	iwlq	Apple Imagewriter LQ in 320 x 216 dpi mode
#	jetp3852  IBM Jetprinter ink-jet color printer (Model #3852)
#	lbp8	Canon LBP-8II laser printer
#	lips3	Canon LIPS III laser printer in English (CaPSL) mode
#	lj250	DEC LJ250 Companion color printer
#	lj3100sw H-P LaserJet 3100 (requires installed HP-Software)
#	lj4dith  H-P LaserJet 4 with Floyd-Steinberg dithering
#	lp8000	Epson LP-8000 laser printer
#	lq850   Epson LQ850 printer at 360 x 360 DPI resolution;
#               also good for Canon BJ300 with LQ850 emulation
#	lxm5700m Lexmark 5700 monotone
#	m8510	C.Itoh M8510 printer
#	necp6	NEC P6/P6+/P60 printers at 360 x 360 DPI resolution
#	nwp533  Sony Microsystems NWP533 laser printer   [Sony only]
#	oki182	Okidata MicroLine 182
#	okiibm	Okidata MicroLine IBM-compatible printers
#	paintjet  alternate H-P PaintJet color printer
#	photoex  Epson Stylus Color Photo, Photo EX, Photo 700
#	pj	H-P PaintJet XL driver 
#	pjetxl	alternate H-P PaintJet XL driver
#	pjxl	H-P PaintJet XL color printer
#	pjxl300  H-P PaintJet XL300 color printer;
#		also good for PaintJet 1200C and CopyJet
#	r4081	Ricoh 4081 laser printer
#	sj48	StarJet 48 inkjet printer
#	sparc	SPARCprinter
#	st800	Epson Stylus 800 printer
#	stcolor	Epson Stylus Color
#	t4693d2  Tektronix 4693d color printer, 2 bits per R/G/B component
#	t4693d4  Tektronix 4693d color printer, 4 bits per R/G/B component
#	t4693d8  Tektronix 4693d color printer, 8 bits per R/G/B component
#	tek4696  Tektronix 4695/4696 inkjet plotter
#	uniprint  Unified printer driver -- Configurable Color ESC/P-,
#		ESC/P2-, HP-RTL/PCL mono/color driver
# Fax systems:
#	cfax	SFF format for CAPI fax interface
#	dfaxhigh  DigiBoard, Inc.'s DigiFAX software format (high resolution)
#	dfaxlow  DigiFAX low (normal) resolution
# Other raster file formats and devices:
#	cif	CIF file format for VLSI
#	inferno  Inferno bitmaps
#	mgrmono  1-bit monochrome MGR devices
#	mgrgray2  2-bit gray scale MGR devices
#	mgrgray4  4-bit gray scale MGR devices
#	mgrgray8  8-bit gray scale MGR devices
#	mgr4	4-bit (VGA) color MGR devices
#	mgr8	8-bit color MGR devices
#	sgirgb	SGI RGB pixmap format
#	sunhmono  Harlequin variant of 1-bit Sun raster file

# If you add drivers, it would be nice if you kept each list
# in alphabetical order.

###### ----------------------- End of catalog ----------------------- ######

###### ------------------- MS-DOS display devices ------------------- ######

### ------------------- The Hercules Graphics display ------------------- ###

herc_=$(GLOBJ)gdevherc.$(OBJ)
$(DD)herc.dev : $(herc_)
	$(SETDEV) $(DD)herc $(herc_)

$(GLOBJ)gdevherc.$(OBJ) : $(GLSRC)gdevherc.c $(GDEV) $(dos__h)\
 $(gsmatrix_h) $(gxbitmap_h)
	$(GLCC) $(GLO_)gdevherc.$(OBJ) $(C_) $(GLSRC)gdevherc.c

### ---------------------- The Private Eye display ---------------------- ###
### Note: this driver was contributed by a user:                          ###
###   please contact narf@media-lab.media.mit.edu if you have questions.  ###

pe_=$(GLOBJ)gdevpe.$(OBJ)
$(DD)pe.dev : $(pe_)
	$(SETDEV) $(DD)pe $(pe_)

$(GLOBJ)gdevpe.$(OBJ) : $(GLSRC)gdevpe.c $(GDEV) $(memory__h)
	$(GLCC) $(GLO_)gdevpe.$(OBJ) $(C_) $(GLSRC)gdevpe.c

###### ----------------------- Other displays ------------------------ ######

### -------------- The AT&T 3b1 Unixpc monochrome display --------------- ###
### Note: this driver was contributed by a user: please contact           ###
###       Andy Fyfe (andy@cs.caltech.edu) if you have questions.          ###

att3b1_=$(GLOBJ)gdev3b1.$(OBJ)
$(DD)att3b1.dev : $(att3b1_)
	$(SETDEV) $(DD)att3b1 $(att3b1_)

$(GLOBJ)gdev3b1.$(OBJ) : $(GLSRC)gdev3b1.c $(GDEV)
	$(GLCC) $(GLO_)gdev3b1.$(OBJ) $(C_) $(GLSRC)gdev3b1.c

### ------------------- Sony NeWS frame buffer device ------------------ ###
### Note: this driver was contributed by a user: please contact          ###
###       Mike Smolenski (mike@intertech.com) if you have questions.     ###

# This is implemented as a 'printer' device.
sonyfb_=$(GLOBJ)gdevsnfb.$(OBJ)
$(DD)sonyfb.dev : $(sonyfb_) $(DD)page.dev
	$(SETPDEV) $(DD)sonyfb $(sonyfb_)

$(GLOBJ)gdevsnfb.$(OBJ) : $(GLSRC)gdevsnfb.c $(PDEVH)
	$(GLCC) $(GLO_)gdevsnfb.$(OBJ) $(C_) $(GLSRC)gdevsnfb.c

### ------------------------ The SunView device ------------------------ ###
### Note: this driver is maintained by a user: if you have questions,    ###
###       please contact Andreas Stolcke (stolcke@icsi.berkeley.edu).    ###

sunview_=$(GLOBJ)gdevsun.$(OBJ)
$(DD)sunview.dev : $(sunview_)
	$(SETDEV) $(DD)sunview $(sunview_)
	$(ADDMOD) $(GLGEN)sunview -lib suntool sunwindow pixrect

$(GLOBJ)gdevsun.$(OBJ) : $(GLSRC)gdevsun.c $(GDEV) $(malloc__h)\
 $(gscdefs_h) $(gserrors_h) $(gsmatrix_h)
	$(GLCC) $(GLO_)gdevsun.$(OBJ) $(C_) $(GLSRC)gdevsun.c

###### --------------- Memory-buffered printer devices --------------- ######

### --------------------- The Apple printer devices --------------------- ###
### Note: these drivers were contributed by users.                        ###
###   If you have questions about the DMP driver, please contact          ###
###	Mark Wedel (master@cats.ucsc.edu).                                ###
###   If you have questions about the Imagewriter drivers, please contact ###
###	Jonathan Luckey (luckey@rtfm.mlb.fl.us).                          ###
###   If you have questions about the Imagewriter LQ driver, please       ###
###	contact Scott Barker (barkers@cuug.ab.ca).                        ###

appledmp_=$(GLOBJ)gdevadmp.$(OBJ)

$(GLOBJ)gdevadmp.$(OBJ) : $(GLSRC)gdevadmp.c $(PDEVH)
	$(GLCC) $(GLO_)gdevadmp.$(OBJ) $(C_) $(GLSRC)gdevadmp.c

$(DD)appledmp.dev : $(appledmp_) $(DD)page.dev
	$(SETPDEV) $(DD)appledmp $(appledmp_)

$(DD)iwhi.dev : $(appledmp_) $(DD)page.dev
	$(SETPDEV) $(DD)iwhi $(appledmp_)

$(DD)iwlo.dev : $(appledmp_) $(DD)page.dev
	$(SETPDEV) $(DD)iwlo $(appledmp_)

$(DD)iwlq.dev : $(appledmp_) $(DD)page.dev
	$(SETPDEV) $(DD)iwlq $(appledmp_)

### ------------ The Canon BubbleJet BJ10e and BJ200 devices ------------ ###

bj10e_=$(GLOBJ)gdevbj10.$(OBJ)

$(DD)bj10e.dev : $(bj10e_) $(DD)page.dev
	$(SETPDEV) $(DD)bj10e $(bj10e_)

$(DD)bj200.dev : $(bj10e_) $(DD)page.dev
	$(SETPDEV) $(DD)bj200 $(bj10e_)

$(GLOBJ)gdevbj10.$(OBJ) : $(GLSRC)gdevbj10.c $(PDEVH)
	$(GLCC) $(GLO_)gdevbj10.$(OBJ) $(C_) $(GLSRC)gdevbj10.c

### ------------- The CalComp Raster Format ----------------------------- ###
### Note: this driver was contributed by a user: please contact           ###
###       Ernst Muellner (ernst.muellner@oenzl.siemens.de) if you have    ###
###       questions.                                                      ###

ccr_=$(GLOBJ)gdevccr.$(OBJ)
$(DD)ccr.dev : $(ccr_) $(DD)page.dev
	$(SETPDEV) $(DD)ccr $(ccr_)

$(GLOBJ)gdevccr.$(OBJ) : $(GLSRC)gdevccr.c $(PDEVH)
	$(GLCC) $(GLO_)gdevccr.$(OBJ) $(C_) $(GLSRC)gdevccr.c

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
###   The BJC600/BJC4000, BJC800, and ESCP were originally contributed    ###
###       by yves.arrouye@usa.net, but he no longer answers questions     ###
###       about them.                                                     ###

cdeskjet_=$(GLOBJ)gdevcdj.$(OBJ) $(HPPCL)

$(DD)cdeskjet.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)cdeskjet $(cdeskjet_)

$(DD)cdjcolor.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)cdjcolor $(cdeskjet_)

$(DD)cdjmono.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)cdjmono $(cdeskjet_)

$(DD)cdj500.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)cdj500 $(cdeskjet_)

$(DD)cdj550.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)cdj550 $(cdeskjet_)

$(DD)declj250.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)declj250 $(cdeskjet_)

$(DD)dnj650c.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)dnj650c $(cdeskjet_)

$(DD)lj4dith.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)lj4dith $(cdeskjet_)

$(DD)pj.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)pj $(cdeskjet_)

$(DD)pjxl.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)pjxl $(cdeskjet_)

# Note: the pjxl300 driver also works for the CopyJet.
$(DD)pjxl300.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)pjxl300 $(cdeskjet_)

# Note: the BJC600 driver also works for the BJC4000.
$(DD)bjc600.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)bjc600 $(cdeskjet_)

$(DD)bjc800.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)bjc800 $(cdeskjet_)

$(DD)escp.dev : $(cdeskjet_) $(DD)page.dev
	$(SETPDEV) $(DD)escp $(cdeskjet_)

# NB: you can also customise the build if required, using
# -DBitsPerPixel=<number> if you wish the default to be other than 24
# for the generic drivers (cdj500, cdj550, pjxl300, pjtest, pjxltest).

gdevbjc_h=$(GLSRC)gdevbjc.h

$(GLOBJ)gdevcdj.$(OBJ) : $(GLSRC)gdevcdj.c $(std_h) $(PDEVH)\
 $(gsparam_h) $(gsstate_h) $(gxlum_h)\
 $(gdevbjc_h) $(gdevpcl_h)
	$(GLCC) $(GLO_)gdevcdj.$(OBJ) $(C_) $(GLSRC)gdevcdj.c

djet500c_=$(GLOBJ)gdevdjtc.$(OBJ) $(HPPCL)
$(DD)djet500c.dev : $(djet500c_) $(DD)page.dev
	$(SETPDEV) $(DD)djet500c $(djet500c_)

$(GLOBJ)gdevdjtc.$(OBJ) : $(GLSRC)gdevdjtc.c $(PDEVH) $(malloc__h) $(gdevpcl_h)
	$(GLCC) $(GLO_)gdevdjtc.$(OBJ) $(C_) $(GLSRC)gdevdjtc.c

### -------------------- The H-P Color LaserJet 5/5M -------------------- ###

### There are two different drivers for this device.
### For questions about the cljet5/cljet5pr (more general) driver, contact
###	Jan Stoeckenius <jan@orimp.com>
### For questions about the cljet5c (simple) driver, contact
###	Henry Stiles <henrys@meerkat.dimensional.com>
### Note that this is a long-edge-feed device, so the default page size is
### wider than it is high.  To print portrait pages, specify the page size
### explicitly, e.g. -c letter or -c a4 on the command line.

cljet5_=$(GLOBJ)gdevclj.$(OBJ) $(HPPCL)

$(DD)cljet5.dev : $(DEVS_MAK) $(cljet5_) $(GLD)page.dev
	$(SETPDEV) $(DD)cljet5 $(cljet5_)

# The cljet5pr driver has hacks for trying to handle page rotation.
# The hacks only work with one special PCL interpreter.  Don't use it!
$(DD)cljet5pr.dev : $(DEVS_MAK) $(cljet5_) $(GLD)page.dev
	$(SETPDEV) $(DD)cljet5pr $(cljet5_)

$(GLOBJ)gdevclj.$(OBJ) : $(GLSRC)gdevclj.c $(math__h) $(PDEVH)\
 $(gx_h) $(gsparam_h) $(gdevpcl_h)
	$(GLCC) $(GLO_)gdevclj.$(OBJ) $(C_) $(GLSRC)gdevclj.c

cljet5c_=$(GLOBJ)gdevcljc.$(OBJ) $(HPPCL)
$(DD)cljet5c.dev : $(DEVS_MAK) $(cljet5c_) $(GLD)page.dev
	$(SETPDEV) $(DD)cljet5c $(cljet5c_)

$(GLOBJ)gdevcljc.$(OBJ) : $(GLSRC)gdevcljc.c $(math__h) $(PDEVH) $(gdevpcl_h)
	$(GLCC) $(GLO_)gdevcljc.$(OBJ) $(C_) $(GLSRC)gdevcljc.c

### --------------- The H-P LaserJet 3100 software device --------------- ###

### NOTE: This driver requires installed HP-Software to print.            ###
###       It can be used with smbclient to print from an UNIX box to a    ###
###       LaserJet 3100 printer attached to a MS-Windows box.             ###
### NOTE: this driver was contributed by a user: please contact           ###
###       Ulrich Schmid (uschmid@mail.hh.provi.de) if you have questions. ###

lj3100sw_=$(GLOBJ)gdevl31s.$(OBJ) $(GLOBJ)gdevmeds.$(OBJ)
$(DD)lj3100sw.dev : $(lj3100sw_) $(DD)page.dev
	$(SETPDEV) $(DD)lj3100sw $(lj3100sw_)

gdevmeds_h=$(GLSRC)gdevmeds.h $(gdevprn_h)

$(GLOBJ)gdevl31s.$(OBJ) : $(GLSRC)gdevl31s.c $(gdevmeds_h) $(PDEVH)
	$(GLCC) $(GLO_)gdevl31s.$(OBJ) $(C_) $(GLSRC)gdevl31s.c

$(GLOBJ)gdevmeds.$(OBJ) : $(GLSRC)gdevmeds.c $(AK) $(gdevmeds_h)
	$(GLCC) $(GLO_)gdevmeds.$(OBJ) $(C_) $(GLSRC)gdevmeds.c

### ------ CoStar LabelWriter II II/Plus device ------ ###
### Contributed by Mike McCauley mikem@open.com.au     ###

coslw_=$(GLOBJ)gdevcslw.$(OBJ)

$(DD)coslw2p.dev : $(coslw_) $(DD)page.dev
	$(SETPDEV) $(DD)coslw2p $(coslw_)

$(DD)coslwxl.dev : $(coslw_) $(DD)page.dev
	$(SETPDEV) $(DD)coslwxl $(coslw_)

$(GLOBJ)gdevcslw.$(OBJ) : $(GLSRC)gdevcslw.c $(PDEVH)
	$(GLCC) $(GLO_)gdevcslw.$(OBJ) $(C_) $(GLSRC)gdevcslw.c

### -------------------- The Mitsubishi CP50 printer -------------------- ###
### Note: this driver was contributed by a user: please contact           ###
###       Michael Hu (michael@ximage.com) if you have questions.          ###

cp50_=$(GLOBJ)gdevcp50.$(OBJ)
$(DD)cp50.dev : $(cp50_) $(DD)page.dev
	$(SETPDEV) $(DD)cp50 $(cp50_)

$(GLOBJ)gdevcp50.$(OBJ) : $(GLSRC)gdevcp50.c $(PDEVH)
	$(GLCC) $(GLO_)gdevcp50.$(OBJ) $(C_) $(GLSRC)gdevcp50.c

### ----------------- The generic Epson printer device ----------------- ###
### Note: most of this code was contributed by users.  Please contact    ###
###       the following people if you have questions:                    ###
###   eps9mid - Guenther Thomsen (thomsen@cs.tu-berlin.de)               ###
###   eps9high - David Wexelblat (dwex@mtgzfs3.att.com)                  ###
###   ibmpro - James W. Birdsall (jwbirdsa@picarefy.picarefy.com)        ###

epson_=$(GLOBJ)gdevepsn.$(OBJ)

$(DD)epson.dev : $(epson_) $(DD)page.dev
	$(SETPDEV) $(DD)epson $(epson_)

$(DD)eps9mid.dev : $(epson_) $(DD)page.dev
	$(SETPDEV) $(DD)eps9mid $(epson_)

$(DD)eps9high.dev : $(epson_) $(DD)page.dev
	$(SETPDEV) $(DD)eps9high $(epson_)

$(GLOBJ)gdevepsn.$(OBJ) : $(GLSRC)gdevepsn.c $(PDEVH)
	$(GLCC) $(GLO_)gdevepsn.$(OBJ) $(C_) $(GLSRC)gdevepsn.c

### ----------------- The IBM Proprinter printer device ---------------- ###

$(DD)ibmpro.dev : $(epson_) $(DD)page.dev
	$(SETPDEV) $(DD)ibmpro $(epson_)

### -------------- The Epson LQ-2550 color printer device -------------- ###
### Note: this driver was contributed by users: please contact           ###
###       Dave St. Clair (dave@exlog.com) if you have questions.         ###

epsonc_=$(GLOBJ)gdevepsc.$(OBJ)
$(DD)epsonc.dev : $(epsonc_) $(DD)page.dev
	$(SETPDEV) $(DD)epsonc $(epsonc_)

$(GLOBJ)gdevepsc.$(OBJ) : $(GLSRC)gdevepsc.c $(PDEVH)
	$(GLCC) $(GLO_)gdevepsc.$(OBJ) $(C_) $(GLSRC)gdevepsc.c

### ------------- The Epson ESC/P 2 language printer devices ------------- ###
### Note: these drivers were contributed by users.                         ###
### For questions about the Stylus 800 and AP3250 drivers, please contact  ###
###        Richard Brown (rab@tauon.ph.unimelb.edu.au).                    ###
### For questions about the Stylus Color drivers, please contact           ###
###        Gunther Hess (gunther@elmos.de).                                ###

ESCP2=$(GLOBJ)gdevescp.$(OBJ)

$(GLOBJ)gdevescp.$(OBJ) : $(GLSRC)gdevescp.c $(PDEVH)
	$(GLCC) $(GLO_)gdevescp.$(OBJ) $(C_) $(GLSRC)gdevescp.c

$(DD)ap3250.dev : $(ESCP2) $(DD)page.dev
	$(SETPDEV) $(DD)ap3250 $(ESCP2)

$(DD)st800.dev : $(ESCP2) $(DD)page.dev
	$(SETPDEV) $(DD)st800 $(ESCP2)

stcolor1_=$(GLOBJ)gdevstc.$(OBJ) $(GLOBJ)gdevstc1.$(OBJ) $(GLOBJ)gdevstc2.$(OBJ)
stcolor2_=$(GLOBJ)gdevstc3.$(OBJ) $(GLOBJ)gdevstc4.$(OBJ)
$(DD)stcolor.dev : $(stcolor1_) $(stcolor2_) $(DD)page.dev
	$(SETPDEV) $(DD)stcolor $(stcolor1_)
	$(ADDMOD) $(GLGEN)stcolor -obj $(stcolor2_)

gdevstc_h=$(GLSRC)gdevstc.h $(gdevprn_h) $(gsparam_h) $(gsstate_h)

$(GLOBJ)gdevstc.$(OBJ) : $(GLSRC)gdevstc.c $(gdevstc_h) $(PDEVH)
	$(GLCC) $(GLO_)gdevstc.$(OBJ) $(C_) $(GLSRC)gdevstc.c

$(GLOBJ)gdevstc1.$(OBJ) : $(GLSRC)gdevstc1.c $(gdevstc_h) $(PDEVH)
	$(GLCC) $(GLO_)gdevstc1.$(OBJ) $(C_) $(GLSRC)gdevstc1.c

$(GLOBJ)gdevstc2.$(OBJ) : $(GLSRC)gdevstc2.c $(gdevstc_h) $(PDEVH)
	$(GLCC) $(GLO_)gdevstc2.$(OBJ) $(C_) $(GLSRC)gdevstc2.c

$(GLOBJ)gdevstc3.$(OBJ) : $(GLSRC)gdevstc3.c $(gdevstc_h) $(PDEVH)
	$(GLCC) $(GLO_)gdevstc3.$(OBJ) $(C_) $(GLSRC)gdevstc3.c

$(GLOBJ)gdevstc4.$(OBJ) : $(GLSRC)gdevstc4.c $(gdevstc_h) $(PDEVH)
	$(GLCC) $(GLO_)gdevstc4.$(OBJ) $(C_) $(GLSRC)gdevstc4.c

###--------------- Added Omni --------------------------###

epclr_h1=$(GLSRC)defs.h

$(DD)omni.dev : $(GLOBJ)gomni.$(OBJ) $(DD)page.dev
	$(SETPDEV) $(DD)omni $(GLOBJ)gomni.$(OBJ)

$(GLOBJ)gomni.$(OBJ) : $(GLSRC)gomni.c $(epclr_h1) $(PDEVH)
	$(GLCC) $(GLO_)gomni.$(OBJ) $(C_) $(GLSRC)gomni.c

### --------------- Ugly/Update -> Unified Printer Driver ---------------- ###
### For questions about this driver, please contact:                       ###
###        Gunther Hess (gunther@elmos.de)                                 ###

uniprint_=$(GLOBJ)gdevupd.$(OBJ)
$(DD)uniprint.dev : $(uniprint_) $(DD)page.dev
	$(SETPDEV) $(DD)uniprint $(uniprint_)

$(GLOBJ)gdevupd.$(OBJ) : $(GLSRC)gdevupd.c $(PDEVH) $(gsparam_h)
	$(GLCC) $(GLO_)gdevupd.$(OBJ) $(C_) $(GLSRC)gdevupd.c

### ------------ The H-P PaintJet color printer device ----------------- ###
### Note: this driver also supports the DEC LJ250 color printer, which   ###
###       has a PaintJet-compatible mode, and the PaintJet XL.           ###
### If you have questions about the XL, please contact Rob Reiss         ###
###       (rob@moray.berkeley.edu).                                      ###

PJET=$(GLOBJ)gdevpjet.$(OBJ) $(HPPCL)

$(GLOBJ)gdevpjet.$(OBJ) : $(GLSRC)gdevpjet.c $(PDEVH) $(gdevpcl_h)
	$(GLCC) $(GLO_)gdevpjet.$(OBJ) $(C_) $(GLSRC)gdevpjet.c

$(DD)lj250.dev : $(PJET) $(DD)page.dev
	$(SETPDEV) $(DD)lj250 $(PJET)

$(DD)paintjet.dev : $(PJET) $(DD)page.dev
	$(SETPDEV) $(DD)paintjet $(PJET)

$(DD)pjetxl.dev : $(PJET) $(DD)page.dev
	$(SETPDEV) $(DD)pjetxl $(PJET)

###--------------------- The Brother HL 7x0 printer --------------------- ### 
### Note: this driver was contributed by users: please contact            ###
###       Pierre-Olivier Gaillard (pierre.gaillard@hol.fr)                ###
###         for questions about the basic driver;                         ###
###       Ross Martin (ross@ross.interwrx.com, martin@walnut.eas.asu.edu) ###
###         for questions about usage with the MFC6550MC Fax Machine.     ###

hl7x0_=$(GLOBJ)gdevhl7x.$(OBJ)
$(DD)hl7x0.dev : $(hl7x0_) $(DD)page.dev
	$(SETPDEV) $(DD)hl7x0 $(hl7x0_)

$(GLOBJ)gdevhl7x.$(OBJ) : $(GLSRC)gdevhl7x.c $(PDEVH) $(gdevpcl_h)
	$(GLCC) $(GLO_)gdevhl7x.$(OBJ) $(C_) $(GLSRC)gdevhl7x.c

### -------------- Imagen ImPress Laser Printer device ----------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Alan Millar (AMillar@bolis.sf-bay.org) if you have questions.  ###
### Set USE_BYTE_STREAM if using parallel interface;                     ###
### Don't set it if using 'ipr' spooler (default).                       ###
### You may also add -DA4 if needed for A4 paper.			 ###

imagen_=$(GLOBJ)gdevimgn.$(OBJ)
$(DD)imagen.dev : $(imagen_) $(DD)page.dev
	$(SETPDEV) $(DD)imagen $(imagen_)

# Uncomment the first line for the ipr spooler, the second line for parallel.
IMGN_OPT=
#IMGN_OPT=-DUSE_BYTE_STREAM
$(GLOBJ)gdevimgn.$(OBJ) : $(GLSRC)gdevimgn.c $(PDEVH)
	$(GLCC) $(IMGN_OPT) $(GLO_)gdevimgn.$(OBJ) $(C_) $(GLSRC)gdevimgn.c

### ------- The IBM 3852 JetPrinter color inkjet printer device -------- ###
### Note: this driver was contributed by users: please contact           ###
###       Kevin Gift (kgift@draper.com) if you have questions.           ###
### Note that the paper size that can be addressed by the graphics mode  ###
###   used in this driver is fixed at 7-1/2 inches wide (the printable   ###
###   width of the jetprinter itself.)                                   ###

jetp3852_=$(GLOBJ)gdev3852.$(OBJ)
$(DD)jetp3852.dev : $(jetp3852_) $(DD)page.dev
	$(SETPDEV) $(DD)jetp3852 $(jetp3852_)

$(GLOBJ)gdev3852.$(OBJ) : $(GLSRC)gdev3852.c $(PDEVH) $(gdevpcl_h)
	$(GLCC) $(GLO_)gdev3852.$(OBJ) $(C_) $(GLSRC)gdev3852.c

### ---------- The Canon LBP-8II and LIPS III printer devices ---------- ###
### Note: these drivers were contributed by users.                       ###
### For questions about these drivers, please contact                    ###
###       Lauri Paatero, lauri.paatero@paatero.pp.fi                     ###

lbp8_=$(GLOBJ)gdevlbp8.$(OBJ)
$(DD)lbp8.dev : $(lbp8_) $(DD)page.dev
	$(SETPDEV) $(DD)lbp8 $(lbp8_)

$(DD)lips3.dev : $(lbp8_) $(DD)page.dev
	$(SETPDEV) $(DD)lips3 $(lbp8_)

$(GLOBJ)gdevlbp8.$(OBJ) : $(GLSRC)gdevlbp8.c $(PDEVH)
	$(GLCC) $(GLO_)gdevlbp8.$(OBJ) $(C_) $(GLSRC)gdevlbp8.c

### -------------- The Epson LP-8000 laser printer device -------------- ###
### Note: this driver was contributed by a user: please contact Oleg     ###
###       Oleg Fat'yanov <faty1@rlem.titech.ac.jp> if you have questions.###

lp8000_=$(GLOBJ)gdevlp8k.$(OBJ)
$(DD)lp8000.dev : $(lp8000_) $(DD)page.dev
	$(SETPDEV) $(DD)lp8000 $(lp8000_)

$(GLOBJ)gdevlp8k.$(OBJ) : $(GLSRC)gdevlp8k.c $(PDEVH)
	$(GLCC) $(GLO_)gdevlp8k.$(OBJ) $(C_) $(GLSRC)gdevlp8k.c

### -------------- The C.Itoh M8510 printer device --------------------- ###
### Note: this driver was contributed by a user: please contact Bob      ###
###       Smith <bob@snuffy.penfield.ny.us> if you have questions.       ###

m8510_=$(GLOBJ)gdev8510.$(OBJ)
$(DD)m8510.dev : $(m8510_) $(DD)page.dev
	$(SETPDEV) $(DD)m8510 $(m8510_)

$(GLOBJ)gdev8510.$(OBJ) : $(GLSRC)gdev8510.c $(PDEVH)
	$(GLCC) $(GLO_)gdev8510.$(OBJ) $(C_) $(GLSRC)gdev8510.c

### -------------- 24pin Dot-matrix printer with 360DPI ---------------- ###
### Note: this driver was contributed by users.  Please contact:         ###
###    Andreas Schwab (schwab@ls5.informatik.uni-dortmund.de) for        ###
###      questions about the NEC P6;                                     ###
###    Christian Felsch (felsch@tu-harburg.d400.de) for                  ###
###      questions about the Epson LQ850.                                ###

dm24_=$(GLOBJ)gdevdm24.$(OBJ)
$(DD)necp6.dev : $(dm24_) $(DD)page.dev
	$(SETPDEV) $(DD)necp6 $(dm24_)

$(DD)lq850.dev : $(dm24_) $(DD)page.dev
	$(SETPDEV) $(DD)lq850 $(dm24_)

$(GLOBJ)gdevdm24.$(OBJ) : $(GLSRC)gdevdm24.c $(PDEVH)
	$(GLCC) $(GLO_)gdevdm24.$(OBJ) $(C_) $(GLSRC)gdevdm24.c

### ----------------- Lexmark 5700 printer ----------------------------- ###
### Note: this driver was contributed by users.  Please contact:         ###
###   Stephen Taylor (setaylor@ma.ultranet.com) if you have questions.   ###

lxm5700m_=$(GLOBJ)gdevlxm.$(OBJ)
$(DD)lxm5700m.dev : $(lxm5700m_) $(DD)page.dev
	$(SETPDEV) $(DD)lxm5700m $(lxm5700m_)

$(GLOBJ)gdevlxm.$(OBJ) : $(GLSRC)gdevlxm.c $(PDEVH) $(gsparams_h)
	$(GLCC) $(GLO_)gdevlxm.$(OBJ) $(C_) $(GLSRC)gdevlxm.c

### ----------------- The Okidata MicroLine 182 device ----------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Maarten Koning (smeg@bnr.ca) if you have questions.            ###

oki182_=$(GLOBJ)gdevo182.$(OBJ)
$(DD)oki182.dev : $(oki182_) $(DD)page.dev
	$(SETPDEV) $(DD)oki182 $(oki182_)

$(GLOBJ)gdevo182.$(OBJ) : $(GLSRC)gdevo182.c $(PDEVH)
	$(GLCC) $(GLO_)gdevo182.$(OBJ) $(C_) $(GLSRC)gdevo182.c

### ------------- The Okidata IBM compatible printer device ------------ ###
### Note: this driver was contributed by a user: please contact          ###
###       Charles Mack (chasm@netcom.com) if you have questions.         ###

okiibm_=$(GLOBJ)gdevokii.$(OBJ)
$(DD)okiibm.dev : $(okiibm_) $(DD)page.dev
	$(SETPDEV) $(DD)okiibm $(okiibm_)

$(GLOBJ)gdevokii.$(OBJ) : $(GLSRC)gdevokii.c $(PDEVH)
	$(GLCC) $(GLO_)gdevokii.$(OBJ) $(C_) $(GLSRC)gdevokii.c

### ------------------ The Epson Stylus Photo devices ------------------ ###
### This driver was contributed by a user: please contact                ###
###	Zoltan Kocsi (zoltan@bendor.com.au) if you have questions.       ###

photoex_=$(GLOBJ)gdevphex.$(OBJ)
$(DD)photoex.dev : $(photoex_) $(DD)page.dev
	$(SETPDEV) $(DD)photoex $(photoex_)

$(GLOBJ)gdevphex.$(OBJ) : $(GLSRC)gdevphex.c $(PDEVH)
	$(GLCC) $(GLO_)gdevphex.$(OBJ) $(C_) $(GLSRC)gdevphex.c

### ------------- The Ricoh 4081 laser printer device ------------------ ###
### Note: this driver was contributed by users:                          ###
###       please contact kdw@oasis.icl.co.uk if you have questions.      ###

r4081_=$(GLOBJ)gdev4081.$(OBJ)
$(DD)r4081.dev : $(r4081_) $(DD)page.dev
	$(SETPDEV) $(DD)r4081 $(r4081_)


$(GLOBJ)gdev4081.$(OBJ) : $(GLSRC)gdev4081.c $(PDEVH)
	$(GLCC) $(GLO_)gdev4081.$(OBJ) $(C_) $(GLSRC)gdev4081.c

### -------------------- Sony NWP533 printer device -------------------- ###
### Note: this driver was contributed by a user: please contact Tero     ###
###       Kivinen (kivinen@joker.cs.hut.fi) if you have questions.       ###

nwp533_=$(GLOBJ)gdevn533.$(OBJ)
$(DD)nwp533.dev : $(nwp533_) $(DD)page.dev
	$(SETPDEV) $(DD)nwp533 $(nwp533_)

$(GLOBJ)gdevn533.$(OBJ) : $(GLSRC)gdevn533.c $(PDEVH)
	$(GLCC) $(GLO_)gdevn533.$(OBJ) $(C_) $(GLSRC)gdevn533.c

### ------------------------- The SPARCprinter ------------------------- ###
### Note: this driver was contributed by users: please contact Martin    ###
###       Schulte (schulte@thp.uni-koeln.de) if you have questions.      ###
###       He would also like to hear from anyone using the driver.       ###
### Please consult the source code for additional documentation.         ###

sparc_=$(GLOBJ)gdevsppr.$(OBJ)
$(DD)sparc.dev : $(sparc_) $(DD)page.dev
	$(SETPDEV) $(DD)sparc $(sparc_)

$(GLOBJ)gdevsppr.$(OBJ) : $(GLSRC)gdevsppr.c $(PDEVH)
	$(GLCC) $(GLO_)gdevsppr.$(OBJ) $(C_) $(GLSRC)gdevsppr.c

### ----------------- The StarJet SJ48 device -------------------------- ###
### Note: this driver was contributed by a user: if you have questions,  ###
###	                      .                                          ###
###       please contact Mats Akerblom (f86ma@dd.chalmers.se).           ###

sj48_=$(GLOBJ)gdevsj48.$(OBJ)
$(DD)sj48.dev : $(sj48_) $(DD)page.dev
	$(SETPDEV) $(DD)sj48 $(sj48_)

$(GLOBJ)gdevsj48.$(OBJ) : $(GLSRC)gdevsj48.c $(PDEVH)
	$(GLCC) $(GLO_)gdevsj48.$(OBJ) $(C_) $(GLSRC)gdevsj48.c

### ----------------- Tektronix 4396d color printer -------------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Karl Hakimian (hakimian@haney.eecs.wsu.edu)                    ###
###       if you have questions.                                         ###

t4693d_=$(GLOBJ)gdev4693.$(OBJ)
$(DD)t4693d2.dev : $(t4693d_) $(DD)page.dev
	$(SETPDEV) $(DD)t4693d2 $(t4693d_)

$(DD)t4693d4.dev : $(t4693d_) $(DD)page.dev
	$(SETPDEV) $(DD)t4693d4 $(t4693d_)

$(DD)t4693d8.dev : $(t4693d_) $(DD)page.dev
	$(SETPDEV) $(DD)t4693d8 $(t4693d_)

$(GLOBJ)gdev4693.$(OBJ) : $(GLSRC)gdev4693.c $(PDEVH)
	$(GLCC) $(GLO_)gdev4693.$(OBJ) $(C_) $(GLSRC)gdev4693.c

### -------------------- Tektronix ink-jet printers -------------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Karsten Spang (spang@nbivax.nbi.dk) if you have questions.     ###

tek4696_=$(GLOBJ)gdevtknk.$(OBJ)
$(DD)tek4696.dev : $(tek4696_) $(DD)page.dev
	$(SETPDEV) $(DD)tek4696 $(tek4696_)

$(GLOBJ)gdevtknk.$(OBJ) : $(GLSRC)gdevtknk.c $(PDEVH) $(malloc__h)
	$(GLCC) $(GLO_)gdevtknk.$(OBJ) $(C_) $(GLSRC)gdevtknk.c

###### ------------------------- Fax devices ------------------------- ######

### ------------------------- CAPI fax devices -------------------------- ###
### Note: this driver was contributed by a user: please contact           ###
###       Peter Schaefer <peter.schaefer@gmx.de> if you have questions.   ###

cfax_=$(GLOBJ)gdevcfax.$(OBJ)

$(DD)cfax.dev : $(cfax_) $(DD)fax.dev
	$(SETDEV) $(DD)cfax $(cfax_)
	$(ADDMOD) $(DD)cfax -include $(DD)fax

$(GLOBJ)gdevcfax.$(OBJ) : $(GLSRC)gdevcfax.c $(PDEVH)\
 $(gdevfax_h) $(scfx_h) $(strimpl_h)
	$(GLCC) $(GLO_)gdevcfax.$(OBJ) $(C_) $(GLSRC)gdevcfax.c

### ------------------------- The DigiFAX device ------------------------ ###
###    This driver outputs images in a format suitable for use with       ###
###    DigiBoard, Inc.'s DigiFAX software.  Use -sDEVICE=dfaxhigh for     ###
###    high resolution output, -sDEVICE=dfaxlow for normal output.        ###
### Note: this driver was contributed by a user: please contact           ###
###       Rick Richardson (rick@digibd.com) if you have questions.        ###

dfax_=$(GLOBJ)gdevdfax.$(OBJ)

$(DD)dfaxlow.dev : $(dfax_) $(DD)tfax.dev
	$(SETDEV) $(DD)dfaxlow $(dfax_)
	$(ADDMOD) $(GLGEN)dfaxlow -include $(DD)tfax

$(DD)dfaxhigh.dev : $(dfax_) $(DD)tfax.dev
	$(SETDEV) $(DD)dfaxhigh $(dfax_)
	$(ADDMOD) $(GLGEN)dfaxhigh -include $(DD)tfax

$(GLOBJ)gdevdfax.$(OBJ) : $(GLSRC)gdevdfax.c $(PDEVH)\
 $(gdevfax_h) $(gdevtfax_h) $(scfx_h) $(strimpl_h)
	$(GLCC) $(GLO_)gdevdfax.$(OBJ) $(C_) $(GLSRC)gdevdfax.c

###### --------------------- Raster file formats --------------------- ######

### -------------------- The CIF file format for VLSI ------------------ ###
### Note: this driver was contributed by a user: please contact          ###
###       Frederic Petrot (petrot@masi.ibp.fr) if you have questions.    ###

cif_=$(GLOBJ)gdevcif.$(OBJ)
$(DD)cif.dev : $(cif_) $(DD)page.dev
	$(SETPDEV) $(DD)cif $(cif_)

$(GLOBJ)gdevcif.$(OBJ) : $(GLSRC)gdevcif.c $(PDEVH)
	$(GLCC) $(GLO_)gdevcif.$(OBJ) $(C_) $(GLSRC)gdevcif.c

### ------------------------- Inferno bitmaps -------------------------- ###
### Note: this driver was contributed by a user: please contact          ###
###       Russ Cox <rsc@plan9.bell-labs.com> if you have questions.      ###

inferno_=$(GLOBJ)gdevifno.$(OBJ)
$(DD)inferno.dev : $(inferno_) $(DD)page.dev
	$(SETPDEV) $(DD)inferno $(inferno_)

$(GLOBJ)gdevifno.$(OBJ) : $(GLSRC)gdevifno.c $(PDEVH)\
 $(gsparam_h) $(gxlum_h)
	$(GLCC) $(GLO_)gdevifno.$(OBJ) $(C_) $(GLSRC)gdevifno.c

### --------------------------- MGR devices ---------------------------- ###
### Note: these drivers were contributed by a user: please contact       ###
###       Carsten Emde (ce@ceag.ch) if you have questions.               ###

MGR=$(GLOBJ)gdevmgr.$(OBJ) $(GLOBJ)gdevpccm.$(OBJ)

gdevmgr_h= $(GLSRC)gdevmgr.h

$(GLOBJ)gdevmgr.$(OBJ) : $(GLSRC)gdevmgr.c $(PDEVH)\
 $(gdevmgr_h) $(gdevpccm_h)
	$(GLCC) $(GLO_)gdevmgr.$(OBJ) $(C_) $(GLSRC)gdevmgr.c

$(DD)mgrmono.dev : $(MGR) $(DD)page.dev
	$(SETPDEV) $(DD)mgrmono $(MGR)

$(DD)mgrgray2.dev : $(MGR) $(DD)page.dev
	$(SETPDEV) $(DD)mgrgray2 $(MGR)

$(DD)mgrgray4.dev : $(MGR) $(DD)page.dev
	$(SETPDEV) $(DD)mgrgray4 $(MGR)

$(DD)mgrgray8.dev : $(MGR) $(DD)page.dev
	$(SETPDEV) $(DD)mgrgray8 $(MGR)

$(DD)mgr4.dev : $(MGR) $(DD)page.dev
	$(SETPDEV) $(DD)mgr4 $(MGR)

$(DD)mgr8.dev : $(MGR) $(DD)page.dev
	$(SETPDEV) $(DD)mgr8 $(MGR)

### -------------------------- SGI RGB pixmaps -------------------------- ###

sgirgb_=$(GLOBJ)gdevsgi.$(OBJ)
$(DD)sgirgb.dev : $(sgirgb_) $(DD)page.dev
	$(SETPDEV) $(DD)sgirgb $(sgirgb_)

gdevsgi_h=$(GLSRC)gdevsgi.h

$(GLOBJ)gdevsgi.$(OBJ) : $(GLSRC)gdevsgi.c $(PDEVH) $(gdevsgi_h)
	$(GLCC) $(GLO_)gdevsgi.$(OBJ) $(C_) $(GLSRC)gdevsgi.c

### ---------------- Sun raster files ---------------- ###

sunr_=$(GLOBJ)gdevsunr.$(OBJ)

# Harlequin variant, 1-bit
$(DD)sunhmono.dev : $(sunr_) $(DD)page.dev
	$(SETPDEV) $(DD)sunhmono $(sunr_)

$(GLOBJ)gdevsunr.$(OBJ) : $(GLSRC)gdevsunr.c $(PDEVH)
	$(GLCC) $(GLO_)gdevsunr.$(OBJ) $(C_) $(GLSRC)gdevsunr.c

