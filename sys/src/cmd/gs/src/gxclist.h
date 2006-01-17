/* Copyright (C) 1991, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxclist.h,v 1.7 2005/03/14 18:08:36 dan Exp $ */
/* Command list definitions for Ghostscript. */
/* Requires gxdevice.h and gxdevmem.h */

#ifndef gxclist_INCLUDED
#  define gxclist_INCLUDED

#include "gscspace.h"
#include "gxband.h"
#include "gxbcache.h"
#include "gxclio.h"
#include "gxdevbuf.h"
#include "gxistate.h"
#include "gxrplane.h"

/*
 * A command list is essentially a compressed list of driver calls.
 * Command lists are used to record an image that must be rendered in bands
 * for high-resolution and/or limited-memory printers.
 *
 * Command lists work in two phases.  The first phase records driver calls,
 * sorting them according to the band(s) they affect.  The second phase
 * reads back the commands band-by-band to create the bitmap images.
 * When opened, a command list is in the recording state; it switches
 * automatically from recording to reading when its get_bits procedure
 * is called.  Currently, there is a hack to reopen the device after
 * each page is processed, in order to switch back to recording.
 */

/*
 * The command list contains both commands for particular bands (the vast
 * majority) and commands that apply to a range of bands.  In order to
 * synchronize the two, we maintain the following invariant for buffered
 * commands:
 *
 *      If there are any band-range commands in the buffer, they are the
 *      first commands in the buffer, before any specific-band commands.
 *
 * To maintain this invariant, whenever we are about to put an band-range
 * command in the buffer, we check to see if the buffer already has any
 * band-range commands in it, and if so, whether they are the last commands
 * in the buffer and are for the same range; if the answer to any of these
 * questions is negative, we flush the buffer.
 */

/* ---------------- Public structures ---------------- */

/*
 * Define a saved page object.  This consists of a snapshot of the device
 * structure, information about the page per se, and the num_copies
 * parameter of output_page.
 */
typedef struct gx_saved_page_s {
    gx_device device;
    char dname[8 + 1];		/* device name for checking */
    gx_band_page_info_t info;
    int num_copies;
} gx_saved_page;

/*
 * Define a saved page placed at a particular (X,Y) offset for rendering.
 */
typedef struct gx_placed_page_s {
    gx_saved_page *page;
    gs_int_point offset;
} gx_placed_page;
  
/*
 * Define a procedure to cause some bandlist memory to be freed up,
 * probably by rendering current bandlist contents.
 */
#define proc_free_up_bandlist_memory(proc)\
  int proc(gx_device *dev, bool flush_current)

/* ---------------- Internal structures ---------------- */

/*
 * Currently, halftoning occurs during the first phase, producing calls
 * to tile_rectangle.  Both phases keep a cache of recently seen bitmaps
 * (halftone cells and characters), which allows writing only a short cache
 * index in the command list rather than the entire bitmap.
 *
 * We keep only a single cache for all bands, but since the second phase
 * reads the command lists for each band separately, we have to keep track
 * for each cache entry E and band B whether the definition of E has been
 * written into B's list.  We do this with a bit mask in each entry.
 *
 * Eventually, we will replace this entire arrangement with one in which
 * we pass the actual halftone screen (whitening order) to all bands
 * through the command list, and halftoning occurs on the second phase.
 * This not only will shrink the command list, but will allow us to apply
 * other rendering algorithms such as error diffusion in the second phase.
 */
typedef struct {
    ulong offset;		/* writing: offset from cdev->data, */
    /*   0 means unused */
    /* reading: offset from cdev->chunk.data */
} tile_hash;
typedef struct {
    gx_cached_bits_common;
    /* To save space, instead of storing rep_width and rep_height, */
    /* we store width / rep_width and height / rep_height. */
    byte x_reps, y_reps;
    ushort rep_shift;
    ushort index;		/* index in table (hash table when writing) */
    ushort num_bands;		/* # of 1-bits in the band mask */
    /* byte band_mask[]; */
#define ts_mask(pts) (byte *)((pts) + 1)
    /* byte bits[]; */
#define ts_bits(cldev,pts) (ts_mask(pts) + (cldev)->tile_band_mask_size)
} tile_slot;

/* Define the prefix on each command run in the writing buffer. */
typedef struct cmd_prefix_s cmd_prefix;
struct cmd_prefix_s {
    cmd_prefix *next;
    uint size;
};

/* Define the pointers for managing a list of command runs in the buffer. */
/* There is one of these for each band, plus one for band-range commands. */
typedef struct cmd_list_s {
    cmd_prefix *head, *tail;	/* list of commands for band */
} cmd_list;

/*
 * In order to keep the per-band state down to a reasonable size,
 * we store only a single set of the imager state parameters;
 * for each parameter, each band has a flag that says whether that band
 * 'knows' the current value of the parameters.
 */
