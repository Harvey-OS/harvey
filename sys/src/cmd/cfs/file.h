/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

void	fmerge(Dptr*, char*, char*, int, int);
int	fbwrite(Icache*, Ibuf*, char*, ulong, int);
long	fwrite(Icache*, Ibuf*, char*, ulong, long);
Dptr*	fpget(Icache*, Ibuf*, ulong);
long	fread(Icache*, Ibuf*, char*, ulong, long);
