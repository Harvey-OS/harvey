#include <alef.h>
#include <fcall.h>

/*
 *  This fs presents a 1 level file system.  It contains
 *  two files per console (xxx and xxxctl)
 */

typedef aggr Console;
typedef aggr Fid;
typedef aggr Request;
typedef aggr Reqlist;
typedef adt Fs;

enum
{
	TICKREQLEN=	(3*NAMELEN+CHALLEN+DOMLEN+1),

	/* last 5 bits of qid.path */
	Textern=	0,	/* fake parent of top level */
	Ttopdir,		/* top level directory */
	Tctl,			/* contets of consdir */
	Tdata,			/* ... */

	Bsize=		4096,	/* chars buffered per reader */
	Maxcons=	64,	/* maximum consoles */
	Nhash=		64,	/* Fid hash buckets */
};

#define TYPE(x) (x.path & 0xf)
#define CONS(x) ((x.path >> 4)&0xfff)
#define QID(c, x) (((c)<<4) | (x))

extern int ALEF_qlrendez;

aggr Request
{
	Request	*next;
	Fid	*fid;
	Fs	*fs;
	Fcall	f;
	byte	buf[1];
};

aggr Reqlist
{
	Lock;
	Request	*first;
	Request *last;
};

aggr Fid
{
	Lock;
	Fid	*next;		/* hash list */
	Fid	*cnext;		/* list of Fid's on a console */
	int	fid;
	int	ref;

	int	attached;
	int	open;
	byte	user[NAMELEN];
	Qid	qid;

	Console	*c;

	byte	buf[Bsize];
	byte	*rp;
	byte	*wp;

	Reqlist	r;		/* active read requests */
};

aggr Console
{
	Lock;

	byte	*name;
	byte	*dev;

	int	fd;
	int	cfd;

	Fid	*flist;		/* open fids to broadcast to */
};

adt Fs
{
	Lock;

	int	fd;		/* to kernel mount point */
	Fid	*hash[Nhash];
	Console	*cons[Maxcons];
	int	ncons;

	void	console(*Fs, byte*, byte*);
	Fs*	mount(byte*);

intern	void	reader(*Fs, Console*);
intern	void	run(*Fs, int*);
intern	Fid*	getfid(*Fs, int);
intern	void	putfid(*Fs, Fid*);
intern	int	dirgen(*Fs, Qid, int, Dir*, byte*);
intern	void	reply(*Fs, Request*, byte*);
	void	kick(*Fs, Fid*);

	void	nop(*Fs, Request*, Fid*);
	void	session(*Fs, Request*, Fid*);
	void	flush(*Fs, Request*, Fid*);
	void	attach(*Fs, Request*, Fid*);
	void	clone(*Fs, Request*, Fid*);
	void	walk(*Fs, Request*, Fid*);
	void	clwalk(*Fs, Request*, Fid*);
	void	open(*Fs, Request*, Fid*);
	void	create(*Fs, Request*, Fid*);
	void	read(*Fs, Request*, Fid*);
	void	write(*Fs, Request*, Fid*);
	void	clunk(*Fs, Request*, Fid*);
	void	remove(*Fs, Request*, Fid*);
	void	stat(*Fs, Request*, Fid*);
	void	wstat(*Fs, Request*, Fid*);
};


void 	(*fcall[])(*Fs, Request*, Fid*) =
{
	[Tflush]	.Fs.flush,
	[Tsession]	.Fs.session,
	[Tnop]		.Fs.nop,
	[Tattach]	.Fs.attach,
	[Tclone]	.Fs.clone,
	[Twalk]		.Fs.walk,
	[Topen]		.Fs.open,
	[Tcreate]	.Fs.create,
	[Tread]		.Fs.read,
	[Twrite]	.Fs.write,
	[Tclunk]	.Fs.clunk,
	[Tremove]	.Fs.remove,
	[Tstat]		.Fs.stat,
	[Twstat]	.Fs.wstat

};

byte Eperm[]   = "permission denied";
byte Eexist[]  = "file does not exist";
byte Enotdir[] = "not a directory";
byte Eisopen[] = "file already open";
byte Ebadoffset[] = "bad read/write offset";
byte Ebadcount[] = "bad read/write count";
byte Enofid[] = "no such fid";

int debug;

/*
 *  any request that can get queued for a delayed reply
 */
