/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

Buf*	bopen(long (*)(Buf*, void*, long, ulong), int, int, int);
long	bread(Buf*, void*, long, vlong);
void	bterm(Buf*);
long	bufread(Otrack*, void*, long, vlong);
long	bufwrite(Otrack*, void*, long);
long	bwrite(Buf*, void*, long);
char*	disctype(Drive *drive);
void	*emalloc(ulong);
char*	geterrstr(void);
Drive*	mmcprobe(Scsi*);
