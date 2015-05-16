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

extern uint32_t	kerndate;

void
mkqid(Qid *q, int64_t path, uint32_t vers, int type)
{
	print_func_entry();
	q->type = type;
	q->vers = vers;
	q->path = path;
	print_func_exit();
}

void
devdir(Chan *c, Qid qid, char *n, int64_t length, char *user,
       int32_t perm,
       Dir *db)
{
	print_func_entry();
	db->name = n;
	if(c->flag&CMSG)
		qid.type |= QTMOUNT;
	db->qid = qid;
	/*
	 * When called via devwalk c->dev is nil
	 * until the walk succeeds.
	 */
	if(c->dev != nil)
		db->type = c->dev->dc;
	else
		db->type = -1;
	db->dev = c->devno;
	db->mode = perm;
	db->mode |= qid.type << 24;
	db->atime = seconds();
	db->mtime = kerndate;
	db->length = length;
	db->uid = user;
	db->gid = eve;
	db->muid = user;
	print_func_exit();
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
	print_func_entry();
	if(tab == 0) {
	print_func_exit();
	return -1;
	}
	if(i == DEVDOTDOT){
		/* nothing */
	}else if(name){
		for(i=1; i<ntab; i++)
			if(strcmp(tab[i].name, name) == 0)
				break;
		if(i==ntab) {
			print_func_exit();
			return -1;
		}
		tab += i;
	}else{
		/* skip over the first element, that for . itself */
		i++;
		if(i >= ntab) {
		print_func_exit();
		return -1;
		}
		tab += i;
	}
	devdir(c, tab->qid, tab->name, tab->length, eve, tab->perm, dp);
	print_func_exit();
	return 1;
}

void
devreset(void)
{
	print_func_entry();
	print_func_exit();
}

void
devinit(void)
{
	print_func_entry();
	print_func_exit();
}

void
devshutdown(void)
{
	print_func_entry();
	print_func_exit();
}

Chan*
devattach(int dc, char *spec)
{
	print_func_entry();
	Chan *c;
	char *buf;

	/*
	 * There are no error checks here because
	 * this can only be called from the driver of dc
	 * which pretty much guarantees devtabget will
	 * succeed.
	 */
	c = newchan();
	mkqid(&c->qid, 0, 0, QTDIR);
	c->dev = devtabget(dc, 0);
	if(spec == nil)
		spec = "";
	buf = smalloc(1+UTFmax+strlen(spec)+1);
	sprint(buf, "#%C%s", dc, spec);
	c->path = newpath(buf);
	free(buf);
	print_func_exit();
	return c;
}


Chan*
devclone(Chan *c)
{
	print_func_entry();
	Chan *nc;

	if(c->flag & COPEN){
		panic("devclone: file of type %C already open\n",
			c->dev != nil? c->dev->dc: -1);
	}

	nc = newchan();

	/*
	 * The caller fills dev in if and when necessary.
	nc->dev = nil;					//XDYNXX
	 */
	nc->devno = c->devno;
	nc->mode = c->mode;
	nc->qid = c->qid;
	nc->offset = c->offset;
	nc->umh = nil;
	nc->aux = c->aux;
	nc->mqid = c->mqid;
	nc->mc = c->mc;
	print_func_exit();
	return nc;
}