intern Request*
allocreq(Fs *fs, int bufsize)
{
	Request *r;

	r = malloc(sizeof(Request)+bufsize);
	r->fs = fs;
	r->next = nil;
	return r;
}

/*
 *  for maintaining lists of requests
 */
intern void
addreq(Reqlist *l, Request *r)
{
	l->lock();
	if(l->first == nil)
		l->first = r;
	else
		l->last->next = r;
	l->last = r;
	r->next = nil;
	l->unlock();
}

/*
 *  remove the first request from a list of requests
 */
intern Request*
remreq(Reqlist *l)
{
	Request *r;

	l->lock();
	r = l->first;
	if(r != nil)
		l->first = r->next;
	l->unlock();
	return r;
}

/*
 *  remove a request with the given tag from a list of requests
 */
intern Request*
remtag(Reqlist *l, int tag)
{
	Request *or, **ll;

	l->lock();
	ll = &l->first;
	for(or = *ll; or; or = or->next){
		if(or->f.tag == tag){
			*ll = or->next;
			l->unlock();
			return or;
		}
		ll = &or->next;
	}
	l->unlock();
	return nil;
}

intern Qid
parentqid(Qid q)
{
	if(q.path & CHDIR)
		return (Qid)(CHDIR|QID(0, Textern), 0);
	else
		return (Qid)(CHDIR|QID(0, Ttopdir), 0);
}

int
Fs.dirgen(Fs *fs, Qid parent, int i, Dir *d, byte *buf)
{
	byte name[NAMELEN];
	byte *p;
	int xcons;

	strcpy(d->uid, "network");
	strcpy(d->gid, "network");
	d->length = 0;
	d->hlength = 0;
	d->atime = time();
	d->mtime = d->atime;
	d->type = 'C';
	d->dev = '0';

	switch(TYPE(parent)){
	case Textern:
		if(i != 0)
			return -1;
		p = "consoles";
		d->mode = CHDIR|0555;
		d->qid = (Qid)(CHDIR|QID(0, Ttopdir), 0);
		break;
	case Ttopdir:
		xcons = i>>1;
		if(xcons >= fs->ncons)
			return -1;
		p = fs->cons[xcons]->name;
		if(i&1){
			snprint(name, NAMELEN, "%sctl", p);
			p = name;
			d->qid = (Qid)(QID(xcons, Tctl), 0);
		} else
			d->qid = (Qid)(QID(xcons, Tdata), 0);
		d->mode = 0666;
		break;
	default:
		return -1;
	}
	memset(d->name, 0, NAMELEN);
	strcpy(d->name, p);
	if(buf != nil)
		convD2M(d, buf);
	return 1;
}

/*
 *  mount the user interface and start a request processor
 */
Fs*
Fs.mount(byte *mntpt)
{
	Fs *fs;
	int pfd[2], srv;
	byte trbuf[TICKREQLEN], buf[32];
	Dir d;
	int n;

	alloc fs;

	if(pipe(pfd) < 0)
		fatal("opening pipe: %r");

	/* start up the file system process */
	proc fs->run(pfd);

	/* Typically mounted before /srv exists */
	if(dirstat("#s/consoles", &d) < 0){
		srv = create("#s/consoles", OWRITE, 0666);
		if(srv < 0)
			fatal("post: %r");

		n = sprint(buf, "%d", pfd[1]);
		if(write(srv, buf, n) < 0)
			fatal("write srv: %r");

		close(srv);
	}

	if(fsession(pfd[1], trbuf) >= 0)
		mount(pfd[1], mntpt, MBEFORE, "");
	close(pfd[1]);
	return fs;
}

/*
 *  reopen a console
 */
intern int
reopen(Console *c)
{
	byte buf[128];

	close(c->fd);
	close(c->cfd);
	c->cfd = -1;

	c->fd = open(c->dev, ORDWR);
	if(c->fd < 0)
		return -1;

	snprint(buf, sizeof(buf), "%sctl", c->dev);
	c->cfd = open(buf, ORDWR);
	return 0;
}


/*
 *  create a console interface
 */
void
Fs.console(Fs* fs, byte *name, byte *dev)
{
	Console *c;

	rescue {
		fprint(2, "consoles: can't open %s %s: %r", name, dev);
		fs->ncons--;
		free(c);
		return;
	}

	if(fs->ncons >= Maxcons)
		fatal("too many consoles, too little time");

	alloc c;
	check c != nil;
	fs->cons[fs->ncons++] = c;
	c->name = name;
	c->dev = dev;
	c->fd = -1;
	c->cfd = -1;
	if(reopen(c) < 0)
		raise;

	proc fs->reader(c);
}

