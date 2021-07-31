/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gscdevn.h,v 1.3 2000/09/19 19:00:26 lpd Exp $ */
/* Client interface to DeviceN color */

#ifndef gscdevn_INCLUDED
#  define gscdevn_INCLUDED

#include "gscspace.h"

/*
 * Allocate and fill in a DeviceN color space.
 * Note that the client is responsible for memory management of the
 * name array and (if used) the tint transform Function.
 */
int gs_cspace_build_DeviceN(P5(
			       gs_color_space **ppcspace,
			       gs_separation_name *psnames,
			       uint num_components,
			       const gs_color_space *palt_cspace,
			       gs_memory_t *pmem
			       ));

/* Set the tint transformation procedure for a DeviceN color space. */
/* VMS limits procedure names to 31 characters, and some systems only */
/* compare the first 23 characters. */
extern int gs_cspace_set_devn_proc(P3(
				      gs_color_space * pcspace,
			int (*proc)(P5(const gs_device_n_params *,
				       const float *,
				       float *,
				       const gs_imager_state *,
				       void *
				       )),
				      void *proc_data
				      ));

/* Set the DeviceN tint transformation procedure to a Function. */
#ifndef gs_function_DEFINED
typedef struct gs_function_s gs_function_t;
#  define gs_function_DEFINED
#endif
int gs_cspace_set_devn_function(P2(gs_color_space *pcspace,
				   gs_function_t *pfn));

/*
 * If the DeviceN tint transformation procedure is a Function,
 * return the function object, otherwise return 0.
 */
gs_function_t *gs_cspace_get_devn_function(P1(const gs_color_space *pcspace));

#endif /* gscdevn_INCLUDED */
