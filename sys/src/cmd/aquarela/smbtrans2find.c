/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"
#include <pool.h>

void
smbsearchfree(SmbSearch **searchp)
{
	SmbSearch *search = *searchp;
	if (search) {
		smbdircachefree(&search->dc);
		free(search->rep);
		free(search);
		*searchp = nil;
	}
}

void
smbsearchclose(SmbSession *s, SmbSearch *search)
{
	if (search) {
		smblogprintif(smbglobals.log.sids, "smbsearchclose: tid 0x%.4ux sid 0x%.4ux\n", search->t->id, search->id);
		smbidmapremove(s->sidmap, search);
		smbsearchfree(&search);
	}
}

void
smbsearchclosebyid(SmbSession *s, uint16_t sid)
{
	smbsearchclose(s,  smbidmapfind(s->sidmap, sid));
}

SmbSearch *
smbsearchnew(SmbSession *s, SmbDirCache *dc, Reprog *r, SmbTree *t)
{
	SmbSearch *search;
	if (s->sidmap == nil)
		s->sidmap = smbidmapnew();
	search = smbemalloc(sizeof(SmbSearch));
	smbidmapadd(s->sidmap, search);
	search->dc = dc;
	search->rep = r;
	search->t = t;
	smblogprintif(smbglobals.log.sids, "smbsearchnew: 0x%.4ux\n", search->id);
	return search;
}

static int
standardflatten(SmbSession *s, SmbBuffer *b, Dir *d, uint32_t *nameoffsetp)
{
	uint16_t mdate, mtime;
	uint16_t adate, atime;
	uint16_t fnlfixupoffset;

	smbplan9time2datetime(d->mtime, s->tzoff, &mdate, &mtime);
	smbplan9time2datetime(d->atime, s->tzoff, &adate, &atime);
	if (!smbbufferputs(b, mdate)
		|| !smbbufferputs(b, mtime)
		|| !smbbufferputs(b, adate)
		|| !smbbufferputs(b, atime)
		|| !smbbufferputs(b, mdate)
		|| !smbbufferputs(b, mtime)
		|| !smbbufferputl(b, d->length)
		|| !smbbufferputl(b, 512)			// ha
		|| !smbbufferputs(b, (d->qid.type & QTDIR) ? 0x10 : 0))
		return 0;
	fnlfixupoffset = smbbufferwriteoffset(b);
	if (!smbbufferputs(b, 0))
		return 0;
	*nameoffsetp = smbbufferwriteoffset(b);
	if (!smbbufferputstring(b, &s->peerinfo, 0, d->name))
		return 0;
	return smbbufferfixuprelatives(b, fnlfixupoffset);
}

static int
findbothflatten(SmbBuffer *b, SmbPeerInfo *p, Dir *d, uint32_t resumekey,
		uint32_t *nameoffsetp)
{
	int64_t mtime, atime;
	uint32_t fixup;

	fixup = smbbufferwriteoffset(b);
	mtime = smbplan9time2time(d->mtime);
	atime = smbplan9time2time(d->atime);
poolcheck(mainmem);
	if (!smbbufferputl(b, 0)
		|| !smbbufferputl(b, resumekey)
		|| !smbbufferputv(b, mtime)
		|| !smbbufferputv(b, atime)
		|| !smbbufferputv(b, mtime)
		|| !smbbufferputv(b, mtime)
		|| !smbbufferputv(b, d->length)
		|| !smbbufferputv(b, smbl2roundupvlong(d->length, smbglobals.l2allocationsize))			// ha
		|| !smbbufferputl(b, (d->qid.type & QTDIR) ? 0x10 : 0x80)
		|| !smbbufferputl(b, smbstringlen(p, d->name))
		|| !smbbufferputl(b, 0)
		|| !smbbufferputb(b, 0)
		|| !smbbufferputb(b, 0)
		|| !smbbufferfill(b, 0, 24))
		return 0;
poolcheck(mainmem);
	*nameoffsetp = smbbufferwriteoffset(b);
	if (!smbbufferputstring(b, p, 0, d->name) || !smbbufferalignl2(b, 2))
		return 0;
poolcheck(mainmem);
	return smbbufferfixuprelativeinclusivel(b, fixup);
}