/*
 *  buffer data from console to a client.
 *  circular q with writer able to catch up to reader.
 *  the reader may miss data but always sees an in order sequence.
 */
intern void
fromconsole(Fid *f, byte *p, int n)
{
	byte *rp, *wp, *ep;
	int pass;

	f->lock();
	rp = f->rp;
	wp = f->wp;
	ep = f->buf + sizeof(f->buf);
	pass = 0;
	while(n--){
		*wp++ = *p++;
		if(wp >= ep)
			wp = f->buf;
		if(rp == wp)
			pass = 1;
	}
	f->wp = wp;

	/*  we overtook the read pointer, push it up so readers always
	 *  see the tail of what was written
	 */
	if(pass){
		wp++;
		if(wp >= ep)
			f->rp = f->buf;
		else
			f->rp = wp;
	}
	f->unlock();
}

/*
 *  broadcast a list of members to all listeners
 */
void
bcastmembers(Fs *fs, Console *c, byte *msg, Fid *f)
{
	int n;
	Fid *fl;
	byte buf[512];

	sprint(buf, "[%s%s", msg, f->user);
	for(fl = c->flist; fl != nil && strlen(buf) + NAMELEN + 8 < sizeof(buf); fl = fl->cnext){
		if(f == fl)
			continue;
		strcat(buf, ", ");
		strcat(buf, fl->user);
	}
	strcat(buf, "]\n");

	n = strlen(buf);
	for(fl = c->flist; fl; fl = fl->cnext){
		fromconsole(fl, buf, n);
		fs->kick(fl);
	}
}

/*
 *  a process to read console output and broadcast it (one per console)
 */
void
Fs.reader(Fs *fs, Console *c)
{
	int n;
	Fid *fl;
	byte buf[1024];

	for(;;){
		n = read(c->fd, buf, sizeof(buf));
		if(n <= 0){
			sleep(10000);
			reopen(c);
			continue;
		}
		c->lock();
		for(fl = c->flist; fl; fl = fl->cnext){
			fromconsole(fl, buf, n);
			fs->kick(fl);
		}
		c->unlock();
	}
}

/*
 *  a request processor (one per Fs)
 */
void
Fs.run(Fs* fs, int *pfd)
{
	int n, t;
	Request *r;
	Fid *f;

	fs->fd = pfd[0];
	for(;;){
		r = allocreq(fs, MAXRPC);
		n = read9p(fs->fd, r->buf, MAXRPC);
		if(n <= 0)
			fatal("unmounted");

		if(convM2S(r->buf, &r->f, n) == 0){
			fprint(2, "can't convert %ux %ux %ux\n", r->buf[0],
				r->buf[1], r->buf[2]);
			free(r);
			continue;
		}


		f = fs->getfid(r->f.fid);
		r->fid = f;
		if(debug)
			fprint(2, "%F path %lux\n", &r->f, f->qid.path);

		t = r->f.type;
		r->f.type++;
		(*fcall[t])(fs, r, f);
	}
}

Fid*
Fs.getfid(Fs *fs, int fid)
{
	Fid *f, *nf;

	fs->lock();
	for(f = fs->hash[fid%Nhash]; f; f = f->next){
		if(f->fid == fid){
			f->ref++;
			fs->unlock();
			return f;
		}
	}

	alloc nf;
	check nf != nil;
	memset(nf, 0, sizeof(*nf));
	nf->next = fs->hash[fid%Nhash];
	fs->hash[fid%Nhash] = nf;
	nf->fid = fid;
	nf->ref = 1;
	nf->wp = nf->buf;
	nf->rp = nf->wp;
	fs->unlock();
	return nf;
}

void
Fs.putfid(Fs *fs, Fid *f)
{
	Fid **l, *nf;

	fs->lock();
	if(--f->ref > 0){
		fs->unlock();
		return;
	}
	for(l = &fs->hash[f->fid%Nhash]; nf = *l; l = &nf->next)
		if(nf == f){
			*l = f->next;
			break;
		}
	fs->unlock();
	free(f);
}


void
Fs.nop(Fs *fs, Request *r, Fid*)
{
	fs->reply(r, nil);
}

