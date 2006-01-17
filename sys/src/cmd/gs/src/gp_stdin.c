/* Copyright (C) 2001 artofcode LLC.  All rights reserved.
  
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

/* $Id: gp_stdin.c,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* Read stdin on platforms that do not support unbuffered read.
 * This is the most portable implementation, but it is very slow
 * when reading stdin because it will read one byte at a time.
 * Platforms that support unbuffered read should use gp_stdia.c
 * or provide their own implementation
 */

#include "stdio_.h"
#include "gx.h"
#include "gp.h"

/* Read bytes from stdin, using unbuffered if possible.
 * This implementation doesn't do unbuffered, so if 
 * interactive read one byte at a time.
 */
int gp_stdin_read(char *buf, int len, int interactive, FILE *f)
{
    return fread(buf, 1, interactive ? 1 : len, f);
}

