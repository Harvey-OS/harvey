/* Copyright (C) 1995, 2000 Aladdin Enterprises.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: gdevphex.c,v 1.7 2004/08/10 13:02:36 stefan Exp $ */

/****************************************************************************/
/*	Ghostscript printer driver for Epson Color Photo, Photo EX, Photo 700	*/
/****************************************************************************/

#include "gdevprn.h"
#include <math.h>

/****************************************************************************/
/*								Legend										*/
/****************************************************************************/

/*

HISTORY
~~~~~~~

8 June 1999 Zoltán Kócsi (aka Kocsonya) zoltan@bendor.com.au

	Initial revision. 
	No shingling, depletion.
	Colour only.
	Dither matrix is blatantly copied from gslib.c.

17 April 2000 Zoltán Kócsi

	After much play worked out a reasonably simple colour mapping
	that gives fairly good results. It has some very hairy things 
	in it but ot seems to work reasonably well on a variety of natural
	as well as artificial images.
	
		
LEGALISE
~~~~~~~~

The usual disclaimer applies, neither me (Zoltán Kócsi) nor 
Bendor Research Pty. Ltd. assume any liability whatsoever in 
relation to events arising out of or related to the use of 
the software or the included documentation in any form, way 
or purpose. This software is not guaranteed to work, you
get it "as is" and use it for your own risk.

This code has been donated to Aladdin Enterprises, see their
license for details.

CREDIT
~~~~~~
This driver was written from scratch, however, I have used the 
HP/BJ driver very heavily as a reference (GhostScript's documentation
needs some working :-). In addition, I got some help in understanding
the more arcane features of the printer by digging into the colour
Epson driver and its documentation (documentation for the Photo EX 
did not exist). I thank to the authors of these drivers and the 
related docs. 

I do also hereby express my despising Epson, Inc. who try to enlarge 
Microsoft's monopoly by witholding programming information about such 
a commodity item as a printer.

KNOWN BUGS/LIMITATIONS
~~~~~~~~~~~~~~~~~~~~~~
- Monochrome driver is not finished yet
- The driver is not optimised for speed
- The driver does not support TIFF compression
- Shingling and depletion is not implemented
- The colour correction and ink transfer curve are hardcoded 
- The dither matrix is straight stolen from Ghostscript
- The alternative error diffusion included but does not work (yet)

I plan to attend these issues later, however, I don't promise any timeframe
for I have a lot else to do for bread & butter too.

PREFACE
~~~~~~~
The Epson Stylus Photo EX is a colour ink-jet printer.
It can handle papers up to A3. It uses 6 inks, black in one cartridge
and cyan, magenta, yellow, light cyan and light magenta in an other 
cartridge. The head has 32 nozzles, with 1/90" spacing. 
The maximal resolution is 1440 dpi horizontal 720 dpi vertical.
In 720x720 and 360x360 dpi it supports microweave. To achieve
1440x720 you must use software weaving. It has only one built-in font,
namely 12pt Courier; the printer in general havily relies on the
driver software. It comes with (what else ?) Windows 9x and Mac drivers.

The printer uses the ESC/P Raster protocol. This protocol is somewhat
similar to the ESC/P2 one. Initially Epson refused to give any info
about it. Later (unfortunately after I had already spent lot of time
to reverse engineer it) they released its definition. It could be 
found on their website (http://www.ercipd.com/isv/level1/6clr_98b.pdf).
Alas, they removed it, so at the moment I do not know about any existing
docs of the printer. 
There are still a few commands which are not covered by the docs
and for example the Windows driver uses them. There are others which
are in the docs, saying that you can find them in other docs but you 
can't. Fortunately, these commands apparently have no effect on the 
printing process so this driver simply ignores them. Tricky business.

By the way, my personal experience is that Epson tech support is 
a joke, or in Usenet lingvo it sucks big time - they know absolutely
nothing about the product they supposed to support. Epson's webpage
contains false info as well (they state that the Photo EX uses ESC/P2,
which is simply not true).

This driver should in theory support the Stylus 700 and the Stylus Photo
as well but I have not tested it on them.

If you think that you can get some useful info from me above of what you
can find below, feel free to email me at zoltan@bendor.com.au.
If you enhance the driver or find a bug *please* send me info about 
it. 

DRIVER
~~~~~~
The driver was written under Ghostscript 5.10.
This file should contain two drivers, one for colour mode and one for B&W.
The devices are "photoex" and "photoexm". The mono device driver is
catered for (that is, the rendering part knows how to render for B&W)
but it is not finished yet (no device structure and gray colour mapping
procedures) mainly because all my B&W needs are fairly well satisfied 
by our laser printer.

The driver features the following:

Supported resolutions

	 360x360	Y weaving (not that micro :-) by the printer
	 720x720	Y microweave by the driver (quicker than the printer)
	1440x720	Y and X microweave by the driver
	
	Resolutions other than these will result in a rangecheck error.
	
Papersize:
	
	Whatever Ghostscript supports. The printer docs say that if you load
	multiple sheets of transparencies into the tray you should at least
	have 30mm or 1.2" top margin. The driver always sets the smallest 
	possible top margin (3mm or 0.12"), it's up to you to comply.
	
	In addition, the printer says that the bottom margin is at least
	14mm or 0.54". I violate it by setting it to 0.5" or 12.7mm.
	0.5" seems to be a common margin value for documents and you 
	would hate it when the last line of your page gets printed on the
	top of the next sheet ...
	
Options:
	
	-dDotSize=n
	
		n = 0		Let the driver choose a dotsize
		n = 1		small dots
		n = 2		more ink
		n = 3		ink flood
		n = 4		'super microdots' (whatever they are, they are *big*)
		
		The default is 0 which is n=1 for 1440x720, 2 for 720x720 and
		3 for 360x360. Do not use large dots if you don't have to, you
		will soak the paper. If you print 720x720 on normal paper, try
		using n=1.
		
	-dRender=n
	
		n = 0		Floyd-Steinbeck error diffusion
		n = 1		Clustered dither
		n = 2		Bendor's error diffusion (experimental, do not use)
		
		Default is Floyd-Steinbeck error diffusion
		
	-dLeakage=nn
	
		nn is between 0 and 25. It only effects Bendor's error diffusion.
		It sets the percentage of the error which is left to 'leak', that
		is it is the coefficient of an exponential decay of the error.
		Experiments show that it can be beneficial on image quality.
		Default is 0 (no leakage).
		
	-dSplash=nn
	
		nn is between 0 and 100. It only affects Bendor's error diffusion.
		The ED routine tries to take the increase of dot diameter on certain
		paper types into account. 
		It sets the percentage of the ink dot size increase as it splashes
		onto the paper and spreads. 0 means no splashing, 100 means that 
		the dot is twice as large as it should be. 
		Default is 0.
		
	-dBinhibit=n
	
		If n is 1, then if black ink is deposited to a pixel, it will
		inhibit the deposition of any other ink to the same pixel.
		If 0, black ink may be deposited together with other inks.
		Default is on (1).
		 
ESC/P RASTER DOCS
~~~~~~~~~~~~~~~~~
The parts of the ESC/P Raster protocol which I've managed to decipher, 
and which are actually used in this driver can be found below.
nn, mm, xx, etc. represent a single byte with a binary value in it.
nnnn, xxxx etc. represent a 16-bit binary number, sent in two bytes,
in little endian order (low byte first). 2-digit numbers are a single 
byte in hex. Other chars are themselves.
Quite a few commands are identical to the ESC/P2 commands, these are 
marked with (P2).

ESC @								(P2)	
	
	Resets the printer.


ESC ( U 01 00 nn					(P2)

	Sets the unit to 3600/nn dpi. Note that 1440 can not be set !


ESC ( C 02 00 nnnn					(P2)

	Sets the page (paper) length to nnnn units
	

ESC ( c 04 00 bbbb tttt				(P2)

	Sets the top margin to tttt units, the bottom margin to
	bbbb units. The bottom margin is measured from the top
	of the page not from the bottom of the page !
		
	
ESC	U nn							(P2)

	Unidirectional printing
	
	nn
	00	off
	01	on
	30	off (this is ASCII 0)
	31	on	(this is ASCII 1)
	
		
ESC	( i 01 00 nn					(P2)

	Microweave
	
	nn
	00	off
	01	on
	30	off (this is ASCII 0)
	31	on	(this is ASCII 1)
	
	Turns microweave on for 720x720 dpi printing.
				
ESC r nn							(P2)

	Select colour
	
	nn
	01		Cyan
	02		Magenta
	04		Yellow
	08		Black
		

ESC ( G 01 00 nn					(P2)

	Selects graphics mode:
	
	nn
	00		Off
	01		On
	30		Off
	31		On
	
	
ESC ( v 02 00 dddd					(P2)

	Advance the paper by dddd units defined by ESC ( U


ESC . cc vv hh nn mmmm <data>		(P2)

	Sends graphics data to the printer.
	
	cc	Encoding mode
	
		00	Raw data
		01	Run-length encoded data
	
	vv	Vertical resolution
	
		28	  90 dpi	*interleave*
		14	 180 dpi	*interleave*
		0a	 360 dpi
		05	 720 dpi
		
	hh	Horizontal resolution
	
		0a	 360 dpi
		05	 720 dpi
		
	nn	Number of nozzles
	
		It should be set to 32 (normal printing) or 1 (microweave)
		
	mmmm Number of collumns of data (not number of data bytes !)
	
	<data>
	
		The data should contain as many bytes as needed to fill the
		mmmm * nn pixels. Data is presented horizontally, that is,
		the bits of a byte will be represented by eight pixels in
		a row. If the number of collumns is not an integer multiple 
		of eight, then some bits from the last byte belonging to the
		row will be discarded and the next row starts on a byte boundary.
		If a bit in a byte is '1' ink is deposited, if '0' not.
		The leftmost pixel is represented by the MSB, rightmost by LSB.
		In case of raw data that's about it.
		
		In case of run-length encoded data, the following is done:
		The first byte is a counter. If the counter is <= 127 then
		the following counter+1 bytes are uncompressed data.
		If the counter is >= 128 then the following single byte should
		be repeated 257-counter times. 
		
	There are resolution restrictions:
	
		360x360 nozzle= 1 microweave on
		360x360 nozzle=32 microweave off
		720x 90 nozzle=32 microweave off
		720x720	nozzle= 1 microweave on

	Other combinations are not supported.

ESC ( e 02 00 00 nn

	Sets the amount of ink spat onto the paper.
	
	nn
	01		microdots (faint printing)
	02		normal dots (not so faint printing)
	03		double dots (full inking)
	04		super microdots (ink is continuously dripping :-)
	
	Values other than that have apparently no effect.
	
ESC ( K 02 00 xxxx

	This command is sent by the Windows driver but it is not used
	in the Epson test images. I have not found it having any effect
	whatsoever. The driver does not use it. The Epson docs don't
	mention it.
	
ESC ( r 02 00 nn mm					
	
	Selects the ink according to this:

	nn mm
	00 00	black
	00 01	magenta
	00 02	cyan
	00 04	yellow
	01 01	light magenta
	01 02	light yellow


ESC ( \ 04 00 xxxx llll

	Horizontal positioning of the head.
	
	Moves the head to the position llll times 1/xxxx inches from
	the left margin. 
	On the example images xxxx was always set to 1440.
	I tried other values in which case the command was ignored,
	so stick to 1440.

	
ESC ( R ll 00 00 <text> <cc> xxxx nn .. nn 
ESC 00 00 00

	This is supposedly sets the printer into 'remote' mode.
	ll is the length of the <text> + 1 which consists of ASCII
	characters (e.g. REMOTE1). 
	<cc> is a two-character code, for example "SN" or "LD".
	xxxx is the number of bytes (nn -s) which will follow.
	After that there's either a new <cc> xxxx nn .. nn sequence or
	the ESC 00 00 00.
	I have absolutely no idea about this command and the Epson document
	says that it's in an other document. It's not in that other one.
	The driver does not use it. The printer does not miss it.
	The Epson test images use it and the Windows driver uses it too.
	They send different <cc>-s and different values for identical <cc>-s.
	Go figure.
	
DRIVER INTERNALS
~~~~~~~~~~~~~~~~
First, some comments.
Anything I know about the printer can be found above. 
Anything I know about Ghostscript internals (not much) can be 
found in the comments in the code. I do not believe in the 'it was hard 
to write, it should be hard to read' principle since I once had to 
understand my own code.
Therefore, the code has lots of comments in it, sometimes apparently
superfluous but I find it easier to understand the program 6 months 
later that way.
I did not follow the Ghostscript or GNU style guide, I write code the way
I like it - I'm a lazy dog :-) I use hard tabs at every 4th position,
I use a *lot* of whitespace (as recommended by K&R in their original
C book) and I have a formatting style similar to the K&R with the 
notable exception that I do not indent variable declarations that follow 
the curly. Anyway, you can run your favourite C formatter through the 
source.

In addition to the above, the driver is not hand-optimised, it assumes 
that it is compiled with a good optimising compiler which will handle
common subexpression ellimination, move loop independent code out of
the loop, transform repeated array accesses to cached pointer arithmetics
and so on. The code is much more readable this way and gcc is fairly 
good at doing optimisation. Feel free to hand-optimise it.

So, the driver works the following way:

When it has to render a page, first it sets up the basics such as margins
and papersize and alike.

Line scheduling
---------------

Then it calls the line scheduler. To see why do we have a scheduler, you
have to understand weaving. The printer head has 32 nozzles which are
spaced at 8 line intervals. Therefore, it prints 32 lines at a time but they
are distributed over a 256 line high area. Obviously, if you want to print 
all the lines under the head, you should pass over the paper 8 times.  
You can do it the obvious way:
Print, move down by one line, print ... repeat 8 times then move down
by 256 - 8 lines and start again. Unfortunately, this would result in
stripy images due to the differences between individual nozzles.
Lines 0-7 would be printed by nozzle 0, 8-15 by nozzle 1 and so on. An
8 line band has a visible height, so difference between nozzles will
cause 8-line high bands to appear on the image.

The solution is 'microweave', a funny way of doing interlaced printing.
Instead of moving down 1, 1, 1, 1, .. 1, 248, 1, 1 .. you move down
a constant, larger amount (called a band). This amount must be chosen 
in such a way that each line will be printed and preferably it will be 
printed only once.

Let for example the move down amount (the band) be 31. Let's say, 
in band N nozzle 31 is over line 300, in which case nozzle 30 is over
line 292. We move the head down by 31 lines, then line 299 will be 
under nozzle 27 and line 307 under nozzle 28.
Next move, nozzle 23 will print line 298 and nozzle 24 line 306, then
19/297 20/305, 15/296 16/304, 11/295 12/303, 7/294 8/302, 3/293 4/302,
0/292 3/301 which covers the entire area between 292 and 307. 
The same will apply to any other area on the page. Also note that 
adjacent lines are always printed by different nozzles. 
You probably have realised that line 292 was printed in the first pass
and in the last one. In this case, of course, the line must not be printed 
twice, one or the other pass should not deliver data to the nozzle which
passes over this line.

Now there's a twist. When the horizontal resolution is 1440 dpi you have
to print each line twice, first depositing all even pixels then offset
the head by 1/1440" and deposit all odd pixels (the printer can only
print with 720 dpi but you can initially position the head with 1440 dpi
resolution). You could do it the easy way, passing over the same area 
twice but you can do better. You can find a band size which will result 
each line being printed twice. Instead of suppressing the double print, 
you use this mechanism to print the odd and the even pixels.
Now if you print one line's odd pixels, obviously, all lines belonging
to the 31 other nozzles of the head will have their odd pixels printed too.
Therefore, you have to keep track which lines have been printed in which
phase and try to find an odd-even phase assignment to bands so that each line
has both groups printed (and each group only once). 
The added bonus is that even the same line will be printed by two different
nozzles thus effects of nozzle differences can be decreased further.

The whole issue is further complicated with the beginning of the page and 
the end of the page. When you print the first 8 lines you *must* use the
print, down by 1, print ... method but then you have to switch over to the
banding method. To do it well, you should minimise the number of lines which
are printed out of band. This optimisation is not complex but not trivial 
either. Our solution is to employ precalculated tables for the first 8 lines.
(Epson's solution is not to print the 'problematic' lines at all - they
warn you in the manual that at the top and bottom you may have "slight
distortions". Analyzing their output reveals the reason ... ).
The bottom is different. It is easier, because you are already banding, so 
you can't screw up the rest of the image. On the other hand, you can't use
tables because these tables would depend on the page height which you don't
know a priori. Our solution is to switch to single line mode when we can 
not do the banding any more and try to finish the page with the minimal 
amount of passes.

So, first the driver calls the scheduler which returns a list of lines which 
it dispatched to print in the current band. Then the driver checks if it has 
all these lines halftoned. Since the head covers an area of 256 lines, we 
have to buffer that many lines (actually, 256-7). As the head moves down, 
we can flush lines which it has left and halftone the new ones.


Colour transformations
----------------------

The next important issue is the colour transformation. The reason for doing
this is that the ink is not perfect. Ideally, you have 3 inks, namely cyan
magenta and yellow. Mixing these you can have all colours. Now the inks
are not pure, that is the cyan ink contains some particles that have a
colour other than the ideal cyan and so on. In addition, the inks are
not exactly cyan, magenta and yellow. Therefore, you have to do some
transformations that will map the ideal C, M, Y values to amounts of
ink of the real kind. You also have a black ink. Although in theory
mixing C, M, Y in equal amount will give you black, it doesn't exactly
work that way. In addition, black ink is cheap compared to the colour
so if you can use black, you rather use that. On top of all that, 
because of other effects (ink splashing on the paper and things like that) 
you have to apply some non-linear functions to get reasonable colours.

Halftoning
----------

The driver has different halftoning methods.
There is the classic Floyd-Stenberg error diffusion. There is an other
ED, of which I'm hammering the matrix. The matrix is larger than the
FS one and IMHO results in somewhat lower halftoning noise. However,
it completely screws up some flat colours so don't use it.
There is also dithering, which is quick but noisy.

For any halftoning method, it is assumed that the haltoning can be 
done on the 4 colours (CMYK) separately and all interdependencies are
already handled. It is an optimistic assumption, however, close enough.

You can add any halftoning method you like by writing a halftoner
module. A halftoner module consists of 4 functions:

- Init, which is called before halftoning starts.
- Threshold, which should return a number which tells the driver how many
  empty lines needed before halftoning can be stopped (i.e. for how many 
  lines will a line affect halftoning of subsequent lines).
- Halftone, which halftones one colour of one line
- EndOfLine which is called when all colours of a scanline are halftoned,
  you can do your housekeeping functions here.

For example, in the case of ED init() clears the error buffers, threshold()
returns ~5 (5 empty lines are enough for the accumulated error to go to 
almost zero), endofline() shuffles the error buffers and halftone() itself
does the error diffusion. In case of dithering, threshold is 0 (dithering
has no memory), init and endofline do nothing and halftone simply
dithers a line.

A few options are available for all halftoners:

- the black is rendered first. Now this black line is presented to all
  further passes. If a pixel is painted black, there's no point to
  deposit any other colour on it, even if the halftoning itself would do.
  Therefore, an already set black pixel can block the halftoning of colours
  for that pixel. Whether this thing is activated or not is a command line
  switch (default is on). Your halftoner may choose to ignore this flag.
  
- the intensity value of the light-cyan and light-magenta ink can be
  set from the command line. My experience is that the default 127 is
  good enough, but you can override it if you want to.
  
Apart from these features, each halftoner can have all sorts of other 
switches. Currently there are switches for the Bendor ED, see the 
comments in front of the BendorLine() function to see what they are.

Postprocessing
--------------

After lines are halftoned, they are packed into bitstreams. If you use
1440x720 then the 2 passes for the horizontal interleave are separated.
Postprocessing should also do the shingling/depletion, but it is not
yet done.

Compression
-----------

The driver, before it sends the data to the printer, compresses it using
RLE (run-length encoding) compression. It is not very effective but still
more than nothing. I have not yet ventured into using TIFF as output format,
it may come later.

*/

