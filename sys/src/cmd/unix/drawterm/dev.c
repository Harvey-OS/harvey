#include	"lib9.h"
#include	"sys.h"
#include	"error.h"

static ulong	kerndate;

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
devinit(void)
{
}

void
devreset(void)
{
}

void
devcreate(Chan *c, char *name, int mode, ulong perm)
{
	USED(c);
	USED(name);
	USED(mode);
	USED(perm);

	error(Eperm);
}

void
devremove(Chan *c)
{
	USED(c);

	error(Eperm);
}

void
devwstat(Chan *c, char *s)
{
	USED(c);
	USED(s);

	error(Eperm);
}


void
devdir(Chan *c, Qid qid, char *n, long length, char *user, long perm, Dir *db)
{
	strcpy(db->name, n);
	db->qid = qid;
	db->type = devchar[c->type];
	db->dev = c->dev;
	db->mode = perm;
	if(qid.path & CHDIR)
		db->mode |= CHDIR;
	if(c->flag&CMSG)
		db->mode |= CHMOUNT;
	db->atime = time(0);
	db->mtime = kerndate;
	db->length = length;
	memmove(db->uid, user, NAMELEN);
	memmove(db->gid, eve, NAMELEN);
}

int
devgen(Chan *c, Dirtab *tab, int ntab, int i, Dir *dp)
{
	Qid qid;

	if(tab==0 || i>=ntab)
		return -1;
	tab += i;
	qid = tab->qid;	/* avoid unaligned access in digital unix */
	devdir(c, qid, tab->name, tab->length, eve, tab->perm, dp);
	return 1;
}

Chan*
devattach(int tc, char *spec)
{
	Chan *c;
	char buf[NAMELEN+4];

	c = newchan();
	c->qid.path = CHDIR;
	c->qid.vers = 0;

	c->type = devno(tc, 0);
	if(tc != 'M') {
		sprint(buf, "#%C%s", tc, spec);
		c->path = ptenter(&syspt, 0, buf);
	}
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
	nc->u = c->u;
	nc->mchan = c->mchan;
	nc->mqid = c->mqid;
	nc->path = c->path;
	refinc(&nc->path->r);
	return nc;
}

int
devwalk(Chan *c, char *name, Dirtab *tab, int ntab, Devgen *gen)
{
	long i;
	Dir dir;
	Path *op;

	isdir(c);
	if(name[0]=='.' && name[1]==0)
		return 1;
	for(i=0;; i++) {
		switch((*gen)(c, tab, ntab, i, &dir)){
		case -1:
			strncpy(threaderr(), Enonexist, NAMELEN);
			return 0;
		case 0:
			continue;
		case 1:
			if(strcmp(name, dir.name) == 0) {
				c->qid = dir.qid;
				op = c->path;
				c->path = ptenter(&syspt, op, name);
				refdec(&op->r);
				return 1;
			}
			continue;
		}
	}
	return 0;	/* not reached */
}

void
devstat(Chan *c, char *db, Dirtab *tab, int ntab, Devgen *gen)
{
	int i;
	Dir dir;
	Qid cqid, dirqid;

	for(i=0;; i++)
		switch((*gen)(c, tab, ntab, i, &dir)){
		case -1:
			if(c->qid.path & CHDIR){
				devdir(c, c->qid, c->path->elem, i*DIRLEN, eve, CHDIR|0555, &dir);
				convD2M(&dir, db);
				return;
			}
			print("%s: devstat %C %lux\n",
				up->user, devchar[c->type], c->qid.path);
			error(Enonexist);
		case 0:
			break;
		case 1:
			cqid = c->qid;
			dirqid = dir.qid;
			if(eqqid(cqid, dirqid)) {
				if(c->flag&CMSG)
					dir.mode |= CHMOUNT;
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
	for(m=0; m<n; k++) {
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
	}

	return m;
}

Chan *
devopen(Chan *c, int omode, Dirtab *tab, int ntab, Devgen *gen)
{
	int i;
	Dir dir;
	Qid cqid, dirqid;
	ulong t, mode;
	static int access[] = { 0400, 0200, 0600, 0100 };

	for(i=0;; i++) {
		switch((*gen)(c, tab, ntab, i, &dir)){
		case -1:
			goto Return;
		case 0:
			break;
		case 1:
			cqid = c->qid;
			dirqid = dir.qid;
			if(eqqid(cqid, dirqid)) {
				if(strcmp(up->user, dir.uid) == 0)
					mode = dir.mode;
				else
				if(strcmp(up->user, eve) == 0)
					mode = dir.mode<<3;
				else
					mode = dir.mode<<6;

				t = access[omode&3];
				if((t & mode) == t)
					goto Return;
				error(Eperm);
			}
			break;
		}
	}
Return:
	c->offset = 0;
	if((c->qid.path&CHDIR) && omode!=OREAD)
		error(Eperm);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	return c;
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
	bp->wp += devtab[c->type].read(c, bp->wp, n, offset);
	poperror();
	return bp;
}

long
devbwrite(Chan *c, Block *bp, ulong offset)
{
	long n;

	n = devtab[c->type].write(c, bp->rp, BLEN(bp), offset);
	freeb(bp);

	return n;	
}

int
readstr(ulong off, char *buf, ulong n, char *str)
{
	int size;

	size = strlen(str);
	if((int)off >= size)
		return 0;
	if((int)(off+n) > size)
		n = size-off;
	memmove(buf, str+off, n);
	return n;
}

int
openmode(ulong o)
{
	if(o >= (OTRUNC|OCEXEC|ORCLOSE|OEXEC))
		error(Ebadarg);
	o &= ~(OTRUNC|OCEXEC|ORCLOSE);
	if(o > OEXEC)
		error(Ebadarg);
	if(o == OEXEC)
		return OREAD;
	return o;
}
