/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#ifndef NA_H
#define NA_H

struct na_patch {
	unsigned lwoff;
	unsigned char type;
};

int na_fixup(unsigned long *script, unsigned long pa_script, unsigned long pa_reg,
    struct na_patch *patch, int patches,
    int (*externval)(int x, unsigned long *v));

#endif
