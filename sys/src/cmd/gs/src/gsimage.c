/* Copyright (C) 1996, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsimage.c,v 1.15 2005/06/21 16:50:50 igor Exp $ */
/* Image setup procedures for Ghostscript library */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gscspace.h"
#include "gsmatrix.h"		/* for gsiparam.h */
#include "gsimage.h"
#include "gxarith.h"		/* for igcd */
#include "gxdevice.h"
#include "gxiparam.h"
#include "gxpath.h"		/* for gx_effective_clip_path */
#include "gzstate.h"


/*
  The main internal invariant for the gs_image machinery is
  straightforward.  The state consists primarily of N plane buffers
  (planes[]).
*/
typedef struct image_enum_plane_s {
/*
  The state of each plane consists of:

  - A row buffer, aligned and (logically) large enough to hold one scan line
    for that plane.  (It may have to be reallocated if the plane width or
    depth changes.)  A row buffer is "full" if it holds exactly a full scan
    line.
*/
    gs_string row;
/*
  - A position within the row buffer, indicating how many initial bytes are
    occupied.
*/
    uint pos;
/*
  - A (retained) source string, which may be empty (size = 0).
*/
    gs_const_string source;
} image_enum_plane_t;
/*
  The possible states for each plane do not depend on the state of any other
  plane.  Either:

  - pos = 0, source.size = 0.

  - If the underlying image processor says the plane is currently wanted,
    either:

    - pos = 0, source.size >= one full row of data for this plane.  This
      case allows us to avoid copying the data from the source string to the
      row buffer if the client is providing data in blocks of at least one
      scan line.

    - pos = full, source.size may have any value.

    - pos > 0, pos < full, source.size = 0;

  - If the underlying image processor says the plane is not currently
    wanted:

    - pos = 0, source.size may have any value.

  This invariant holds at the beginning and end of each call on
  gs_image_next_planes.  Note that for each plane, the "plane wanted" status
  and size of a full row may change after each call of plane_data.  As
  documented in gxiparam.h, we assume that a call of plane_data can only
  change a plane's status from "wanted" to "not wanted", or change the width
  or depth of a wanted plane, if data for that plane was actually supplied
  (and used).
*/

/* Define the enumeration state for this interface layer. */
/*typedef struct gs_image_enum_s gs_image_enum; *//* in gsimage.h */
struct gs_image_enum_s {
    /* The following are set at initialization time. */
    gs_memory_t *memory;
    gx_device *dev;		/* if 0, just skip over the data */
    gx_image_enum_common_t *info;	/* driver bookkeeping structure */
    int num_planes;
    int height;
    bool wanted_varies;
    /* The following are updated dynamically. */
    int plane_index;		/* index of next plane of data, */
				/* only needed for gs_image_next */
    int y;
    bool error;
    byte wanted[gs_image_max_planes]; /* cache gx_image_planes_wanted */
    byte client_wanted[gs_image_max_planes]; /* see gsimage.h */
    image_enum_plane_t planes[gs_image_max_planes]; /* see above */
    /*
     * To reduce setup for transferring complete rows, we maintain a
     * partially initialized parameter array for gx_image_plane_data_rows.
     * The data member is always set just before calling
     * gx_image_plane_data_rows; the data_x and raster members are reset
     * when needed.
     */
    gx_image_plane_t image_planes[gs_image_max_planes];
};

gs_private_st_composite(st_gs_image_enum, gs_image_enum, "gs_image_enum",
			gs_image_enum_enum_ptrs, gs_image_enum_reloc_ptrs);
#define gs_image_enum_num_ptrs 2

/* GC procedures */
private 
ENUM_PTRS_WITH(gs_image_enum_enum_ptrs, gs_image_enum *eptr)
{
    /* Enumerate the data planes. */
    index -= gs_image_enum_num_ptrs;
    if (index < eptr->num_planes)
	ENUM_RETURN_STRING_PTR(gs_image_enum, planes[index].source);
    index -= eptr->num_planes;
    if (index < eptr->num_planes)
	ENUM_RETURN_STRING_PTR(gs_image_enum, planes[index].row);
    return 0;
}
ENUM_PTR(0, gs_image_enum, dev);
ENUM_PTR(1, gs_image_enum, info);
ENUM_PTRS_END
private RELOC_PTRS_WITH(gs_image_enum_reloc_ptrs, gs_image_enum *eptr)
{
    int i;

    RELOC_PTR(gs_image_enum, dev);
    RELOC_PTR(gs_image_enum, info);
    for (i = 0; i < eptr->num_planes; i++)
	RELOC_CONST_STRING_PTR(gs_image_enum, planes[i].source);
    for (i = 0; i < eptr->num_planes; i++)
	RELOC_STRING_PTR(gs_image_enum, planes[i].row);
}
RELOC_PTRS_END

