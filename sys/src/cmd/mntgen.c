/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include <mp.h>
#include <libsec.h>

static void
usage(void)
{
	fprint(2, "mntgen [-s srvname] [mtpt]\n");
	exits("usage");
}

uint32_t time0;

typedef struct Tab Tab;
struct Tab
{
	char *name;
	int64_t qid;
	uint32_t time;
	int ref;
};

Tab *tab;
int ntab;
int mtab;

static Tab*
findtab(int64_t path)
{
	int i;

	for(i=0; i<ntab; i++)
		if(tab[i].qid == path)
			return &tab[i];
	return nil;
}

static int64_t
hash(char *name)
{
	int64_t digest[MD5dlen / sizeof(int64_t) + 1];
	md5((uint8_t *)name, strlen(name), (uint8_t *)digest, nil);
	return digest[0] & ((1ULL<<48)-1);
}

static void
fsopen(Req *r)
{
	if(r->ifcall.mode != OREAD)
		respond(r, "permission denied");
	else
		respond(r, nil);
}

static int
dirgen(int i, Dir *d, void *v)
{
	if(i >= ntab)
		return -1;
	memset(d, 0, sizeof *d);
	d->qid.type = QTDIR;
	d->uid = estrdup9p("sys");
	d->gid = estrdup9p("sys");
	d->mode = DMDIR|0555;
	d->length = 0;
	if(i == -1){
		d->name = estrdup9p("/");
		d->atime = d->mtime = time0;
	}else{
		d->qid.path = tab[i].qid;
		d->name = estrdup9p(tab[i].name);
		d->atime = d->mtime = tab[i].time;
	}
	return 0;
}

static void
fsread(Req *r)
{
	if(r->fid->qid.path == 0)
		dirread9p(r, dirgen, nil);
	else
		r->ofcall.count = 0;
	respond(r, nil);
}

static void
fsstat(Req *r)
{
	Tab *t;
	int64_t qid;

	qid = r->fid->qid.path;
	if(qid == 0)
		dirgen(-1, &r->d, nil);
	else{
		if((t = findtab(qid)) == nil){
			respond(r, "path not found (? ? ?)");
			return;
		}
		dirgen(t-tab, &r->d, nil);
	}
	respond(r, nil);
}

static char*
fswalk1(Fid *fid, char *name, void *v)
{
	int i;
	Tab *t;
	int64_t h;

	if(fid->qid.path != 0){
		/* nothing in child directory */
		if(strcmp(name, "..") == 0){
			if((t = findtab(fid->qid.path)) != nil)
				t->ref--;
			fid->qid.path = 0;
			return nil;
		}
		return "path not found";
	}
	/* root */
	if(strcmp(name, "..") == 0)
		return nil;
	for(i=0; i<ntab; i++)
		if(strcmp(tab[i].name, name) == 0){
			tab[i].ref++;
			fid->qid.path = tab[i].qid;
			return nil;
		}
	h = hash(name);
	if(findtab(h) != nil)
		return "hash collision";

	/* create it */
	if(ntab == mtab){
		if(mtab == 0)
			mtab = 16;
		else
			mtab *= 2;
		tab = erealloc9p(tab, sizeof(tab[0])*mtab);
	}
	tab[ntab].qid = h;
	fid->qid.path = tab[ntab].qid;
	tab[ntab].name = estrdup9p(name);
	tab[ntab].time = time(0);
	tab[ntab].ref = 1;
	ntab++;

	return nil;
}

static char*
fsclone(Fid *fid, Fid *f, void *v)
{
	Tab *t;

	if((t = findtab(fid->qid.path)) != nil)
		t->ref++;
	return nil;
}

static void
fswalk(Req *r)
{
	walkandclone(r, fswalk1, fsclone, nil);
}

static void
fsclunk(Fid *fid)
{
	Tab *t;
	int64_t qid;

	qid = fid->qid.path;
	if(qid == 0)
		return;
	if((t = findtab(qid)) == nil){
		fprint(2, "warning: cannot find %llux\n", qid);
		return;
	}
	if(--t->ref == 0){
		free(t->name);
		tab[t-tab] = tab[--ntab];
	}else if(t->ref < 0)
		fprint(2, "warning: negative ref count for %s\n", t->name);
}

static void
fsattach(Req *r)
{
	char *spec;

	spec = r->ifcall.aname;
	if(spec && spec[0]){
		respond(r, "invalid attach specifier");
		return;
	}

	r->ofcall.qid = (Qid){0, 0, QTDIR};
	r->fid->qid = r->ofcall.qid;
	respond(r, nil);
}

Srv fs=
{
.attach=	fsattach,
.open=	fsopen,
.read=	fsread,
.stat=	fsstat,
.walk=	fswalk,
.destroyfid=	fsclunk
};

void
main(int argc, char **argv)
{
	char *service;

	time0 = time(0);
	service = nil;
	ARGBEGIN{
	case 'D':
		chatty9p++;
		break;
	case 's':
		service = EARGF(usage());
		break;
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();
	postmountsrv(&fs, service, argc ? argv[0] : "/n", MAFTER);
	exits(nil);
}
