/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "all.h"

static Xfile* xflfree;
static Xfid* xfdfree;

#define FIDMOD 127 /* prime */

static Xfile* xfiles[FIDMOD];
static Lock xlocks[FIDMOD];

Xfile*
xfile(Qid* qid, void* s, int new)
{
	int k;
	Xfile** hp, *f, *pf;

	k = ((uint32_t)qid->path ^ (((uint32_t)(uintptr)s) << 24)) % FIDMOD;
	hp = &xfiles[k];

	lock(&xlocks[k]);
	for(f = *hp, pf = 0; f; pf = f, f = f->next)
		if(f->qid.path == qid->path &&
		   (uint32_t)(uintptr)f->s == (uint32_t)(uintptr)s)
			break;
	if(f && pf) {
		pf->next = f->next;
		f->next = *hp;
		*hp = f;
	}
	if(new < 0 && f) {
		*hp = f->next;
		f->next = xflfree;
		xflfree = f;
		f = 0;
	}
	if(new > 0 && !f) {
		if(!(f = xflfree)) /* assign = */
			f = listalloc(1024 / sizeof(Xfile), sizeof(Xfile));
		xflfree = f->next;
		memset(f, 0, sizeof(Xfile));
		f->next = *hp;
		*hp = f;
		f->qid = *qid;
		f->s = s;
	}
	unlock(&xlocks[k]);
	return f;
}

Xfid*
xfid(char* uid, Xfile* xp, int new)
{
	Xfid* f, *pf;

	for(f = xp->users, pf = 0; f; pf = f, f = f->next)
		if(f->uid[0] == uid[0] && strcmp(f->uid, uid) == 0)
			break;
	if(f && pf) {
		pf->next = f->next;
		f->next = xp->users;
		xp->users = f;
	}
	if(new < 0 && f) {
		if(f->urfid)
			clunkfid(xp->s, f->urfid);
		if(f->opfid)
			clunkfid(xp->s, f->opfid);
		xp->users = f->next;
		f->next = xfdfree;
		xfdfree = f;
		f = 0;
	}
	if(new > 0 && !f) {
		if(!(f = xfdfree)) /* assign = */
			f = listalloc(1024 / sizeof(Xfid), sizeof(Xfid));
		xfdfree = f->next;
		memset(f, 0, sizeof(Xfid));
		f->next = xp->users;
		xp->users = f;
		f->xp = xp;
		f->uid = strstore(uid);
		f->urfid = 0;
		f->opfid = 0;
	}
	return f;
}

int
xfpurgeuid(Session* s, char* uid)
{
	Xfile** hp, *f;
	Xfid* xf;
	int k, count = 0;

	for(k = 0; k < FIDMOD; k++) {
		lock(&xlocks[k]);
		hp = &xfiles[k];
		for(f = *hp; f; f = f->next)
			if(f->s == s && (xf = xfid(uid, f, 0))) { /* assign = */
				xfclear(xf);
				++count;
			}
		unlock(&xlocks[k]);
	}
	return count;
}