/* Create an image enumerator given image parameters and a graphics state. */
int
gs_image_begin_typed(const gs_image_common_t * pic, gs_state * pgs,
		     bool uses_color, gx_image_enum_common_t ** ppie)
{
    gx_device *dev = gs_currentdevice(pgs);
    gx_clip_path *pcpath;
    int code = gx_effective_clip_path(pgs, &pcpath);

    if (code < 0)
	return code;
    if (uses_color) {
	gx_set_dev_color(pgs);
        code = gs_state_color_load(pgs);
        if (code < 0)
	    return code;
    }
    return gx_device_begin_typed_image(dev, (const gs_imager_state *)pgs,
		NULL, pic, NULL, pgs->dev_color, pcpath, pgs->memory, ppie);
}

/* Allocate an image enumerator. */
private void
image_enum_init(gs_image_enum * penum)
{
    /* Clean pointers for GC. */
    penum->info = 0;
    penum->dev = 0;
    penum->plane_index = 0;
    penum->num_planes = 0;
}
gs_image_enum *
gs_image_enum_alloc(gs_memory_t * mem, client_name_t cname)
{
    gs_image_enum *penum =
	gs_alloc_struct(mem, gs_image_enum, &st_gs_image_enum, cname);

    if (penum != 0) {
	penum->memory = mem;
	image_enum_init(penum);
    }
    return penum;
}

/* Start processing an ImageType 1 image. */
int
gs_image_init(gs_image_enum * penum, const gs_image_t * pim, bool multi,
	      gs_state * pgs)
{
    gs_image_t image;
    gx_image_enum_common_t *pie;
    int code;

    image = *pim;
    if (image.ImageMask) {
	image.ColorSpace = NULL;
	if (pgs->in_cachedevice <= 1)
	    image.adjust = false;
    } else {
	if (pgs->in_cachedevice)
	    return_error(gs_error_undefined);
	if (image.ColorSpace == NULL) {
            /* parameterless color space - no re-entrancy problems */
            static gs_color_space cs;

            /*
             * Mutiple initialization of a DeviceGray color space is
             * not harmful, as the space has no parameters. Use of a
             * non-current color space is potentially incorrect, but
             * it appears this case doesn't arise.
             */
            gs_cspace_init_DeviceGray(pgs->memory, &cs);
	    image.ColorSpace = &cs;
        }
    }
    code = gs_image_begin_typed((const gs_image_common_t *)&image, pgs,
				image.ImageMask | image.CombineWithColor,
				&pie);
    if (code < 0)
	return code;
    return gs_image_enum_init(penum, pie, (const gs_data_image_t *)&image,
			      pgs);
}

/*
 * Return the number of bytes of data per row for a given plane.
 */
inline uint
gs_image_bytes_per_plane_row(const gs_image_enum * penum, int plane)
{
    const gx_image_enum_common_t *pie = penum->info;

    return (pie->plane_widths[plane] * pie->plane_depths[plane] + 7) >> 3;
}

/* Cache information when initializing, or after transferring plane data. */
private void
cache_planes(gs_image_enum *penum)
{
    int i;

    if (penum->wanted_varies) {
	penum->wanted_varies =
	    !gx_image_planes_wanted(penum->info, penum->wanted);
	for (i = 0; i < penum->num_planes; ++i)
	    if (penum->wanted[i])
		penum->image_planes[i].raster =
		    gs_image_bytes_per_plane_row(penum, i);
	    else
		penum->image_planes[i].data = 0;
    }
}
/* Advance to the next wanted plane. */
private void
next_plane(gs_image_enum *penum)
{
    int px = penum->plane_index;

    do {
	if (++px == penum->num_planes)
	    px = 0;
    } while (!penum->wanted[px]);
    penum->plane_index = px;
}
/*
 * Initialize plane_index and (if appropriate) wanted and
 * wanted_varies at the beginning of a group of planes.
 */
private void
begin_planes(gs_image_enum *penum)
{
    cache_planes(penum);
    penum->plane_index = -1;
    next_plane(penum);
}