/****************************************************************************/
/*						Device specific definitions							*/
/****************************************************************************/

/*
*	Device limits
*/

#define	MAX_WIDTH	11.46			/* Maximum printable width, 8250 dots	*/
#define	MAX_PIXELS	8250
#define	MAX_BYTES	(MAX_PIXELS+7)/8

/*
*	Margins (in inch)
*/

#define	MARGIN_L	0.12			/* Left margin							*/
#define	MARGIN_R	0.12			/* Right margin							*/
#define	MARGIN_T	0.12			/* Top margin							*/
#define	MARGIN_B	0.50			/* Bottom margin (should be 0.54 !)		*/

/*
*	We default to 720x720 dpi
*/

#define	Y_DPI		720				/* Default vertical resolution	[dpi]	*/
#define	X_DPI		720				/* Default horizontal resolution [dpi]	*/

/*
*	Encoding of resolutions. Does *not* work with 1440 dpi !
*/

#define	RESCODE( x )	(3600/(x))

/*
*	The device has 6 different inks
*/

#define	DCOLN			6

/*
*	Device colour codes
*	CAVEAT: if you change them change the SendColour() procedure too !
*/

#define	DEV_BLACK		0
#define	DEV_CYAN		1
#define	DEV_MAGENTA		2
#define	DEV_YELLOW		3
#define	DEV_LCYAN		4
#define	DEV_LMAGENTA	5

/*
*	The head has 32 nozzles, with 8 x 1/720" spacing
*/

#define	NOZZLES			32
#define	HEAD_SPACING	8

/*
*	Some ASCII control characters
*/

#define	CR				13			/* Carriage return						*/
#define	FF				12			/* Form feed							*/
#define	ESC				"\033"		/* Escape								*/

/****************************************************************************/
/*						Internally used definitions							*/
/****************************************************************************/

#ifndef	TRUE
#define	TRUE	1
#endif

#ifndef	FALSE
#define	FALSE	0
#endif

/*
*	Since the printer is CMYK, we use 4 colours internally
*/

#define	ICOLN			4

/*
*	This is the maximum number of error lines needed by any 
*	currently implemented rendering function.
*	If you need more, increase it.
*/

#define	MAX_ED_LINES	3

/*
*	If this is defined to !0 then we use Adobe's CMYK -> RGB mapping, 
*	Ghostscript's otherwise. Ghostscript claims that their mapping
*	is better. The mapping of CMYK to RGB according to Adobe is:
*	
*		R = 1.0 - min( 1.0, C + K )
*		G = 1.0 - min( 1.0, M + K )
*		B = 1.0 - min( 1.0, Y + K )
*
*	while Ghostscript uses this:
*	
*		R = ( 1.0 - C ) * ( 1.0 - K )
*		G = ( 1.0 - M ) * ( 1.0 - K )
*		B = ( 1.0 - Y ) * ( 1.0 - K )
*/

#define	MAP_RGB_ADOBE	0

/*
*	We store a CMYK value in a 32 bit entity, each component being 8 bit.
*	These macros pack and unpack these blocks.
*	Ghostscript guarantees that when we get them back the unsigned long
*	will be placed in memory in a big-endian format (regardless of the
*	actual architecture it's running on), so we declare the colour offsets
*	accordingly.
*/

#define	OFFS_C		0
#define	OFFS_M		1
#define	OFFS_Y		2
#define	OFFS_K		3

#define	DECOMPOSE_CMYK( index, c, m, y, k ) \
	{										\
		(k) = (index) & 255;				\
		(y) = ( (index) >> 8 ) & 255;		\
		(m) = ( (index) >> 16 ) & 255;		\
		(c) = ( (index) >> 24 ) & 255; 		\
	}

#define	BUILD_CMYK( c, m, y, k ) \
	((((long)(c)&255)<<24)|(((long)(m)&255)<<16)|\
	(((long)(y)&255)<<8)|((long)(k)&255))

/*
*	This structure is for colour compensation
*/
	
typedef	struct {

	int		ra;						/* Real colour angle (hue)				*/
	int		ia;						/* Theoretical ink colour angle 		*/
	int		c;						/* Cyan component						*/
	int		m;						/* Magenta component					*/
	int		y;						/* Yellow component						*/
	
} CCOMP;

/*
*	Our device structure has some extensions
*/

typedef	struct gx_photoex_device_s {

	gx_device_common;				/* This macro defines a graphics dev.	*/
	gx_prn_device_common;			/* This macro extends for printer dev.	*/
	int		shingling;				/* Shingling (multipass, overlap) mode	*/
	int		depletion;				/* Excess dot removal					*/
	int		halftoner;				/* Rendering type						*/
	int		splash;					/* Splashing compensation factor		*/
	int		leakage;				/* Error leakage (percentage)			*/
	int		mono;					/* Monochrome mode (black only)			*/
	int		pureblack;				/* Black ink blocks others				*/
	int		midcyan;				/* Light cyan ink value					*/
	int		midmagenta;				/* Light magenta ink value				*/
	int		dotsize;				/* Size of the ink dot					*/

} gx_photoex_device;

/*
*	These can save some typing
*/

