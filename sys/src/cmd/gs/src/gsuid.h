/* Copyright (C) 1992, 1993, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsuid.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
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
bool uid_equal(P2(const gs_uid *, const gs_uid *));	/* in gsutil.c */

/* Free the XUID array of a uid if necessary. */
#define uid_free(puid, mem, cname)\
  gs_free_object(mem, (puid)->xvalues, cname)

#endif /* gsuid_INCLUDED */
