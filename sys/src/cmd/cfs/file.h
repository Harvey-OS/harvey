/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void	fmerge(Dptr*, char*, char*, int, int);
int	fbwrite(Icache*, Ibuf*, char*, u32, int);
i32	fwrite(Icache*, Ibuf*, char*, u32, i32);
Dptr*	fpget(Icache*, Ibuf*, u32);
i32	fread(Icache*, Ibuf*, char*, u32, i32);
