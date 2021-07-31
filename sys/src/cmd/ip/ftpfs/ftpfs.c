#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <String.h>
#include "ftpfs.h"

/* an active fid */
typedef struct Fid	Fid;
struct Fid
{
	int	fid;
	Node	*node;		/* path to remote file */
	int	busy;
	Fid	*next;
	int	open;
};

Fid	*fids;			/* linked list of fids */
char	errstring[128];		/* error to return */
int	mfd;			/* fd for 9fs */
int	messagesize = 4*1024*IOHDRSZ;
uchar	mdata[8*1024*IOHDRSZ];
uchar	mbuf[8*1024*IOHDRSZ];
Fcall	rhdr;
Fcall	thdr;
int	debug;
int	usenlst;
int	usetls;
char	*ext;
int	quiet;
int	kapid = -1;
int	dying;		/* set when any process decides to die */
int	dokeepalive;

char	*rflush(Fid*), *rnop(Fid*), *rversion(Fid*),
	*rattach(Fid*), *rclone(Fid*), *rwalk(Fid*),
	*rclwalk(Fid*), *ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*),
	*rauth(Fid*);;
void	mountinit(char*);
void	io(void);
int	readpdir(Node*);

char 	*(*fcalls[])(Fid*) = {
	[Tflush]	rflush,
	[Tversion]	rversion,
	[Tattach]	rattach,
	[Tauth]		rauth,
	[Twalk]		rwalk,
	[Topen]		ropen,
	[Tcreate]	rcreate,
	[Tread]		rread,
	[Twrite]	rwrite,
	[Tclunk]	rclunk,
	[Tremove]	rremove,
	[Tstat]		rstat,
	[Twstat]	rwstat,
};

/* these names are matched as prefixes, so VMS must precede VM */
OS oslist[] = {
	{ Plan9,	"Plan 9", },
	{ Plan9,	"Plan9", },
	{ Plan9,	"UNIX Type: L8 Version: Plan 9", },
	{ Unix,		"SUN", },
	{ Unix,		"UNIX", },
	{ VMS,		"VMS", },
	{ VM,		"VM", },
	{ Tops,		"TOPS", },
	{ MVS,		"MVS", },
	{ NetWare,	"NetWare", },
	{ NetWare,	"NETWARE", },
	{ OS½,		"OS/2", },
	{ TSO,		"TSO", },
	{ NT,		"Windows_NT", },	/* DOS like interface */
	{ NT,		"WINDOWS_NT", },	/* Unix like interface */
	{ Unknown,	0 },
};

char *nouid = "?uid?";

#define S2P(x) (((ulong)(x)) & 0xffffff)

char *nosuchfile = "file does not exist";
char *keyspec = "";

