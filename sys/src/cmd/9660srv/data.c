/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

int8_t	Enonexist[] =	"file does not exist";
int8_t	Eperm[] =	"permission denied";
int8_t	Enofile[] =	"no file system specified";
int8_t	Eauth[] =	"authentication failed";

int8_t	*srvname = "9660";
int8_t	*deffile = 0;

extern Xfsub	isosub;

Xfsub*	xsublist[] =
{
	&isosub,
	0
};
