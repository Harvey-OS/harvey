/* Copyright (C) 1996, 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: zfcid.c,v 1.10.2.1 2000/11/09 23:04:44 rayjj Exp $ */
/* CID-keyed font utilities */
#include "ghost.h"
#include "oper.h"
#include "gsmatrix.h"
#include "gxfcid.h"
#include "bfont.h"
#include "icid.h"
#include "idict.h"
#include "idparam.h"
#include "ifcid.h"
#include "store.h"

/* Get the CIDSystemInfo of a CIDFont. */
int
cid_font_system_info_param(gs_cid_system_info_t *pcidsi, const ref *prfont)
{
    ref *prcidsi;

    if (dict_find_string(prfont, "CIDSystemInfo", &prcidsi) <= 0)
	return_error(e_rangecheck);
    return cid_system_info_param(pcidsi, prcidsi);
}

/* Get the additional information for a CIDFontType 0 or 2 CIDFont. */
int
cid_font_data_param(os_ptr op, gs_font_cid_data *pdata, ref *pGlyphDirectory)
{
    int code;
    ref *pgdir;

    check_type(*op, t_dictionary);
    if ((code = cid_font_system_info_param(&pdata->CIDSystemInfo, op)) < 0 ||
	(code = dict_int_param(op, "CIDCount", 0, max_int, -1,
			       &pdata->CIDCount)) < 0
	)
	return code;
    /*
     * If the font doesn't have a GlyphDirectory, GDBytes is required.
     * If it does have a GlyphDirectory, GDBytes may still be needed for
     * CIDMap: it's up to the client to check this.
     */
    if (dict_find_string(op, "GlyphDirectory", &pgdir) <= 0) {
	/* Standard CIDFont, require GDBytes. */
	make_null(pGlyphDirectory);
	return dict_int_param(op, "GDBytes", 1, MAX_GDBytes, 0,
			      &pdata->GDBytes);
    }
    if (r_has_type(pgdir, t_dictionary) || r_is_array(pgdir)) {
	/* GlyphDirectory, GDBytes is optional. */
	*pGlyphDirectory = *pgdir;
	code = dict_int_param(op, "GDBytes", 1, MAX_GDBytes, 1,
			      &pdata->GDBytes);
	if (code == 1) {
	    pdata->GDBytes = 0;
	    code = 0;
	}
	return code;
    } else {
	return_error(e_typecheck);
    }
}