static void
populate(SmbSession *s, SmbDirCache *dc, Reprog *r, uint16_t informationlevel,
	 uint16_t flags, uint16_t scount,
	uint16_t *ep, uint32_t *nameoffsetp)
{
	uint16_t e;
	uint32_t nameoffset;
	e = 0;
	nameoffset = 0;
	while (dc->i < dc->n && e < scount) {
		uint32_t backup;
		int rv;
		
		if (!smbmatch(dc->buf[dc->i].name, r)) {
			dc->i++;
			continue;
		}
		rv = 0;
		backup = smbbufferwriteoffset(s->transaction.out.data);
		switch (informationlevel) {
		case SMB_INFO_STANDARD:
			if (flags & SMB_FIND_RETURN_RESUME_KEYS) {
				if (!smbbufferputl(s->transaction.out.data, dc->i)) {
					rv = 0;
					break;
				}
			}
			rv = standardflatten(s, s->transaction.out.data, dc->buf + dc->i, &nameoffset);
			break;
		case SMB_FIND_FILE_BOTH_DIRECTORY_INFO:
			rv = findbothflatten(s->transaction.out.data, &s->peerinfo, dc->buf + dc->i, dc->i, &nameoffset);
			break;
		}
		if (rv == 0) {
			smbbufferwritebackup(s->transaction.out.data, backup);
			break;
		}
		dc->i++;
		e++;
	}
	*ep = e;
	*nameoffsetp = nameoffset;
}

SmbProcessResult
smbtrans2findfirst2(SmbSession *s, SmbHeader *h)
{
	SmbBuffer *b;
	char *pattern = nil;
	char *dir = nil;
	char *name = nil;
	uint16_t searchattributes, searchcount, flags, informationlevel;
	uint32_t searchstoragetype;
	SmbDirCache *dc = nil;
	uint16_t e;
	uint32_t nameoffset;
	uint16_t eos;
	SmbSearch *search;
	SmbProcessResult pr;
	Reprog *r = nil;
	SmbTree *t;
	int debug;

	debug = smboptable[h->command].debug
		|| smbtrans2optable[SMB_TRANS2_FIND_FIRST2].debug
		|| smbglobals.log.find;
poolcheck(mainmem);
	b = smbbufferinit(s->transaction.in.parameters, s->transaction.in.parameters, s->transaction.in.tpcount);
	if (!smbbuffergets(b, &searchattributes)
		|| !smbbuffergets(b, &searchcount)
		|| !smbbuffergets(b, &flags)
		|| !smbbuffergets(b, &informationlevel)
		|| !smbbuffergetl(b, &searchstoragetype)
		|| !smbbuffergetstring(b, h, SMB_STRING_PATH, &pattern)) {
		pr = SmbProcessResultFormat;
		goto done;
	}
	smbloglock();
	smblogprintif(debug, "searchattributes: 0x%.4ux\n", searchattributes);
	smblogprintif(debug, "searchcount: 0x%.4ux\n", searchcount);
	smblogprintif(debug, "flags: 0x%.4ux\n", flags);
	smblogprintif(debug, "informationlevel: 0x%.4ux\n", informationlevel);
	smblogprintif(debug, "searchstoragetype: 0x%.8lux\n", searchstoragetype);
	smblogprintif(debug, "pattern: %s\n", pattern);
	smblogunlock();
	smbpathsplit(pattern, &dir, &name);
	if (informationlevel != SMB_INFO_STANDARD && informationlevel != SMB_FIND_FILE_BOTH_DIRECTORY_INFO) {
		smblogprint(-1, "smbtrans2findfirst2: infolevel 0x%.4ux not implemented\n", informationlevel);
		smbseterror(s, ERRDOS, ERRunknownlevel);
		pr = SmbProcessResultError;
		goto done;
	}

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		pr = SmbProcessResultError;
		goto done;
	}

	dc = smbmkdircache(t, dir);
	if (dc == nil) {
		smbseterror(s, ERRDOS, ERRnoaccess);
		pr = SmbProcessResultError;
		goto done;
	}
poolcheck(mainmem);
	r = smbmkrep(name);
	populate(s, dc, r, informationlevel, flags, searchcount, &e, &nameoffset);
poolcheck(mainmem);
	eos = dc->i >= dc->n;
	if ((flags & SMB_FIND_CLOSE) != 0 || ((flags & SMB_FIND_CLOSE_EOS) != 0 && eos))
		smbdircachefree(&dc);
