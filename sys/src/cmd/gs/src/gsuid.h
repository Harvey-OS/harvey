/* Copyright (C) 1992, 1993, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsuid.h,v 1.6 2002/06/19 01:06:12 lpd Exp $ */
/* Unique id definitions for Ghostscript */

#ifndef gsuid_INCLUDED
#  define gsuid_INCLUDED

/* A unique id (uid) may be either a UniqueID or an XUID. */
/* (XUIDs are a Level 2 feature.) */
#ifndef gs_uid_DEFINED
#  define gs_uid_DEFINED
typedef struct gs_uid_s gs_uid;
#endif
struct gs_uid_s {
    /* id >= 0 is a UniqueID, xvalues is 0. */
    /* id < 0 is an XUID, size of xvalues is -id. */
    long id;
    long *xvalues;
};

/*
 * A UniqueID of no_UniqueID is an indication that there is no uid.
 * Since we sometimes use gs_ids as UniqueIDs, we want to choose as large
 * a (positive) value as possible for no_UniqueID.
 */
#define no_UniqueID max_long
#define uid_is_valid(puid)\
  ((puid)->id != no_UniqueID)
#define uid_set_invalid(puid)\
  ((puid)->id = no_UniqueID, (puid)->xvalues = 0)
#define uid_is_UniqueID(puid)\
  (((puid)->id & ~0xffffff) == 0)
#define uid_is_XUID(puid)\
  ((puid)->id < 0)

/* Initialize a uid. */
#define uid_set_UniqueID(puid, idv)\
  ((puid)->id = idv, (puid)->xvalues = 0)
#define uid_set_XUID(puid, pvalues, siz)\
  ((puid)->id = -(long)(siz), (puid)->xvalues = pvalues)

/* Get the size and the data of an XUID. */
#define uid_XUID_size(puid) ((uint)(-(puid)->id))
#define uid_XUID_values(puid) ((puid)->xvalues)

/* Compare two uids for equality. */
/* This could be a macro, but the Zortech compiler compiles it wrong. */
bool uid_equal(const gs_uid *, const gs_uid *);	/* in gsutil.c */

/* Copy the XUID data for a uid, if needed, updating the uid in place. */
int uid_copy(gs_uid *puid, gs_memory_t *mem, client_name_t cname);

/* Free the XUID array of a uid if necessary. */
#define uid_free(puid, mem, cname)\
  gs_free_object(mem, (puid)->xvalues, cname)

#endif /* gsuid_INCLUDED */
