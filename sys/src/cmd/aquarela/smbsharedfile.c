/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

typedef struct SmbSharedFileEntry SmbSharedFileEntry;
struct SmbSharedFileEntry {
	SmbSharedFile	ssf;
	Ref	ref;
	SmbSharedFileEntry *next;
};

static struct {
	QLock	qlock;
	SmbSharedFileEntry *list;
} sharedfiletable;

typedef struct SmbLockListEntry SmbLockListEntry;

struct SmbLockListEntry {
	SmbLock smblock;
	SmbLockListEntry *next;
};

struct SmbLockList {
	SmbLockListEntry *head;
};

static int
lockconflict(SmbLock *l1, SmbLock *l2)
{
	return l1->base < l2->limit && l2->base < l1->limit;
}

static int
lockorder(SmbLock *l1, SmbLock *l2)
{
	if (l1->base < l2->base)
		return -1;
	if (l1->base > l2->base)
		return 1;
	if (l1->limit > l2->limit)
		return -1;
	if (l1->limit < l2->limit)
		return 1;
	return 0;
}

static void
locklistfree(SmbLockList **llp)
{
	SmbLockList *ll = *llp;
	if (ll) {
		while (ll->head) {
			SmbLockListEntry *next = ll->head->next;
			free(ll->head);
			ll->head = next;
		}
		free(ll);
		*llp = nil;
	}
}

int
smbsharedfilelock(SmbSharedFile *sf, SmbSession *s, uint16_t pid,
		  int64_t base,
		  int64_t limit)
{
	SmbLockListEntry smblock;
	SmbLockListEntry *l, *nl, **lp;
	smblock.smblock.s = s;
	smblock.smblock.pid = pid;
	smblock.smblock.base = base;
	smblock.smblock.limit = limit;
	if (sf->locklist) {
		for (l = sf->locklist->head; l; l = l->next)
			if (lockconflict(&l->smblock, &smblock.smblock)) {
				smblogprintif(smbglobals.log.locks, "smbsharedfilelock: lock [%lld, %lld) failed because conflicts with [%lld, %lld)\n",
					base, limit, l->smblock.base, l->smblock.limit);
				return 0;
			}
	}
	if (sf->locklist == nil)
		sf->locklist = smbemallocz(sizeof(SmbLockList), 1);
	for (lp = &sf->locklist->head; (l = *lp) != nil; lp = &l->next)
		if (lockorder(&smblock.smblock, &l->smblock) <= 0)
			break;
	smblogprintif(smbglobals.log.locks, "smbsharedfilelock: lock [%lld, %lld) succeeded\n", base, limit);
	nl = smbemalloc(sizeof(*nl));
	*nl = smblock;
	nl->next = *lp;
	*lp = nl;
//{
//	smblogprintif(smbglobals.log.locks,"smbsharedfilelock: list\n");
//	for (l = sf->locklist->head; l; l = l->next)
//		smblogprintif(smbglobals.log.locks, "smbsharedfilelock: [%lld, %lld)\n", l->base, l->limit);
//}
	return 1;
}

int
smbsharedfileunlock(SmbSharedFile *sf, SmbSession *s, uint16_t pid,
		    int64_t base, int64_t limit)
{
	SmbLockListEntry smblock;
	SmbLockListEntry *l, **lp;
	smblock.smblock.s = s;
	smblock.smblock.pid = pid;
	smblock.smblock.base = base;
	smblock.smblock.limit = limit;
	if (sf->locklist == nil)
		goto failed;
	for (lp = &sf->locklist->head; (l = *lp) != nil; lp = &l->next) {
		if (l->smblock.s != s || l->smblock.pid != pid)
			continue;
		switch (lockorder(&smblock.smblock, &l->smblock)) {
		case 0:
			*lp = l->next;
			free(l);
			smblogprintif(smbglobals.log.locks, "smbsharedfilelock: unlock [%lld, %lld) succeeded\n", base, limit);
			return 1;
		case -1:
			goto failed;
		}
	}
failed:
	smblogprintif(smbglobals.log.locks, "smbsharedfilelock: unlock [%lld, %lld) failed\n", base, limit);
	return 0;
}

static int
p9denied(int p9mode, int share)
{
//smblogprint(-1, "p9denied(%d, %d)\n", p9mode, share);
	if (share == SMB_OPEN_MODE_SHARE_EXCLUSIVE)
		return 1;
	switch (p9mode & 3) {
	case OREAD:
	case OEXEC:
		if (share == SMB_OPEN_MODE_SHARE_DENY_READOREXEC)
			return 1;
		break;
	case OWRITE:
		if (share == SMB_OPEN_MODE_SHARE_DENY_WRITE)
			return 1;
		break;
	case ORDWR:
		if (share != SMB_OPEN_MODE_SHARE_DENY_NONE)
			return 1;
		break;
	}
	return 0;
}

