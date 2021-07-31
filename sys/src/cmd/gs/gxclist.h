/* Copyright (C) 1991, 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxclist.h */
/* Command list definitions for Ghostscript. */
/* Requires gxdevice.h and gxdevmem.h */

/*
 * A command list is essentially a compressed list of driver calls.
 * Command lists are used to record an image that must be rendered in bands
 * for a high-resolution printer.  In the future, they may be used
 * for other purposes as well, such as providing a more compact representation
 * of cached characters than a full bitmap (at high resolution), or
 * representing fully rendered user paths (a Level 2 feature).
 */

/* A command list device outputs commands to a stream, */
/* then reads them back to render in bands. */
typedef struct gx_device_clist_s gx_device_clist;
typedef struct gx_clist_state_s gx_clist_state;
typedef struct {
	int slot_index;
} tile_hash;
typedef struct {
	gx_bitmap_id id;
	/* byte band_mask[]; */
	/* byte bits[]; */
} tile_slot;
struct gx_device_clist_s {
	gx_device_forward_common;	/* (see gxdevice.h) */
	/* Following must be set before writing or reading. */
	/* gx_device *target; */	/* device for which commands */
					/* are being buffered */
	byte *data;			/* buffer area */
	uint data_size;			/* size of buffer */
	FILE *cfile;			/* command list file */
	FILE *bfile;			/* command list block file */
	/* Following are used only when writing. */
	gx_clist_state *states;		/* current state of each band */
	byte *cbuf;			/* start of command buffer */
	byte *cnext;			/* next slot in command buffer */
	byte *cend;			/* end of command buffer */
	gx_clist_state *ccls;		/* clist_state of last command */
	/* Following is the tile cache, used only when writing. */
	gx_tile_bitmap tile;		/* current tile cache parameters */
					/* (data pointer & id not used) */
	tile_hash *tile_hash_table;	/* hash table for tile cache: */
					/* -1 = unused, >= 0 = slot index */
	uint tile_max_size;		/* max size of a single tile */
	uint tile_hash_mask;		/* size of tile hash table -1 */
	uint tile_band_mask_size;	/* size of band mask preceding */
					/* each tile in the cache */
	uint tile_count;		/* # of occupied entries in cache */
	uint tile_max_count;		/* capacity of the cache */
	/* Following are used for both writing and reading. */
	byte *tile_data;		/* data for cached tiles */
	uint tile_data_size;		/* total size of tile cache data */
	uint tile_slot_size;		/* size of each slot in tile cache */
	int tile_depth;			/* depth of current tile (1 or */
					/* dev->color_info.depth) */
#define tile_slot_ptr(cldev,index)\
  (tile_slot *)((cldev)->tile_data + (index) * (cldev)->tile_slot_size)
#define ts_mask(pts) (byte *)((pts) + 1)
#define ts_bits(cldev,pts) (ts_mask(pts) + (cldev)->tile_band_mask_size)
	/* Following are set when writing, read when reading. */
	int band_height;		/* height of each band */
	int nbands;			/* # of bands */
	long bfile_end_pos;		/* ftell at end of bfile */
	/* Following are used only when reading. */
	int ymin, ymax;			/* current band */
	gx_device_memory mdev;
};
/* The device template itself is never used, only the procs. */
extern gx_device_procs gs_clist_device_procs;
