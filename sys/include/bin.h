/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma	lib	"libbin.a"
#pragma	src	"/sys/src/libbin"

typedef struct Bin	Bin;

#pragma incomplete Bin

void	*binalloc(Bin **, ulong size, int zero);
void	*bingrow(Bin **, void *op, ulong osize, ulong size, int zero);
void	binfree(Bin **);
