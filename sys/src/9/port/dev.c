#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#define	DEVTAB
#include	"devtab.h"

extern ulong	kerndate;

int
devno(int c, int user)
{
	Rune *s;
	int i;

	s = devchar;
	i = 0;
	while(*s){
		if(c == *s)
			return i;
		i++;
		s++;
	}

	if(user)
		return -1;
	panic("devno %C 0x%ux", c, c);
	return 0;
}

void
devdir(Chan *c, Qid qid, char *n, long length, char *user, long perm, Dir *db)
{
	strcpy(db->name, n);
	db->qid = qid;
	db->type = devchar[c->type];
	db->dev = c->dev;
	if(qid.path & CHDIR)
		db->mode = CHDIR|perm;
	else
		db->mode = perm;
	db->atime = seconds();
	db->mtime = kerndate;
	db->hlength = 0;
	db->length = length;
	strncpy(db->uid, user, NAMELEN);
	strncpy(db->gid, eve, NAMELEN);
}

int
devgen(Chan *c, Dirtab *tab, int ntab, int i, Dir *dp)
{
	if(tab==0 || i>=ntab)
		return -1;
	tab += i;
	devdir(c, tab->qid, tab->name, tab->length, eve, tab->perm, dp);
	return 1;
}

Chan *
devattach(int tc, char *spec)
{
	Chan *c;

	USED(spec);
	c = newchan();
	c->qid = (Qid){CHDIR, 0};
	c->type = devno(tc, 0);
	return c;
}

Chan *
devclone(Chan *c, Chan *nc)
{
	if(c->flag & COPEN)
		panic("clone of open file type %C\n", devchar[c->type]);
	if(nc == 0)
		nc = newchan();
	nc->type = c->type;
	nc->dev = c->dev;
	nc->mode = c->mode;
	nc->qid = c->qid;
	nc->offset = c->offset;
	nc->flag = c->flag;
	nc->mnt = c->mnt;
	nc->mountid = c->mountid;
	nc->aux = c->aux;
	nc->mchan = c->mchan;
	nc->mqid = c->mqid;
	return nc;
}

int
devwalk(Chan *c, char *name, Dirtab *tab, int ntab, Devgen *gen)
{
	long i;
	Dir dir;

	isdir(c);
	if(name[0]=='.' && name[1]==0)
		return 1;
	for(i=0;; i++)
		switch((*gen)(c, tab, ntab, i, &dir)){
		case -1:
			strncpy(u->error, Enonexist, NAMELEN);
			return 0;
		case 0:
			continue;
		case 1:
			if(strcmp(name, dir.name) == 0){
				c->qid = dir.qid;
				return 1;
			}
			continue;
		}
	return 1;	/* not reached */
}

void
devstat(Chan *c, char *db, Dirtab *tab, int ntab, Devgen *gen)
{
	int i;
	Dir dir;

	for(i=0;; i++)
		switch((*gen)(c, tab, ntab, i, &dir)){
		case -1:
			/*
			 *  given a channel, we cannot derive the directory name
			 *  that the channel was generated from since it was lost
			 *  by namec.
			 */
			if(c->qid.path & CHDIR){
				devdir(c, c->qid, ".", 0L, eve, CHDIR|0700, &dir);
				convD2M(&dir, db);
				return;
			}
			print("%s %s: devstat %C %lux\n", u->p->text, u->p->user,
							devchar[c->type], c->qid.path);
			error(Enonexist);
		case 0:
			break;
		case 1:
			if(eqqid(c->qid, dir.qid)){
				convD2M(&dir, db);
				return;
			}
			break;
		}
}

long
devdirread(Chan *c, char *d, long n, Dirtab *tab, int ntab, Devgen *gen)
{
	long k, m;
	Dir dir;

	k = c->offset/DIRLEN;
	for(m=0; m<n; k++)
		switch((*gen)(c, tab, ntab, k, &dir)){
		case -1:
			return m;

		case 0:
			c->offset += DIRLEN;
			break;

		case 1:
			convD2M(&dir, d);
			m += DIRLEN;
			d += DIRLEN;
			break;
		}
	return m;
}

Chan *
devopen(Chan *c, int omode, Dirtab *tab, int ntab, Devgen *gen)
{
	int i;
	Dir dir;
	ulong t, mode;
	static int access[] = { 0400, 0200, 0600, 0100 };

	for(i=0;; i++)
		switch((*gen)(c, tab, ntab, i, &dir)){
		case -1:
			goto Return;
		case 0:
			break;
		case 1:
			if(eqqid(c->qid, dir.qid)) {
				if(strcmp(u->p->user, dir.uid) == 0)	/* User */
					mode = dir.mode;
				else if(strcmp(u->p->user, eve) == 0)	/* eve is group */
					mode = dir.mode<<3;
				else
					mode = dir.mode<<6;		/* Other */

				t = access[omode&3];
				if((t & mode) == t)
					goto Return;
				error(Eperm);
			}
			break;
		}
    Return:
	c->offset = 0;
	if((c->qid.path&CHDIR) && omode!=OREAD)
		error(Eperm);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	return c;
}
