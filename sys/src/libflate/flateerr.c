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
#include <flate.h>

char *
flateerr(int err)
{
	switch(err){
	case FlateOk:
		return "no error";
	case FlateNoMem:
		return "out of memory";
	case FlateInputFail:
		return "input error";
	case FlateOutputFail:
		return "output error";
	case FlateCorrupted:
		return "corrupted data";
	case FlateInternal:
		return "internal error";
	}
	return "unknown error";
}
