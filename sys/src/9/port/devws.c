/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"


/*
 * reported times can be translated to a more readable format by
 * using something like:
 * awk '{printf("print(\"%s: %s times; %s us worst; %s ws total\");\nsrc(%s)\n",
 *	 $1, $3, $4, $5, $2); }'  | acid ../k10/9k8cpu
 * on the wsdata file, after doing a sort +2nr on it.
 */

enum{
	WSdirqid,
	WSdataqid,
	WSctlqid,
};

Dirtab Wstab[]={
	".",		{WSdirqid, 0, QTDIR},0,	DMDIR|0550,
	"wsdata",	{WSdataqid},		0,	0600,
	"wsctl",	{WSctlqid},		0,	0600,
};


/*
 * waitstats functions are in taslock.c, because they use Locks but
 * callers in taslock.c must not call them to avoid
 * a loop.
 * This is only the user interface.
 */

static char*
collect(void)
{
	extern Lock waitstatslk;
	char *buf, *s;
	int i, n;
	static char *wname[] = {
	[WSlock] "lock",
	[WSqlock] "qlock",
	[WSslock] "slock",
	};

	n = waitstats.npcs * (strlen("slock") + 1 + 19 * 3 + 1) + 1;
	buf = smalloc(n);
	s = buf;
	lock(&waitstatslk);
	for(i = 0; i < NWstats; i++)
		if(waitstats.pcs[i] != 0)
			s = seprint(s, buf+n, "%s %#llux %d %#llud %#llud\n",
				wname[waitstats.type[i]],
				waitstats.pcs[i], waitstats.ns[i], waitstats.wait[i],
				waitstats.total[i]);
	unlock(&waitstatslk);
	if(s == buf + n)
		print("collect: fix devws.c, buffer was too short");
	return buf;
}

static Chan*
wsattach(char *spec)
{
	return devattach('W', spec);
}

static Walkqid*
wswalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, Wstab, nelem(Wstab), devgen);
}

static int32_t
wsstat(Chan *c, uint8_t *db, int32_t n)
{
	return devstat(c, db, n, Wstab, nelem(Wstab), devgen);
}

static Chan*
wsopen(Chan *c, int omode)
{
	if(c->qid.type & QTDIR){
		if(omode != OREAD)
			error(Eperm);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	c->aux = nil;
	if(c->qid.path == WSdataqid)
		c->aux = collect();
	return c;
}

static void
wsclose(Chan *c)
{
	free(c->aux);
}

static int32_t
wsread(Chan *c, void *va, int32_t n, int64_t off)
{

	switch((int)c->qid.path){
	case WSdirqid:
		n = devdirread(c, va, n, Wstab, nelem(Wstab), devgen);
		break;
	case WSdataqid:
		n = readstr(off, va, n, c->aux);
		break;
	default:
		n = 0;
	}
	return n;
}

static int32_t
wswrite(Chan *c, void *a, int32_t n, int64_t m)
{
	char *buf;

	switch((int)(c->qid.path)){
	case WSctlqid:
		buf = smalloc(n + 1);
		memmove(buf, a, n);
		buf[n] = 0;
		if(n > 0 && buf[n-1] == '\n')
			buf[n-1] = 0;
		if(strcmp(buf, "clear") == 0){
			lockstats.locks = lockstats.glare = lockstats.inglare = 0;
			qlockstats.qlock = qlockstats.qlockq = 0;
			clearwaitstats();
		}else if(strcmp(buf, "start") == 0)
			startwaitstats(1);
		else if(strcmp(buf, "stop") == 0)
			startwaitstats(0);
		else{
			free(buf);
			error(Ebadctl);
		}
		free(buf);
		break;
	default:
		error(Ebadusefd);
	}
	return n;
}

Dev wsdevtab = {
	'W',
	"waitstats",

	devreset,
	devinit,
	devshutdown,
	wsattach,
	wswalk,
	wsstat,
	wsopen,
	devcreate,
	wsclose,
	wsread,
	devbread,
	wswrite,
	devbwrite,
	devremove,
	devwstat,
};
