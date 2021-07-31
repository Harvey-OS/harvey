/* Copyright (C) 1991, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxcldev.h */
/* Internal definitions for Ghostscript command lists. */
#include "gxclist.h"

#define cdev ((gx_device_clist *)dev)

/* A command always consists of an operation followed by operands. */
/* The operands are implicit in the procedural code. */
/* Operands marked # are variable-size. */
typedef enum {
	cmd_op_misc = 0x00,		/* (see below) */
	  cmd_opv_end_run = 0x00,	/* (nothing) */
	  cmd_opv_set_tile_size = 0x01,	/* width#, height# */
	  cmd_opv_set_tile_phase = 0x02, /* x#, y# */
	  cmd_opv_set_tile_index = 0x03, /* index */
	  cmd_opv_set_tile_bits = 0x04,	/* index, <bits> */
	  cmd_opv_set_tile_size_colored = 0x05, /* width#, height# */
	cmd_op_set_color0 = 0x10,	/* color+2 in op byte | color# */
	cmd_op_set_color1 = 0x20,	/* color+2 in op byte | color# */
	cmd_op_delta_tile_bits = 0x30,	/* n-1 in op byte, n x <offset, bits> */
	cmd_op_fill_rect = 0x40,	/* rect# */
	cmd_op_fill_rect_short = 0x50,	/* dh in op byte,dx,dw | rect_short */
	cmd_op_fill_rect_tiny = 0x60,	/* dw in op byte, rect_tiny */
	cmd_op_tile_rect = 0x70,	/* rect# */
	cmd_op_tile_rect_short = 0x80,	/* dh in op byte,dx,dw | rect_short */
	cmd_op_tile_rect_tiny = 0x90,	/* dw in op byte, rect_tiny */
	cmd_op_copy_mono = 0xa0,	/* (rect#, data_x#, raster# | */
					/*  d_x+1 in op byte, x#, y#, w, h), */
					/* <bits> */
	cmd_op_copy_mono_rle = 0xb0,	/* rect#, data_x#, raster#, <bits> */
	cmd_op_copy_color = 0xc0,	/* rect#, data_x#, raster#, <bits> */
	cmd_op_end
} gx_cmd_op;

#define cmd_op_name_strings\
  "misc", "set_color_0", "set_color_1", "delta_tile_bits",\
  "fill_rect", "fill_rect_short", "fill_rect_tiny", "tile_rect",\
  "tile_rect_short", "tile_rect_tiny", "copy_mono", "copy_mono_rle",\
  "copy_color", "?d0?", "?e0?", "?f0?"

#define cmd_misc_op_name_strings\
  "end_run", "set_tile_size", "set_tile_phase", "set_tile_index",\
  "set_tile_bits", "set_tile_size_colored", "?06?", "?07?",\
  "?08?", "?09?", "?0a?", "?0b?",\
  "?0c?", "?0d?", "?0e?", "?0f?"\

#ifdef DEBUG
extern const char *cmd_op_names[16];
extern const char *cmd_misc_op_names[16];
#endif

/* Define the size of the largest command, not counting any bitmap. */
#define cmd_tile_d 3			/* size of tile delta */
/* The variable-size integer encoding is little-endian.  The low 7 bits */
/* of each byte contain data; the top bit is 1 for all but the last byte. */
#define cmd_max_intsize(siz)\
  (((siz) * 8 + 6) / 7)
#define cmd_largest_size\
  max(1 + 6 * cmd_max_intsize(sizeof(int)) /* copy_mono */, 2 + 16 * cmd_tile_d	/* delta_tile 15 */)

/* Define command parameter structures. */
typedef struct {
	int x, y, width, height;
} gx_cmd_rect;
typedef struct {
	byte dx, dwidth, dy, dheight;	/* dy and dheight are optional */
} gx_cmd_rect_short;
#define cmd_min_short (-128)
#define cmd_max_short 127
typedef struct {
	unsigned dx : 4;
	unsigned dy : 4;
} gx_cmd_rect_tiny;
#define cmd_min_tiny (-8)
#define cmd_max_tiny 7

/* Define the prefix on each command run in the writing buffer. */
typedef struct cmd_prefix_s cmd_prefix;
struct cmd_prefix_s {
	cmd_prefix *next;
	uint size;
};

/* Define the entries in the block file. */
typedef struct cmd_block_s {
	int band;
	long pos;			/* starting position in cfile */
} cmd_block;

/* Remember the current state of one band when writing or reading. */
struct gx_clist_state_s {
	gx_color_index color0, color1;	/* most recent colors */
	tile_slot *tile;		/* most recent tile */
	gs_int_point tile_phase;	/* most recent tile phase */
	gx_cmd_rect rect;		/* most recent rectangle */
	/* Following are only used when writing */
	cmd_prefix *head, *tail;	/* list of commands for band */
};
static tile_slot no_tile = { ~0L };

/* The initial values for a band state */
static const gx_clist_state cls_initial =
   {	gx_no_color_index, gx_no_color_index, &no_tile,
	 { 0, 0 }, { 0, 0, 0, 0 },
	0, 0
   };

#define clist_write(f, str, len)\
  fwrite(str, 1, len, f)

/* Define the size of the command buffer used for reading. */
/* This is needed to split up very large copy_ operations. */
#define cbuf_size 500
