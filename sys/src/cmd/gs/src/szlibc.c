/* Copyright (C) 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: szlibc.c,v 1.6 2004/08/04 19:36:13 stefan Exp $ */
/* Code common to zlib encoding and decoding streams */
#include "std.h"
#include "gserror.h"
#include "gserrors.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gsmalloc.h"
#include "gsstruct.h"
#include "strimpl.h"
#include "szlibxx.h"
#include "zconf.h"

private_st_zlib_block();
private_st_zlib_dynamic_state();
public_st_zlib_state();

/* Set defaults for stream parameters. */
void
s_zlib_set_defaults(stream_state * st)
{
    stream_zlib_state *const ss = (stream_zlib_state *)st;

    ss->windowBits = MAX_WBITS;
    ss->no_wrapper = false;
    ss->level = Z_DEFAULT_COMPRESSION;
    ss->method = Z_DEFLATED;
    /* DEF_MEM_LEVEL should be in zlib.h or zconf.h, but it isn't. */
    ss->memLevel = min(MAX_MEM_LEVEL, 8);
    ss->strategy = Z_DEFAULT_STRATEGY;
    /* Clear pointers */
    ss->dynamic = 0;
}

/* Allocate the dynamic state. */
int
s_zlib_alloc_dynamic_state(stream_zlib_state *ss)
{
    gs_memory_t *mem = ss->memory; 
    zlib_dynamic_state_t *zds =
	gs_alloc_struct_immovable(mem, zlib_dynamic_state_t,
				  &st_zlib_dynamic_state,
				  "s_zlib_alloc_dynamic_state");

    ss->dynamic = zds;
    if (zds == 0)
	return_error(gs_error_VMerror);
    zds->blocks = 0;
    zds->memory = mem;
    zds->zstate.zalloc = (alloc_func)s_zlib_alloc;
    zds->zstate.zfree = (free_func)s_zlib_free;
    zds->zstate.opaque = (voidpf)zds;
    return 0;
}

/* Free the dynamic state. */
void
s_zlib_free_dynamic_state(stream_zlib_state *ss)
{
    if (ss->dynamic)
	gs_free_object(ss->dynamic->memory, ss->dynamic,
		       "s_zlib_free_dynamic_state");
}

/* Provide zlib-compatible allocation and freeing functions. */
void *
s_zlib_alloc(void *zmem, uint items, uint size)
{
    zlib_dynamic_state_t *const zds = zmem;
    gs_memory_t *mem = zds->memory->stable_memory;
    zlib_block_t *block =
	gs_alloc_struct(mem, zlib_block_t, &st_zlib_block,
			"s_zlib_alloc(block)");
    void *data =
	gs_alloc_byte_array_immovable(mem, items, size, "s_zlib_alloc(data)");

    if (block == 0 || data == 0) {
	gs_free_object(mem, data, "s_zlib_alloc(data)");
	gs_free_object(mem, block, "s_zlib_alloc(block)");
	return Z_NULL;
    }
    block->data = data;
    block->next = zds->blocks;
    block->prev = 0;
    if (zds->blocks)
	zds->blocks->prev = block;
    zds->blocks = block;
    return data;
}
void
s_zlib_free(void *zmem, void *data)
{
    zlib_dynamic_state_t *const zds = zmem;
    gs_memory_t *mem = zds->memory->stable_memory;
    zlib_block_t *block = zds->blocks;

    gs_free_object(mem, data, "s_zlib_free(data)");
    for (; ; block = block->next) {
	if (block == 0) {
	    lprintf1("Freeing unrecorded data 0x%lx!\n", (ulong)data);
	    return;
	}
	if (block->data == data)
	    break;
    }
    if (block->next)
	block->next->prev = block->prev;
    if (block->prev)
	block->prev->next = block->next;
    else
	zds->blocks = block->next;
    gs_free_object(mem, block, "s_zlib_free(block)");
}