extern const gs_imager_state clist_imager_state_initial;

/*
 * Define the main structure for holding command list state.
 * Unless otherwise noted, all elements are used in both the writing (first)
 * and reading (second) phase.
 */
typedef struct gx_clist_state_s gx_clist_state;

#define gx_device_clist_common_members\
	gx_device_forward_common;	/* (see gxdevice.h) */\
		/* Following must be set before writing or reading. */\
		/* See gx_device_clist_writer, below, for more that must be init'd */\
	/* gx_device *target; */	/* device for which commands */\
					/* are being buffered */\
	gx_device_buf_procs_t buf_procs;\
	gs_memory_t *bandlist_memory;	/* allocator for in-memory bandlist files */\
	byte *data;			/* buffer area */\
	uint data_size;			/* size of buffer */\
	gx_band_params_t band_params;	/* band buffering parameters */\
	bool do_not_open_or_close_bandfiles;	/* if true, do not open/close bandfiles */\
	bool page_uses_transparency;	/* if true then page uses PDF 1.4 transparency */\
		/* Following are used for both writing and reading. */\
	gx_bits_cache_chunk chunk;	/* the only chunk of bits */\
	gx_bits_cache bits;\
	uint tile_hash_mask;		/* size of tile hash table -1 */\
	uint tile_band_mask_size;	/* size of band mask preceding */\
					/* each tile in the cache */\
	tile_hash *tile_table;		/* table for tile cache: */\
					/* see tile_hash above */\
					/* (a hash table when writing) */\
	int ymin, ymax;			/* current band, <0 when writing */\
		/* Following are set when writing, read when reading. */\
	gx_band_page_info_t page_info;	/* page information */\
	int nbands		/* # of bands */

typedef struct gx_device_clist_common_s {
    gx_device_clist_common_members;
} gx_device_clist_common;

#define clist_band_height(cldev) ((cldev)->page_info.band_height)
#define clist_cfname(cldev) ((cldev)->page_info.cfname)
#define clist_cfile(cldev) ((cldev)->page_info.cfile)
#define clist_bfname(cldev) ((cldev)->page_info.bfname)
#define clist_bfile(cldev) ((cldev)->page_info.bfile)

/* Define the length of the longest dash pattern we are willing to store. */
/* (Strokes with longer patterns are converted to fills.) */
#define cmd_max_dash 11

/* Define the state of a band list when writing. */
typedef struct clist_color_space_s {
    byte byte1;			/* see cmd_opv_set_color_space in gxclpath.h */
    gs_id id;			/* space->id for comparisons */
    const gs_color_space *space;
} clist_color_space_t;
typedef struct gx_device_clist_writer_s {
    gx_device_clist_common_members;	/* (must be first) */
    int error_code;		/* error returned by cmd_put_op */
    gx_clist_state *states;	/* current state of each band */
    byte *cbuf;			/* start of command buffer */
    byte *cnext;		/* next slot in command buffer */
    byte *cend;			/* end of command buffer */
    cmd_list *ccl;		/* &clist_state.list of last command */
    cmd_list band_range_list;	/* list of band-range commands */
    int band_range_min, band_range_max;		/* range for list */
    uint tile_max_size;		/* max size of a single tile (bytes) */
    uint tile_max_count;	/* max # of hash table entries */
    gx_strip_bitmap tile_params;	/* current tile parameters */
    int tile_depth;		/* current tile depth */
    int tile_known_min, tile_known_max;  /* range of bands that knows the */
				/* current tile parameters */
    /*
     * NOTE: we must not set the line_params.dash.pattern member of the
     * imager state to point to the dash_pattern member of the writer
     * state (except transiently), since this would confuse the
     * garbage collector.
     */
    gs_imager_state imager_state;	/* current values of imager params */
    float dash_pattern[cmd_max_dash];	/* current dash pattern */
    const gx_clip_path *clip_path;	/* current clip path, */
				/* only non-transient for images */
    gs_id clip_path_id;		/* id of current clip path */
    clist_color_space_t color_space;	/* current color space, */
				/* only used for non-mask images */
    gs_id transfer_ids[4];	/* ids of transfer maps */
    gs_id black_generation_id;	/* id of black generation map */
    gs_id undercolor_removal_id;	/* id of u.c.r. map */
    gs_id device_halftone_id;	/* id of device halftone */
    gs_id image_enum_id;	/* non-0 if we are inside an image */
				/* that we are passing through */
	int error_is_retryable;		/* Extra status used to distinguish hard VMerrors */
	                           /* from warnings upgraded to VMerrors. */
	                           /* T if err ret'd by cmd_put_op et al can be retried */
	int permanent_error;		/* if < 0, error only cleared by clist_reset() */
	int driver_call_nesting;	/* nesting level of non-retryable driver calls */
	int ignore_lo_mem_warnings;	/* ignore warnings from clist file/mem */
		/* Following must be set before writing */
	proc_free_up_bandlist_memory((*free_up_bandlist_memory)); /* if nz, proc to free some bandlist memory */
	int disable_mask;		/* mask of routines to disable clist_disable_xxx */
} gx_device_clist_writer;

