#include	<u.h>
#include	<libc.h>
#include	<fcall.h>
#include	"compat.h"
#include	"error.h"

extern ulong	kerndate;

void
mkqid(Qid *q, vlong path, ulong vers, int type)
{
	q->type = type;
	q->vers = vers;
	q->path = path;
}

int
devno(int c, int user)
{
	int i;

	for(i = 0; devtab[i] != nil; i++){
		if(devtab[i]->dc == c)
			return i;
	}
	if(user == 0)
		panic("devno %C 0x%ux", c, c);

	return -1;
}

void
devdir(Chan *c, Qid qid, char *n, vlong length, char *user, long perm, Dir *db)
{
	db->name = n;
	db->qid = qid;
	db->type = devtab[c->type]->dc;
	db->dev = c->dev;
	db->mode = (qid.type << 24) | perm;
	db->atime = seconds();
	db->mtime = kerndate;
	db->length = length;
	db->uid = user;
	db->gid = eve;
	db->muid = user;
}

/*
 * the zeroth element of the table MUST be the directory itself for ..
*/
int
devgen(Chan *c, Dirtab *tab, int ntab, int i, Dir *dp)
{
	if(tab == 0)
		return -1;
	if(i != DEVDOTDOT){
		i++; /* skip first element for . itself */
		if(i >= ntab)
			return -1;
		tab += i;
	}
	devdir(c, tab->qid, tab->name, tab->length, eve, tab->perm, dp);
	return 1;
}

void
devreset(void)
{
}

void
devinit(void)
{
}

Chan*
devattach(int tc, char *spec)
{
	Chan *c;
	char *buf;

	c = newchan();
	mkqid(&c->qid, 0, 0, QTDIR);
	c->type = devno(tc, 0);
	if(spec == nil)
		spec = "";
	buf = smalloc(4+strlen(spec)+1);
	sprint(buf, "#%C%s", tc, spec);
	c->name = newcname(buf);
	free(buf);
	return c;
}


Chan*
devclone(Chan *c)
{
	Chan *nc;

	if(c->flag & COPEN)
		panic("clone of open file type %C\n", devtab[c->type]->dc);

	nc = newchan();

	nc->type = c->type;
	nc->dev = c->dev;
	nc->mode = c->mode;
	nc->qid = c->qid;
	nc->offset = c->offset;
	nc->aux = c->aux;
	return nc;
}

Walkqid*
devwalk(Chan *c, Chan *nc, char **name, int nname, Dirtab *tab, int ntab, Devgen *gen)
{
	int i, j, alloc;
	Walkqid *wq;
	char *n;
	Dir dir;

	isdir(c);

	alloc = 0;
	wq = smalloc(sizeof(Walkqid)+(nname-1)*sizeof(Qid));
	if(waserror()){
		if(alloc && wq->clone!=nil)
			cclose(wq->clone);
		free(wq);
		return nil;
	}
	if(nc == nil){
		nc = devclone(c);
		nc->type = 0;	/* device doesn't know about this channel yet */
		alloc = 1;
	}
	wq->clone = nc;

	for(j=0; j<nname; j++){
		isdir(nc);
		n = name[j];
		if(strcmp(n, ".") == 0){
    Accept:
			wq->qid[wq->nqid++] = nc->qid;
			continue;
		}
		if(strcmp(n, "..") == 0){
			(*gen)(nc, tab, ntab, DEVDOTDOT, &dir);
			nc->qid = dir.qid;
			goto Accept;
		}
		for(i=0;; i++){
			switch((*gen)(nc, tab, ntab, i, &dir)){
			case -1:
				if(j == 0)
					error(Enonexist);
				strncpy(up->error, Enonexist, ERRMAX);
				goto Done;
			case 0:
				continue;
			case 1:
				if(strcmp(n, dir.name) == 0){
					nc->qid = dir.qid;
					goto Accept;
				}
				continue;
			}
		}
	}
	/*
	 * We processed at least one name, so will return some data.
	 * If we didn't process all nname entries succesfully, we drop
	 * the cloned channel and return just the Qids of the walks.
	 */
Done:
	poperror();
	if(wq->nqid < nname){
		if(alloc)
			cclose(wq->clone);
		wq->clone = nil;
	}else if(wq->clone){
		/* attach cloned channel to same device */
		wq->clone->type = c->type;
	}
	return wq;
}

int
devstat(Chan *c, uchar *db, int n, Dirtab *tab, int ntab, Devgen *gen)
{
	int i;
	Dir dir;
	char *p, *elem;

	for(i=0;; i++)
		switch((*gen)(c, tab, ntab, i, &dir)){
		case -1:
			if(c->qid.type & QTDIR){
				if(c->name == nil)
					elem = "???";
				else if(strcmp(c->name->s, "/") == 0)
					elem = "/";
				else
					for(elem=p=c->name->s; *p; p++)
						if(*p == '/')
							elem = p+1;
				devdir(c, c->qid, elem, 0, eve, DMDIR|0555, &dir);
				return convD2M(&dir, db, n);
			}

			error(Enonexist);
		case 0:
			break;
		case 1:
			if(c->qid.path == dir.qid.path){
				return convD2M(&dir, db, n);
			}
			break;
		}
}

long
devdirread(Chan *c, char *d, long n, Dirtab *tab, int ntab, Devgen *gen)
{
	long k, m, dsz;
	struct{
		Dir;
		char slop[100];
	}dir;

	k = c->offset;
	for(m=0; m<n; k++){
		switch((*gen)(c, tab, ntab, k, &dir)){
		case -1:
			return m;

		case 0:
			c->offset++;	/* BUG??? (was DIRLEN: skip entry) */
			break;

		case 1:
			dsz = convD2M(&dir, (uchar*)d, n-m);
			if(dsz <= BIT16SZ){	/* <= not < because this isn't stat; read is stuck */
				if(m == 0)
					return -1;
				return m;
			}
			m += dsz;
			d += dsz;
			break;
		}
	}

	return m;
}

/*
 * error(Eperm) if open permission not granted for up->user.
 */
void
devpermcheck(char *fileuid, ulong perm, int omode)
{
	ulong t;
	static int access[] = { 0400, 0200, 0600, 0100 };

	if(strcmp(up->user, fileuid) == 0)
		perm <<= 0;
	else
	if(strcmp(up->user, eve) == 0)
		perm <<= 3;
	else
		perm <<= 6;

	t = access[omode&3];
	if((t&perm) != t)
		error(Eperm);
}

Chan*
devopen(Chan *c, int omode, Dirtab *tab, int ntab, Devgen *gen)
{
	int i;
	Dir dir;

	for(i=0;; i++){
		switch((*gen)(c, tab, ntab, i, &dir)){
		case -1:
			goto Return;
		case 0:
			break;
		case 1:
			if(c->qid.path == dir.qid.path){
				devpermcheck(dir.uid, dir.mode, omode);
				goto Return;
			}
			break;
		}
	}
Return:
	c->offset = 0;
	if((c->qid.type&QTDIR) && omode!=OREAD)
		error(Eperm);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	return c;
}

void
devcreate(Chan*, char*, int, ulong)
{
	error(Eperm);
}

Block*
devbread(Chan *, long, ulong)
{
	panic("no block read");
	return nil;
}

long
devbwrite(Chan *, Block *, ulong)
{
	panic("no block write");
	return 0;
}

void
devremove(Chan*)
{
	error(Eperm);
}

int
devwstat(Chan*, uchar*, int)
{
	error(Eperm);
	return 0;
}
