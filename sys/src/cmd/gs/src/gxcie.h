/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxcie.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Internal definitions for CIE color implementation */
/* Requires gxcspace.h */

#ifndef gxcie_INCLUDED
#  define gxcie_INCLUDED

#include "gscie.h"

/*
 * These color space implementation procedures are defined in gscie.c or
 * gsciemap.c, and referenced from the color space structures in gscscie.c.
 */
/*
 * We use CIExxx rather than CIEBasedxxx in some places because
 * gcc under VMS only retains 23 characters of procedure names,
 * and DEC C truncates all identifiers at 31 characters.
 */

/* Defined in gscie.c */

cs_proc_init_color(gx_init_CIE);
cs_proc_restrict_color(gx_restrict_CIEDEFG);
cs_proc_install_cspace(gx_install_CIEDEFG);
cs_proc_restrict_color(gx_restrict_CIEDEF);
cs_proc_install_cspace(gx_install_CIEDEF);
cs_proc_restrict_color(gx_restrict_CIEABC);
cs_proc_install_cspace(gx_install_CIEABC);
cs_proc_restrict_color(gx_restrict_CIEA);
cs_proc_install_cspace(gx_install_CIEA);

/* Defined in gsciemap.c */

cs_proc_concretize_color(gx_concretize_CIEDEFG);
cs_proc_concretize_color(gx_concretize_CIEDEF);
cs_proc_concretize_color(gx_concretize_CIEABC);
cs_proc_remap_color(gx_remap_CIEABC);
cs_proc_concretize_color(gx_concretize_CIEA);

#endif /* gxcie_INCLUDED */