static int
gs_image_common_init(gs_image_enum * penum, gx_image_enum_common_t * pie,
	    const gs_data_image_t * pim, gx_device * dev)
{
    /*
     * HACK : For a compatibility with gs_image_cleanup_and_free_enum,
     * penum->memory must be initialized in advance 
     * with the memory heap that owns *penum.
     */
    int i;

    if (pim->Width == 0 || pim->Height == 0) {
	gx_image_end(pie, false);
	return 1;
    }
    image_enum_init(penum);
    penum->dev = dev;
    penum->info = pie;
    penum->num_planes = pie->num_planes;
    /*
     * Note that for ImageType 3 InterleaveType 2, penum->height (the
     * expected number of data rows) differs from pim->Height (the height
     * of the source image in scan lines).  This doesn't normally cause
     * any problems, because penum->height is not used to determine when
     * all the data has been processed: that is up to the plane_data
     * procedure for the specific image type.
     */
    penum->height = pim->Height;
    for (i = 0; i < pie->num_planes; ++i) {
	penum->planes[i].pos = 0;
	penum->planes[i].source.size = 0;	/* for gs_image_next_planes */
	penum->planes[i].row.data = 0; /* for GC */
	penum->planes[i].row.size = 0; /* ditto */
	penum->image_planes[i].data_x = 0; /* just init once, never changes */
    }
    /* Initialize the dynamic part of the state. */
    penum->y = 0;
    penum->error = false;
    penum->wanted_varies = true;
    begin_planes(penum);
    return 0;
}

/* Initialize an enumerator for a general image. 
   penum->memory must be initialized in advance.
*/
int
gs_image_enum_init(gs_image_enum * penum, gx_image_enum_common_t * pie,
		   const gs_data_image_t * pim, gs_state *pgs)
{
    return gs_image_common_init(penum, pie, pim,
				(pgs->in_charpath ? NULL :
				 gs_currentdevice_inline(pgs)));
}

/* Return the set of planes wanted. */
const byte *
gs_image_planes_wanted(gs_image_enum *penum)
{
    int i;

    /*
     * A plane is wanted at this interface if it is wanted by the
     * underlying machinery and has no buffered or retained data.
     */
    for (i = 0; i < penum->num_planes; ++i)
	penum->client_wanted[i] =
	    (penum->wanted[i] &&
	     penum->planes[i].pos + penum->planes[i].source.size <
	       penum->image_planes[i].raster);
    return penum->client_wanted;
}

/*
 * Return the enumerator memory used for allocating the row buffers.
 * Because some PostScript files use save/restore within an image data
 * reading procedure, this must be a stable allocator.
 */
private gs_memory_t *
gs_image_row_memory(const gs_image_enum *penum)
{
    return gs_memory_stable(penum->memory);
}

/* Free the row buffers when cleaning up. */
private void
free_row_buffers(gs_image_enum *penum, int num_planes, client_name_t cname)
{
    int i;

    for (i = num_planes - 1; i >= 0; --i) {
	if_debug3('b', "[b]free plane %d row (0x%lx,%u)\n",
		  i, (ulong)penum->planes[i].row.data,
		  penum->planes[i].row.size);
	gs_free_string(gs_image_row_memory(penum), penum->planes[i].row.data,
		       penum->planes[i].row.size, cname);
	penum->planes[i].row.data = 0;
	penum->planes[i].row.size = 0;
    }
}

/* Process the next piece of an image. */
int
gs_image_next(gs_image_enum * penum, const byte * dbytes, uint dsize,
	      uint * pused)
{
    int px = penum->plane_index;
    int num_planes = penum->num_planes;
    int i, code;
    uint used[gs_image_max_planes];
    gs_const_string plane_data[gs_image_max_planes];

    if (penum->planes[px].source.size != 0)
	return_error(gs_error_rangecheck);
    for (i = 0; i < num_planes; i++)
	plane_data[i].size = 0;
    plane_data[px].data = dbytes;
    plane_data[px].size = dsize;
    penum->error = false;
    code = gs_image_next_planes(penum, plane_data, used);
    *pused = used[px];
    if (code >= 0)
	next_plane(penum);
    return code;
}