typedef	gx_device			DEV;
typedef	gx_device_printer	PDEV;
typedef	gx_photoex_device	EDEV;
typedef	gx_color_index		CINX;
typedef	gx_color_value		CVAL;
typedef	gs_param_list		PLIST;
typedef	gs_param_name		PNAME;

/*
*	How many lines do we have to think ahead
*/

#define	MAX_MARK		((NOZZLES)*(HEAD_SPACING))

/*
*	This structure stores a device scanline for one colour
*/

typedef	struct	{

	int		first;					/* Index of the first useful byte	*/
	int		last;					/* Index of the last useful byte	*/
	byte	data[ MAX_BYTES ];		/* Actual raw data					*/

} RAWLINE;

/*
*	These definitions are used by the microweave scheduler.
*	These are the band height definitions. Do not fiddle with them,
*	they are the largest number with which no lines are skipped 
*	and the unused nozzles in the head for each band is minimal.
*	They, of course, depend on the number of nozzles in the head
*	and their spacing, these numbers are for 32 and 8, respectively.
*/

#define	BAND_1440		13			/* Band height for 1440dpi, double scan	*/
#define	BAND_720		31			/* Band height for 720dpi, single scan	*/
#define	BAND_360		1			/* Band height for 360dpi, single scan	*/

#define	NOZZLE_1440		(NOZZLES)	/* Number of nozzles used for 1440dpi	*/
#define	NOZZLE_720		(NOZZLES)	/* Number of nozzles used for 720dpi	*/
#define	NOZZLE_360		1			/* Number of nozzles used for 360dpi	*/

/*
*	This structure is used to generate the line scheduling data.
*	Input/output refers to the scheduler I/F: input means data 
*	given to the scheduler, output is what it gives back. Unspecified
*	data is scheduler private.
*/

typedef	struct {

	int		last;					/* Input	Last line to print			*/
	int		resol;					/* Input	X Resolution				*/
	int		nozzle;					/* Output	Number of nozzles			*/
	int		down;					/* Output	Lines to move down			*/
	int		head[ NOZZLES ];		/* Output	Which lines to be sent		*/
	int		offset;					/* Output	Offset line by 1/1440"		*/
	int		top;					/* 			Head position now			*/
	int		markbeg;				/* 			First marked line			*/
	byte	mark[ MAX_MARK ];		/* 			Marks already printed lines	*/
	
} SCHEDUL;

/*
*	These macros are used to access the printer device
*/

#define	SendByte( s, x )	fputc( (x), (s) )

#define	SendWord( s, x )	SendByte((s), (x) & 255); \
							SendByte((s), ((x) >> 8 ) & 255);

/*
*	This structure stores all the data during rendering
*/

typedef	struct {
	
	EDEV	*dev;					/* The actual device struct			*/
	FILE	*stream;				/* Output stream					*/
	int		yres;					/* Y resolution						*/
	int		xres;					/* X resolution						*/
	int		start;					/* Left margin in 1/1440 inches		*/	
	int		width;					/* Input data width in pixels		*/
	int		lines;					/* Number of lines					*/
	int		mono;					/* Black only						*/
	byte	*dbuff;					/* Data buffer 						*/
	int		htone_thold;			/* Halftoner restart threshold		*/
	int		htone_last;				/* Last line halftoned				*/
	SCHEDUL	schedule;				/* Line scheduling info				*/
	
	/* These are the error buffers for error diffusion. MAX_PIXELS*2
	   is needed for 1440 dpi printing. */
	
	short	err[ MAX_ED_LINES ][ ICOLN ][ MAX_PIXELS*2 ];
	
	/* Error buffer pointers. I love C :-) */
	
	short	( *error[ MAX_ED_LINES ] )[ MAX_PIXELS*2 ];
	
	/* This stores the halftoning result for a line, 
	   not yet in device format. (It's CMYK 1 byte/pixel/colour) */
	
	byte	res[ ICOLN ][ MAX_PIXELS*2 ];
	
	/* This is the buffer for rendered lines, converted
	   to raw device data (not yet run-length encoded).
	   That is, it's 6 colours, 1 bit/pixel/colour.
	   The first index is the 1440 dpi X-weave phase. */
	   
	RAWLINE	raw[ 2 ][ DCOLN ][ MAX_MARK ];	
	
	/* This buffer stores a single line of one colour,
	   run-length encoded, ready to send to the printer */
	
	byte	rle[ MAX_PIXELS * 2 ];
	
} RENDER;

/*
*	This is the sctructure used by the actual halftoner algorithms
*/

typedef	struct	{
	
	RENDER	*render;				/* Render info, if needed				*/
	byte	*data;					/* Input data							*/
	int 	step;					/* Steps on input data					*/
	byte	*res;					/* Result								*/
	byte	*block;					/* Blocking data						*/
	short	**err;					/* Pointers to error buffers			*/
	int		lim1; 					/* Halftoning lower limit				*/
	int		lim2; 					/* Halftoning upper limit				*/
	int		mval; 					/* Level represented by 'light' colour	*/
	
} HTONE;	

/*
*	Halftoner function table
*/

typedef	struct {

	int		(*hthld)( RENDER *rend );
	void	(*hstrt)( RENDER *rend, int line );
	void	(*hteol)( RENDER *rend, int line );
	void	(*htone)( HTONE *htone, int line );

} HFUNCS;

/*
*	Number of known halftoning methods
*/

#define	MAXHTONE		3

/*
*	Dither matrix size
*/

#define	DMATRIX_X		16
#define	DMATRIX_Y		16
	
/****************************************************************************/
/*							Prototypes										*/
/****************************************************************************/

private int		photoex_open( gx_device *pdev );
private	int		photoex_print_page( PDEV *dev, FILE *prn_stream );
private	CINX	photoex_map_rgb_color( DEV *dev, CVAL r, CVAL g, CVAL b );
private int		photoex_map_color_rgb( DEV *dev, CINX index, CVAL prgb[3] );
private	int		photoex_get_params( DEV *dev, PLIST *plist );
private	int		photoex_put_params( DEV *dev, PLIST *plist );

private int 	PutInt( PLIST *plist, PNAME name, int *val,
						int minval, int maxval, int code );
private	int		GetInt( PLIST *list, PNAME name, int *value, int code );

private	int		Cmy2A( int c, int m, int y );

private	void	SchedulerInit( SCHEDUL *p );
private	int		ScheduleLines( SCHEDUL *p );
private	void	ScheduleLeading( SCHEDUL *p );
private	void	ScheduleMiddle( SCHEDUL *p );
private	void	ScheduleTrailing( SCHEDUL *p );
private	void	ScheduleBand( SCHEDUL *p, int mask );

private	void	RenderPage( RENDER *p );
private	void	RenderLine( RENDER *p, int line );
private	int		IsScanlineEmpty( RENDER *p, byte *line );

private	int		RleCompress( RAWLINE *raw, int min, int max, byte *rle_data );
private	int		RleFlush( byte *first, byte *reps, byte *now, byte *out );

private	void	SendReset( FILE *stream );
private	void	SendMargin( FILE *stream, int top, int bot );
private	void	SendPaper( FILE *stream, int length );
private	void	SendGmode( FILE *stream, int on );
private void	SendUnit( FILE *stream, int res );
private	void	SendUnidir( FILE *stream, int on );
private	void	SendMicro( FILE *stream, int on );
private void	SendInk( FILE *stream, int x );
private	void	SendDown( FILE *stream, int x );
private	void	SendRight( FILE *stream, int amount );
private	void	SendColour( FILE *stream, int col );
private void	SendData( FILE *stream, int hres, int vres, int noz, int col );
private	void	SendString( FILE *stream, const char *s );

private	void	HalftonerStart( RENDER *render, int line );
private	int		HalftoneThold( RENDER *render );
private	void	HalftoneLine( RENDER *render, int line, byte *data );

private	int		BendorThold( RENDER *p );
private	void	BendorStart( RENDER *p, int line );
private	void	BendorEol( RENDER *p, int line );
private	void	BendorLine( HTONE *htone, int y );

private	int		FloydSThold( RENDER *p );
private	void	FloydSStart( RENDER *p, int line );
private	void	FloydSEol( RENDER *p, int line );
private	void	FloydSLine( HTONE *htone, int y );

private	int		DitherThold( RENDER *p );
private	void	DitherStart( RENDER *p, int line );
private	void	DitherEol( RENDER *p, int line );
private	void	DitherLine( HTONE *htone, int y );

/****************************************************************************/
/*							Static data										*/
/****************************************************************************/

/*
*	Halftoner function table
*/

private	const HFUNCS	htable[ MAXHTONE ] = {

	{ FloydSThold, FloydSStart, FloydSEol, FloydSLine },
	{ DitherThold, DitherStart, DitherEol, DitherLine },
	{ BendorThold, BendorStart, BendorEol, BendorLine }
};

/*
*	Define the printer procedures.
*	The definition is based on GS macros, the only real stuff that we 
*	define here are the photoex_ functions.
*/

private	gx_device_procs photoex_device_procs = prn_color_params_procs(

	photoex_open,					/* Opens the device						*/
	gdev_prn_output_page,
	gdev_prn_close,
	photoex_map_rgb_color,			/* Maps an RGB pixel to device colour	*/
	photoex_map_color_rgb,			/* Maps device colour back to RGB		*/
	photoex_get_params,				/* Gets device parameters				*/
	photoex_put_params				/* Puts device parameters				*/
);

/*
*	Device descriptor structure - this is what GhostScript looks
*	for and uses to identify our device.
*	Do not make it private (or static) !
*/

gx_photoex_device far_data gs_photoex_device = {
	
	/* This is a macro that fills GS specific fields in the struct */
	
	prn_device_body(
	
		gx_photoex_device,			/* Device struct type					*/
		photoex_device_procs, 		/* Procedure table						*/
		"photoex",					/* Name of the device					*/
		DEFAULT_WIDTH_10THS,		/* Default width						*/
		DEFAULT_HEIGHT_10THS,		/* Default height						*/
		X_DPI, 						/* Vertical resolution					*/
		Y_DPI,						/* Horizontal resolution				*/
		MARGIN_L, 					/* Left margin							*/
		MARGIN_B, 					/* Bottom margin						*/
		MARGIN_R,					/* Right margin							*/
		MARGIN_T, 					/* Top margin							*/
		ICOLN,						/* Number of colours (4:CMYK)			*/
		32,							/* Bit per pixel for the device(!)		*/
		255,						/* Max. gray level						*/
		255, 						/* Max. colour level					*/
		256,						/* Number of gray gradations			*/
		256,						/* Number of colour gradations			*/
		photoex_print_page			/* Print page procedure					*/
	),
	
	/* Here come our extensions */
		
	0,								/* Shingling off, not implemented		*/
	0,								/* Depletion off, not implemented		*/
	0,								/* Dither type: FS ED					*/
	0,								/* No splash correction					*/
	0,								/* No leakage							*/
	0,								/* Not monochrome						*/
	1,								/* Colour inhibition on black			*/
	127,							/* Mid level cyan						*/
	127,							/* Mid level magenta					*/
	0								/* Automatic dot size setting			*/
};

/*
*	This table contains the line scheduling table for the first 
*	few runs if we are in 720 dpi mode.
*/

private	const int	start_720[ HEAD_SPACING ][ NOZZLES ] = {

	{	  0,	  8,	 16,	 24,	 32,	 40,	 48,	 56,
		 64,	 72,	 80,	 88,	 96,	104,	112,	120,
		128,	136,	144,	152,	160,	168,	176,	184,
		192,	200,	208,	216,	224,	232,	240,	248 },

	{	  1,	  9,	 17,	 25,	 33,	 41,	 49,	 57,
		 65,	 73,	 81,	 89,	 97,	105,	113,	121,
		129,	137,	145,	153,	161,	169,	177,	185,
		193,	201,	209,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  2,	 10,	 18,	 26,	 34,	 42,	 50,	 58,
		 66,	 74,	 82,	 90,	 98,	106,	114,	122,
		130,	138,	146,	154,	162,	170,	178,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  3,	 11,	 19,	 27,	 35,	 43,	 51,	 59,
		 67,	 75,	 83,	 91,	 99,	107,	115,	123,
		131,	139,	147,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  4,	 12,	 20,	 28,	 36,	 44,	 52,	 60,
		 68,	 76,	 84,	 92,	100,	108,	116,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  5,	 13,	 21,	 29,	 37,	 45,	 53,	 61,
		 69,	 77,	 85,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  6,	 14,	 22,	 30,	 38,	 46,	 54,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  7,	 15,	 23,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 }
};


/*
*	This table contains the scheduling table for the first 
*	few lines if we are in 1440 dpi mode
*/

