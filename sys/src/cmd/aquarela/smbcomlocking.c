#include "headers.h"

static int
getlock(SmbBuffer *b, int large, ushort *pidp, uvlong *offsetp, uvlong *lengthp)
{
	ulong ohigh, olow;
	ulong lhigh, llow;
	if (!smbbuffergets(b, pidp))
		return 0;
	if (large && !smbbuffergetbytes(b, nil, 2))
		return 0;
	if (large) {
		if (!smbbuffergetl(b, &ohigh) || !smbbuffergetl(b, &olow)
			|| !smbbuffergetl(b, &lhigh) || !smbbuffergetl(b, &llow))
			return 0;
		*offsetp = ((uvlong)ohigh << 32) | olow;
		*lengthp = ((uvlong)lhigh << 32) | llow;
		return 1;
	}
	if (!smbbuffergetl(b, &olow) || !smbbuffergetl(b, &llow))
		return 0;
	*offsetp = olow;
	*lengthp = llow;
	return 1;
}

SmbProcessResult
smbcomlockingandx(SmbSession *s, SmbHeader *h, uchar *pdata, SmbBuffer *b)
{
	uchar andxcommand;
	ushort andxoffset;
	ulong andxoffsetfixup;
	ushort fid;
	uchar locktype;
	uchar oplocklevel;
	ulong timeout;
	ushort numberofunlocks;
	ushort numberoflocks;
	SmbTree *t;
	SmbFile *f;
	int l;
	SmbProcessResult pr;
	ulong backupoffset;
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
	smblogprint(h->command, "smbcomlockingandx: fid 0x%.4ux locktype 0x%.2ux oplocklevel 0x%.2ux timeout %lud numberofunlocks %d numberoflocks %ud\n",
		fid, locktype, oplocklevel, timeout, numberofunlocks, numberoflocks);
	large = locktype & 0x10;
	locktype &= ~0x10;
	if (locktype != 0 || oplocklevel != 0) {
		smblogprint(-1, "smbcomlockingandx: locktype 0x%.2ux unimplemented\n", locktype);
		return SmbProcessResultUnimp;
	}
	if (oplocklevel != 0) {
		smblogprint(-1, "smbcomlockingandx: oplocklevel 0x%.2ux unimplemented\n", oplocklevel);
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
		ushort pid;
		uvlong offset;
		uvlong length;
		if (!getlock(b, large, &pid, &offset, &length)) {
			pr = SmbProcessResultFormat;
			goto done;
		}
		smblogprint(h->command, "smbcomlockingandx: unlock pid 0x%.4ux offset %llud length %llud\n",
			pid, offset, length);
		smbsharedfileunlock(f->sf, s, h->pid, offset, offset + length);
	}
	for (l = 0; l < numberoflocks; l++) {
		ushort pid;
		uvlong offset;
		uvlong length;
		if (!getlock(b, large, &pid, &offset, &length)) {
			pr = SmbProcessResultFormat;
			goto done;
		}
		smblogprint(h->command, "smbcomlockingandx: lock pid 0x%.4ux offset %llud length %llud\n",
			pid, offset, length);
		if (!smbsharedfilelock(f->sf, s, h->pid, offset, offset + length))
			break;
	}
	if (l < numberoflocks) {
		ushort i;
		ushort pid;
		uvlong offset;
		uvlong length;
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
