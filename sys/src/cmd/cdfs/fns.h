/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

Buf*	bopen(i32 (*)(Buf*, void*, i32, u32), int, int, int);
i32	bread(Buf*, void*, i32, i64);
void	bterm(Buf*);
i32	bufread(Otrack*, void*, i32, i64);
i32	bufwrite(Otrack*, void*, i32);
i32	bwrite(Buf*, void*, i32);
char*	disctype(Drive *drive);
void	*emalloc(u32);
char*	geterrstr(void);
Drive*	mmcprobe(Scsi*);