Walkqid*
devwalk(Chan *c, Chan *nc, char **name, int nname, Dirtab *tab, int ntab,
	Devgen *gen)
{
	Mach *m = machp();
	print_func_entry();
	int i, j, alloc;
	Walkqid *wq;
	char *n;
	Dir dir;

	if(nname > 0)
		isdir(c);

	{ int i; iprint("%d names:", nname); for(i = 0; i < nname; i++) iprint("%s ", name[i]); iprint("\n");}
	alloc = 0;
	wq = smalloc(sizeof(Walkqid)+(nname-1)*sizeof(Qid));
	if(waserror()){
		if(alloc && wq->clone!=nil)
			cclose(wq->clone);
		free(wq);
		print_func_exit();
		return nil;
	}
	if(nc == nil){
		nc = devclone(c);
		/*
		 * nc->dev remains nil for now.		//XDYNX
		 */
		alloc = 1;
	}
	wq->clone = nc;

	for(j=0; j<nname; j++){
		if(!(nc->qid.type & QTDIR)){
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
			/*
			 * Use c->dev->name in the error because
			 * nc->dev should be nil here.
			 */
			if((*gen)(nc, nil, tab, ntab, DEVDOTDOT, &dir) != 1){
				print("devgen walk .. in dev%s %#llux broken\n",
					c->dev->name, nc->qid.path);
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
				kstrcpy(m->externup->errstr, Enonexist, ERRMAX);
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
//what goes here:					//XDYNX
// ->dev must be nil because can't walk an open chan, right?
// what about ref count on dev?
		wq->clone->dev = c->dev;
		//if(wq->clone->dev)			//XDYNX
		//	devtabincr(wq->clone->dev);
	}
	print_func_exit();
	return wq;
}

int32_t
devstat(Chan *c, uint8_t *db, int32_t n, Dirtab *tab, int ntab,
	Devgen *gen)
{
	print_func_entry();
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
				print_func_exit();
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
				print_func_exit();
				return n;
			}
			break;
		}
	}
	print_func_exit();
}

int32_t
devdirread(Chan *c, char *d, int32_t n, Dirtab *tab, int ntab,
	   Devgen *gen)
{
	print_func_entry();
	int32_t m, dsz;
	Dir dir;

	for(m=0; m<n; c->dri++) {
		switch((*gen)(c, nil, tab, ntab, c->dri, &dir)){
		case -1:
			print_func_exit();
			return m;

		case 0:
			break;

		case 1:
			dsz = convD2M(&dir, (uint8_t*)d, n-m);
			if(dsz <= BIT16SZ){	/* <= not < because this isn't stat; read is stuck */
				if(m == 0)
					error(Eshort);
				print_func_exit();
				return m;
			}
			m += dsz;
			d += dsz;
			break;
		}
	}

	print_func_exit();
	return m;
}

/*
 * error(Eperm) if open permission not granted for m->externup->user.
 */
void
devpermcheck(char *fileuid, int perm, int omode)
{
	Mach *m = machp();
	print_func_entry();
	int t;
	static int access[] = { 0400, 0200, 0600, 0100 };

	if(strcmp(m->externup->user, fileuid) == 0)
		perm <<= 0;
	else
	if(strcmp(m->externup->user, eve) == 0)
		perm <<= 3;
	else
		perm <<= 6;

	t = access[omode&3];
	if((t&perm) != t)
		error(Eperm);
	print_func_exit();
}

Chan*
devopen(Chan *c, int omode, Dirtab *tab, int ntab, Devgen *gen)
{
	print_func_entry();
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
	if((c->qid.type & QTDIR) && omode!=OREAD)
		error(Eperm);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	print_func_exit();
	return c;
}

void
devcreate(Chan* c, char* d, int i, int n)
{
	print_func_entry();
	error(Eperm);
	print_func_exit();
}

Block*
devbread(Chan *c, int32_t n, int64_t offset)
{
	Mach *m = machp();
	print_func_entry();
	Block *bp;

	bp = allocb(n);
	if(bp == 0)
		error(Enomem);
	if(waserror()) {
		freeb(bp);
		nexterror();
	}
	bp->wp += c->dev->read(c, bp->wp, n, offset);
	poperror();
	print_func_exit();
	return bp;
}

int32_t
devbwrite(Chan *c, Block *bp, int64_t offset)
{
	Mach *m = machp();
	print_func_entry();
	int32_t n;

	if(waserror()) {
		freeb(bp);
		nexterror();
	}
	n = c->dev->write(c, bp->rp, BLEN(bp), offset);
	poperror();
	freeb(bp);

	print_func_exit();
	return n;
}

void
devremove(Chan* c)
{
	print_func_entry();
	error(Eperm);
	print_func_exit();
}

int32_t
devwstat(Chan* c, uint8_t* i, int32_t n)
{
	print_func_entry();
	error(Eperm);
	print_func_exit();
	return 0;
}

void
devpower(int i)
{
	print_func_entry();
	error(Eperm);
	print_func_exit();
}

int
devconfig(int i, char *c, DevConf *d)
{
	print_func_entry();
	error(Eperm);
	print_func_exit();
	return 0;
}
