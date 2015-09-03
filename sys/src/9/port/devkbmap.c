/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *  keyboard map
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

enum{
	Qdir,
	Qdata,
};
Dirtab kbmaptab[]={
	".",		{Qdir, 0, QTDIR},	0,	0555,
	"kbmap",	{Qdata, 0},		0,	0600,
};
#define	NKBFILE	sizeof(kbmaptab)/sizeof(kbmaptab[0])

#define	KBLINELEN	(3*NUMSIZE+1)	/* t code val\n */

static Chan *
kbmapattach(char *spec)
{
	return devattach(L'κ', spec);
}

static Walkqid*
kbmapwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, kbmaptab, NKBFILE, devgen);
}

static int
kbmapstat(Chan *c, unsigned char *dp, int n)
{
	return devstat(c, dp, n, kbmaptab, NKBFILE, devgen);
}

static Chan*
kbmapopen(Chan *c, int omode)
{
	if(!iseve())
		error(Eperm);
	return devopen(c, omode, kbmaptab, NKBFILE, devgen);
}

static void
kbmapclose(Chan *c)
{
	if(c->aux){
		free(c->aux);
		c->aux = nil;
	}
}

static int32_t
kbmapread(Chan *c, void *a, int32_t n, int64_t offset)
{
	char *bp;
	char tmp[KBLINELEN+1];
	int t, sc;
	Rune r;

	if(c->qid.type == QTDIR)
		return devdirread(c, a, n, kbmaptab, NKBFILE, devgen);

	switch((int)(c->qid.path)){
	case Qdata:
		if(kbdgetmap(offset/KBLINELEN, &t, &sc, &r)) {
			bp = tmp;
			bp += readnum(0, bp, NUMSIZE, t, NUMSIZE);
			bp += readnum(0, bp, NUMSIZE, sc, NUMSIZE);
			bp += readnum(0, bp, NUMSIZE, r, NUMSIZE);
			*bp++ = '\n';
			*bp = 0;
			n = readstr(offset%KBLINELEN, a, n, tmp);
		} else
			n = 0;
		break;
	default:
		n=0;
		break;
	}
	return n;
}

static int32_t
kbmapwrite(Chan *c, void *a, int32_t n, int64_t nn)
{
	char line[100], *lp, *b;
	int key, m, l;
	Rune r;

	if(c->qid.type == QTDIR)
		error(Eperm);

	switch((int)(c->qid.path)){
	case Qdata:
		b = a;
		l = n;
		lp = line;
		if(c->aux){
			strcpy(line, c->aux);
			lp = line+strlen(line);
			free(c->aux);
			c->aux = nil;
		}
		while(--l >= 0) {
			*lp++  = *b++;
			if(lp[-1] == '\n' || lp == &line[sizeof(line)-1]) {
				*lp = 0;
				if(*line == 0)
					error(Ebadarg);
				if(*line == '\n' || *line == '#'){
					lp = line;
					continue;
				}
				lp = line;
				while(*lp == ' ' || *lp == '\t')
					lp++;
				m = strtoul(line, &lp, 0);
				key = strtoul(lp, &lp, 0);
				while(*lp == ' ' || *lp == '\t')
					lp++;
				r = 0;
				if(*lp == '\'' && lp[1])
					chartorune(&r, lp+1);
				else if(*lp == '^' && lp[1]){
					chartorune(&r, lp+1);
					if(0x40 <= r && r < 0x60)
						r -= 0x40;
					else
						error(Ebadarg);
				}else if(*lp == 'M' && ('1' <= lp[1] && lp[1] <= '5'))
					r = 0xF900+lp[1]-'0';
				else if(*lp>='0' && *lp<='9') /* includes 0x... */
					r = strtoul(lp, &lp, 0);
				else
					error(Ebadarg);
				kbdputmap(m, key, r);
				lp = line;
			}
		}
		if(lp != line){
			l = lp-line;
			c->aux = lp = smalloc(l+1);
			memmove(lp, line, l);
			lp[l] = 0;
		}
		break;
	default:
		error(Ebadusefd);
	}
	return n;
}

Dev kbmapdevtab = {
	L'κ',
	"kbmap",

	devreset,
	devinit,
	devshutdown,
	kbmapattach,
	kbmapwalk,
	kbmapstat,
	kbmapopen,
	devcreate,
	kbmapclose,
	kbmapread,
	devbread,
	kbmapwrite,
	devbwrite,
	devremove,
	devwstat,
};