poolcheck(mainmem);
	if (dc) {
		/* create a search handle */
		search = smbsearchnew(s, dc, r, t);
		r = nil;
		dc = nil;
	}
	else
		search = nil;
	smbbufferputs(s->transaction.out.parameters, search ? search->id : 0);
	smbbufferputs(s->transaction.out.parameters, e);
	smbbufferputs(s->transaction.out.parameters, eos);
	smbbufferputs(s->transaction.out.parameters, 0);
	smbbufferputs(s->transaction.out.parameters, nameoffset);
	pr = SmbProcessResultReply;
done:
	smbbufferfree(&b);
	free(pattern);
	free(dir);
	free(name);
	smbdircachefree(&dc);
	free(r);
	return pr;
}

SmbProcessResult
smbtrans2findnext2(SmbSession *s, SmbHeader *h)
{
	SmbBuffer *b;
	int debug;
	uint16_t sid, scount, infolevel;
	uint32_t resumekey;
	uint16_t flags;
	char *filename = nil;
	SmbProcessResult pr;
	uint16_t e;
	uint32_t nameoffset;
	uint16_t eos;
	SmbTree *t;
	SmbSearch *search;

	debug = smboptable[h->command].debug
		|| smbtrans2optable[SMB_TRANS2_FIND_NEXT2].debug
		|| smbglobals.log.find;
	b = smbbufferinit(s->transaction.in.parameters, s->transaction.in.parameters, s->transaction.in.tpcount);
	if (!smbbuffergets(b, &sid)
		|| !smbbuffergets(b, &scount)
		|| !smbbuffergets(b, &infolevel)
		|| !smbbuffergetl(b, &resumekey)
		|| !smbbuffergets(b, &flags)
		|| !smbbuffergetstring(b, h, 0, &filename)) {
		pr = SmbProcessResultFormat;
		goto done;
	}
	smblogprintif(debug,
		"smbtrans2findnext2: sid %d scount %d infolevel 0x%.4ux resumekey %lu flags 0x%.4ux filename %s\n",
		sid, scount, infolevel, resumekey, flags, filename);

	if (infolevel != SMB_INFO_STANDARD && infolevel != SMB_FIND_FILE_BOTH_DIRECTORY_INFO) {
		smblogprint(-1, "smbtrans2findnext2: infolevel 0x%.4ux not implemented\n", infolevel);
		smbseterror(s, ERRDOS, ERRunknownlevel);
		pr = SmbProcessResultError;
		goto done;
	}

	t = smbidmapfind(s->tidmap, h->tid);
	if (t == nil) {
		smbseterror(s, ERRSRV, ERRinvtid);
		pr = SmbProcessResultError;
		goto done;
	}

	search = smbidmapfind(s->sidmap, sid);
	if (search == nil) {
		smbseterror(s, ERRDOS, ERRnofiles);
		pr = SmbProcessResultError;
		goto done;
	}

	if (search->t != t) {
		smbseterror(s, ERRSRV, ERRinvtid);
		pr = SmbProcessResultError;
		goto done;
	}

	if ((flags & (1 << 3)) == 0) {
		int32_t i;
		if (filename == nil) {
			smbseterror(s, ERRDOS, ERRnofiles);
			pr = SmbProcessResultError;
			goto done;
		}
		for (i = 0; i < search->dc->n; i++)
			if (strcmp(search->dc->buf[i].name, filename) == 0) {
				search->dc->i = i + 1;
				break;
			}
	}

	populate(s, search->dc, search->rep, infolevel, flags, scount, &e, &nameoffset);
	
	eos = search->dc->i >= search->dc->n;
	if ((flags & SMB_FIND_CLOSE) != 0 || ((flags & SMB_FIND_CLOSE_EOS) != 0 && eos))
		smbsearchclose(s, search);
	smbbufferputs(s->transaction.out.parameters, e);
	smbbufferputs(s->transaction.out.parameters, eos);
	smbbufferputs(s->transaction.out.parameters, 0);
	smbbufferputs(s->transaction.out.parameters, nameoffset);
	pr = SmbProcessResultReply;
done:
	smbbufferfree(&b);
	free(filename);
	return pr;
}