int
gs_image_next_planes(gs_image_enum * penum,
		     gs_const_string *plane_data /*[num_planes]*/,
		     uint *used /*[num_planes]*/)
{
    const int num_planes = penum->num_planes;
    int i;
    int code = 0;

#ifdef DEBUG
    if (gs_debug_c('b')) {
	int pi;

	for (pi = 0; pi < num_planes; ++pi)
	    dprintf6("[b]plane %d source=0x%lx,%u pos=%u data=0x%lx,%u\n",
		     pi, (ulong)penum->planes[pi].source.data,
		     penum->planes[pi].source.size, penum->planes[pi].pos,
		     (ulong)plane_data[pi].data, plane_data[pi].size);
    }
#endif
    for (i = 0; i < num_planes; ++i) {
        used[i] = 0;
	if (penum->wanted[i] && plane_data[i].size != 0) {
	    penum->planes[i].source.size = plane_data[i].size;
	    penum->planes[i].source.data = plane_data[i].data;
	}
    }
    for (;;) {
	/* If wanted can vary, only transfer 1 row at a time. */
	int h = (penum->wanted_varies ? 1 : max_int);

	/* Move partial rows from source[] to row[]. */
	for (i = 0; i < num_planes; ++i) {
	    int pos, size;
	    uint raster;

	    if (!penum->wanted[i])
		continue;	/* skip unwanted planes */
	    pos = penum->planes[i].pos;
	    size = penum->planes[i].source.size;
	    raster = penum->image_planes[i].raster;
	    if (size > 0) {
		if (pos < raster && (pos != 0 || size < raster)) {
		    /* Buffer a partial row. */
		    int copy = min(size, raster - pos);
		    uint old_size = penum->planes[i].row.size;

		    /* Make sure the row buffer is fully allocated. */
		    if (raster > old_size) {
			gs_memory_t *mem = gs_image_row_memory(penum);
			byte *old_data = penum->planes[i].row.data;
			byte *row =
			    (old_data == 0 ?
			     gs_alloc_string(mem, raster,
					     "gs_image_next(row)") :
			     gs_resize_string(mem, old_data, old_size, raster,
					      "gs_image_next(row)"));

			if_debug5('b', "[b]plane %d row (0x%lx,%u) => (0x%lx,%u)\n",
				  i, (ulong)old_data, old_size,
				  (ulong)row, raster);
			if (row == 0) {
			    code = gs_note_error(gs_error_VMerror);
			    free_row_buffers(penum, i, "gs_image_next(row)");
			    break;
			}
			penum->planes[i].row.data = row;
			penum->planes[i].row.size = raster;
		    }
		    memcpy(penum->planes[i].row.data + pos,
			   penum->planes[i].source.data, copy);
		    penum->planes[i].source.data += copy;
		    penum->planes[i].source.size = size -= copy;
		    penum->planes[i].pos = pos += copy;
		    used[i] += copy;
		}
	    }
	    if (h == 0)
		continue;	/* can't transfer any data this cycle */
	    if (pos == raster) {
		/*
		 * This plane will be transferred from the row buffer,
		 * so we can only transfer one row.
		 */
		h = min(h, 1);
		penum->image_planes[i].data = penum->planes[i].row.data;
	    } else if (pos == 0 && size >= raster) {
		/* We can transfer 1 or more planes from the source. */
		h = min(h, size / raster);
		penum->image_planes[i].data = penum->planes[i].source.data;
	    } else
		h = 0;		/* not enough data in this plane */
	}
	if (h == 0 || code != 0)
	    break;
	/* Pass rows to the device. */
	if (penum->dev == 0) {
	    /*
	     * ****** NOTE: THE FOLLOWING IS NOT CORRECT FOR ImageType 3
	     * ****** InterleaveType 2, SINCE MASK HEIGHT AND IMAGE HEIGHT
	     * ****** MAY DIFFER (BY AN INTEGER FACTOR).  ALSO, plane_depths[0]
	     * ****** AND plane_widths[0] ARE NOT UPDATED.
	 */
	    if (penum->y + h < penum->height)
		code = 0;
	    else
		h = penum->height - penum->y, code = 1;
	} else {
	    code = gx_image_plane_data_rows(penum->info, penum->image_planes,
					    h, &h);
	    if_debug2('b', "[b]used %d, code=%d\n", h, code);
	    penum->error = code < 0;
	}
	penum->y += h;
	/* Update positions and sizes. */
	if (h == 0)
	    break;
	for (i = 0; i < num_planes; ++i) {
	    int count;

	    if (!penum->wanted[i])
		continue;
	    count = penum->image_planes[i].raster * h;
	    if (penum->planes[i].pos) {
		/* We transferred the row from the row buffer. */
		penum->planes[i].pos = 0;
	    } else {
		/* We transferred the row(s) from the source. */
		penum->planes[i].source.data += count;
		penum->planes[i].source.size -= count;
		used[i] += count;
	    }
	}
	cache_planes(penum);
	if (code > 0)
	    break;
    }
    /* Return the retained data pointers. */
    for (i = 0; i < num_planes; ++i)
	plane_data[i] = penum->planes[i].source;
    return code;
}

/* Clean up after processing an image. */
int
gs_image_cleanup(gs_image_enum * penum)
{
    int code = 0;

    free_row_buffers(penum, penum->num_planes, "gs_image_cleanup(row)");
    if (penum->info != 0)
        code = gx_image_end(penum->info, !penum->error);
    /* Don't free the local enumerator -- the client does that. */
    return code;
}

/* Clean up after processing an image and free the enumerator. */
int
gs_image_cleanup_and_free_enum(gs_image_enum * penum)
{
    int code = gs_image_cleanup(penum);

    gs_free_object(penum->memory, penum, "gs_image_cleanup_and_free_enum");
    return code;
}
