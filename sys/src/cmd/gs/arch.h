/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef _ARCH_H
#define _ARCH_H
#ifdef T386
#include "386.h"
#elif Tmips
#include "mips.h"
#elif Tpower
#include "mips.h"
#elif Talpha
#include "alpha.h"
#elif Tarm
#include "arm.h"
#elif Tamd64
#include "amd64.h"
#else
	I do not know about your architecture.
	Update switch in arch.h with new architecture.
#endif	/* T386 */
#endif /* _ARCH_H */
