/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

Buf*	bopen(int32_t (*)(Buf*, void*, int32_t, uint32_t), int, int, int);
int32_t	bread(Buf*, void*, int32_t, int64_t);
void	bterm(Buf*);
int32_t	bufread(Otrack*, void*, int32_t, int64_t);
int32_t	bufwrite(Otrack*, void*, int32_t);
int32_t	bwrite(Buf*, void*, int32_t);
char*	disctype(Drive *drive);
void	*emalloc(uint32_t);
char*	geterrstr(void);
Drive*	mmcprobe(Scsi*);
