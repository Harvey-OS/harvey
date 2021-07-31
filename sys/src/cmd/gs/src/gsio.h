/* Copyright (C) 1989, 1990, 1993, 1996, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gsio.h,v 1.4 2000/09/19 19:00:29 lpd Exp $ */
/* stdio redirection */

#ifndef gsio_INCLUDED
#  define gsio_INCLUDED

/*
 * Define substitutes for stdin/out/err.  Eventually these will always be
 * referenced through an instance structure.
 */
extern FILE *gs_stdio[3];
#define gs_stdin (gs_stdio[0])
#define gs_stdout (gs_stdio[1])
#define gs_stderr (gs_stdio[2])

/*
 * The library and interpreter must never use stdin/out/err directly.
 * Make references to them illegal.
 */
#undef stdin
#define stdin stdin_not_available
#undef stdout
#define stdout stdout_not_available
#undef stderr
#define stderr stderr_not_available
/* However, for the moment, lprintf must be able to reference stderr. */
#undef dstderr
#define dstderr gs_stderr
#undef estderr
#define estderr gs_stderr

/*
 * Redefine all the relevant stdio functions to reference stdin/out/err
 * explicitly, or to be illegal.
 */
#undef fgetchar
#define fgetchar() fgetc(stdin)
#undef fputchar
#define fputchar(c) fputc(c, stdout)
#undef getchar
#define getchar() getc(stdin)
#undef gets
#define gets Function._gets_.unavailable
/* We should do something about perror, but since many Unix systems */
/* don't provide the strerror function, we can't.  (No Aladdin-maintained */
/* code uses perror.) */
#undef printf
#define printf Function._printf_.unavailable
#undef putchar
#define putchar(c) fputc(c, stdout)
#undef puts
#define puts(s) (fputs(s, stdout), putchar('\n'))
#undef scanf
#define scanf Function._scanf_.unavailable
#undef vprintf
#define vprintf Function._vprintf_.unavailable
#undef vscanf
#define vscanf Function._vscanf_.unavailable

#endif /* gsio_INCLUDED */