private	const int	start_1440[ 2 ][ HEAD_SPACING ][ NOZZLES ] = {
  {
	{	  0,	  8,	 16,	 24,	 32,	 40,	 48,	 56,
		 64,	 72,	 80,	 88,	 96,	104,	112,	120,
		128,	136,	144,	152,	160,	168,	176,	184,
		192,	200,	208,	216,	224,	232,	240,	248 },

	{	  1,	  9,	 17,	 25,	 33,	 41,	 49,	 57,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  2,	 10,	 18,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  3,	 11,	 19,	 27,	 35,	 43,	 51,	 59,
		 67,	 75,	 83,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  4,	 12,	 20,	 28,	 36,	 44,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  5,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  6,	 14,	 22,	 30,	 38,	 46,	 54,	 62,
		 70,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  7,	 15,	 23,	 31,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

  },
  {
	{	  0,	  8,	 16,	 24,	 32,	 40,	 48,	 56,
		 64,	 72,	 80,	 88,	 96,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  1,	  9,	 17,	 25,	 33,	 41,	 49,	 57,
		 65,	 73,	 81,	 89,	 97,	105,	113,	121,
		129,	137,	145,	153,	161,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  2,	 10,	 18,	 26,	 34,	 42,	 50,	 58,
		 66,	 74,	 82,	 90,	 98,	106,	114,	122,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  3,	 11,	 19,	 27,	 35,	 43,	 51,	 59,
		 67,	 75,	 83,	 91,	 99,	107,	115,	123,
		131,	139,	147,	155,	163,	171,	179,	187,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  4,	 12,	 20,	 28,	 36,	 44,	 52,	 60,
		 68,	 76,	 84,	 92,	100,	108,	116,	124,
		132,	140,	148,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  5,	 13,	 21,	 29,	 37,	 45,	 53,	 61,
		 69,	 77,	 85,	 93,	101,	109,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  6,	 14,	 22,	 30,	 38,	 46,	 54,	 62,
		 70,	 78,	 86,	 94,	102,	110,	118,	126,
		134,	142,	150,	158,	166,	174,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

	{	  7,	 15,	 23,	 31,	 39,	 47,	 55,	 63,
		 71,	 79,	 87,	 95,	103,	111,	119,	127,
		135,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,
		 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1,	 -1 },

  }
};

/*
*	This is the dither matrix we use for ordered dither
*	It is a shameless copy of Ghostscript's own ...
*/

private	byte	dmatrix[ DMATRIX_Y ][ DMATRIX_X ] = {
	{
		0x0e, 0x8e, 0x2e, 0xae, 0x06, 0x86, 0x26, 0xa6,
		0x0c, 0x8c, 0x2c, 0xac, 0x04, 0x84, 0x24, 0xa4
	},
	{
		0xce, 0x4e, 0xee, 0x6e, 0xc6, 0x46, 0xe6, 0x66,
		0xcc, 0x4c, 0xec, 0x6c, 0xc4, 0x44, 0xe4, 0x64
	},
	{
		0x3e, 0xbe, 0x1e, 0x9e, 0x36, 0xb6, 0x16, 0x96,
		0x3c, 0xbc, 0x1c, 0x9c, 0x34, 0xb4, 0x14, 0x94
	},
	{
		0xfe, 0x7e, 0xde, 0x5e, 0xf6, 0x76, 0xd6, 0x56,
		0xfc, 0x7c, 0xdc, 0x5c, 0xf4, 0x74, 0xd4, 0x54
	},
	{
		0x01, 0x81, 0x21, 0xa1, 0x09, 0x89, 0x29, 0xa9,
		0x03, 0x83, 0x23, 0xa3, 0x0b, 0x8b, 0x2b, 0xab
	},
	{
		0xc1, 0x41, 0xe1, 0x61, 0xc9, 0x49, 0xe9, 0x69,
		0xc3, 0x43, 0xe3, 0x63, 0xcb, 0x4b, 0xeb, 0x6b
	},
	{
		0x31, 0xb1, 0x11, 0x91, 0x39, 0xb9, 0x19, 0x99,
		0x33, 0xb3, 0x13, 0x93, 0x3b, 0xbb, 0x1b, 0x9b
	},
	{
		0xf1, 0x71, 0xd1, 0x51, 0xf9, 0x79, 0xd9, 0x59,
		0xf3, 0x73, 0xd3, 0x53, 0xfb, 0x7b, 0xdb, 0x5b
	},
	{
		0x0d, 0x8d, 0x2d, 0xad, 0x05, 0x85, 0x25, 0xa5,
		0x0f, 0x8f, 0x2f, 0xaf, 0x07, 0x87, 0x27, 0xa7
	},
	{
		0xcd, 0x4d, 0xed, 0x6d, 0xc5, 0x45, 0xe5, 0x65,
		0xcf, 0x4f, 0xef, 0x6f, 0xc7, 0x47, 0xe7, 0x67
	},
	{
		0x3d, 0xbd, 0x1d, 0x9d, 0x35, 0xb5, 0x15, 0x95,
		0x3f, 0xbf, 0x1f, 0x9f, 0x37, 0xb7, 0x17, 0x97
	},
	{
		0xfd, 0x7d, 0xdd, 0x5d, 0xf5, 0x75, 0xd5, 0x55,
		0xff, 0x7f, 0xdf, 0x5f, 0xf7, 0x77, 0xd7, 0x57
	},
	{
		0x02, 0x82, 0x22, 0xa2, 0x0a, 0x8a, 0x2a, 0xaa,
		0x01, 0x80, 0x20, 0xa0, 0x08, 0x88, 0x28, 0xa8
	},
	{
		0xc2, 0x42, 0xe2, 0x62, 0xca, 0x4a, 0xea, 0x6a,
		0xc0, 0x40, 0xe0, 0x60, 0xc8, 0x48, 0xe8, 0x68
	},
	{
		0x32, 0xb2, 0x12, 0x92, 0x3a, 0xba, 0x1a, 0x9a,
		0x30, 0xb0, 0x10, 0x90, 0x38, 0xb8, 0x18, 0x98
	},
	{
		0xf2, 0x72, 0xd2, 0x52, 0xfa, 0x7a, 0xda, 0x5a,
		0xf0, 0x70, 0xd0, 0x50, 0xf8, 0x78, 0xd8, 0x58
	}
};

/*
*	This is the (minimalistic) colour compensation table
*/

static	CCOMP	ctable[] = {

	{ -255, -255,   0,   0, 255 },		/* same as green */
	{  102,    0, 255,   0,   0 },		/* cyan */
	{  255,  255, 255, 255,   0 },		/* blue */
	{  560,  512,   0, 255,   0 },		/* magenta */
	{  765,  765,   0, 255, 255 },		/* red */
	{ 1045, 1020,   0,   0, 255 },		/* yellow */
	{ 1275, 1275, 255,   0, 255 },		/* green */
	{ 1632, 1530, 255,   0,   0 }		/* same as cyan */
};

/*
*	This is the ink transfer function.
*	We use only one for all inks, this may be wrong.
*/

static const unsigned char	xtrans[ 256 ] = {

	  0,   0,   0,   0,   0,   0,   0,   0,   
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   0,   0,   0,   0,   
	  0,   0,   0,   0,   0,   0,   0,   0, 
	  0,   0,   0,   0,   1,   1,   1,   1,   
	  1,   1,   1,   1,   1,   1,   1,   1, 
	  1,   1,   1,   1,   2,   2,   2,   2,   
	  2,   2,   2,   2,   2,   2,   3,   3, 
	  3,   3,   3,   3,   3,   4,   4,   4,   
	  4,   4,   4,   5,   5,   5,   5,   5, 
	  6,   6,   6,   6,   6,   7,   7,   7,   
	  7,   8,   8,   8,   8,   9,   9,   9, 
	 10,  10,  10,  11,  11,  11,  12,  12,  
	 12,  13,  13,  13,  14,  14,  14,  15, 
	 15,  16,  16,  17,  17,  17,  18,  18,  
	 19,  19,  20,  20,  21,  21,  22,  22, 
	 23,  23,  24,  24,  25,  26,  26,  27,  
	 27,  28,  29,  29,  30,  30,  31,  32, 
	 32,  33,  34,  34,  35,  36,  37,  37,  
	 38,  39,  40,  40,  41,  42,  43,  44, 
	 44,  45,  46,  47,  48,  49,  50,  51,  
	 51,  52,  53,  54,  55,  56,  57,  58, 
	 59,  60,  61,  62,  63,  64,  65,  67,  
	 68,  69,  70,  71,  72,  73,  74,  76, 
	 77,  78,  79,  80,  82,  83,  84,  86,  
	 87,  88,  89,  91,  92,  94,  95,  96, 
	 98,  99, 101, 102, 103, 105, 106, 108, 
	109, 111, 112, 114, 116, 117, 119, 120, 
	122, 124, 125, 127, 129, 130, 132, 134, 
	136, 137, 139, 141, 143, 145, 146, 148, 
	150, 152, 154, 156, 158, 160, 162, 164, 
	166, 168, 170, 172, 174, 176, 178, 180
};

/****************************************************************************/
/*							Device opening									*/
/****************************************************************************/

private int		photoex_open( DEV *pdev )
{
double	height;
double	width;
float	margins[ 4 ];						/* L, B, R, T					*/

	height = pdev->height / pdev->y_pixels_per_inch;
	width  = pdev->width  / pdev->x_pixels_per_inch;
	
	margins[ 0 ] = 0.12;
	margins[ 1 ] = 0.5;
	margins[ 2 ] = 0.12;
	margins[ 3 ] = ( width > 11.46+0.12 ) ? width - (11.46+0.12) : 0.12;
	
	gx_device_set_margins( pdev, margins, true );
	return( gdev_prn_open( pdev ) );
}

/****************************************************************************/
/*							Colour procedures								*/
/****************************************************************************/

/*
*	Map an RGB colour to device colour. 
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*	Since we present ourselves to Ghostscript as if we were a 
*	full colour resolution RGB device, we calculate the CMYK 
*	values and pack them into the result. This depends on
*	color_index being at least 32 bit !!!
*/

private	CINX	photoex_map_rgb_color( DEV *dev, CVAL r, CVAL g, CVAL b )
{
int		c, y, m, k;
int		a, s, f;
EDEV	*edev;
int		i;

	edev = (EDEV *) dev;
		
	/* White and black are treated on their own */
	
	if ( ( r & g & b ) == ( 1 << gx_color_value_bits ) - 1 ) {
	
		/* White */
		
		return( BUILD_CMYK( 0, 0, 0, 0 ) );	
	}
	
	if ( ( r | g | b ) == 0 ) {
	
		/* Black */
		
		return( BUILD_CMYK( 0, 0, 0, xtrans[ 0xff ] ) );
	}

	/* Map RGB to 8 bit/colour CMY */
	
	c = 255 - ( r >> ( gx_color_value_bits - 8 ) );
	m = 255 - ( g >> ( gx_color_value_bits - 8 ) );
	y = 255 - ( b >> ( gx_color_value_bits - 8 ) );
	
	k = xtrans[ min( c, min( m, y ) ) ] * 0.8; /* FIXME:empirical constant */
	c -= k;
	m -= k;
	y -= k;
	
	s = max ( c, max( y, m ) );
			
	/* Map the colour to an angle and find the relevant table range */
	
	a = Cmy2A( c, m, y );
	for ( i = 1 ; a > ctable[ i ].ra ; i++ );

	/* Now map c, m, y. */
	
	f = ((a - ctable[ i-1 ].ra) << 16 ) / (ctable[ i ].ra - ctable[ i-1 ].ra);
	c = (( ctable[i-1].c << 16 ) + ( ctable[i].c - ctable[i-1].c ) * f ) >> 16;
	m = (( ctable[i-1].m << 16 ) + ( ctable[i].m - ctable[i-1].m ) * f ) >> 16;
	y = (( ctable[i-1].y << 16 ) + ( ctable[i].y - ctable[i-1].y ) * f ) >> 16;
	
	s = xtrans[ s ];
	c = ( c * s ) >> 8;
	m = ( m * s ) >> 8;
	y = ( y * s ) >> 8;
	
	return( BUILD_CMYK( c, m, y, k ) );
}

/*
*	Map a device colour value back to RGB. 
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*	CAVEAT:
*	This mapping is *not* the inverse of the RGB->CMYK.
*	It does not do any ink transfer compensation, colour compensation etc.
*/

private int		photoex_map_color_rgb( DEV *dev, CINX index, CVAL prgb[3] )
{
uint	c, m, y, k;
CVAL	r, g, b;

	/* Let's separate the colours */
	
	DECOMPOSE_CMYK( index, c, m, y, k );
	
	k = index & 255;
	y = ( index >> 8 ) & 255;
	m = ( index >> 16 ) & 255;
	c = ( index >> 24 ) & 255;
	
	/* Depending on whether we use Adobe or Ghostscript mapping,
	   calculate the colours */
	   	
	if ( MAP_RGB_ADOBE ) {

		r = gx_max_color_value * ( 1.0 - min( 1.0, (c / 255.0 + k / 255.0) ) );
		g = gx_max_color_value * ( 1.0 - min( 1.0, (m / 255.0 + k / 255.0) ) );
		b = gx_max_color_value * ( 1.0 - min( 1.0, (y / 255.0 + k / 255.0) ) );
	}
	else {
	
		r = gx_max_color_value * ( 1.0 - c / 255.0 ) * ( 1.0 - k / 255.0);
		g = gx_max_color_value * ( 1.0 - m / 255.0 ) * ( 1.0 - k / 255.0);
		b = gx_max_color_value * ( 1.0 - y / 255.0 ) * ( 1.0 - k / 255.0);
	}
	
	prgb[ 0 ] = r;
	prgb[ 1 ] = g;
	prgb[ 2 ] = b;
	
	return( 0 );
}

/*
*	This function maps a (c,m,y) triplet into an angle.
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*	Angle:    0 cyan	C=255 M=  0 Y=  0 
*			255 blue    C=255 M=255 Y=  0
*			510 magenta	C=  0 M=255 Y=  0
*			765 red     C=  0 M=255 Y=255
*		   1020 yellow	C=  0 M=  0 Y=255
*		   1275 green   C=255 M=  0 Y=255
*		   1530 cyan 
*/

private	int		Cmy2A( int c, int m, int y )
{
int		black;
int		maxim;
int		a;

	/* Calculate the black level */
	
	black = min( c, min( m, y ) );
	 
	/* Remove the black from the colours themselves */
	
	c -= black;
	m -= black;
	y -= black;
	
	/* If all 3 remaining colours are 0, then it is a gray: special case */
	
	if ( ! c && ! m && ! y ) return( 0 );
	
	/* Normalise the colours. At least one at most two of them is 0
	   and at least one at most two of them is 255 */
	
	maxim = max( c, max( m, y ) );
	
	c = ( 255 * c ) / maxim;
	m = ( 255 * m ) / maxim;
	y = ( 255 * y ) / maxim;
	
	if ( c == 255 ) {
	
		if ( ! y )
		
			a = m;					/* cyan - blue */
		else
			a = 1530 - y;			/* green - cyan */
	}
	else if ( m == 255 ) {
	
		if ( ! c )
			
			a = 510 + y;			/* magenta - red */
		else
			a = 510 - c;			/* blue - magenta */
	}
	else {
	
		if ( ! m )
			
			a = 1020 + c;			/* yellow - green */
		else
			a = 1020 - m;			/* red - yellow */
	}
	
	return( a );
}

/****************************************************************************/
/*						Device parameter handling							*/
/****************************************************************************/

/*
*	Tell Ghostscript all about our extra device parameters
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	int		photoex_get_params( DEV *device, PLIST *plist )
{
int		code;
EDEV	*dev;

	dev  = (EDEV *) device;
	
	code = gdev_prn_get_params( device, plist );
	
	code = GetInt( plist, "Depletion",	&dev->depletion, code );
	code = GetInt( plist, "Shingling",	&dev->shingling, code );
	code = GetInt( plist, "Render",  	&dev->halftoner, code );
	code = GetInt( plist, "Splash",		&dev->splash,    code );
	code = GetInt( plist, "Leakage",	&dev->leakage,   code );
	code = GetInt( plist, "Binhibit",	&dev->pureblack, code );
	code = GetInt( plist, "DotSize",	&dev->dotsize,	 code );	
	return( code );
}

/*
*	Get all extra device-dependent parameters
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	int		photoex_put_params( DEV *device, PLIST *plist )
{
int		code;
EDEV	*dev;

	dev  = (EDEV *) device;
	code = 0;
	
	code = PutInt( plist, "Depletion",	&dev->depletion, 0,		    2, code );
	code = PutInt( plist, "Shingling",	&dev->shingling, 0, 	    2, code );
	code = PutInt( plist, "Render",		&dev->halftoner, 0,MAXHTONE-1, code );
	code = PutInt( plist, "Splash",		&dev->splash,    0,		   50, code );
	code = PutInt( plist, "Leakage",	&dev->leakage,   0,		   25, code );
	code = PutInt( plist, "Binhibit",	&dev->pureblack, 0,		    1, code );
	code = PutInt( plist, "DotSize",	&dev->dotsize,   0,		    4, code );

	if ( code < 0 )
	
		return( code );
	else
		return( gdev_prn_put_params( device, plist ) );	
}

/*
*	Reads a named integer from Ghostscript
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private int 	PutInt( PLIST *plist, PNAME name, int *val,
						int minval, int maxval, int code )
{
int		new;

	/* If code is already an error, we return it and do nothing. */
	
	if ( code ) return( code );
	
	/* Otherwise we try to read the value */
	
	new = *val;
		
	switch ( code = param_read_int( plist, name, &new ) ) {
	
		case 1:						/* No such parameter defined, it's OK	*/
		
			code = 0;
			break;
		
		case 0:						/* We have received a value, rangecheck	*/
	
			if ( minval > new || new > maxval )
			
				param_signal_error( plist, name, gs_error_rangecheck );
			else
				*val = new;
				
			break;
		
		default:					/* Error								*/
			break;
	}
	
	return( code );
}

/*
*	Writes a named integer to Ghostscript
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	int		GetInt( PLIST *list, PNAME name, int *value, int code )
{
	if ( code < 0 ) return( code );
	return( param_write_int( list, name, value ) );
}

/****************************************************************************/
/*							Page rendering									*/
/****************************************************************************/

/*
*	This is the function that Ghostscript calls to render a page
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	int		photoex_print_page( PDEV *device, FILE *stream )
{
int			pixels;						/* Length of the line 				*/
int			x;							/* Work vars						*/
EDEV		*dev;						/* Our device						*/
RENDER		*render;					/* Rendering info					*/

int			xres, yres;
int			start, width;
int			unit;
double		psize;

	dev = (EDEV *) device;	
	
	/* Check if the resolution is one of the supported ones */
	
	yres = (int) dev->y_pixels_per_inch;
	xres = (int) dev->x_pixels_per_inch;
	
	if ( ! ( ( xres ==  360 && yres == 360 ) ||
			 ( xres ==  720 && yres == 720 ) ||
			 ( xres == 1440 && yres == 720 ) ) )
			 
		return( gs_error_rangecheck );
		
	pixels = gdev_prn_raster( device ) / sizeof( long );
	psize  = device->height / device->y_pixels_per_inch;
	
	/* Check if the requested width is within device limits.
	   The calculations are in 1440 dpi units. */
	
	start = 1440.0 * dev_l_margin( device );

	x = xres == 360 ? 4 : xres == 720 ? 2 : 1;
	
	if ( start + x * pixels > 2 * MAX_PIXELS ) {
	
		/* We're over the limit, clip width to the required level */
		
		width = ( 2 * MAX_PIXELS - start ) / x;
		
		/* It is rather inprobable that someone would set up a 
		   left margin wider than the printer, still ... */
		
		if ( width <= 0 ) return( gs_error_rangecheck );
	}
	else {
	
		/* We accept the width as it is */
		
		width = pixels;
	}
	
	/* Now try to get the memory we need. It's actually quite a lot,
	   since we have to cache 256 processed lines at 6kbyte each plus
	   we need error buffers and stuff. All in all, we'll request
	   about 1.5 ~ 2M. */
		
	if ( ! ( render = (RENDER *) gs_malloc( dev->memory, 1, sizeof( RENDER ), "PhotoEX" )))
		
		return_error( gs_error_VMerror );
	
	if ( ! ( render->dbuff = (byte *) gs_malloc( dev->memory, pixels, sizeof( long ), 
			"PhotoEX" ) ) ) {
			
		gs_free( dev->memory, render, 1, sizeof( RENDER ), "PhotoEX" );
		return_error( gs_error_VMerror );
	}
	
	/* We've done every possible check and preparation, now 
	   do the work. Fill the rest of the structure so we can pass 
	   it to the actual render routine. */
	
	render->dev		= dev;
	render->yres	= yres;
	render->xres	= xres;
	render->width	= width;
	render->lines	= dev->height;
	render->stream	= stream;		
	render->mono	= dev->mono;
	
 	/* Initialise the printer */
				
	SendReset( stream );
	SendReset( stream );
	SendGmode( stream, 1 );
	
	/* Set up units */
	
	unit = ( yres == 360 ) ? 360 : 720;
	SendUnit( stream, RESCODE( unit ) );
	
	/* Set up papersize and margins */
	
	SendPaper( stream, device->height / device->y_pixels_per_inch * unit );
	SendMargin( stream, ( psize - dev_b_margin( device ) ) * unit, 
					    dev_t_margin( device ) * unit );

	/* Dot size as per user setting */
	
	if ( dev->dotsize )
	
		SendInk( stream, dev->dotsize );
	else
		SendInk( stream, yres == 360 ? 3 : ( xres == 720 ? 2 : 1 ) );
	
	/* Microveawe is off, unidirectional printing on */
	
	SendMicro( stream, 0 );
	SendUnidir( stream, 1 );
	
	/* Render the page and send image data to printer */
	
	RenderPage( render );
		   			
	/* Eject the paper, reset printer */
	
	SendByte( stream, FF );
	SendReset( stream );
	
	/* Release the memory and return */
	
	gs_free( dev->memory, render->dbuff, pixels, sizeof( long ), "PhotoEX" );
	gs_free( dev->memory, render, 1, sizeof( RENDER ), "PhotoEX" );
	return( 0 );
}

