/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: pipe_.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Declaration of popen and pclose */

#ifndef pipe__INCLUDED
#  define pipe__INCLUDED

#include "stdio_.h"

#ifdef __WIN32__
/*
 * MS Windows has popen and pclose in stdio.h, but under different names.
 */
#  define popen(cmd, mode) _popen(cmd, mode)
#  define pclose(file) _pclose(file)
#else  /* !__WIN32__ */
/*
 * popen isn't POSIX-standard, so we declare it here.
 * Because of inconsistent (and sometimes incorrect) header files,
 * we must omit the argument list.  Unfortunately, this sometimes causes
 * more trouble than it cures.
 */
extern FILE *popen( /* P2(const char *, const char *) */ );
extern int pclose(P1(FILE *));
#endif /* !__WIN32__ */

#endif /* pipe__INCLUDED */
