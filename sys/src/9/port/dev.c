#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

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

	for(i = 0; devtab[i] != nil; i++) {
		if(devtab[i]->dc == c)
			return i;
	}
	if(user == 0)
		panic("devno %C %#ux", c, c);

	return -1;
}

void
devdir(Chan *c, Qid qid, char *n, vlong length, char *user, long perm, Dir *db)
{
	db->name = n;
	if(c->flag&CMSG)
		qid.type |= QTMOUNT;
	db->qid = qid;
	db->type = devtab[c->type]->dc;
	db->dev = c->dev;
	db->mode = perm;
	db->mode |= qid.type << 24;
	db->atime = seconds();
	db->mtime = kerndate;
	db->length = length;
	db->uid = user;
	db->gid = eve;
	db->muid = user;
}

/*
 * (here, Devgen is the prototype; devgen is the function in dev.c.)
 * 
 * a Devgen is expected to return the directory entry for ".."
 * if you pass it s==DEVDOTDOT (-1).  otherwise...
 * 
 * there are two contradictory rules.
 * 
 * (i) if c is a directory, a Devgen is expected to list its children
 * as you iterate s.
 * 
 * (ii) whether or not c is a directory, a Devgen is expected to list
 * its siblings as you iterate s.
 * 
 * devgen always returns the list of children in the root
 * directory.  thus it follows (i) when c is the root and (ii) otherwise.
 * many other Devgens follow (i) when c is a directory and (ii) otherwise.
 * 
 * devwalk assumes (i).  it knows that devgen breaks (i)
 * for children that are themselves directories, and explicitly catches them.
 * 
 * devstat assumes (ii).  if the Devgen in question follows (i)
 * for this particular c, devstat will not find the necessary info.
 * with our particular Devgen functions, this happens only for
 * directories, so devstat makes something up, assuming
 * c->name, c->qid, eve, DMDIR|0555.
 * 
 * devdirread assumes (i).  the callers have to make sure
 * that the Devgen satisfies (i) for the chan being read.
 */
/*
 * the zeroth element of the table MUST be the directory itself for ..
*/
int
devgen(Chan *c, char *name, Dirtab *tab, int ntab, int i, Dir *dp)
{
	if(tab == 0)
		return -1;
	if(i == DEVDOTDOT){
		/* nothing */
	}else if(name){
		for(i=1; i<ntab; i++)
			if(strcmp(tab[i].name, name) == 0)
				break;
		if(i==ntab)
			return -1;
		tab += i;
	}else{
		/* skip over the first element, that for . itself */
		i++;
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

void
devshutdown(void)
{
}

Chan*
devattach(int tc, char *spec)
{
	int n;
	Chan *c;
	char *buf;

	c = newchan();
	mkqid(&c->qid, 0, 0, QTDIR);
	c->type = devno(tc, 0);
	if(spec == nil)
		spec = "";
	n = 1+UTFmax+strlen(spec)+1;
	buf = smalloc(n);
	snprint(buf, n, "#%C%s", tc, spec);
	c->path = newpath(buf);
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
	nc->umh = nil;
	nc->aux = c->aux;
	nc->mqid = c->mqid;
	nc->mcp = c->mcp;
	return nc;
}

Walkqid*
devwalk(Chan *c, Chan *nc, char **name, int nname, Dirtab *tab, int ntab, Devgen *gen)
{
	int i, j, alloc;
	Walkqid *wq;
	char *n;
	Dir dir;

	if(nname > 0)
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
		if(!(nc->qid.type&QTDIR)){
			if(j==0)
				error(Enotdir);
			goto Done;
		}
		n = name[j];
		if(strcmp(n, ".") == 0){
    Accept:
			wq->qid[wq->nqid++] = nc->qid;
			continue;
		}
		if(strcmp(n, "..") == 0){
			if((*gen)(nc, nil, tab, ntab, DEVDOTDOT, &dir) != 1){
				print("devgen walk .. in dev%s %llux broken\n",
					devtab[nc->type]->name, nc->qid.path);
				error("broken devgen");
			}
			nc->qid = dir.qid;
			goto Accept;
		}
		/*
		 * Ugly problem: If we're using devgen, make sure we're
		 * walking the directory itself, represented by the first
		 * entry in the table, and not trying to step into a sub-
		 * directory of the table, e.g. /net/net. Devgen itself
		 * should take care of the problem, but it doesn't have
		 * the necessary information (that we're doing a walk).
		 */
		if(gen==devgen && nc->qid.path!=tab[0].qid.path)
			goto Notfound;
		for(i=0;; i++) {
			switch((*gen)(nc, n, tab, ntab, i, &dir)){
			case -1:
			Notfound:
				if(j == 0)
					error(Enonexist);
				kstrcpy(up->errstr, Enonexist, ERRMAX);
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

	for(i=0;; i++){
		switch((*gen)(c, nil, tab, ntab, i, &dir)){
		case -1:
			if(c->qid.type & QTDIR){
				if(c->path == nil)
					elem = "???";
				else if(strcmp(c->path->s, "/") == 0)
					elem = "/";
				else
					for(elem=p=c->path->s; *p; p++)
						if(*p == '/')
							elem = p+1;
				devdir(c, c->qid, elem, 0, eve, DMDIR|0555, &dir);
				n = convD2M(&dir, db, n);
				if(n == 0)
					error(Ebadarg);
				return n;
			}

			error(Enonexist);
		case 0:
			break;
		case 1:
			if(c->qid.path == dir.qid.path) {
				if(c->flag&CMSG)
					dir.mode |= DMMOUNT;
				n = convD2M(&dir, db, n);
				if(n == 0)
					error(Ebadarg);
				return n;
			}
			break;
		}
	}
}

long
devdirread(Chan *c, char *d, long n, Dirtab *tab, int ntab, Devgen *gen)
{
	long m, dsz;
	Dir dir;

	for(m=0; m<n; c->dri++) {
		switch((*gen)(c, nil, tab, ntab, c->dri, &dir)){
		case -1:
			return m;

		case 0:
			break;

		case 1:
			dsz = convD2M(&dir, (uchar*)d, n-m);
			if(dsz <= BIT16SZ){	/* <= not < because this isn't stat; read is stuck */
				if(m == 0)
					error(Eshort);
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

	for(i=0;; i++) {
		switch((*gen)(c, nil, tab, ntab, i, &dir)){
		case -1:
			goto Return;
		case 0:
			break;
		case 1:
			if(c->qid.path == dir.qid.path) {
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
devbread(Chan *c, long n, ulong offset)
{
	Block *bp;

	bp = allocb(n);
	if(bp == 0)
		error(Enomem);
	if(waserror()) {
		freeb(bp);
		nexterror();
	}
	bp->wp += devtab[c->type]->read(c, bp->wp, n, offset);
	poperror();
	return bp;
}

long
devbwrite(Chan *c, Block *bp, ulong offset)
{
	long n;

	if(waserror()) {
		freeb(bp);
		nexterror();
	}
	n = devtab[c->type]->write(c, bp->rp, BLEN(bp), offset);
	poperror();
	freeb(bp);

	return n;
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

void
devpower(int)
{
	error(Eperm);
}

int
devconfig(int, char *, DevConf *)
{
	error(Eperm);
	return 0;
}
