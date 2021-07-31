/* Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* gdevmgr.h */
/* Common header file for MGR devices */

#define MGR_RESERVEDCOLORS 16


/* extract from dump.h */

/*
 * format for saved bitmaps
 */

#define B_PUTHDR8(hdr, w, h, d) (			\
	(hdr)->magic[0]  = 'y', (hdr)->magic[1] = 'z',  \
	(hdr)->h_wide    = (((w) >> 6) & 0x3f) + ' ',	\
	(hdr)->l_wide    = ((w) & 0x3f) + ' ',		\
	(hdr)->h_high    = (((h) >> 6) & 0x3f) + ' ',	\
	(hdr)->l_high    = ((h) & 0x3f) + ' ',		\
	(hdr)->depth     = ((d) & 0x3f) + ' ',		\
	(hdr)->_reserved = ' ' )

struct b_header {
  char magic[2];           /* magics */
  char h_wide;             /* upper byte width (biased with 0x20) */
  char l_wide;             /* lower byte width (biased with 0x20) */
  char h_high;             /* upper byte height (biased with 0x20) */
  char l_high;             /* lower byte height (biased with 0x20) */
  char depth;              /* depth (biased with 0x20) */
  char _reserved;          /* for alignment */
};

/*
 * Color lookup table information
 */
struct nclut {
  unsigned short colnum;
  unsigned short red, green, blue;
} ;


/* extract from color.h */

/*
 * MGR Color Definitions
 */

#define LUT_BW    0
#define LUT_GREY  1
#define LUT_BGREY 2
#define LUT_VGA   3
#define LUT_BCT   4
#define LUT_USER  5
#define LUT       6
#define LUT_8     LUT

#define RGB_RED   0
#define RGB_GREEN 1
#define RGB_BLUE  2
#define RGB       3

#define LUTENTRIES 16

#define BW_RED       15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0, 15, 0
#define BW_GREEN     BW_RED
#define BW_BLUE      BW_RED

#define GREY_RED     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
#define GREY_GREEN   GREY_RED
#define GREY_BLUE    GREY_RED

#define BGREY_RED    1, 0, 2, 8, 4, 3, 13, 11, 7, 6, 10, 12, 14, 5, 9, 15
#define BGREY_GREEN  BGREY_RED
#define BGREY_BLUE   BGREY_RED

#define VGA_RED      0, 0, 0, 0, 8, 8, 8, 12, 8,  0,  0,  0, 15, 15, 15, 15
#define VGA_GREEN    0, 0, 8, 8, 0, 0, 8, 12, 8,  0, 15, 15,  0,  0, 15, 15
#define VGA_BLUE     0, 8, 0, 8, 0, 8, 0, 12, 8, 15,  0, 15,  0, 15,  0, 15

#define BCT_RED      1,  7,  6, 15, 14, 3, 13, 11, 7, 13, 13, 15, 15, 5, 9, 15
#define BCT_GREEN    1,  7, 13, 12,  5, 3, 13, 11, 7, 14, 15, 15, 14, 5, 9, 15
#define BCT_BLUE     1, 14,  6,  8,  5, 3, 13, 11, 7, 15, 14, 12, 13, 5, 9, 15

#define USER_RED     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define USER_GREEN   USER_RED
#define USER_BLUE    USER_RED

static char mgrlut[LUT][RGB][LUTENTRIES] = {
  { { BW_RED },    { BW_GREEN },    { BW_BLUE } },
  { { GREY_RED },  { GREY_GREEN },  { GREY_BLUE } },
  { { BGREY_RED }, { BGREY_GREEN }, { BGREY_BLUE } },
  { { VGA_RED },   { VGA_GREEN },   { VGA_BLUE } },
  { { BCT_RED },   { BCT_GREEN },   { BCT_BLUE } },
  { { USER_RED },  { USER_GREEN },  { USER_BLUE } }
};
