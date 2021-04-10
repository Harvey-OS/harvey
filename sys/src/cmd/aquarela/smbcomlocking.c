/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

static int
getlock(SmbBuffer *b, int large, u16 *pidp, u64 *offsetp,
	u64 *lengthp)
{
	u32 ohigh, olow;
	u32 lhigh, llow;
	if (!smbbuffergets(b, pidp))
		return 0;
	if (large && !smbbuffergetbytes(b, nil, 2))
		return 0;
	if (large) {
		if (!smbbuffergetl(b, &ohigh) || !smbbuffergetl(b, &olow)
			|| !smbbuffergetl(b, &lhigh) || !smbbuffergetl(b, &llow))
			return 0;
		*offsetp = ((u64)ohigh << 32) | olow;
		*lengthp = ((u64)lhigh << 32) | llow;
		return 1;
	}
	if (!smbbuffergetl(b, &olow) || !smbbuffergetl(b, &llow))
		return 0;
	*offsetp = olow;
	*lengthp = llow;
	return 1;
}

SmbProcessResult
smbcomlockingandx(SmbSession *s, SmbHeader *h, u8 *pdata, SmbBuffer *b)
{
	u8 andxcommand;
	u16 andxoffset;
	u32 andxoffsetfixup;
	u16 fid;
	u8 locktype;
	u8 oplocklevel;
	u32 timeout;
	u16 numberofunlocks;
	u16 numberoflocks;
	SmbTree *t;
	SmbFile *f;
	int l;
	SmbProcessResult pr;
	u32 backupoffset;
	int large;

	if (!smbcheckwordcount("comlockingandx", h, 8))
		return SmbProcessResultFormat;

	andxcommand = *pdata++;
	pdata++;
	andxoffset = smbnhgets(pdata); pdata += 2;
	fid = smbnhgets(pdata); pdata += 2;
	locktype = *pdata++;
	oplocklevel = *pdata++;
	timeout = smbnhgetl(pdata); pdata += 4;
	numberofunlocks = smbnhgets(pdata); pdata += 2;
	numberoflocks = smbnhgets(pdata);
	smblogprint(h->command, "smbcomlockingandx: fid 0x%.4x locktype 0x%.2x oplocklevel 0x%.2x timeout %lu numberofunlocks %d numberoflocks %u\n",
		fid, locktype, oplocklevel, timeout, numberofunlocks, numberoflocks);
	large = locktype & 0x10;
	locktype &= ~0x10;
	if (locktype != 0 || oplocklevel != 0) {
		smblogprint(-1, "smbcomlockingandx: locktype 0x%.2x unimplemented\n", locktype);
		return SmbProcessResultUnimp;
	}
	if (oplocklevel != 0) {
		smblogprint(-1, "smbcomlockingandx: oplocklevel 0x%.2x unimplemented\n", oplocklevel);
		return SmbProcessResultUnimp;
	}
	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
	error:
		return SmbProcessResultError;
	}
	f = smbidmapfind(s->fidmap, fid);
	if (f == nil) {
		smbseterror(s, ERRDOS, ERRbadfid);
		goto error;
	}
	backupoffset = smbbufferreadoffset(b);
	for (l = 0; l < numberofunlocks; l++) {
		u16 pid;
		u64 offset;
		u64 length;
		if (!getlock(b, large, &pid, &offset, &length)) {
			pr = SmbProcessResultFormat;
			goto done;
		}
		smblogprint(h->command, "smbcomlockingandx: unlock pid 0x%.4x offset %llu length %llu\n",
			pid, offset, length);
		smbsharedfileunlock(f->sf, s, h->pid, offset, offset + length);
	}
	for (l = 0; l < numberoflocks; l++) {
		u16 pid;
		u64 offset;
		u64 length;
		if (!getlock(b, large, &pid, &offset, &length)) {
			pr = SmbProcessResultFormat;
			goto done;
		}
		smblogprint(h->command, "smbcomlockingandx: lock pid 0x%.4x offset %llu length %llu\n",
			pid, offset, length);
		if (!smbsharedfilelock(f->sf, s, h->pid, offset, offset + length))
			break;
	}
	if (l < numberoflocks) {
		u16 i;
		u16 pid;
		u64 offset;
		u64 length;
		smbbufferreadbackup(b, backupoffset);
		for (i  = 0; i < l; i++) {
			assert(getlock(b, large, &pid, &offset, &length));
			smbsharedfileunlock(f->sf, s, h->pid, offset, offset + length);
		}
		smbseterror(s, ERRDOS, ERRlock);
		goto error;
	}
	h->wordcount = 2;
	if (!smbbufferputandxheader(s->response, h, &s->peerinfo, andxcommand, &andxoffsetfixup)
		|| !smbbufferputs(s->response, 0)) {	// bytecount 0
		pr = SmbProcessResultMisc;
		goto done;
	}
	if (andxcommand != SMB_COM_NO_ANDX_COMMAND)
		pr = smbchaincommand(s, h, andxoffsetfixup, andxcommand, andxoffset, b);
	else
		pr = SmbProcessResultReply;
done:
	return pr;
}