void
usage(void)
{
	fprint(2, "ftpfs [-/dqnt] [-a passwd] [-m mountpoint] [-e ext] [-k keyspec] [-o os] [-r root] [net!]address\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *mountroot = 0;
	char *mountpoint = "/n/ftp";
	char *cpassword = 0;
	char *cp;
	int p[2];
	OS *o;

	defos = Unix;
	user = strdup(getuser());
	usetls = 0;

	ARGBEGIN {
	case '/':
		mountroot = "/";
		break;
	case 'a':
		cpassword = ARGF();
		break;
	case 'd':
		debug = 1;
		break;
	case 'k':
		keyspec = EARGF(usage());
		break;
	case 'K':
		dokeepalive = 1;
		break;
	case 'm':
		mountpoint = ARGF();
		break;
	case 'n':
		usenlst = 1;
		break;
	case 'e':
		ext = ARGF();
		break;
	case 't':
		usetls = 1;
		break;
	case 'o':
		cp = ARGF();
		for(o = oslist; o->os != Unknown; o++)
			if(strncmp(cp, o->name, strlen(o->name)) == 0){
				defos = o->os;
				break;
			}
		break;
	case 'r':
		mountroot = ARGF();
		break;
	case 'q':
		quiet = 1;
		break;
	} ARGEND
	if(argc != 1)
		usage();

	/* get a pipe to mount and run 9fs on */
	if(pipe(p) < 0)
		fatal("pipe failed: %r");
	mfd = p[0];

	/* initial handshakes with remote side */
	hello(*argv);
	if(cpassword == 0)
		rlogin(*argv, keyspec);
	else
		clogin("anonymous", cpassword);
	preamble(mountroot);

	/* start the 9fs protocol */
	switch(rfork(RFPROC|RFNAMEG|RFENVG|RFFDG|RFNOTEG|RFREND)){
	case -1:
		fatal("fork: %r");
	case 0:
		/* seal off standard input/output */
		close(0);
		open("/dev/null", OREAD);
		close(1);
		open("/dev/null", OWRITE);

		close(p[1]);
		fmtinstall('F', fcallfmt); /* debugging */
		fmtinstall('D', dirfmt); /* expected by %F */
		fmtinstall('M', dirmodefmt); /* expected by %F */
		io();
		quit();
		break;
	default:
		close(p[0]);
		if(mount(p[1], -1, mountpoint, MREPL|MCREATE, "") < 0)
			fatal("mount failed: %r");
	}
	exits(0);
}

/*
 *  lookup an fid. if not found, create a new one.
 */
Fid *
newfid(int fid)
{
	Fid *f, *ff;

	ff = 0;
	for(f = fids; f; f = f->next){
		if(f->fid == fid){
			if(f->busy)
				return f;
			else{
				ff = f;
				break;
			}
		} else if(!ff && !f->busy)
			ff = f;
	}
	if(ff == 0){
		ff = mallocz(sizeof(*f), 1);
		ff->next = fids;
		fids = ff;
	}
	ff->node = nil;
	ff->fid = fid;
	return ff;
}

/*
 *  a process that sends keep alive messages to
 *  keep the server from shutting down the connection
 */
int
kaproc(void)
{
	int pid;

	if(!dokeepalive)
		return -1;

	switch(pid = rfork(RFPROC|RFMEM)){
	case -1:
		return -1;
	case 0:
		break;
	default:
		return pid;
	}

	while(!dying){
		sleep(5000);
		nop();
	}

	_exits(0);
	return -1;
}

void
io(void)
{
	char *err, buf[ERRMAX];
	int n;

	kapid = kaproc();

	while(!dying){
		n = read9pmsg(mfd, mdata, messagesize);
		if(n <= 0){
			errstr(buf, sizeof buf);
			if(buf[0]=='\0' || strstr(buf, "hungup"))
				exits("");
			fatal("mount read: %s\n", buf);
		}
		if(convM2S(mdata, n, &thdr) == 0)
			continue;

		if(debug)
			fprint(2, "<-%F\n", &thdr);/**/

		if(!fcalls[thdr.type])
			err = "bad fcall type";
		else
			err = (*fcalls[thdr.type])(newfid(thdr.fid));
		if(err){
			rhdr.type = Rerror;
			rhdr.ename = err;
		}else{
			rhdr.type = thdr.type + 1;
			rhdr.fid = thdr.fid;
		}
		rhdr.tag = thdr.tag;
		if(debug)
			fprint(2, "->%F\n", &rhdr);/**/
		n = convS2M(&rhdr, mdata, messagesize);
		if(write(mfd, mdata, n) != n)
			fatal("mount write");
	}
}

char*
rnop(Fid *f)
{
	USED(f);
	return 0;
}

char*
rversion(Fid*)
{
	if(thdr.msize > sizeof(mdata))
		rhdr.msize = messagesize;
	else
		rhdr.msize = thdr.msize;
	messagesize = thdr.msize;

	if(strncmp(thdr.version, "9P2000", 6) != 0)
		return "unknown 9P version";
	rhdr.version = "9P2000";
	return nil;
}

char*
rflush(Fid*)
{
	return 0;
}

char*
rauth(Fid*)
{
	return "auth unimplemented";
}

char*
rattach(Fid *f)
{
	f->busy = 1;
	f->node = remroot;
	rhdr.qid = f->node->d->qid;
	return 0;
}

char*
rwalk(Fid *f)
{
	Node *np;
	Fid *nf;
	char **elems;
	int i, nelems;
	char *err;
	Node *node;

	/* clone fid */
	nf = nil;
	if(thdr.newfid != thdr.fid){
		nf = newfid(thdr.newfid);
		if(nf->busy)
			return "newfid in use";
		nf->busy = 1;
		nf->node = f->node;
		f = nf;
	}

	err = nil;
	elems = thdr.wname;
	nelems = thdr.nwname;
	node = f->node;
	rhdr.nwqid = 0;
	if(nelems > 0){
		/* walk fid */
		for(i=0; i<nelems && i<MAXWELEM; i++){
			if((node->d->qid.type & QTDIR) == 0){
				err = "not a directory";
				break;
			}
			if(strcmp(elems[i], ".") == 0){
   Found:
				rhdr.wqid[i] = node->d->qid;
				rhdr.nwqid++;
				continue;
			}
			if(strcmp(elems[i], "..") == 0){
				node = node->parent;
				goto Found;
			}
			if(strcmp(elems[i], ".flush.ftpfs") == 0){
				/* hack to flush the cache */
				invalidate(node);
				readdir(node);
				goto Found;
			}

			/* some top level names are special */
			if((os == Tops || os == VM || os == VMS) && node == remroot){
				np = newtopsdir(elems[i]);
				if(np){
					node = np;
					goto Found;
				} else {
					err = nosuchfile;
					break;
				}
			}

			/* everything else */
			node = extendpath(node, s_copy(elems[i]));
			if(ISCACHED(node->parent)){
				/* the cache of the parent is good, believe it */
				if(!ISVALID(node)){
					err = nosuchfile;
					break;
				}
				if(node->parent->chdirunknown || (node->d->mode & DMSYML))
					fixsymbolic(node);
			} else if(!ISVALID(node)){
				/* this isn't a valid node, try cd'ing */
				if(changedir(node) == 0){
					node->d->qid.type = QTDIR;
					node->d->mode |= DMDIR;
				}else{
					node->d->qid.type = QTFILE;
					node->d->mode &= ~DMDIR;
				}
			}
			goto Found;
		}
		if(i == 0 && err == 0)
			err = "file does not exist";
	}

	/* clunk a newly cloned fid if the walk didn't succeed */
	if(nf != nil && (err != nil || rhdr.nwqid<nelems)){
		nf->busy = 0;
		nf->fid = 0;
	}

	/* if it all worked, point the fid to the enw node */
	if(err == nil)
		f->node = node;

	return err;
}

char *
ropen(Fid *f)
{
	int mode;

	mode = thdr.mode;
	if(f->node->d->qid.type & QTDIR)
		if(mode != OREAD)
			return "permission denied";

	if(mode & OTRUNC){
		uncache(f->node);
		uncache(f->node->parent);
		filedirty(f->node);
	} else {
		/* read the remote file or directory */
		if(!ISCACHED(f->node)){
			filefree(f->node);
			if(f->node->d->qid.type & QTDIR){
				invalidate(f->node);
				if(readdir(f->node) < 0)
					return nosuchfile;
			} else {
				if(readfile(f->node) < 0)
					return errstring;
			}
			CACHED(f->node);
		}
	}

	rhdr.qid = f->node->d->qid;
	f->open = 1;
	f->node->opens++;
	return 0;
}

char*
rcreate(Fid *f)
{
	char *name;

	if((f->node->d->qid.type&QTDIR) == 0)
		return "not a directory";

	name = thdr.name;
	f->node = extendpath(f->node, s_copy(name));
	uncache(f->node);
	if(thdr.perm & DMDIR){
		if(createdir(f->node) < 0)
			return "permission denied";
	} else
		filedirty(f->node);
	invalidate(f->node->parent);
	uncache(f->node->parent);

	rhdr.qid = f->node->d->qid;
	f->open = 1;
	f->node->opens++;
	return 0;
}

char*
rread(Fid *f)
{
	long off;
	int n, cnt, rv;
	Node *np;

	rhdr.count = 0;
	off = thdr.offset;
	cnt = thdr.count;
	if(cnt > messagesize-IOHDRSZ)
		cnt = messagesize-IOHDRSZ;

	if(f->node->d->qid.type & QTDIR){
		rv = 0;
		for(np = f->node->children; np != nil; np = np->sibs){
			if(!ISVALID(np))
				continue;
			if(off <= BIT16SZ)
				break;
			n = convD2M(np->d, mbuf, messagesize-IOHDRSZ);
			off -= n;
		}
		for(; rv < cnt && np != nil; np = np->sibs){
			if(!ISVALID(np))
				continue;
			if(np->d->mode & DMSYML)
				fixsymbolic(np);
			n = convD2M(np->d, mbuf + rv, cnt-rv);
			if(n <= BIT16SZ)
				break;
			rv += n;
		}
	} else {
		/* reread file if it's fallen out of the cache */
		if(!ISCACHED(f->node))
			if(readfile(f->node) < 0)
				return errstring;
		CACHED(f->node);
		rv = fileread(f->node, (char*)mbuf, off, cnt);
		if(rv < 0)
			return errstring;
	}

	rhdr.data = (char*)mbuf;
	rhdr.count = rv;
	return 0;
}

char*
rwrite(Fid *f)
{
	long off;
	int cnt;

	if(f->node->d->qid.type & QTDIR)
		return "directories are not writable";

	rhdr.count = 0;
	off = thdr.offset;
	cnt = thdr.count;
	cnt = filewrite(f->node, thdr.data, off, cnt);
	if(cnt < 0)
		return errstring;
	filedirty(f->node);
	rhdr.count = cnt;
	return 0;
}

char *
rclunk(Fid *f)
{
	if(fileisdirty(f->node)){
		if(createfile(f->node) < 0)
			fprint(2, "ftpfs: couldn't create %s\n", f->node->d->name);
		fileclean(f->node);
		uncache(f->node);
	}
	if(f->open){
		f->open = 0;
		f->node->opens--;
	}
	f->busy = 0;
	return 0;
}

/*
 *  remove is an implicit clunk
 */
char *
rremove(Fid *f)
{
	char *e;
	e = nil;
	if(QTDIR & f->node->d->qid.type){
		if(removedir(f->node) < 0)
			e = errstring;
	} else {
		if(removefile(f->node) < 0)
			e = errstring;
	}
	uncache(f->node->parent);
	uncache(f->node);
	if(e == nil)
		INVALID(f->node);
	f->busy = 0;
	return e;
}

char *
rstat(Fid *f)
{
	Node *p;

	p = f->node->parent;
	if(!ISCACHED(p)){
		invalidate(p);
		readdir(p);
		CACHED(p);
	}
	if(!ISVALID(f->node))
		return nosuchfile;
	if(p->chdirunknown)
		fixsymbolic(f->node);
	rhdr.nstat = convD2M(f->node->d, mbuf, messagesize-IOHDRSZ);
	rhdr.stat = mbuf;
	return 0;
}

char *
rwstat(Fid *f)
{
	USED(f);
	return "wstat not implemented";
}

/*
 *  print message and die
 */
void
fatal(char *fmt, ...)
{
	va_list arg;
	char buf[8*1024];

	dying = 1;

	va_start(arg, fmt);
	vseprint(buf, buf + (sizeof(buf)-1) / sizeof(*buf), fmt, arg);
	va_end(arg);

	fprint(2, "ftpfs: %s\n", buf);
	if(kapid > 0)
		postnote(PNGROUP, kapid, "die");
	exits(buf);
}

/*
 *  like strncpy but make sure there's a terminating null
 */
void*
safecpy(void *to, void *from, int n)
{
	char *a = ((char*)to) + n - 1;

	strncpy(to, from, n);
	*a = 0;
	return to;
}

/*
 *  set the error string
 */
int
seterr(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	vsnprint(errstring, sizeof errstring, fmt, arg);
	va_end(arg);
	return -1;
}

/*
 *  create a new node
 */
Node*
newnode(Node *parent, String *name)
{
	Node *np;
	static ulong path;
	Dir d;

	np = mallocz(sizeof(Node), 1);
	if(np == 0)
		fatal("out of memory");

	np->children = 0;
	if(parent){
		np->parent = parent;
		np->sibs = parent->children;
		parent->children = np;
		np->depth = parent->depth + 1;
		d.dev = 0;		/* not stat'd */
	} else {
		/* the root node */
		np->parent = np;
		np->sibs = 0;
		np->depth = 0;
		d.dev = 1;
	}
	np->remname = name;
	d.name = s_to_c(name);
	d.atime = time(0);
	d.mtime = d.atime;
	d.uid = nouid;
	d.gid = nouid;
	d.muid = nouid;
	np->fp = 0;
	d.qid.path = ++path;
	d.qid.vers = 0;
	d.qid.type = QTFILE;
	d.type = 0;
	np->d = reallocdir(&d, 0);

	return np;
}

/*
 *  walk one down the local mirror of the remote directory tree
 */
Node*
extendpath(Node *parent, String *elem)
{
	Node *np;

	for(np = parent->children; np; np = np->sibs)
		if(strcmp(s_to_c(np->remname), s_to_c(elem)) == 0){
			s_free(elem);
			return np;
		}

	return newnode(parent, elem);
}

/*
 *  flush the cached file, write it back if it's dirty
 */
void
uncache(Node *np)
{
	if(fileisdirty(np))
		createfile(np);
	filefree(np);
	UNCACHED(np);
}

/*
 *  invalidate all children of a node
 */
void
invalidate(Node *node)
{
	Node *np;

	if(node->opens)
		return;		/* don't invalidate something that's open */

	uncachedir(node, 0);

	/* invalidate children */
	for(np = node->children; np; np = np->sibs){
		if(np->opens)
			continue;	/* don't invalidate something that's open */
		UNCACHED(np);
		invalidate(np);
		np->d->dev = 0;
	}
}

/*
 *  make a top level tops-20 directory.  They are automaticly valid.
 */
Node*
newtopsdir(char *name)
{
	Node *np;

	np = extendpath(remroot, s_copy(name));
	if(!ISVALID(np)){
		np->d->qid.type = QTDIR;
		np->d->atime = time(0);
		np->d->mtime = np->d->atime;
		np->d->uid = "?uid?";
		np->d->gid = "?uid?";
		np->d->muid = "?uid?";
		np->d->mode = DMDIR|0777;
		np->d->length = 0;
		np->d = reallocdir(np->d, 1);
		
		if(changedir(np) >= 0)
			VALID(np);
	}
	return np;
}

/*
 *  figure out if a symbolic link is to a directory or a file
 */
void
fixsymbolic(Node *node)
{
	if(changedir(node) == 0){
		node->d->mode |= DMDIR;
		node->d->qid.type = QTDIR;
	} else
		node->d->qid.type = QTFILE;
	node->d->mode &= ~DMSYML; 
}