static void
sharesplit(int share, int *denyread, int *denywrite)
{
	switch (share) {
	case SMB_OPEN_MODE_SHARE_EXCLUSIVE:
		*denyread = 1;
		*denywrite = 1;
		break;
	case SMB_OPEN_MODE_SHARE_DENY_READOREXEC:
		*denyread = 1;
		*denywrite = 0;
		break;
	case SMB_OPEN_MODE_SHARE_DENY_WRITE:
		*denywrite = 0;
		*denywrite = 1;
		break;
	default:
		*denyread = 0;
		*denywrite = 0;
	}
}

static int
sharemake(int denyread, int denywrite)
{
	if (denyread)
		if (denywrite)
			return SMB_OPEN_MODE_SHARE_EXCLUSIVE;
		else
			return SMB_OPEN_MODE_SHARE_DENY_READOREXEC;
	else if (denywrite)
		return SMB_OPEN_MODE_SHARE_DENY_WRITE;
	else
		return SMB_OPEN_MODE_SHARE_DENY_NONE;
}

static uint16_t
sharesubtract(int share1, int share2)
{
	int dr1, dw1;
	int dr2, dw2;
	sharesplit(share1, &dr1, &dw1);
	sharesplit(share2, &dr2, &dw2);
	if (dw2)
		dw1 = 0;
	if (dr2)
		dr1 = 0;
	return sharemake(dr1, dw1);
}

static int
shareadd(int share1, int share2)
{
	int dr1, dw1;
	int dr2, dw2;
	sharesplit(share1, &dr1, &dw1);
	sharesplit(share2, &dr2, &dw2);
	if (dw2)
		dw1 = 1;
	if (dr2)
		dr1 = 1;
	return sharemake(dr1, dw1);
}

SmbSharedFile *
smbsharedfileget(Dir *d, int p9mode, int *sharep)
{
	SmbSharedFileEntry *sfe;
	qlock(&sharedfiletable.qlock);
	for (sfe = sharedfiletable.list; sfe; sfe = sfe->next) {
		if (sfe->ssf.type == d->type && sfe->ssf.dev == d->dev && sfe->ssf.path == d->qid.path) {
			if (p9denied(p9mode, sfe->ssf.share)) {
				qunlock(&sharedfiletable.qlock);
				return nil;
			}
			*sharep = sharesubtract(*sharep, sfe->ssf.share);
			sfe->ssf.share = shareadd(sfe->ssf.share, *sharep);
			sfe->ref.ref++;
			goto done;
		}
	}
	sfe = smbemallocz(sizeof(SmbSharedFileEntry), 1);
	sfe->ssf.type = d->type;
	sfe->ssf.dev = d->dev;
	sfe->ssf.path = d->qid.path;
//	sfe->name = smbestrdup(name);
	sfe->ref.ref = 1;
	sfe->ssf.share = *sharep;
	sfe->next = sharedfiletable.list;
	sharedfiletable.list = sfe;
done:
	smblogprintif(smbglobals.log.sharedfiles, "smbsharedfileget: ref %d share %d\n",
		sfe->ref.ref, sfe->ssf.share);
	qunlock(&sharedfiletable.qlock);
	return &sfe->ssf;
}

void
smbsharedfileput(SmbFile *f, SmbSharedFile *sf, int share)
{
	SmbSharedFileEntry *sfe, **sfep;
	qlock(&sharedfiletable.qlock);
	for (sfep = &sharedfiletable.list; (sfe = *sfep) != nil; sfep = &sfe->next) {
		if (&sfe->ssf == sf) {
			sfe->ref.ref--;
			if (sfe->ref.ref == 0) {
				*sfep = sfe->next;
				if (sfe->ssf.deleteonclose && f)
					smbremovefile(f->t, nil, f->name);
				smblogprintif(smbglobals.log.sharedfiles, "smbsharedfileput: removed\n");
				locklistfree(&sfe->ssf.locklist);
				free(sfe);
			}
			else {
				sfe->ssf.share = sharesubtract(sfe->ssf.share, share);
				smblogprintif(smbglobals.log.sharedfiles,
					"smbsharedfileput: ref %d share %d\n", sfe->ref.ref, sfe->ssf.share);
			}
			break;
		}
	}
	qunlock(&sharedfiletable.qlock);
}