/*
*	Renders a page
*	~~~~~~~~~~~~~~
*/

private	void	RenderPage( RENDER *p )
{
int		last_done;					/* The last line rendered				*/
int		last_need;					/* The largest line number we need		*/
int		move_down;					/* Amount of delayed head positioning	*/
int		last_band;					/* Indicates the last band				*/
int		min, max;					/* Min/max active bytes in a raw line	*/
int		phase;						/* 1440dpi X weave offset				*/
int		i, j, l, col;

	p->htone_thold = HalftoneThold( p );
	p->htone_last  = -1 - p->htone_thold;
	
	p->schedule.top   = -1;
	p->schedule.resol = p->xres;
	p->schedule.last  = p->lines;
	
	last_done = -1;
	move_down = 0;
		
	do {
	
		/* Schedule the next batch of lines */
		
		last_band = ScheduleLines( &p->schedule );
		
		/* Find the largest line number we have to process and
		   halftone all lines which have not yet been done */
		
		last_need = last_done;
		for ( i = NOZZLES-1 ; i >= 0 && p->schedule.head[ i ] == -1 ; i-- );
		if ( i >= 0 ) last_need = p->schedule.head[ i ];
		while ( last_need > last_done ) RenderLine( p, ++last_done );
		
		/* Now loop through the colours and build the data stream */
		
		phase = p->schedule.offset;
		
		for ( col = 0 ; col < DCOLN ; col++ ) {
		
			/* First see if we have to send any data at all */
			
			min = MAX_BYTES;
			max = 0;
				
			for ( i = 0 ; i < NOZZLES && i < p->schedule.nozzle ; i++ ) {
			
				if ( ( j = p->schedule.head[ i ] ) != -1 ) {
				
					j %= MAX_MARK;
					
					if ( p->raw[ phase ][ col ][ j ].first < min )
	
						min = p->raw[ phase ][ col ][ j ].first;
						
					if ( p->raw[ phase ][ col ][ j ].last > max )
	
						max = p->raw[ phase ][ col ][ j ].last;
				}
			}
			
			if ( min <= max ) {
			
				max++;
				
				/* We have to send data to the printer. If we have 
				   to position the head, do so now */
				
				if ( move_down ) {
				
					SendDown( p->stream, move_down );
					move_down = 0;
				}
				
				/* Set the desired colour */
				
				SendColour( p->stream, col );
				
				/* Move the head to the desired position */
				
				if ( p->xres == 360 )
				
					SendRight( p->stream, 4 * 8 * min );
					
				else if ( p->xres == 720 )
				
					SendRight( p->stream, 2 * 8 * min );
				else
					SendRight( p->stream, 8 * min + phase );
				
				/* Send the data */
				
				SendData( p->stream, p->xres, p->yres, p->schedule.nozzle, 
						  ( max-min ) * 8 );
				
				for ( i = 0 ; i < p->schedule.nozzle ; i++ ) {
				
					if ( ( j = p->schedule.head[ i ] ) == -1 ||
						 ( p->raw[ phase ][ col ][ j % MAX_MARK ].last <
						   p->raw[ phase ][ col ][ j % MAX_MARK ].first ) ) {

						l = RleCompress( NULL, min, max, p->rle );
					}
					else {
					
						l = RleCompress( p->raw[ phase ][ col ] + j % MAX_MARK,
									   min, max, p->rle );
					}
					
					fwrite( p->rle, l, 1, p->stream );
				}
				
				SendByte( p->stream, CR );
			}
		}
		
		/* Note the amount the head should go down before it prints the
		   next band */
		
		move_down += p->schedule.down;
	
	} while	( ! last_band );
}

