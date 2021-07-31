/* Copyright (C) 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gdevpm.h */
/* Defines common to gdevpm.c, gspmdrv.c and PM GSview */

#define SHARED_NAME "\\SHAREMEM\\%s"
#define SYNC_NAME   "\\SEM32\\SYNC_%s"
#define NEXT_NAME   "\\SEM32\\NEXT_%s"
#define MUTEX_NAME  "\\SEM32\\MUTEX_%s"
#define QUEUE_NAME  "\\QUEUES\\%s"

#define GS_UPDATING	1
#define GS_SYNC		2
#define GS_PAGE		3
#define GS_CLOSE	4
#define GS_ERROR	5
#define GS_PALCHANGE	6
#define GS_BEGIN	7
#define GS_END		8