/* Bits for gx_device_clist_writer.disable_mask. Bit set disables behavior */
#define clist_disable_fill_path	(1 << 0)
#define clist_disable_stroke_path (1 << 1)
#define clist_disable_hl_image (1 << 2)
#define clist_disable_complex_clip (1 << 3)
#define clist_disable_nonrect_hl_image (1 << 4)
#define clist_disable_pass_thru_params (1 << 5)	/* disable EXCEPT at top of page */
#define clist_disable_copy_alpha (1 << 6) /* target does not support copy_alpha */

/* Define the state of a band list when reading. */
/* For normal rasterizing, pages and num_pages are both 0. */
typedef struct gx_device_clist_reader_s {
    gx_device_clist_common_members;	/* (must be first) */
    gx_render_plane_t yplane;		/* current plane, index = -1 */
					/* means all planes */
    const gx_placed_page *pages;
    int num_pages;
} gx_device_clist_reader;

typedef union gx_device_clist_s {
    gx_device_clist_common common;
    gx_device_clist_reader reader;
    gx_device_clist_writer writer;
} gx_device_clist;

extern_st(st_device_clist);
#define public_st_device_clist()	/* in gxclist.c */\
  gs_public_st_complex_only(st_device_clist, gx_device_clist,\
    "gx_device_clist", 0, device_clist_enum_ptrs, device_clist_reloc_ptrs,\
    gx_device_finalize)
#define st_device_clist_max_ptrs\
  (st_device_forward_max_ptrs + st_imager_state_num_ptrs + 1)

/* setup before opening clist device */
#define clist_init_params(xclist, xdata, xdata_size, xtarget, xbuf_procs, xband_params, xexternal, xmemory, xfree_bandlist, xdisable, pageusestransparency)\
    BEGIN\
	(xclist)->common.data = (xdata);\
	(xclist)->common.data_size = (xdata_size);\
	(xclist)->common.target = (xtarget);\
	(xclist)->common.buf_procs = (xbuf_procs);\
	(xclist)->common.band_params = (xband_params);\
	(xclist)->common.do_not_open_or_close_bandfiles = (xexternal);\
	(xclist)->common.bandlist_memory = (xmemory);\
	(xclist)->writer.free_up_bandlist_memory = (xfree_bandlist);\
	(xclist)->writer.disable_mask = (xdisable);\
	(xclist)->writer.page_uses_transparency = (pageusestransparency);\
    END

/* Determine whether this clist device is able to recover VMerrors */
#define clist_test_VMerror_recoverable(cldev)\
  ((cldev)->free_up_bandlist_memory != 0)

/* The device template itself is never used, only the procedures. */
extern const gx_device_procs gs_clist_device_procs;

/* Reset (or prepare to append to) the command list after printing a page. */
int clist_finish_page(gx_device * dev, bool flush);

/* Close the band files and delete their contents. */
int clist_close_output_file(gx_device *dev);

/*
 * Close and delete the contents of the band files associated with a
 * page_info structure (a page that has been separated from the device).
 */
int clist_close_page_info(gx_band_page_info_t *ppi);

/*
 * Compute the colors-used information in the page_info structure from the
 * information in the individual writer bands.  This is only useful at the
 * end of a page.  gdev_prn_colors_used calls this procedure if it hasn't
 * been called since the page was started.  clist_end_page also calls it.
 */
void clist_compute_colors_used(gx_device_clist_writer *cldev);

/* Define the abstract type for a printer device. */
#ifndef gx_device_printer_DEFINED
#  define gx_device_printer_DEFINED
typedef struct gx_device_printer_s gx_device_printer;
#endif

/* Do device setup from params passed in the command list. */
int clist_setup_params(gx_device *dev);

/*
 * Render a rectangle to a client-supplied image.  This implements
 * gdev_prn_render_rectangle for devices that are using banding.
 * 
 * Note that clist_render_rectangle only guarantees to render *at least* the
 * requested rectangle to bdev, offset by (-prect->p.x, -prect->p.y):
 * anything it does to avoid rendering regions outside the rectangle is
 * merely an optimization.  If the client really wants the output clipped to
 * some rectangle smaller than ((0, 0), (bdev->width, bdev->height)), it
 * must set up a clipping device.
 */
int clist_render_rectangle(gx_device_clist *cdev,
			   const gs_int_rect *prect, gx_device *bdev,
			   const gx_render_plane_t *render_plane,
			   bool clear);

#endif /* gxclist_INCLUDED */
