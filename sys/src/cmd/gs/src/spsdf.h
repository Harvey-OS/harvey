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

/*$Id: spsdf.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Common output syntax and parameters for PostScript and PDF writers */

#ifndef spsdf_INCLUDED
#  define spsdf_INCLUDED

#include "gsparam.h"

/* Define an opaque type for streams. */
#ifndef stream_DEFINED
#  define stream_DEFINED
typedef struct stream_s stream;
#endif

/* ---------------- Symbolic data printing ---------------- */

/* Print a PostScript string in the most efficient form. */
#define PRINT_BINARY_OK 1
#define PRINT_ASCII85_OK 2
#define PRINT_HEX_NOT_OK 4
void s_write_ps_string(P4(stream * s, const byte * str, uint size,
			  int print_ok));

/*
 * Create a stream that just keeps track of how much has been written
 * to it.  We use this for measuring data that will be stored rather
 * than written to an actual stream.
 */
int s_alloc_position_stream(P2(stream ** ps, gs_memory_t * mem));

/*
 * Create/release a parameter list for printing (non-default) filter
 * parameters.  This should probably migrate to a lower level....
 */
typedef struct param_printer_params_s {
    const char *prefix;		/* before entire object, if any params */
    const char *suffix;		/* after entire object, if any params */
    const char *item_prefix;	/* before each param */
    const char *item_suffix;	/* after each param */
    int print_ok;
} param_printer_params_t;
/*
 * The implementation structure should be opaque, but there are a few
 * clients that need to be able to stack-allocate it.
 */
typedef struct printer_param_list_s {
    gs_param_list_common;
    stream *strm;
    param_printer_params_t params;
    bool any;
} printer_param_list_t;
#define private_st_printer_param_list()	/* in spsdf.c */\
  gs_private_st_ptrs1(st_printer_param_list, printer_param_list_t,\
    "printer_param_list_t", printer_plist_enum_ptrs, printer_plist_reloc_ptrs,\
    strm)

#define param_printer_params_default_values 0, 0, 0, "\n", 0
extern const param_printer_params_t param_printer_params_default;
int s_alloc_param_printer(P4(gs_param_list ** pplist,
			     const param_printer_params_t * ppp, stream * s,
			     gs_memory_t * mem));
void s_free_param_printer(P1(gs_param_list * plist));
/* Initialize or release a list without allocating or freeing it. */
int s_init_param_printer(P3(printer_param_list_t *prlist,
			    const param_printer_params_t * ppp, stream * s));
void s_release_param_printer(P1(printer_param_list_t *prlist));


#endif /* spsdf_INCLUDED */
