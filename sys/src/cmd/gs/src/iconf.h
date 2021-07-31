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

/*$Id: iconf.h,v 1.1.2.1 2000/11/09 22:39:35 rayjj Exp $ */
/* Collected imports for interpreter configuration and initialization */

#ifndef iconf_INCLUDED
#define iconf_INCLUDED
/* ------ Imported data ------ */

/* Configuration information imported from iccinit[01].c */
extern const byte gs_init_string[];
extern const uint gs_init_string_sizeof;

/* Configuration information imported from iconf.c */
extern const ref gs_init_file_array[];
extern const ref gs_emulator_name_array[];

#endif /* iconf_INCLUDED */
