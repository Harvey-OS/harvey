#include <alef.h>
#include <fcall.h>
#include <bio.h>

enum
{
	TICKREQLEN=	3*NAMELEN+CHALLEN+DOMLEN+1,
	Nfidhash=	64,
	Ndfhash=	128,
};

aggr Symbol
{
	Symbol	*next;		/* hash list chaining */
	byte	sym[NAMELEN];
	int	fno;		/* file symbol is defined in */
};

/* source file */
aggr File
{
	QLock;

	byte	*name;
	Symbol	*ref;
	byte	*refvec;	/* files resolving the references */
	uint	len;		/* length of file */
	uint	tarlen;		/* length of tar file */
	uint	mode;
	uint	mtime;

	int	use;
	int	fd;
};

/* .depend file */
aggr Dfile
{
	Lock;
	int	use;		/* use count */
	int	old;		/* true if this is an superceded dfile */

	File	*file;		/* files */
	int	nfile;		/* number of files */
	int	flen;		/* length of file table */

	Symbol	**dhash;	/* hash table of symbols */
	int	hlen;		/* length of hash table */

	Dfile	*next;		/* hash chain */
	byte	*path;		/* path name of dependency file */
	Qid	qid;		/* qid of the dependency file */
};

aggr Fid
{
	Fid	*next;
	int	fid;
	int	ref;

	int	attached;
	int	open;
	Qid	qid;
	byte	*path;
	Dfile	*df;
	Symbol	*dp;
	int	fd;
};

aggr Request
{
	Request	*next;
	Fid	*fid;
	Fcall	f;
	byte	buf[1];
};

enum
{
	Tblocksize=	512,	/* tar block size */
	Tnamesize=	100,	/* tar name size */
};
aggr Tardir
{
	byte	name[Tnamesize];
	byte	mode[8];
	byte	uid[8];
	byte	gid[8];
	byte	size[12];
	byte	mtime[12];
	byte	chksum[8];
	byte	linkflag;
	byte	linkname[Tnamesize];
};

