#include	"dat.h"
#include	"fns.h"
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

	for(i = 0; devtab[i] != nil; i++) {
		if(devtab[i]->dc == c)
			return i;
	}
	if(user == 0)
		panic("devno %C 0x%ux", c, c);

	return -1;
}

void
devdir(Chan *c, Qid qid, char *n, long length, char *user, long perm, Dir *db)
{
	db->name = n;
	if(c->flag&CMSG)
		qid.type |= QTMOUNT;
	db->qid = qid;
	db->type = devtab[c->type]->dc;
	db->dev = c->dev;
	db->mode = perm | (qid.type << 24);
	db->atime = time(0);
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
devgen(Chan *c, char *name, Dirtab *tab, int ntab, int i, Dir *dp)
{
	USED(name);
	if(tab == 0)
		return -1;
	if(i != DEVDOTDOT){
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
	nc->umh = nil;
	nc->mountid = c->mountid;
	nc->aux = c->aux;
	nc->mqid = c->mqid;
	nc->mcp = c->mcp;
	return nc;
}

Walkqid*
devwalk(Chan *c, Chan *nc, char **name, int nname, Dirtab *tab, int ntab, Devgen *gen)
{
	int i, j;
	volatile int alloc;
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
				p9error(Enotdir);
			goto Done;
		}
		n = name[j];
		if(strcmp(n, ".") == 0){
    Accept:
			wq->qid[wq->nqid++] = nc->qid;
			continue;
		}
		if(strcmp(n, "..") == 0){
			(*gen)(nc, nil, tab, ntab, DEVDOTDOT, &dir);
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
					p9error(Enonexist);
				kstrcpy(up->env->errstr, Enonexist, ERRMAX);
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
		switch((*gen)(c, nil, tab, ntab, i, &dir)){
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
				n = convD2M(&dir, db, n);
				if(n == 0)
					p9error(Ebadarg);
				return n;
			}
			print("%s %s: devstat %C %llux\n",
				up->text, up->env->user,
				devtab[c->type]->dc, c->qid.path);

			p9error(Enonexist);
		case 0:
			break;
		case 1:
			if(c->qid.path == dir.qid.path) {
				if(c->flag&CMSG)
					dir.mode |= DMMOUNT;
				n = convD2M(&dir, db, n);
				if(n == 0)
					p9error(Ebadarg);
				return n;
			}
			break;
		}
}

long
devdirread(Chan *c, char *d, long n, Dirtab *tab, int ntab, Devgen *gen)
{
	long m, dsz;
	struct{
		Dir d;
		char slop[100];	/* TO DO */
	}dir;

	for(m=0; m<n; c->dri++) {
		switch((*gen)(c, nil, tab, ntab, c->dri, &dir.d)){
		case -1:
			return m;

		case 0:
			break;

		case 1:
			dsz = convD2M(&dir.d, (uchar*)d, n-m);
			if(dsz <= BIT16SZ){	/* <= not < because this isn't stat; read is stuck */
				if(m == 0)
					p9error(Eshort);
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
 * p9error(Eperm) if open permission not granted for up->env->user.
 */
void
devpermcheck(char *fileuid, ulong perm, int omode)
{
	ulong t;
	static int access[] = { 0400, 0200, 0600, 0100 };

	if(strcmp(up->env->user, fileuid) == 0)
		perm <<= 0;
	else
	if(strcmp(up->env->user, eve) == 0)
		perm <<= 3;
	else
		perm <<= 6;

	t = access[omode&3];
//	print("devpermcheck fileuid %s user %s eve %s perm %o omode %o t %o t&perm %o\n", fileuid, up->env->user, eve, perm, omode, t, t&perm);
	if((t&perm) != t)
		p9error(Eperm);
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
		p9error(Eperm);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	return c;
}

Block*
devbread(Chan *c, long n, ulong offset)
{
	Block *bp;

	bp = p9allocb(n);
	if(waserror()) {
		p9freeb(bp);
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
		p9freeb(bp);
		nexterror();
	}
	n = devtab[c->type]->write(c, bp->rp, BLEN(bp), offset);
	poperror();
	p9freeb(bp);

	return n;
}

void
devcreate(Chan *c, char *name, int mode, ulong perm)
{
	USED(c);
	USED(name);
	USED(mode);
	USED(perm);
	p9error(Eperm);
}

void
devremove(Chan *c)
{
	USED(c);
	p9error(Eperm);
}

int
devwstat(Chan *c, uchar *dp, int n)
{
	USED(c);
	USED(dp);
	USED(n);
	p9error(Eperm);
	return 0;
}

int
readstr(ulong off, char *buf, ulong n, char *str)
{
	int size;

	size = strlen(str);
	if(off >= size)
		return 0;
	if(off+n > size)
		n = size-off;
	memmove(buf, str+off, n);
	return n;
}

int
readnum(ulong off, char *buf, ulong n, ulong val, int size)
{
	char tmp[64];

	if(size > 64) size = 64;

	snprint(tmp, sizeof(tmp), "%*.0lud ", size, val);
	if(off >= size)
		return 0;
	if(off+n > size)
		n = size-off;
	memmove(buf, tmp+off, n);
	return n;
}

/*
 * check that the name in a wstat is plausible
 */
void
validwstatname(char *name)
{
	validname(name, 0);
	if(strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		p9error(Efilename);
}

Dev*
devbyname(char *name)
{
	int i;

	for(i = 0; devtab[i] != nil; i++)
		if(strcmp(devtab[i]->name, name) == 0)
			return devtab[i];
	return nil;
}