/*
*	Render the the next scanline
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*	If it finds a continuous sequence of empty lines, it renders
*	the first htone_thold number of them then stops calling the
*	actual rendering function (which is computationally expensive).
*	When it sees a nonempty line again, it restarts the renderer.
*/

private	void	RenderLine( RENDER *p, int line )
{
byte	*data;
int		i;

	/* Get the line from Ghostscript and see if its empty */
	
	gdev_prn_get_bits( (PDEV *) p->dev, line, p->dbuff, &data );

	if ( IsScanlineEmpty( p, data ) ) {
	
		if ( line - p->htone_last > p->htone_thold ) {
		
			/* The line is empty and is farer from the last nonempty
			   line than the threshold, no need to render it. */
			   
			for ( i = 0 ; i < DCOLN ; i++ ) {
			
				p->raw[ 0 ][ i ][ line % MAX_MARK ].first = MAX_BYTES;
				p->raw[ 0 ][ i ][ line % MAX_MARK ].last  = 0;
				p->raw[ 1 ][ i ][ line % MAX_MARK ].first = MAX_BYTES;
				p->raw[ 1 ][ i ][ line % MAX_MARK ].last  = 0;
				
			}
		}
		else {
			
			/* The line is empty but it is within the threshold, so we 
			   have to render it. We do not move the index, though */
			   
			HalftoneLine( p, line, data );
		}
	}
	else {
	
		/* This line is not empty */
		
		if ( line - p->htone_last >= p->htone_thold ) {
		
			/* Previous lines were empty and we have already stopped 
			   rendering them. We have to restart the renderer */
			   
			HalftonerStart( p, line );
		}
		
		/* Render the line and move the last active index to this line */
		
		HalftoneLine( p, line, data );
		p->htone_last = line;
	}
}

/*
*	This function tests if a scanline is empty
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	int		IsScanlineEmpty( RENDER *r, byte *line )
{
int		i;
long	*p;
	
	p = (long *) line;
	
	for ( i = 0 ; i < r->width ; i++ ) {
	
		if ( *p++ ) return( FALSE );
	}
	
	return( TRUE );	
}

/****************************************************************************/
/*						Microweaved line scheduling							*/
/****************************************************************************/

/*
*	Schedule head data for the next band
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*	This function fills the SCHEDUL structure with information
*	about what to print. The head field will contain the line numbers
*	which are assigned to the nozzles in the head. -1 means that
*	no active data is assigned to the nozzle in this band.
*	The offset field is only used for horizontal microweaving, if it
*	is set then the line should be offseted by 1/1440".
*	The down field contains the number of units which the head should
*	move down when printing of the band is finished. Other fields are
*	mainly for the routine's internal use. At the first call, however,
*	the top field should be set to -1, the resol field should be set 
*	to 360, 720 or 1440 and the last field should contain the number
*	of lines to print (that is, last + 1 :-). 
*
*	The routine returns a flag indicating if this was the last print
*	for the page.
*/
	
private	int	ScheduleLines( SCHEDUL *p )
{
int		i;

	if ( p->top == -1 ) {
	
		/* First call, init everything, then fall through to the rest */
		
		SchedulerInit( p );
	}
	
	/* If nozzle is one, just schedule the next line and that's it.
	   You can use this feature for hardware microweave at 720 dpi,
	   the driver uses it for 360 dpi. */
	
	if ( p->nozzle == 1 ) {
	
		p->head[ 0 ] = p->top;
		p->down = 1;
		p->top++;
		return( p->top == p->last );
	}
	
	/* Release all expired entries in the mark array */
	
	for ( i = p->markbeg ; i < p->top ; i++ ) p->mark[ i % MAX_MARK ] = 0;
	p->markbeg = p->top;
	
	/* If top is less than the the head spacing, then create the image 
	   by single steps. This will cause banding on the very top, but
	   there's nothing we can do about it. We're still better than
	   Epson's driver which simply ignores the first few lines,
	   it does not even try to schedule them ... */
	   	
	if ( p->top < HEAD_SPACING ) {
	
		ScheduleLeading( p );
		return( FALSE );
	}
	
	/* See if we are almost at the end. If yes, we will advance line by
	   line. */
	
	if ( p->top + p->resol + (NOZZLES) * HEAD_SPACING > p->last ) {
		
 		ScheduleTrailing( p );
		
		if ( p->down )
		
			return( p->top + (NOZZLES-1) * HEAD_SPACING >= p->last );
		else
			return( FALSE );
	}
		
	/* Otherwise we're in the middle of the page, just do the
	   simple banding and selecting as many lines as we can. */

	ScheduleMiddle( p );
	return( FALSE );
}

/*
*	Initialise the scheduler
*	~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	SchedulerInit( SCHEDUL *p )
{
int		i;
			
	p->top = 0;
		
	switch ( p->resol ) {
		
		case 360:	
			p->offset = 0;
			p->resol  = BAND_360; 
			p->nozzle = NOZZLE_360;
			break;
			
		case 720:	
			p->offset = 0;
			p->resol  = BAND_720; 
			p->nozzle = NOZZLE_720;
			break;
			
		case 1440:	
			p->offset = 1;			/* Need to be set for the algorithm! */
			p->resol  = BAND_1440; 
			p->nozzle = NOZZLE_1440;
			break;
	}
	
	for ( i = 0 ; i < NOZZLES  ; i++ ) p->head[ i ] = -1;
	for ( i = 0 ; i < MAX_MARK ; i++ ) p->mark[ i ] = 0;
	p->markbeg = 0;
}

/*
*	Scheduling the first BAND lines for the image
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	ScheduleLeading( SCHEDUL *p )
{
int		i;

	if ( p->resol == BAND_720 ) {

		/* Copy the line scheduling data to the struct */
			
		memcpy( p->head, start_720[ p->top ], sizeof( int ) * NOZZLES );
			
		/* Mark all lines to be set */
			
		for ( i = 0 ; i < NOZZLES ; i++ )
			
			if ( p->head[ i ] != -1 ) 
				
				p->mark[ p->head[ i ] % MAX_MARK ] = 1;
			
		/* We move down by one line except at the end */
				
		if ( p->top == HEAD_SPACING - 1 ) {
			
			p->down = BAND_720 - p->top;
			p->top  = BAND_720;
		}
		else {
			
			p->down = 1;
			p->top++;
		}
	}
	else {
			
		/* 1440 dpi version, two passes needed for each scanline */
				
		if ( p->offset ) {
						
			/* Copy the non-offseted scheduling data to the struct */
			
			memcpy( p->head, start_1440[0][p->top], sizeof( int ) * NOZZLES );
			
			/* Mark all lines to be set */
			
			for ( i = 0 ; i < NOZZLES ; i++ )
			
				if ( p->head[ i ] != -1 )
				
					p->mark[ p->head[ i ] % MAX_MARK ] = 1;
			
			/* This is the non-offseted line, do not move ! */
				
			p->offset = 0;
			p->down = 0;
		}
		else {
			
			/* Copy the non-offseted schduling data to the struct */
			
			memcpy( p->head, start_1440[1][p->top], sizeof( int ) * NOZZLES );
			
			/* Mark all lines to be set */
			
			for ( i = 0 ; i < NOZZLES ; i++ )
			
				if ( p->head[ i ] != -1 )
	
					p->mark[ p->head[ i ] % MAX_MARK ] |= 2;
			
			/* We move down by one line except at the end and set offset */
				
			if ( p->top == HEAD_SPACING - 1 ) {
			
				p->down = BAND_1440 - p->top;
				p->top  = BAND_1440;
			}
			else {
			
				p->down = 1;
				p->top++;
			}
			
			p->offset = 1;
		}
	}	
}

/*
*	Scheduling the bulk of the image
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	ScheduleMiddle( SCHEDUL *p )
{
int		ph0, ph1;
int		line, mask;
int		i;

	if ( p->resol == BAND_720 ) {
	
		/* 720 DPI printing. See which lines should we print and
		   fill the head array accordingly, then move down a band. */
		
		ScheduleBand( p, 1 );
		p->down = BAND_720;
		p->top += BAND_720;
	}
	else {
	
		/* 1440 dpi printing. This is a bit more complex than the
		   720 dpi one. First, see how many lines in each phase
		   has already been printed. */
		
		ph0 = ph1 = 0;
		
		for ( line = p->top, i=0 ; i < NOZZLES ; i++, line += HEAD_SPACING ) {
		
			line = p->top + i * HEAD_SPACING;
			ph0 += p->mark[ line % MAX_MARK ] & 1;
			ph1 += p->mark[ line % MAX_MARK ] & 2;
		}
		
		ph1 >>= 1;
	
		/* Choose the phase which has less lines in it. */
		
		if ( ph0 <= ph1 ) {
		
			p->offset = 0;
			mask = 1;
		}
		else {
		
			p->offset = 1;
			mask = 2;
		}
		
		/* Fill the line array and mark the phase.
		   We should check here if moving down the head will leave
		   any line empty, but we do not because we *know* that it
		   won't - the BAND_1440 is selected by finding a value 
		   which guarantees that it will cover every line. */
		
		ScheduleBand( p, mask );
		p->down = BAND_1440;
		p->top += BAND_1440;
	}
}

/*
*	Scheduling the last lines of the image
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	ScheduleTrailing( SCHEDUL *p )
{
int		mask;

	if ( p->down > 1 ) {
		
		/* This is the first time we came here. */
		
		p->offset = 1;
	}
	
	if ( p->resol == BAND_720 ) {
		
		p->offset = 0;
		p->down   = 1;
		mask	  = 1;
	}
	else {
		
		if ( p->offset ) {
			
			p->offset = 0;
			p->down	  = 0;
			mask	  = 1;
		}
		else {
			
			p->offset = 1;
			p->down	  = 1;
			mask	  = 2;
		}
	}
	
	ScheduleBand( p, mask );
	p->top += p->down;
}

/*
*	Select lines from a given set
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	ScheduleBand( SCHEDUL *p, int mask )
{
int		i;
int		line;

	for ( line = p->top, i = 0 ; i < NOZZLES ; i++, line += HEAD_SPACING ) {
			

		if ( p->mark[ line % MAX_MARK ] & mask ) {
			
			p->head[ i ] = -1;
		}
		else {
			
			p->head[ i ] = line;
			p->mark[ line % MAX_MARK ] |= mask;
		}
	}
}

/****************************************************************************/
/*						Formatting printer data								*/
/****************************************************************************/

/*
*	Packs a line to raw device format
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*	Reads pixnum pixels and if the pixel is lev_on, then sets the
*	appropriate bit in the resulting datastream. The length of the
*	result is pixnum/8 (rounded up).
*/

private	void	PackLine( byte *input, int pixnum, int lev_on, int step,
						  RAWLINE *line )
{
byte	bits;
char	*result;
int		i, j, k;

	result = line->data;
	line->first = MAX_PIXELS;
	line->last  = 0;
		
	for ( j = 0x80, bits = k = i = 0 ; i < pixnum ; i += step, input += step ){
	
		if ( *input == lev_on ) bits |= j;
		
		if ( ! ( j >>= 1 ) ) {
		
			if ( bits ) {
			
				if ( line->first > k ) line->first = k;
				if ( line->last  < k ) line->last  = k;
			}
			
			*result++ = bits;
			j		  = 0x80;
			bits	  = 0;
			k++;
		}
	}
	
	if ( j != 0x80 ) {
	
		*result = bits;
		
		if ( bits ) {
			
			if ( line->first > k ) line->first = k;
			if ( line->last  < k ) line->last  = k;
		}
	}
}

/*
*	Compresses (run-length encodes) a line
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*	Returns the length of the RLE data.
*/
								 