void
Fs.session(Fs *fs, Request *r, Fid*)
{
	memset(r->f.authid, 0, sizeof(r->f.authid));
	memset(r->f.authdom, 0, sizeof(r->f.authdom));
	memset(r->f.chal, 0, sizeof(r->f.chal));

	fs->reply(r, nil);
}

void
Fs.flush(Fs *fs, Request *r, Fid *f)
{
	Request *or;

	or = remtag(&f->r, r->f.oldtag);
	if(or != nil){
		fs->putfid(or->fid);
		free(or);
	}
	fs->reply(r, nil);
}

void
Fs.attach(Fs *fs, Request *r, Fid *f)
{
	f->qid = (Qid)(CHDIR|QID(0, Ttopdir), 0);

	if(r->f.uname[0])
		memmove(f->user, r->f.uname, sizeof(f->user));
	else
		strcpy(f->user, "none");

	/* hold down the fid till the clunk */
	f->attached = 1;
	fs->lock();
	f->ref++;
	fs->unlock();

	memset(r->f.rauth, 0, sizeof(r->f.rauth));
	r->f.qid = f->qid;
	fs->reply(r, nil);
}

void
Fs.clone(Fs *fs, Request *r, Fid *f)
{
	Fid *nf;

	if(f->attached == 0){
		fs->reply(r, Enofid);
		return;
	}

	nf = fs->getfid(r->f.newfid);
	nf->attached = f->attached;
	nf->open = f->open;
	nf->qid = f->qid;
	memmove(nf->user, f->user, sizeof(f->user));
	nf->c = f->c;
	nf->wp = nf->buf;
	nf->rp = nf->wp;

	fs->reply(r, nil);
}

void
Fs.walk(Fs *fs, Request *r, Fid *f)
{
	byte *name;
	Dir d;
	int i;
	byte *err;

	rescue {
		fs->reply(r, err);
		return;
	};

	if(f->attached == 0){
		err = Enofid;
		raise;
	}

	name = r->f.name;
	if(strcmp(name, "..") == 0)
		f->qid = parentqid(f->qid);
	else if(strcmp(name, ".") != 0){
		for(i = 0; ; i++){
			if(fs->dirgen(f->qid, i, &d, nil) < 0){
				err = Eexist;
				raise;
			}
			if(strcmp(name, d.name) == 0){
				f->qid = d.qid;
				break;
			}
		}
	}
	r->f.qid = f->qid;
	fs->reply(r, nil);
}

int m2p[] ={
	[OREAD]		4,
	[OWRITE]	2,
	[ORDWR]		6
};

void
Fs.open(Fs *fs, Request *r, Fid *f)
{
	int mode;
	byte *err;
	Console *c;

	rescue {
		fs->reply(r, err);
		return;
	}

	if(f->attached == 0){
		err = Enofid;
		raise;
	}

	if(f->open){
		err = Eisopen;
		raise;
	}

	mode = r->f.mode & 3;

	if((CHDIR & f->qid.path) && mode != OREAD){
		err = Eperm;
		raise;
	}

	switch(TYPE(f->qid)){
	case Tdata:
		c = fs->cons[CONS(f->qid)];
		f->rp = f->buf;
		f->wp = f->buf;
		f->c = c;
		c->lock();
		f->cnext = c->flist;
		c->flist = f;
		bcastmembers(fs, c, "+", f);
		c->unlock();
		break;
	case Tctl:
		break;
	}

	f->open = 1;
	r->f.qid = f->qid;
	fs->reply(r, nil);
}

void
Fs.create(Fs *fs, Request *r, Fid*)
{
	fs->reply(r, Eperm);
}

void
Fs.read(Fs *fs, Request *r, Fid *f)
{
	byte *err;
	byte *p, *e;
	int i;
	Dir d;

	rescue{
		fs->reply(r, err);
		return;
	}

	if(f->attached == 0){
		err = Enofid;
		raise;
	}

	if(r->f.count < 0){
		err = Ebadcount;
		raise;
	}

	if(CHDIR & f->qid.path){
		if((r->f.offset % DIRLEN) != 0){
			err = Ebadoffset;
			raise;
		}
		if(r->f.count < DIRLEN){
			err = Ebadcount;
			raise;
		}
		p = r->buf;
		e = r->buf + (r->f.count/DIRLEN)*DIRLEN;
		for(i = r->f.offset/DIRLEN; p < e; i++){
			if(fs->dirgen(f->qid, i, &d, p) < 0)
				break;
			p += DIRLEN;
		}
		r->f.data = r->buf;
		r->f.count = p - r->buf;
	} else {
		switch(TYPE(f->qid)){
		case Tdata:
			addreq(&f->r, r);
			fs->kick(f);
			return;
		case Tctl:
			r->f.data = r->buf;
			r->f.count = 0;
			break;
		default:
			err = Eexist;
			raise;
		}
	}
	fs->reply(r, nil);
}

