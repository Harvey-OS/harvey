/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* sjpegerr.c */
/* IJG error message table for Ghostscript. */
#include "stdio_.h"
#include "jpeglib.h"
#include "jversion.h"

/*
 * This file exists solely to hold the rather large IJG error message string
 * table (now about 4K, and likely to grow in future releases).  The table
 * is large enough that we don't want it to be in the default data segment
 * in a 16-bit executable.  Under Borland C, this is accomplished simply by
 * compiling this one file in "huge" memory model rather than "large".
 * The string constants will then go into a private far data segment.
 * In less brain-damaged architectures, this file is simply compiled normally,
 * and we pay only the price of one extra function call.
 */

#include "jerror.h"		/* get error codes */
#define JMAKE_MSG_TABLE
#include "jerror.h"		/* create message string table */

/* Return a pointer to the message table.
 * It is unsafe to do much more than this within the "huge" environment.
 */

const char * const *
gs_jpeg_message_table (void)
{	return jpeg_message_table;
	/* NOTE: in IJG release 5 the message table is named
	 * "jpeg_message_table" by jerror.h.  In subsequent releases it will
	 * be named "jpeg_std_message_table".  Update the above line of code
	 * when upgrading to a new IJG release.
	 */
}
