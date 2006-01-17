/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gspcolor.h,v 1.6 2003/03/25 21:18:06 igor Exp $ */
/* Client interface to Pattern color */

#ifndef gspcolor_INCLUDED
#  define gspcolor_INCLUDED

#include "gsccolor.h"
#include "gsrefct.h"
#include "gsuid.h"

/* ---------------- Types and structures ---------------- */

/*
 * We originally defined the gs_client_pattern structure before we
 * realized that we would have to accommodate multiple PatternTypes.
 * In version 5.68, we bit the bullet and made an incompatible change
 * to this structure so that multiple PatternTypes could be supported.
 * In order to make this work:
 *
 *	Clients creating instances of any Pattern template structure
 *	(gs_patternN_template_t) must call gs_patternN_init to
 *	initialize all the members, before filling in any of the
 *	members themselves.
 *
 * This is a non-backward-compatible requirement relative to previous
 * versions, but it was unavoidable.
 */

/*
 * Define the abstract pattern template (called "prototype pattern" in Red
 * Book).
 */

#ifndef gs_pattern_type_DEFINED
#  define gs_pattern_type_DEFINED
typedef struct gs_pattern_type_s gs_pattern_type_t;
#endif

#define gs_pattern_template_common\
  const gs_pattern_type_t *type;\
  int PatternType;		/* copied from the type structure */\
  gs_uid uid;\
  void *client_data		/* additional data for rendering */

typedef struct gs_pattern_template_s {
    gs_pattern_template_common;
} gs_pattern_template_t;

/* The descriptor is public for subclassing. */
extern_st(st_pattern_template);
#define public_st_pattern_template() /* in gspcolor.c */\
  gs_public_st_ptrs2(st_pattern_template, gs_pattern_template_t,\
    "gs_pattern_template_t", pattern_template_enum_ptrs,\
    pattern_template_reloc_ptrs, uid.xvalues, client_data)
#define st_pattern_template_max_ptrs 2

/* Definition of Pattern instances. */
#ifndef gs_pattern_instance_DEFINED
#  define gs_pattern_instance_DEFINED
typedef struct gs_pattern_instance_s gs_pattern_instance_t;
#endif

#define gs_pattern_instance_common\
    rc_header rc;\
    /* Following are set by makepattern */\
    const gs_pattern_type_t *type;  /* from template */\
    gs_state *saved;\
    gs_id pattern_id
struct gs_pattern_instance_s {
    gs_pattern_instance_common;
};

/* The following is public for subclassing. */
extern_st(st_pattern_instance);
#define public_st_pattern_instance() /* in gspcolor.c */\
  gs_public_st_ptrs1(st_pattern_instance, gs_pattern_instance_t,\
    "gs_pattern_instance_t", pattern_instance_enum_ptrs,\
    pattern_instance_reloc_ptrs, saved)
#define st_pattern_instance_max_ptrs 1

/* ---------------- Procedures ---------------- */

/* Set a Pattern color or a Pattern color space. */
int gs_setpattern(gs_state *, const gs_client_color *);
int gs_setpatternspace(gs_state *);

/*
 * Construct a Pattern color of any PatternType.
 * The gs_memory_t argument for gs_make_pattern may be NULL, meaning use the
 * same allocator as for the gs_state argument.  Note that gs_make_pattern
 * uses rc_alloc_struct_1 to allocate pattern instances.
 */
int gs_make_pattern(gs_client_color *, const gs_pattern_template_t *,
		    const gs_matrix *, gs_state *, gs_memory_t *);
const gs_pattern_template_t *gs_get_pattern(const gs_client_color *);

/*
 * Adjust the reference count of a pattern. This is intended to support
 * applications (such as PCL) which maintain client colors outside of the
 * graphic state. Since the pattern instance structure is opaque to these
 * applications, they need some way to release or retain the instances as
 * needed.
 */
void gs_pattern_reference(gs_client_color * pcc, int delta);

#endif /* gspcolor_INCLUDED */
