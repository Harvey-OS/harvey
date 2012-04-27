#include "headers.h"

typedef struct SmbSharedFileEntry SmbSharedFileEntry;
struct SmbSharedFileEntry {
	SmbSharedFile;
	Ref;
	SmbSharedFileEntry *next;
};

static struct {
	QLock;
	SmbSharedFileEntry *list;
} sharedfiletable;

typedef struct SmbLockListEntry SmbLockListEntry;

struct SmbLockListEntry {
	SmbLock;
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
smbsharedfilelock(SmbSharedFile *sf, SmbSession *s, ushort pid, vlong base, vlong limit)
{
	SmbLockListEntry smblock;
	SmbLockListEntry *l, *nl, **lp;
	smblock.s = s;
	smblock.pid = pid;
	smblock.base = base;
	smblock.limit = limit;
	if (sf->locklist) {
		for (l = sf->locklist->head; l; l = l->next)
			if (lockconflict(l, &smblock)) {
				smblogprintif(smbglobals.log.locks, "smbsharedfilelock: lock [%lld, %lld) failed because conflicts with [%lld, %lld)\n",
					base, limit, l->base, l->limit);
				return 0;
			}
	}
	if (sf->locklist == nil)
		sf->locklist = smbemallocz(sizeof(SmbLockList), 1);
	for (lp = &sf->locklist->head; (l = *lp) != nil; lp = &l->next)
		if (lockorder(&smblock, l) <= 0)
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
smbsharedfileunlock(SmbSharedFile *sf, SmbSession *s, ushort pid, vlong base, vlong limit)
{
	SmbLockListEntry smblock;
	SmbLockListEntry *l, **lp;
	smblock.s = s;
	smblock.pid = pid;
	smblock.base = base;
	smblock.limit = limit;
	if (sf->locklist == nil)
		goto failed;
	for (lp = &sf->locklist->head; (l = *lp) != nil; lp = &l->next) {
		if (l->s != s || l->pid != pid)
			continue;
		switch (lockorder(&smblock, l)) {
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

static ushort
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
	qlock(&sharedfiletable);
	for (sfe = sharedfiletable.list; sfe; sfe = sfe->next) {
		if (sfe->type == d->type && sfe->dev == d->dev && sfe->path == d->qid.path) {
			if (p9denied(p9mode, sfe->share)) {
				qunlock(&sharedfiletable);
				return nil;
			}
			*sharep = sharesubtract(*sharep, sfe->share);
			sfe->share = shareadd(sfe->share, *sharep);
			sfe->ref++;
			goto done;
		}
	}
	sfe = smbemallocz(sizeof(SmbSharedFileEntry), 1);
	sfe->type = d->type;
	sfe->dev = d->dev;
	sfe->path = d->qid.path;
//	sfe->name = smbestrdup(name);
	sfe->ref = 1;
	sfe->share = *sharep;
	sfe->next = sharedfiletable.list;
	sharedfiletable.list = sfe;
done:
	smblogprintif(smbglobals.log.sharedfiles, "smbsharedfileget: ref %d share %d\n",
		sfe->ref, sfe->share);
	qunlock(&sharedfiletable);
	return sfe;
}

void
smbsharedfileput(SmbFile *f, SmbSharedFile *sf, int share)
{
	SmbSharedFileEntry *sfe, **sfep;
	qlock(&sharedfiletable);
	for (sfep = &sharedfiletable.list; (sfe = *sfep) != nil; sfep = &sfe->next) {
		if (sfe == sf) {
			sfe->ref--;
			if (sfe->ref == 0) {
				*sfep = sfe->next;
				if (sfe->deleteonclose && f)
					smbremovefile(f->t, nil, f->name);
				smblogprintif(smbglobals.log.sharedfiles, "smbsharedfileput: removed\n");
				locklistfree(&sfe->locklist);
				free(sfe);
			}
			else {
				sfe->share = sharesubtract(sfe->share, share);
				smblogprintif(smbglobals.log.sharedfiles,
					"smbsharedfileput: ref %d share %d\n", sfe->ref, sfe->share);
			}
			break;
		}
	}
	qunlock(&sharedfiletable);
}