private	int		RleCompress( RAWLINE *raw, int min, int max, byte *rle_data )
{
int		i, n;
byte	pbyte;
byte	*start, *rstrt;
int		length;
byte	*input;
int 	len;

	if ( ! raw ) {
	
		/* This is an empty line */
	
		for ( n = 0, i = max - min ; i >= 129 ; i -= 129 ) {
		
			*rle_data++ = 128;
			*rle_data++ = 0;
			n += 2;
		}
		
		if ( i >= 2 ) {
		
			*rle_data++ = 257 - i;
			*rle_data++ = 0;
			n += 2;
		}
		else if ( i ) {
		
			*rle_data++ = 0;
			*rle_data++ = 0;
			n+= 2;
		}
		
		return( n );
	}
	
	/* There's data, set up encoding parameters */
	
	input = raw->data + min;
	len   = max - min;
		
	/* Create a run-length encoded version. We do it even if no pixel
	   was set because it may be that this line is just part of a 
	   multi-line band. */
	
	length = 0;
	start  = input;
	rstrt  = NULL;
	pbyte  = *input++;
	
	for ( i = 1 ; i < len ; i++, input++ ) {
		
		if ( *input == pbyte ) {
		
			/* This byte is identical to the previous one(s). */
			
			if ( ! rstrt ) {
			
				/* This is the start of a new repeating sequence */
				
				rstrt = input - 1;
			}
		}
		else {
		
			/* Different byte than the previous one(s) */
			
			if ( rstrt ) {
			
				/* There was a repetitive sequence. */
				
				if ( rstrt - input < 4 ) {
				
					/* For less than four bytes it isn't worth
					   to do RLE, we discard them */
					
					rstrt = NULL;
				}
				else {
				
					/* We must flush */
					
					n = RleFlush( start, rstrt, input, rle_data );
					rle_data  += n;
					length += n;
					
					/* Initialise again */
					
					start = rle_data;
					rstrt = NULL;
				}
			}						
			
			pbyte = *rle_data;
		}
	}
	
	/* We flush whatever is left over */
	
	length += RleFlush( start, rstrt, input, rle_data );
	
	return( length );
}
			
/*
*	This function flushes the RLE encoding buffer
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*	Assumes that it gets a nonrepetitive pattern followed by a repetitive
*	one. 'first' points to the start of the non-repetitive part.
*	'reps' points to the first byte in the repetitive sequence or it
*	may be NULL, if there were no repetitve bytes. 'now' points to
*	one after the last byte in the sequence.
*	It puts the result into 'out' and returns the number of bytes
*	written out.
*
*	There is one possible performance penalty in using this method:
*	If the repetitive sequence is n*128+1 byte long, then the last
*	byte will be written out as single byte. If the following sequence
*	has a nonrepetitive start, this byte could be combined into that
*	but it isn't. This can cause some penalty, however, we will live
*	with that for now.
*/
				
private	int		RleFlush( byte *first, byte *reps, byte *now, byte *out )
{
int		count;
int		l;

	if ( ! first ) return( 0 );
	
	if ( ! reps ) reps = now;
	
	count = 0;
		
	/* Write the nonrepetitve pattern first */

	while ( ( l = reps - first ) ) {
	
		if ( l > 128 ) {
		
			/* More than 128 consecutive bytes, write out a 128 byte chunk */
			
			*out++ = 127;
			memcpy( out, first, 128 );
			out   += 128;
			first += 128;
			count += 129;
		}
		else {
		
			/* There are not more than 128 bytes, write them into a 
			   single chunk */
			
			*out++ = l - 1;
			memcpy( out, first, l );
			count += l + 1;
			first += l;
			out   += l;
		}
	}				
					
	/* Now write the repeated pattern */
	
	while ( ( l = now - reps ) ) {
	
		if ( l > 128 ) {
		
			/* More than 128 bytes are identical, write out a
			   129 byte chunk */
			   
			*out++ = 128;
			*out++ = *reps;
			count += 2;
			reps  += 129;
		}
		else {
		
			if ( l == 1 ) {
			
				/* There is only one byte left, write it out as a
				   nonrepetitive chunk */
				   
				*out++ = 0;
				*out++ = *reps;
				count += 2;
				reps++;
			}
			else {
				
				/* What remains is at least 2 bytes but not larger than what
				   can be written in a single chunk */
				   
				*out++ = 257 - l;
				*out++ = *reps;
				count += 2;
				reps   = now;
			}
		}
	}
	
	return( count );
}

/****************************************************************************/
/*		Low level procedures to send various commands to the printer		*/
/****************************************************************************/

private	void	SendReset( FILE *stream )
{
	SendString( stream, ESC "@" );
}

private	void	SendMargin( FILE *stream, int top, int bot )
{
	SendString( stream, ESC "(c" );
	SendWord( stream, 4 );
	SendWord( stream, bot );
	SendWord( stream, top );
}

private	void	SendPaper( FILE *stream, int length )
{
	SendString( stream, ESC "(C" );
	SendWord( stream, 2 );
	SendWord( stream, length );
}

private	void	SendGmode( FILE *stream, int on )
{
	SendString( stream, ESC "(G" );
	SendWord( stream, 1 );
	SendByte( stream, on );
}

private void	SendUnit( FILE *stream, int res )
{
	SendString( stream, ESC "(U" );
	SendWord( stream, 1 );
	SendByte( stream, res );
}

private	void	SendUnidir( FILE *stream, int on )
{
	SendString( stream, ESC "U" );
	SendByte( stream, on );
}

private	void	SendMicro( FILE *stream, int on )
{
	SendString( stream, ESC "(i" );
	SendWord( stream, 1 );
	SendByte( stream, on );
}

private void	SendInk( FILE *stream, int x )
{
	SendString( stream, ESC "(e" );
	SendWord( stream, 2 );
	SendByte( stream, 0 );
	SendByte( stream, x );
}

private	void	SendDown( FILE *stream, int x )
{
	SendString( stream, ESC "(v" );
	SendWord( stream, 2 );
	SendWord( stream, x );
}

private	void	SendRight( FILE *stream, int amount )
{
	SendString( stream, ESC "(\\" );
	SendWord( stream, 4 );
	SendWord( stream, 1440 );
	SendWord( stream, amount );
}

private	void	SendColour( FILE *stream, int col )
{
static	int	ccode[] = { 0x000, 0x200, 0x100, 0x400, 0x201, 0x101 };

	SendString( stream, ESC "(r" );
	SendWord( stream, 2 );
	SendWord( stream, ccode[ col ] );
}

private void	SendData( FILE *stream, int hres, int vres, int noz, int col ) 
{
	SendString( stream, ESC "." );
	SendByte( stream, 1 );				/* Run-length encoded data */
	
	/* If we use 1 nozzle, then vertical resolution is what it is.
	   Otherwise it must be set to 90 dpi */
	   
	if ( noz == 1 )
	
		SendByte( stream, RESCODE( vres ) );
	else
		SendByte( stream, RESCODE( 90 ) );

	/* The horizontal resolution is max. 720 dpi */
	
	if ( hres > 720 )
	
		SendByte( stream, RESCODE( 720 ) );
	else
		SendByte( stream, RESCODE( hres ) );
		
	SendByte( stream, noz );
	SendWord( stream, col );
}
		
private	void	SendString( FILE *stream, const char *s )
{
	while ( *s ) SendByte( stream, *s++ );
}

/****************************************************************************/
/*					Halftoning wrapper functions							*/
/****************************************************************************/

/*
*	Calls the start function of the choosen halftoner
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	HalftonerStart( RENDER *render, int line )
{
	(*(htable[ render->dev->halftoner ].hstrt))( render, line );
}

/*
*	Returns the restart threshold for the given halftoner
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	int		HalftoneThold( RENDER *render )
{
	return( (*(htable[ render->dev->halftoner ].hthld))( render ) );
}

/*
*	This function renders a line
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*	This function has one fundamental assumption: halftoning of separate
*	colours is independent of each other.
*	
*	It calls the mono halftoner with the K, C, M, Y components.
*/

private	void	HalftoneLine( RENDER *render, int line, byte *data )
{
void		(*htone)( HTONE *, int );
EDEV		*dev;
int			offs;
HTONE		hdata;
short		*errs[ MAX_ED_LINES ];
int			i;

	/* Get the rendering function */
	
	dev   = render->dev;
	htone = htable[ render->dev->halftoner ].htone;
	offs  = render->mono ? 0 : OFFS_K;
	
	if ( dev->mono ) {
	
		/* Monochrome, do only the black */
			
		for ( i = 0 ; i < MAX_ED_LINES ; i++ ) 
	
			errs[ i ] = render->error[ i ][ OFFS_K ];
		
		hdata.render = render;
		hdata.data   = data + OFFS_K;
		hdata.step	 = sizeof( byte );
		hdata.res	 = render->res[ OFFS_K ];
		hdata.block  = NULL;
		hdata.err	 = errs;
		hdata.mval	 = 255;
		
		(*htone)( &hdata, line );
	}
	else {
		
		/* Colour. D black first */
	
		for ( i = 0 ; i < MAX_ED_LINES ; i++ ) 
			
			errs[ i ] = render->error[ i ][ OFFS_K ];
		
		hdata.render = render;
		hdata.step	 = sizeof( long );
		hdata.data   = data + OFFS_K;
		hdata.res	 = render->res[ OFFS_K ];
		hdata.block  = NULL;
		hdata.err	 = errs;
		hdata.mval	 = 255;
		
		(*htone)( &hdata, line );
		
		/* Yellow has no intermediate ink. The already done black
		   may inhibit it. */
	
		for ( i = 0 ; i < MAX_ED_LINES ; i++ ) 
		
			errs[ i ] = render->error[ i ][ OFFS_Y ];
			
		hdata.render = render;
		hdata.step	 = sizeof( long );
		hdata.data   = data + OFFS_Y;
		hdata.res	 = render->res[ OFFS_Y ];
		hdata.block  = dev->pureblack ? render->res[ OFFS_K ] : NULL;
		hdata.err	 = errs;
		hdata.mval	 = 255;

		(*htone)( &hdata, line );
	
		/* Cyan and magenta has intermediate colour ink, black may inhibit */
	
		for ( i = 0 ; i < MAX_ED_LINES ; i++ ) 
		
			errs[ i ] = render->error[ i ][ OFFS_C ];
			
		hdata.data   = data + OFFS_C;
		hdata.res	 = render->res[ OFFS_C ];
		hdata.block  = dev->pureblack ? render->res[ OFFS_K ] : NULL;
		hdata.mval	 = dev->midcyan;

		(*htone)( &hdata, line );

		for ( i = 0 ; i < MAX_ED_LINES ; i++ ) 
		
			errs[ i ] = render->error[ i ][ OFFS_M ];
			
		hdata.data   = data + OFFS_M;
		hdata.res	 = render->res[ OFFS_M ];
		hdata.block  = dev->pureblack ? render->res[ OFFS_K ] : NULL;
		hdata.mval	 = dev->midmagenta;

		(*htone)( &hdata, line );
	}
	
	/* Here we have create the raw device format scanlines */
	
	if ( dev->mono ) {
	
		if ( render->xres == 1440 ) {
		
			PackLine( render->res[ OFFS_K ], render->width, 255, 2, 
					  render->raw[ 0 ][ DEV_BLACK ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_K ]+1, render->width-1, 255, 2, 
					  render->raw[ 1 ][ DEV_BLACK ]+ line % MAX_MARK );
		}
		else {
		
			PackLine( render->res[ OFFS_K ], render->width, 255, 1, 
					  render->raw[ 0 ][ DEV_BLACK ]+ line % MAX_MARK );
		}
	}
	else {
	
		if ( render->xres == 1440 ) {
		
			PackLine( render->res[ OFFS_K ], render->width, 255, 2, 
					  render->raw[ 0 ][ DEV_BLACK ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_K ]+1, render->width-1, 255, 2, 
					  render->raw[ 1 ][ DEV_BLACK ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_C ], render->width, 255, 2, 
					  render->raw[ 0 ][ DEV_CYAN ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_C ]+1, render->width-1, 255, 2, 
					  render->raw[ 1 ][ DEV_CYAN ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_M ], render->width, 255, 2, 
					  render->raw[ 0 ][ DEV_MAGENTA ]+ line % MAX_MARK);
					  
			PackLine( render->res[ OFFS_M ]+1, render->width-1, 255, 2, 
					  render->raw[ 1 ][ DEV_MAGENTA ]+ line % MAX_MARK);
					  
			PackLine( render->res[ OFFS_Y ], render->width, 255, 2, 
					  render->raw[ 0 ][ DEV_YELLOW ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_Y ]+1, render->width-1, 255, 2, 
					  render->raw[ 1 ][ DEV_YELLOW ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_C ], render->width, dev->midcyan, 
					  2, render->raw[ 0 ][ DEV_LCYAN ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_C ]+1, render->width-1, dev->midcyan, 
					  2, render->raw[ 1 ][ DEV_LCYAN ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_M ], render->width, dev->midmagenta, 
					  2, render->raw[0][ DEV_LMAGENTA ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_M ]+1, render->width-1,dev->midmagenta,
					  2, render->raw[1][ DEV_LMAGENTA ]+ line % MAX_MARK );
		}
		else {
		
			PackLine( render->res[ OFFS_K ], render->width, 255, 1, 
					  render->raw[ 0 ][ DEV_BLACK ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_C ], render->width, 255, 1, 
					  render->raw[ 0 ][ DEV_CYAN ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_M ], render->width, 255, 1, 
					  render->raw[ 0 ][ DEV_MAGENTA ]+ line % MAX_MARK);
					  
			PackLine( render->res[ OFFS_Y ], render->width, 255, 1, 
					  render->raw[ 0 ][ DEV_YELLOW ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_C ], render->width, dev->midcyan, 
					  1, render->raw[ 0 ][ DEV_LCYAN ]+ line % MAX_MARK );
					  
			PackLine( render->res[ OFFS_M ], render->width, dev->midmagenta, 
					  1, render->raw[0][ DEV_LMAGENTA ]+ line % MAX_MARK );
		}
	}
	
	/* Call the halftoner specific end-of-line function */
	
	(*htable[ render->dev->halftoner ].hteol)( render, line );
}

