/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "headers.h"

typedef struct DisconnectData {
	SmbSession *s;
	SmbTree *t;
} DisconnectData;

static void
smbtreefree(SmbTree **tp)
{
	SmbTree *t = *tp;
	if (t) {
		smbserviceput(t->serv);
		free(t);
		*tp = nil;
	}
}

static void
closesearch(void *magic, void *a)
{
	SmbSearch *search = a;
	DisconnectData *d = magic;
	if (search->t == d->t)
		smbsearchclose(d->s, search);
}

static void
closefile(void *magic, void *a)
{
	SmbFile *f = a;
	DisconnectData *d = magic;
	if (f->t == d->t)
		smbfileclose(d->s, f);
}

void
smbtreedisconnect(SmbSession *s, SmbTree *t)
{
	if (t) {
		DisconnectData data;
		smblogprintif(smbglobals.log.tids, "smbtreedisconnect: 0x%.4ux\n", t->id);
		data.s = s;
		data.t = t;
		smbserviceput(t->serv);
		smbidmapapply(s->sidmap, closesearch, &data);
		smbidmapapply(s->fidmap, closefile, &data);
		smbidmapremove(s->tidmap, t);
		smbtreefree(&t);
	}
}

void
smbtreedisconnectbyid(SmbSession *s, ushort id)
{
	smbtreedisconnect(s, smbidmapfind(s->tidmap, id));
}

SmbTree *
smbtreeconnect(SmbSession *s, SmbService *serv)
{
	SmbTree *t;

	if (s->tidmap == nil)
		s->tidmap = smbidmapnew();

	t = smbemallocz(sizeof(*t), 1);
	smbidmapadd(s->tidmap, t);
	t->serv = serv;
	smbserviceget(serv);
	smblogprintif(smbglobals.log.tids, "smbtreeconnect: 0x%.4ux\n", t->id);
	return t;
}
