/* Copyright (C) 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpm.h,v 1.4 2002/02/21 22:24:51 giles Exp $ */
/* Defines common to gdevpm.c, gspmdrv.c and PM GSview */

#ifndef gdevpm_INCLUDED
#  define gdevpm_INCLUDED

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

#endif /* gdevpm_INCLUDED */
