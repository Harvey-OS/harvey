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
getlock(SmbBuffer *b, int large, uint16_t *pidp, uint64_t *offsetp,
	uint64_t *lengthp)
{
	uint32_t ohigh, olow;
	uint32_t lhigh, llow;
	if (!smbbuffergets(b, pidp))
		return 0;
	if (large && !smbbuffergetbytes(b, nil, 2))
		return 0;
	if (large) {
		if (!smbbuffergetl(b, &ohigh) || !smbbuffergetl(b, &olow)
			|| !smbbuffergetl(b, &lhigh) || !smbbuffergetl(b, &llow))
			return 0;
		*offsetp = ((uint64_t)ohigh << 32) | olow;
		*lengthp = ((uint64_t)lhigh << 32) | llow;
		return 1;
	}
	if (!smbbuffergetl(b, &olow) || !smbbuffergetl(b, &llow))
		return 0;
	*offsetp = olow;
	*lengthp = llow;
	return 1;
}

SmbProcessResult
smbcomlockingandx(SmbSession *s, SmbHeader *h, uint8_t *pdata, SmbBuffer *b)
{
	uint8_t andxcommand;
	uint16_t andxoffset;
	uint32_t andxoffsetfixup;
	uint16_t fid;
	uint8_t locktype;
	uint8_t oplocklevel;
	uint32_t timeout;
	uint16_t numberofunlocks;
	uint16_t numberoflocks;
	SmbTree *t;
	SmbFile *f;
	int l;
	SmbProcessResult pr;
	uint32_t backupoffset;
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
		uint16_t pid;
		uint64_t offset;
		uint64_t length;
		if (!getlock(b, large, &pid, &offset, &length)) {
			pr = SmbProcessResultFormat;
			goto done;
		}
		smblogprint(h->command, "smbcomlockingandx: unlock pid 0x%.4x offset %llu length %llu\n",
			pid, offset, length);
		smbsharedfileunlock(f->sf, s, h->pid, offset, offset + length);
	}
	for (l = 0; l < numberoflocks; l++) {
		uint16_t pid;
		uint64_t offset;
		uint64_t length;
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
		uint16_t i;
		uint16_t pid;
		uint64_t offset;
		uint64_t length;
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
