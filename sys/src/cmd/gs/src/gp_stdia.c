/* Copyright (C) 2001 artofcode LLC.  All rights reserved.
  
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

/*$Id: gp_stdia.c,v 1.3 2001/10/12 21:37:08 ghostgum Exp $ */
/* Read stdin on platforms that support unbuffered read. */
/* We want unbuffered for console input and pipes. */

#include "stdio_.h"
#include "time_.h"
#include "unistd_.h"
#include "gx.h"
#include "gp.h"


/* Read bytes from stdin, unbuffered if possible. */
int gp_stdin_read(char *buf, int len, int interactive, FILE *f)
{
    return read(fileno(f), buf, len);
}

