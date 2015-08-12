/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef __STDARG
#define __STDARG

typedef char *va_list;

#define va_start(list, start) list = \
				  (sizeof(start) < 4 ? (char *)((int *)&(start) + 1) : (char *)(&(start) + 1))
#define va_end(list)
#define va_arg(list, mode) \
	((sizeof(mode) <= 4) ? ((list += 4), (mode *)list)[-1] : (signof(mode) != signof(double)) ? ((list += sizeof(mode)), (mode *)list)[-1] : ((list = (char *)((unsigned long)(list + 7) & ~7) + sizeof(mode)), (mode *)list)[-1])

#endif /* __STDARG */
