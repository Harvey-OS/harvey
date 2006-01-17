/* Copyright (C) 2005 artofcode LLC.  All rights reserved.

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

/* $Id: gsiorom.c,v 1.2 2005/10/04 06:30:02 ray Exp $ */
/* %rom% IODevice implementation for a compressed in-memory filesystem */
 
/*
 * This file implements a special %rom% IODevice designed for embedded
 * use. It accesses a compressed filesytem image which may be stored
 * in literal ROM, or more commonly is just static data linked directly
 * into the executable. This can be used for storing postscript library
 * files, fonts, Resources or other data files that Ghostscript needs
 * to run.
 */

#include "std.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxiodev.h"
#include "stream.h"

/* device method prototypes */
private iodev_proc_init(iodev_rom_init);
private iodev_proc_open_file(iodev_rom_open_file);
/* close is handled by stream closure */

/* device definition */
const gx_io_device gs_iodev_rom =
{
    "%rom%", "FileSystem",
    {iodev_rom_init, iodev_no_open_device,
     iodev_rom_open_file,
     iodev_no_fopen, iodev_no_fclose,
     iodev_no_delete_file, iodev_no_rename_file,
     iodev_no_file_status,
     iodev_no_enumerate_files, NULL, NULL,
     iodev_no_get_params, iodev_no_put_params
    }
};

/* internal state for our device */
typedef struct romfs_state_s {
    char *image;
} romfs_state;

gs_private_st_simple(st_romfs_state, struct romfs_state_s, "romfs_state");
/* we don't need to track the image ptr in the descriptors because
   this is by definition static data */

private int
iodev_rom_init(gx_io_device *iodev, gs_memory_t *mem)
{
    romfs_state *state = gs_alloc_struct(mem, romfs_state, 
                                              &st_romfs_state, 
                                              "iodev_rom_init(state)");
    if (!state)
	return gs_error_VMerror;

    state->image = NULL;
    return 0;
}

private int
iodev_rom_open_file(gx_io_device *iodev, const char *fname, uint namelen,
    const char *access, stream **ps, gs_memory_t *mem)
{
    const char* dummy = "this came from the compressed romfs.";
    byte *buf;

    /* return an empty stream on error */
    *ps = NULL;

    /* fake by returning the contents of a string */
    buf = gs_alloc_string(mem, strlen(dummy), "romfs buffer");
    if (buf == NULL) {
	if_debug0('s', "%rom%: could not allocate buffer\n");
	return_error(gs_error_VMerror);
    }
    memcpy(buf, dummy, strlen(dummy));
    *ps = s_alloc(mem, "romfs");
    sread_string(*ps, buf, strlen(dummy));

    /* return success */
    return 0;
}
