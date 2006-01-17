/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdev8bcm.c,v 1.4 2002/02/21 22:24:51 giles Exp $ */
/* Dynamic color mapping for 8-bit displays */
#include "gx.h"
#include "gxdevice.h"
#include "gdev8bcm.h"

/* Initialize an 8-bit color map. */
void
gx_8bit_map_init(gx_8bit_color_map * pcm, int max_count)
{
    int i;

    pcm->count = 0;
    pcm->max_count = max_count;
    for (i = 0; i < gx_8bit_map_size; i++)
	pcm->map[i].rgb = gx_8bit_no_rgb;
}

/* Look up a color in an 8-bit color map. */
/* Return <0 if not found. */
int
gx_8bit_map_rgb_color(const gx_8bit_color_map * pcm, gx_color_value r,
		      gx_color_value g, gx_color_value b)
{
    ushort rgb = gx_8bit_rgb_key(r, g, b);
    const gx_8bit_map_entry *pme =
    &pcm->map[(rgb * gx_8bit_map_spreader) % gx_8bit_map_size];

    for (;; pme++) {
	if (pme->rgb == rgb)
	    return pme->index;
	else if (pme->rgb == gx_8bit_no_rgb)
	    break;
    }
    if (pme != &pcm->map[gx_8bit_map_size])
	return pme - &pcm->map[gx_8bit_map_size];
    /* We ran off the end; wrap around and continue. */
    pme = &pcm->map[0];
    for (;; pme++) {
	if (pme->rgb == rgb)
	    return pme->index;
	else if (pme->rgb == gx_8bit_no_rgb)
	    return pme - &pcm->map[gx_8bit_map_size];
    }
}

/* Add a color to an 8-bit color map after an unsuccessful lookup, */
/* and return its index.  Return <0 if the map is full. */
int
gx_8bit_add_rgb_color(gx_8bit_color_map * pcm, gx_color_value r,
		      gx_color_value g, gx_color_value b)
{
    int index;
    gx_8bit_map_entry *pme;

    if (gx_8bit_map_is_full(pcm))
	return -1;
    index = gx_8bit_map_rgb_color(pcm, r, g, b);
    if (index >= 0)		/* shouldn't happen */
	return index;
    pme = &pcm->map[-index];
    pme->rgb = gx_8bit_rgb_key(r, g, b);
    return (pme->index = pcm->count++);
}
