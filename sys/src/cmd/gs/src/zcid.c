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

/*$Id: zcid.c,v 1.3 2000/09/19 19:00:52 lpd Exp $ */
/* CMap and CID-keyed font services */
#include "ghost.h"
#include "errors.h"
#include "gxcid.h"
#include "icid.h"		/* for checking prototype */
#include "idict.h"
#include "idparam.h"
#include "oper.h"

/* Get the information from a CIDSystemInfo dictionary. */
int
cid_system_info_param(gs_cid_system_info_t *pcidsi, const ref *prcidsi)
{
    ref *pregistry;
    ref *pordering;
    int code;

    if (!r_has_type(prcidsi, t_dictionary))
	return_error(e_typecheck);
    if (dict_find_string(prcidsi, "Registry", &pregistry) <= 0 ||
	dict_find_string(prcidsi, "Ordering", &pordering) <= 0
	)
	return_error(e_rangecheck);
    check_read_type_only(*pregistry, t_string);
    check_read_type_only(*pordering, t_string);
    pcidsi->Registry.data = pregistry->value.const_bytes;
    pcidsi->Registry.size = r_size(pregistry);
    pcidsi->Ordering.data = pordering->value.const_bytes;
    pcidsi->Ordering.size = r_size(pordering);
    code = dict_int_param(prcidsi, "Supplement", 0, max_int, -1,
			  &pcidsi->Supplement);
    return (code < 0 ? code : 0);
}