void
Fs.write(Fs *fs, Request *r, Fid *f)
{
	byte *err;

	rescue{
		fs->reply(r, err);
		return;
	}

	if(f->attached == 0){
		err = Enofid;
		raise;
	}

	if(r->f.count < 0){
		err = Ebadcount;
		raise;
	}

	if(CHDIR & f->qid.path){
		err = Eperm;
		raise;
	}

	switch(TYPE(f->qid)){
	default:
		err = Eperm;
		raise;
	case Tctl:
		write(f->c->cfd, r->f.data, r->f.count);
		break;
	case Tdata:
		write(f->c->fd, r->f.data, r->f.count);
		break;
	}
	fs->reply(r, nil);
}

void
Fs.clunk(Fs *fs, Request *r, Fid *f)
{
	Fid **l, *fl;
	Request *nr;

	if(f->open && TYPE(f->qid) == Tdata){
		while((nr = remreq(&f->r)) != nil){
			fs->putfid(f);
			free(nr);
		}

		f->c->lock();
		for(l = &f->c->flist; *l; l = &fl->cnext){
			fl = *l;
			if(fl == f){
				*l = fl->cnext;
				break;
			}
		}
		bcastmembers(fs, f->c, "-", f);
		f->c->unlock();
	}
	fs->reply(r, nil);
	fs->putfid(f);
}

void
Fs.remove(Fs *fs, Request *r, Fid*)
{
	fs->reply(r, Eperm);
}


void
Fs.stat(Fs *fs, Request *r, Fid *f)
{
	int i;
	Qid q;
	Dir d;

	q = parentqid(f->qid);
	for(i = 0; ; i++){
		if(fs->dirgen(q, i, &d, r->f.stat) < 0){
			fs->reply(r, Eexist);
			return;
		}
		if(d.qid.path == f->qid.path)
			break;
	}
	fs->reply(r, nil);
}

void
Fs.wstat(Fs *fs, Request *r, Fid*)
{
	fs->reply(r, Eperm);
}

void
Fs.reply(Fs *fs, Request *r, byte *err)
{
	int n;
	byte buf[MAXRPC];

	if(err){
		r->f.type = Rerror;
		strncpy(r->f.ename, err, sizeof(r->f.ename));
	}
	n = convS2M(&r->f, buf);
	if(debug)
		fprint(2, "%F path %lux\n", &r->f, r->fid->qid.path);
	fs->putfid(r->fid);
	if(write9p(fs->fd, buf, n) != n)
		fatal("unmounted");
	free(r);
}

/*
 *  called whenever input or a read request has ben received
 */
void
Fs.kick(Fs *fs, Fid *f)
{
	Request *r;
	byte *p, *rp, *wp, *ep;
	int i;

	f->lock();
	while(f->rp != f->wp){
		r = remreq(&f->r);
		if(r == nil)
			break;
		p = r->buf;
		rp = f->rp;
		wp = f->wp;
		ep = &f->buf[Bsize];
		for(i = 0; i < r->f.count && rp != wp; i++){
			*p++ = *rp++;
			if(rp >= ep)
				rp = f->buf;
		}
		f->rp = rp;
		r->f.data = r->buf;
		r->f.count = p - r->buf;
		fs->reply(r, nil);
	}
	f->unlock();
}

Arg *arg;

void
usage(void)
{
	fprint(2, "usage: %s name dev [name dev]\n", arg->arg0);
	exits("usage");
}


void
main(int argc, byte **argv)
{
	Fs *fs;
	int c, i;

	fmtinstall('F', fcallconv);

	arg = arginit(argc, argv);
	while(c = argopt(arg))
		switch(c){
		case 'd':
			debug++;
		}

	if(arg->ac < 2 || (arg->ac & 1) == 1)
		usage();

	fs = .Fs.mount("/mnt/consoles");

	for(i = 0; i < arg->ac; i += 2)
		fs->console(arg->av[i], arg->av[i+1]);
}
