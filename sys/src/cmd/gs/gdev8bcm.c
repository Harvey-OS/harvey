/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gdev8bcm.c */
/* Dynamic color mapping for 8-bit displays */
#include "gx.h"
#include "gxdevice.h"
#include "gdev8bcm.h"

/* Initialize an 8-bit color map. */
void
gx_8bit_map_init(gx_8bit_color_map *pcm, int max_count)
{	int i;
	pcm->count = 0;
	pcm->max_count = max_count;
	for ( i = 0; i < gx_8bit_map_size; i++ )
	  pcm->map[i].rgb = gx_8bit_no_rgb;
}

/* Look up a color in an 8-bit color map. */
/* Return <0 if not found. */
int
gx_8bit_map_rgb_color(const gx_8bit_color_map *pcm, gx_color_value r,
  gx_color_value g, gx_color_value b)
{	ushort rgb = gx_8bit_rgb_key(r, g, b);
	const gx_8bit_map_entry *pme =
	  &pcm->map[(rgb * gx_8bit_map_spreader) % gx_8bit_map_size];
	for ( ; ; pme++ )
	  {	if ( pme->rgb == rgb )
		  return pme->index;
		else if ( pme->rgb == gx_8bit_no_rgb )
		  break;
	  }
	if ( pme != &pcm->map[gx_8bit_map_size] )
	  return pme - &pcm->map[gx_8bit_map_size];
	/* We ran off the end; wrap around and continue. */
	pme = &pcm->map[0];
	for ( ; ; pme++ )
	  {	if ( pme->rgb == rgb )
		  return pme->index;
		else if ( pme->rgb == gx_8bit_no_rgb )
		  return pme - &pcm->map[gx_8bit_map_size];
	  }
}

/* Add a color to an 8-bit color map after an unsuccessful lookup, */
/* and return its index.  Return <0 if the map is full. */
int
gx_8bit_add_rgb_color(gx_8bit_color_map *pcm, gx_color_value r,
  gx_color_value g, gx_color_value b)
{	int index;
	gx_8bit_map_entry *pme;
	if ( gx_8bit_map_is_full(pcm) )
	  return -1;
	index = gx_8bit_map_rgb_color(pcm, r, g, b);
	if ( index >= 0 )			/* shouldn't happen */
	  return index;
	pme = &pcm->map[-index];
	pme->rgb = gx_8bit_rgb_key(r, g, b);
	return (pme->index = pcm->count++);
}