/****************************************************************************/
/*					Floyd - Steinberg error diffusion						*/
/****************************************************************************/

/*
*	This function returns the empty range threshold
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	int		FloydSThold( RENDER *p )
{
	return( 5 );
}

/*
*	This function initialises the halftoner
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	FloydSStart( RENDER *p, int line )
{
	memset( p->err, 0, ICOLN * MAX_PIXELS*2 );
	p->error[ 0 ] = p->err[ 0 ];
}

/*
*	This function does the end-of-line processing
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	FloydSEol( RENDER *p, int line )
{
	/* Since we use single error buffering, nothing to do */
}

/*
*	This is the classical Floyd-Steinberg error diffusion.
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*	The matrix is the following:
*
*          *    7/16	r
*   3/16  5/16  1/16
*
*	r is the residual (0, in theory). 
*	Absolutely nothing fancy is done here.
*
*/

private	void   FloydSLine( HTONE *htone, int y )
{
int		x;							/* Counts the pixels					*/
int		pixel;						/* Current pixel value					*/
int		pixerr;						/* Error value							*/
int		length;						/* Number of pixels to process			*/
byte	*res;						/* Result								*/
byte	*data;						/* Input data							*/
byte	*block;						/* Block pixel							*/
int		lim1, lim2;					/* Limits								*/
short	e0, e1;						/* Propagating errors in current line	*/
short	*l0;						/* Error buffer pointer					*/

	length  = htone->render->width;

	res		= htone->res;
	data	= htone->data;
	block	= htone->block;
	
	lim1	= htone->mval / 2;
	lim2	= ( htone->mval + 256 ) / 2;
	
	l0		= htone->err[ 0 ];
	
	e0		= l0[ 1 ];
	e1		= l0[ 2 ];
	
	l0[ 1 ] = 0;
	l0[ 2 ] = 0;
		
	for ( x = 0 ; x < length ; x++ ) {
	
		/* First, clear the res byte. It is needed for the black */
		
		*res = 0;
		
		/* Add the actual error to the pixel, normalise, init, whatever. */
		
		pixel = ( ( *data << 4 ) + e0 );
		e0 = e1;
		e1 = l0[ 3 ] + ( pixel & 15 );			/* This is the residual */
		
		l0[ 3 ] = 0;
		pixel >>= 4;
		
		if ( ( block && *block ) || ( pixel < lim1 ) )
		
			*res = 0;

		else if ( pixel >= lim2 )
		
			*res = 255;
		else
			*res = htone->mval;
		
		/* Calculate the err */
		
		pixerr = pixel - *res;
		
		/* Diffuse the err */
	
		e0		+= ( pixerr << 3 ) - pixerr;	/* 7/16		*/
		l0[ 0 ] += ( pixerr << 2 ) - pixerr;	/* 3/16		*/
		l0[ 1 ] += ( pixerr << 2 ) + pixerr;	/* 5/16		*/
		l0[ 2 ] += pixerr;						/* 1/16		*/
				
		/* We have done everything, move the pointers */
		
		res++;
		if ( block ) block++;
		data += htone->step;
		l0++;
	}
}

/****************************************************************************/
/*							Ordered dither									*/
/****************************************************************************/

/*
*	This function returns the empty range threshold
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	int		DitherThold( RENDER *p )
{
	return( 0 );
}

/*
*	This function initialises the halftoner
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	DitherStart( RENDER *p, int line )
{
	/* Nothing to initialise */
}

/*
*	This function does the end-of-line processing
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	DitherEol( RENDER *p, int line )
{
	/* Nothing to do - dithering has no memory */
}

/*
*	Clustered dither of a particular colour of a line
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void   DitherLine( HTONE *htone, int y )
{
int		x;							/* Counts the pixels					*/
int		pixel;						/* Current pixel value					*/
int		length;						/* Number of pixels to process			*/
byte	*res;						/* Result								*/
byte	*data;						/* Input data							*/
byte	*block;						/* Block pixel							*/
byte	*matrix;					/* Dither matrix's current line			*/
int		mx;							/* Matrix index							*/
int		lval, hval;					/* Halftoned high/low values			*/

	length  = htone->render->width;

	res		= htone->res;
	data	= htone->data;
	block	= htone->block;
	
	matrix	= dmatrix[ y % DMATRIX_Y ];
		
	for ( mx = x = 0 ; x < length ; x++ ) {
	
		/* First, clear the res byte. It is needed for the black */
		
		*res = 0;
		
		/* Next, see if the pixel is above the mval */
		
		if ( ( pixel = *data ) > htone->mval ) {
		
			lval = htone->mval;
			hval = 255;
			
			if ( htone->mval == 127 )

				pixel = ( ( pixel - htone->mval ) * 2 - 1 ) / 2;
			else
				pixel = ( pixel - htone->mval ) * 255 / ( 255 - htone->mval );
		}
		else {
		
			lval = 0;
			hval = htone->mval;
			
			if ( htone->mval != 255 ) {
			
				if ( htone->mval == 127 )
				
					pixel = ( pixel * 4 + 1 ) / 2;
				else
					pixel = pixel * 255 / htone->mval;
			}
		}
		
		if ( block && *block ) {
		
			*res = 0;
		}
		else {
		
			if ( pixel >= matrix[ mx ] )
		
				*res = hval;
			else
				*res = lval;
		}
		
		res++;
		if ( ++mx == DMATRIX_X ) mx = 0;
		if ( block ) block++;
		data += htone->step;
	}
}

/****************************************************************************/
/*					Bendor's error diffusion								*/
/****************************************************************************/

/*
*	This function returns the empty range threshold
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	int		BendorThold( RENDER *p )
{
	return( 5 );
}

/*
*	This function initialises the halftoner
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	BendorStart( RENDER *p, int line )
{
	memset( p->err, 0, 2 * ICOLN * MAX_PIXELS*2 );
	p->error[ 0 ] = p->err[ 0 ];
	p->error[ 1 ] = p->err[ 1 ];
}

/*
*	This function does the end-of-line processing
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

private	void	BendorEol( RENDER *p, int line )
{
void	*x;

	x = p->error[ 0 ];
	p->error[ 0 ] = p->error[ 1 ];
	p->error[ 1 ] = x;
}

/*
*	Error diffusion of a particular colour of a line
*	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*
*	This is not yet finished (the matrix is bad, actually).
*
*	The matrix is the following (the normalisation factor is 1/128, 
*	'*' represents the current pixel, r is the truncation residual):
*
*				 *	20	10  r
*		8	14	20	14	 8
*		4	 8	10	 8	 4
*
*	We also try to take the splashing effect into account (the ink disperses
*	when it hits the paper so it partially covers surrounding pixels).
*	We use an other matrix for that, which is very simple:
*
*			*	3
*		2	3	2
*
*	and the normalisation factor can be set by the user. 
*	The splash matrix is only applied if we have actually deposited 
*	ink and the amount added to the errors is independent that of the 
*	actual image value, it only depends on the ink applied.
*	Of course, the ink spreads up and left as well and we could compensate
*	for this for a certain extent by keeping track of the errors caused in
*	previous pixels and lines and modifying them accordingly but it
*	would lead to a horrible code mess and it wouldn't be worth the effort.
*
*	A further enhancement that we allow the error to 'leak'. Experimental 
*	results show that with a 5-15% loss of error the image quality 
*	increases and the colour distortion remains very low. If you think
*	about it, this, in effect stops the error to spread its effect over
*	large areas but it will have almost undisturbed effect on neighbouring 
*	areas (you allow for an exponential error decay). 
*	This parameter is user definable, too.
*/

private	void   BendorLine( HTONE *htone, int y )
{
int		x;							/* Counts the pixels					*/
int		pixel;						/* Current pixel value					*/
int		pixerr;						/* Error value							*/
int		pixe14;						/* 14 * err value						*/
int		sval;						/* Splash correction value				*/
int		splash;						/* Splash factor						*/
int		leakage;					/* Leakage factor						*/
int		length;						/* Number of pixels to process			*/
byte	*res;						/* Result								*/
byte	*data;						/* Input data							*/
byte	*block;						/* Block pixel							*/
int		lim1, lim2;					/* Limits								*/
short	e0, e1;						/* Propagating errors in current line	*/
short	*l0, *l1;					/* Error buffer pointers				*/

	splash  = htone->render->dev->splash;
	leakage = htone->render->dev->splash;
	length  = htone->render->width;

	res		= htone->res;
	data	= htone->data;
	block	= htone->block;
	
	lim1	= htone->mval / 2;
	lim2	= ( htone->mval + 256 ) / 2;
	
	l0		= htone->err[ 0 ];
	l1		= htone->err[ 1 ];
	
	e0		= l0[ 2 ];
	e1		= l0[ 3 ];
	
	l0[ 2 ] = 0;
	l0[ 3 ] = 0;
		
	for ( x = 0 ; x < length ; x++ ) {
	
		/* First, clear the res byte. It is needed for the black */
		
		*res = 0;
		
		/* Add the actual error to the pixel, normalise, init, whatever. */
		
		pixel = ( ( *data << 7 ) + e0 );
		e0 = e1;
		e1 = l0[ 4 ] + ( pixel & 127 );			/* This is the residual */
		
		l0[ 4 ] = 0;
		pixel >>= 7;
		
		if ( ( block && *block ) || ( pixel < lim1 ) )
		
			*res = 0;

		else if ( pixel >= lim2 )
		
			*res = 255;
		else
			*res = htone->mval;
		
		/* Calculate the err */
		
		pixerr = pixel - *res;
		
		/* If leakage is defined, apply it */
		
		if ( leakage ) pixerr -= ( pixerr * leakage ) / 100;
		
		/* Diffuse the err */
	
		pixerr <<= 1;							/* Multiplier is 2	*/
		pixe14 = pixerr;						/* pixe14 now 2		*/
		pixerr <<= 1;							/* Multiplier is 4	*/
		pixe14 += pixerr;						/* pixe14 now 6		*/
		
		l0[ 0 ] += pixerr;
		l0[ 4 ] += pixerr;
		
		pixerr <<= 1;							/* Multiplier is 8	*/
		pixe14 += pixerr;						/* pixe14 now 14	*/
		
		l0[ 1 ] += pixerr;
		l0[ 3 ] += pixerr;
		l1[ 0 ] += pixerr;
		l1[ 4 ] += pixerr;
		
		pixerr += pixerr >> 2;					/* Multiplier is 10	*/
		
		l0[ 2 ] += pixerr;
		e1		+= pixerr;
		
		pixerr <<= 1;							/* Multiplier is 20 */
		
		l1[ 2 ] += pixerr;
		e0		+= pixerr;
		
		/* pixe14 already contains 14 * err */
		
		l1[ 1 ] += pixe14;
		l1[ 3 ] += pixe14;
		
		/* If splashing is defined, apply the splash matrix.
		   The splash value is normalised to the same level as the err */
		
		if ( splash && *res ) {
		
			sval = splash * *res;				/* This is the 2x value	*/
			
			l1[ 1 ] -= sval;
			l1[ 3 ] -= sval;
			
			sval += sval >> 1;					/* This represents 3x	*/
			
			e0		-= sval;
			l1[ 2 ] -= sval;
		}
		
		/* We have done everything, move the pointers */
		
		res++;
		if ( block ) block++;
		data += htone->step;
		l0++, l1++;
	}
}