adt Fs
{
	Lock;

	int	fd;		/* to kernel mount point */
	Fid	*hash[Nfidhash];
	byte	*root;
	Qid	rootqid;

	void	run(*Fs, int, byte*);
intern	Fid*	getfid(*Fs, int);
intern	void	putfid(*Fs, Fid*);
intern	void	reply(*Fs, Request*, byte*);

	void	nop(*Fs, Request*, Fid*);
	void	session(*Fs, Request*, Fid*);
	void	flush(*Fs, Request*, Fid*);
	void	attach(*Fs, Request*, Fid*);
	void	clone(*Fs, Request*, Fid*);
	void	walk(*Fs, Request*, Fid*);
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
byte Enofid[] = "no such fid";

Arg *arg;
int debug;
Dfile *dfhash[Ndfhash];		/* dependency file hash */
QLock dfhlock[Ndfhash];

intern Request*	allocreq(int);
intern Dfile*	getdf(byte*);
intern void	releasedf(Dfile*);
intern Symbol*	dfsearch(Dfile*, byte*);
intern void	dfresolve(Dfile*, int);
intern byte*	mkpath(byte*, byte*);
intern int	mktar(Dfile*, Symbol*, byte*, uint, int);
intern void	closetar(Dfile*, Symbol*);

/*
 *  mount the user interface and start one request processor
 *  per CPU
 */
void
main(int argc, byte **argv)
{
	int pfd[2];
	int c, srv, domount;
	byte service[2*NAMELEN];
	Fs *fs;

	fmtinstall('F', fcallconv);

	domount = 0;
	arg = arginit(argc, argv);
	while(c = argopt(arg))
		switch(c){
		case 'd':
			debug++;
			break;
		case 'm':
			domount++;
			break;
		}
	if(arg->ac != 2){
		fprint(2, "usage: %s [-d] svc-name directory", arg->arg0);
		exits("usage");
	}
	sprint(service, "#s/%s", arg->av[0]);
	if(*arg->av[1] != '/')
		fatal("directory must be rooted");

	if(pipe(pfd) < 0)
		fatal("opening pipe: %r");

	/* Typically mounted before /srv exists */
	srv = create(service, OWRITE, 0666);
	if(srv < 0)
		fatal("post: %r");
	fprint(srv, "%d", pfd[1]);
	close(srv);

	switch(rfork(RFPROC|RFFDG|RFNAMEG)){
	case 0:
		close(pfd[1]);
		break;
	default:
		close(pfd[0]);
		if(domount){
			sprint(service, "/n/%s", arg->av[0]);
			mount(pfd[1], service, MREPL, "");
		}
		exits(nil);
	}

	if(bind(arg->av[1], "/", MREPL) == 0)
		fatal("can't bind %s to /", arg->av[1]);

	alloc fs;
	proc fs->run(pfd[0], arg->av[1]);
	proc fs->run(pfd[0], arg->av[1]);
	fs->run(pfd[0], arg->av[1]);
	exits(nil);
}

intern byte*
mkpath(byte *dir, byte *file)
{
	int len;
	byte *path;

	len = strlen(dir) + 1;
	if(file != nil)
		len += strlen(file) + 1;
	path = malloc(len);
	check path != nil;
	if(file != nil)
		sprint(path, "%s/%s", dir, file);
	else
		sprint(path, "%s", dir);
	return path;
}

void
Fs.run(Fs* fs, int fd, byte *root)
{
	int n, t;
	Request *r;
	Fid *f;
	Dir d;

	fs->fd = fd;
	if(dirstat("/", &d) < 0)
		fatal("root %s inaccessible: %r", root);
	fs->rootqid = d.qid;

	for(;;){
		r = allocreq(MAXRPC);
		n = read(fs->fd, r->buf, MAXRPC);
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
			print("%F path %lux\n", &r->f, f->qid.path);

		t = r->f.type;
		r->f.type++;
		(*fcall[t])(fs, r, f);
	}

}

/*
 *  any request that can get queued for a delayed reply
 */
intern Request*
allocreq(int bufsize)
{
	Request *r;

	r = malloc(sizeof(Request)+bufsize);
	r->next = nil;
	return r;
}

Fid*
Fs.getfid(Fs *fs, int fid)
{
	Fid *f, *nf;

	fs->lock();
	for(f = fs->hash[fid%Nfidhash]; f; f = f->next){
		if(f->fid == fid){
			f->ref++;
			fs->unlock();
			return f;
		}
	}

	alloc nf;
	check nf != nil;
	memset(nf, 0, sizeof(*nf));
	nf->next = fs->hash[fid%Nfidhash];
	fs->hash[fid%Nfidhash] = nf;
	nf->fid = fid;
	nf->ref = 1;
	nf->fd = -1;
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
	for(l = &fs->hash[f->fid%Nfidhash]; nf = *l; l = &nf->next)
		if(nf == f){
			*l = f->next;
			break;
		}
	fs->unlock();
	free(f);
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
	if(debug)
		print("%F path %lux\n", &r->f, r->fid->qid.path);
	n = convS2M(&r->f, buf);
	if(write(fs->fd, buf, n) != n)
		fatal("unmounted");
	free(r);
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
Fs.flush(Fs*, Request*, Fid*)
{
}

void
Fs.attach(Fs *fs, Request *r, Fid *f)
{
	f->qid = fs->rootqid;
	f->path = strdup("/");
	f->df = getdf(mkpath(f->path, ".depend"));

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
		fs->reply(r, Eexist);
		return;
	}
	nf = fs->getfid(r->f.newfid);

	nf->attached = 1;
	nf->open = f->open;
	nf->path = strdup(f->path);
	nf->qid = f->qid;
	nf->dp = f->dp;
	nf->fd = f->fd;
	nf->df = f->df;
	if(nf->df){
		nf->df->lock();
		nf->df->use++;
		nf->df->unlock();
	}
	fs->reply(r, nil);
}

void
Fs.walk(Fs *fs, Request *r, Fid *f)
{
	byte *name;
	int i;
	Dir d;
	byte errbuf[ERRLEN];
	byte *path;
	Symbol *dp;

	if(f->attached == 0){
		fs->reply(r, Enofid);
		return;
	}

	if(f->fd >= 0 || f->open)
		fatal("walk of an open file");

	name = r->f.name;
	if(strcmp(name, ".") == 0){
		fs->reply(r, nil);
		return;
	}
	if(strcmp(name, "..") == 0){
		name = strrchr(f->path, '/');
		if(name){
			if(name == f->path){
				fs->reply(r, nil);
				return;
			}
			*name = 0;
		}
		if(dirstat(f->path, &d) < 0){
			*name = '/';
			errstr(errbuf);
			fs->reply(r, errbuf);
			return;
		}
		r->f.qid = f->qid = d.qid;

		if(f->df)
			releasedf(f->df);
		f->df = getdf(mkpath(f->path, ".depend"));

		fs->reply(r, nil);
		return;
	}

	path = mkpath(f->path, name);
	if(dirstat(path, &d) < 0 || (d.qid.path & CHDIR) == 0){
		dp = dfsearch(f->df, name);
		if(dp == nil){
			i = strlen(name);
			if(i > 4 && strcmp(&name[i-4], ".tar") == 0){
				name[i-4] = 0;
				dp = dfsearch(f->df, name);
			}
		}
		if(dp == nil){
			fs->reply(r, Eexist);
			free(path);
			return;
		}
		f->dp = dp;
		d.qid.path = (uint)dp;
		d.qid.vers = 0;
	}

	free(f->path);
	f->path = path;

	if(d.qid.path & CHDIR){
		if(f->df)
			releasedf(f->df);
		f->df = getdf(mkpath(f->path, ".depend"));
	}

	r->f.qid = f->qid = d.qid;
	fs->reply(r, nil);
}

void
Fs.open(Fs *fs, Request *r, Fid *f)
{
	int mode;
	byte errbuf[ERRLEN];
	
	if(f->attached == 0){
		fs->reply(r, Enofid);
		return;
	}
	if(f->open){
		fs->reply(r, Eisopen);
		return;
	}

	mode = r->f.mode & 3;
	if(mode != OREAD){
		fs->reply(r, Eperm);
		return;
	}

	if(f->qid.path & CHDIR){
		f->fd = open(f->path, OREAD);
		if(f->fd < 0){
			errstr(errbuf);
			fs->reply(r, errbuf);
			return;
		}
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
	int i, n, skip;
	Dir d;
	byte dirbuf[DIRLEN];
	Symbol *dp;

	if(f->attached == 0){
		fs->reply(r, Enofid);
		return;
	}
	if(r->f.count < 0){
		fs->reply(r, "bad read count");
		return;
	}

	if(f->qid.path & CHDIR){
		skip = r->f.offset/DIRLEN;
		for(n = 0; r->f.count - n >= DIRLEN;){
			i = read(f->fd, dirbuf, DIRLEN);
			if(i <= 0)
				break;
			convM2D(dirbuf, &d);
			if((d.qid.path & CHDIR) == 0)
				continue;
			if(skip-- > 0)
				continue;
			memmove(r->buf + n, dirbuf, DIRLEN);
			n += DIRLEN;
		}
		if(f->df)
			for(i = 0; i < f->df->hlen; i++)
				for(dp = f->df->dhash[i]; dp; dp = dp->next){
					if(skip-- > 0)
						continue;
					if(r->f.count - n < DIRLEN)
						break 2;
					strcpy(d.name, dp->sym);
					if(strlen(dp->sym) < NAMELEN - 5)
						strcat(d.name, ".tar");
					strcpy(d.uid, "none");
					strcpy(d.gid, "none");
					d.qid.path = (uint)dp;
					d.qid.vers = 0;
					d.length = f->df->file[dp->fno].tarlen;
					d.mode = 0444;
					d.mtime = time();
					d.atime = time();
					convD2M(&d, r->buf + n);
					n += DIRLEN;
				}
	} else
		n = mktar(f->df, f->dp, r->buf, r->f.offset, r->f.count);

	r->f.data = r->buf;
	r->f.count = n;
	fs->reply(r, nil);
}

void
Fs.write(Fs *fs, Request *r, Fid*)
{
	fs->reply(r, Eperm);
}

void
Fs.clunk(Fs *fs, Request *r, Fid *f)
{
	if(f->attached == 0){
		fs->reply(r, Enofid);
		return;
	}
	if(f->fd >= 0){
		close(f->fd);
		f->fd = -1;
	}

	if((f->qid.path & CHDIR) == 0)
		closetar(f->df, f->dp);

	if(f->df)
		releasedf(f->df);

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
	byte err[ERRLEN];
	Dir d;
	Symbol *dp;

	if(f->qid.path & CHDIR){
		if(stat(f->path, r->f.stat) < 0){
			errstr(err);
			fs->reply(r, err);
		} else
			fs->reply(r, nil);
	} else {
		dp = f->dp;
		strcpy(d.name, dp->sym);
		strcpy(d.uid, "none");
		strcpy(d.gid, "none");
		d.qid.path = (uint)dp;
		d.qid.vers = 0;
		d.length = f->df->file[dp->fno].tarlen;
		d.mode = 0444;
		d.mtime = time();
		d.atime = time();
		convD2M(&d, r->f.stat);
		fs->reply(r, nil);
	}
}

void
Fs.wstat(Fs *fs, Request *r, Fid*)
{
	fs->reply(r, Eperm);
}

/*
 *  string hash
 */
uint
shash(byte *str, int len)
{
	uint	hash;
	byte	*val; 

	hash = 0;
	for(val = str; *val; val++)
		hash = (hash*13) + *val-'a';
	return hash % len;
}

/*
 *  free info about a dependency file
 */
intern void
freedf(Dfile *df)
{
	int i;
	Symbol *dp, *next;

	df->lock();
	df->old = 1;
	if(df->use){
		df->unlock();
		return;
	}

	df->unlock();	/* we're no longer referenced */
	for(i = 0; i < df->nfile; i++)
		free(df->file[i].name);
	free(df->file);
	df->file = nil;

	for(i = 0; i < df->hlen; i++)
		for(dp = df->dhash[i]; dp != nil; dp = next){
			next = dp->next;
			free(dp);
		}

	free(df->dhash);
}

/*
 *  crack a dependency file
 */
intern void
newsym(byte *name, int fno, Symbol **l)
{
	Symbol *dp;

	dp = malloc(sizeof(Symbol));
	check dp != nil;
	strncpy(dp->sym, name, sizeof(dp->sym));
	dp->next = *l;
	dp->fno = fno;
	*l = dp;
}
intern int
awk(Biobuf *b, byte **field, int n)
{
	byte *line;
	int i;

	while(line = b->rdline('\n')){
		line[b->linelen()-1] = 0;
		while(*line == ' ' || *line == '\t')
			*line++ = 0;
		for(i = 0; i < n; i++){
			if(*line == 0 || *line == '#')
				break;
			field[i] = line;
			while(*line && *line != ' ' && *line != '\t')
				line++;
			while(*line == ' ' || *line == '\t')
				*line++ = 0;
		}
		if(i)
			return i;
	}

	return 0;
}
intern void
crackdf(Dfile *df, Biobuf *b, uint len, byte *dpath)
{
	byte *name;
	byte *field[3];
	int n, inc;
	Symbol **l, *dp, *next;
	File *f, *ef;
	byte *path;
	Dir d;

	inc = 32;
	df->flen = inc;
	df->file = malloc(df->flen*sizeof(File));
	check df->file != nil;
	df->nfile = 0;

	df->hlen = 1 + len/8;
	df->dhash = malloc(df->hlen*sizeof(Symbol*));
	check df->dhash != nil;

	l = nil;
	while((n = awk(b, field, 3)) > 0){
		if(n != 2)
			continue;

		name = field[1];
		switch(*field[0]){
		case 'F':
			if(df->flen == df->nfile){
				df->flen += inc;
				df->file = realloc(df->file, df->flen*sizeof(File));
				check df->file != nil;
			}
			f = &df->file[df->nfile++];
			f->name = strdup(name);
			l = &f->ref;
			/* fall through and define as a symbol */
		case 'D':
			if(l == nil)
				continue;
			newsym(name, df->nfile-1, &(df->dhash[shash(name, df->hlen)]));
			break;
		case 'R':
			if(l == nil)
				continue;
			newsym(name, 0, l);
			break;
		}
	}

	ef = &df->file[df->nfile];

	/* stat the files to get sizes */
	path = malloc(strlen(dpath) + NAMELEN + 2 + 2);
	check path != nil;
	strcpy(path, dpath);
	name = strrchr(path, '/') + 1;
	for(f = df->file; f < ef; f++){
		strcpy(name, f->name);
		d.length = 0;
		if(dirstat(path, &d) < 0){
			strcat(path, ".Z");
			if(dirstat(path, &d) >= 0){
				free(f->name);
				f->name = strdup(name);
				check f->name != nil;
			}
		}
		f->len = d.length;
		f->mode = d.mode;
		f->mtime = d.mtime;
		f->fd = -1;
	}
	free(path);

	/* resolve all file references */
	for(f = df->file; f < ef; f++)
		dfresolve(df, f-df->file);

	/* free the referenced symbols, don't need them anymore */
	for(f = df->file; f < ef; f++){
		f->tarlen += 2*Tblocksize;	/* tars trailing 0 blocks */
		for(dp = f->ref; dp != nil; dp = next){
			next = dp->next;
			free(dp);
		}
		f->ref = nil;
	}
}

/*
 *  get a cracked dependency file
 */
intern Dfile*
getdf(byte *path)
{
	Dfile *df, **l;
	QLock *lk;
	Dir d;
	int i, rv, fd;
	Biobuf *b;

	i = shash(path, Ndfhash);
	l = &dfhash[i];
	lk = &dfhlock[i];
	lk->lock();
	for(df = *l; df; df = *l){
		if(strcmp(path, df->path) == 0)
			break;
		l = &df->next;
	}
	rv = dirstat(path, &d);

	if(df){
		if(rv >= 0 && d.qid.path == df->qid.path && d.qid.vers == df->qid.vers){
			free(path);
			df->lock();
			df->use++;
			df->unlock();
			lk->unlock();
			return df;
		}
		*l = df->next;
		freedf(df);
	}

	fd = open(path, OREAD);
	if(rv < 0 || fd < 0){
		lk->unlock();
		close(fd);
		return nil;
	}

	df = malloc(sizeof(*df));
	check df != nil;
	df->next = *l;
	*l = df;

	alloc b;
	check b != nil;

	b->init(fd, OREAD);
	df->qid = d.qid;
	df->path = path;
	crackdf(df, b, d.length, path);
	b->term();

	free(b);

	lk->unlock();
	return df;
}

/*
 *  stop using a dependency file.  Free it if it is no longer linked in.
 */
intern void
releasedf(Dfile *df)
{
	Dfile **l, *d;
	QLock *lk;
	int i;

	df->lock();
	df->use--;
	if(df->old == 0 || df->use > 0){
		df->unlock();
		return;
	}

	/* remove from hash chain */
	i = shash(df->path, Ndfhash);
	l = &dfhash[i];
	lk = &dfhlock[i];
	lk->lock();
	for(d = *l; d; d = *l){
		if(d == df){
			*l = d->next;
			break;
		}
		l = &d->next;
	}
	lk->unlock();
	df->unlock();

	/* now we know it is unreferenced, remove it */
	freedf(df);
}

/*
 *  search a dependency file for a symbol
 */
intern Symbol*
dfsearch(Dfile *df, byte *name)
{
	Symbol *dp;

	if(df == nil)
		return nil;
	for(dp = df->dhash[shash(name, df->hlen)]; dp; dp = dp->next)
		if(strcmp(dp->sym, name) == 0)
			return dp;
	return nil;
}

/*
 *  resolve a single file into a vector of referenced files and the sum of their
 *  lengths
 */
/* set a bit in the referenced file vector */
intern int
set(byte *vec, int fno)
{
	if(vec[fno/8] & (1<<(fno&7)))
		return 1;
	vec[fno/8] |= 1<<(fno&7);
	return 0;
}
/* merge two referenced file vectors */
intern void
merge(byte *vec, byte *ovec, int nfile)
{
	nfile = (nfile+7)/8;
	while(nfile-- > 0)
		*vec++ |= *ovec++;
}
intern uint
res(Dfile *df, byte *vec, int fno)
{
	File *f;
	Symbol *rp, *dp;
	int len;

	f = &df->file[fno];
	if(set(vec, fno))
		return 0;				/* already set */
	if(f->refvec != nil){
		merge(vec, f->refvec, df->nfile);	/* we've descended here before */
		return f->tarlen;
	}

	len = 0;
	for(rp = f->ref; rp; rp = rp->next){
		dp = dfsearch(df, rp->sym);
		if(dp == nil)
			continue;
		len += res(df, vec, dp->fno);
	}
	return len + Tblocksize + ((f->len + Tblocksize - 1)/Tblocksize)*Tblocksize;
}
intern void
dfresolve(Dfile *df, int fno)
{
	byte *vec;
	File *f;

	f = &df->file[fno];
	vec = malloc((df->nfile+7)/8);
	check vec != nil;
	f->tarlen = res(df, vec, fno);
	f->refvec = vec;
}

/*
 *  make the tar directory block for a file
 */
intern byte*
mktardir(File *f)
{
	byte *ep;
	Tardir *tp;
	uint sum;
	byte *p, *cp;

	p = malloc(Tblocksize);
	check p != nil;
	tp = (Tardir*)p;

	strcpy(tp->name, f->name);
	sprint(tp->mode, "%6o ", f->mode & 0777);
	sprint(tp->uid, "%6o ", 0);
	sprint(tp->gid, "%6o ", 0);
	sprint(tp->size, "%11lo ", f->len);
	sprint(tp->mtime, "%11lo ", f->mtime);

	/* calculate checksum */
	memset(tp->chksum, ' ', sizeof(tp->chksum));
	sum = 0;
	ep = p + Tblocksize;
	for (cp = p; cp < ep; cp++)
		sum += *cp;
	sprint(tp->chksum, "%6o", sum);

	return p;
}

/*
 *  manage open files
 */
intern int
getfile(Dfile *df, File *f)
{
	byte *path;
	byte *name;

	f->lock();
	f->use++;
	if(f->fd < 0){
		path = malloc(strlen(df->path) + NAMELEN + 2 + 2);
		check path != nil;
		strcpy(path, df->path);
		name = strrchr(path, '/') + 1;
		strcpy(name, f->name);
		f->fd = open(path, OREAD);
		free(path);
	}

	return f->fd;
}
intern void
releasefile(File *f)
{
	--f->use;
	f->unlock();
}
intern void
closefile(File *f)
{
	f->lock();
	if(f->use == 0){
		close(f->fd);
		f->fd = -1;
	}
	f->unlock();
}

/*
 *  return a block of a tar file
 */
intern int
mktar(Dfile *df, Symbol *dp, byte *area, uint offset, int len)
{
	int fd, i, j, n, off;
	byte *p, *buf, *vec;
	File *f;

	f = &df->file[dp->fno];
	vec = f->refvec;
	p = area;

	/* find file */
	for(i = 0; i < df->nfile && len > 0; i++){
		if((vec[i/8] & (1<<(i&7))) == 0)
			continue;

		f = &df->file[i];
		n = Tblocksize + ((f->len + Tblocksize - 1)/Tblocksize)*Tblocksize;
		if(offset >= n){
			offset -= n;
			continue;
		}

		if(offset < Tblocksize){
			buf = mktardir(f);
			if(offset + len > Tblocksize)
				j = Tblocksize - offset;
			else
				j = len;
if(debug)print("reading %d bytes dir of %s\n", j, f->name);
			memmove(p, buf+offset, j);
			p += j;
			len -= j;
			offset += j;
			free(buf);
		}
		if(len <= 0)
			break;
		off = offset - Tblocksize;
		if(off >= 0 && off < f->len){
			if(off + len > f->len)
				j = f->len - off;
			else
				j = len;
			fd = getfile(df, f);
			if(fd >= 0){
if(debug)print("reading %d bytes from offset %d of %s\n", j, off, f->name);
				seek(fd, off, 0);
				read(fd, p, j);		/* ignore errors??? */
			}
			releasefile(f);
			p += j;
			len -= j;
			offset += j;
		}
		if(len <= 0)
			break;
		if(offset < n){
			if(offset + len > n)
				j = n - offset;
			else
				j = len;
if(debug)print("filling %d bytes after %s\n", j, f->name);
			memset(p, 0, j);
			p += j;
			len -= j;
		}
		offset = 0;
	}

	/* null blocks at end of tar file */
	if(offset < 2*Tblocksize && len > 0){
		if(offset + len > 2*Tblocksize)
			j = 2*Tblocksize - offset;
		else
			j = len;
if(debug)print("filling %d bytes at end\n", j);
		memset(p, 0, j);
		p += j;
	}

	return p - area;
}

/*
 *  close the files making up  a tar file
 */
intern void
closetar(Dfile *df, Symbol *dp)
{
	int i;
	byte *vec;
	File *f;

	f = &df->file[dp->fno];
	vec = f->refvec;

	/* find file */
	for(i = 0; i < df->nfile; i++){
		if((vec[i/8] & (1<<(i&7))) == 0)
			continue;
		closefile(&df->file[i]);
	}
}

