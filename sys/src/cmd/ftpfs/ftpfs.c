#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
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
char	errstring[ERRLEN];	/* error to return */
int	mfd;			/* fd for 9fs */
char	mdata[MAXMSG+MAXFDATA];	/* buffer for 9fs messages */
Fcall	rhdr;
Fcall	thdr;
int	debug;
int	usenlst;
char	*ext;
int	quiet;
int	kapid = -1;
int	dying;		/* set when any process decides to die */
int	dokeepalive;

char	*rflush(Fid*), *rnop(Fid*), *rsession(Fid*),
	*rattach(Fid*), *rclone(Fid*), *rwalk(Fid*),
	*rclwalk(Fid*), *ropen(Fid*), *rcreate(Fid*),
	*rread(Fid*), *rwrite(Fid*), *rclunk(Fid*),
	*rremove(Fid*), *rstat(Fid*), *rwstat(Fid*);
void	mountinit(char*);
void	io(void);
int	readpdir(Node*);

char 	*(*fcalls[])(Fid*) = {
	[Tflush]	rflush,
	[Tsession]	rsession,
	[Tnop]		rnop,
	[Tattach]	rattach,
	[Tclone]	rclone,
	[Twalk]		rwalk,
	[Tclwalk]	rclwalk,
	[Topen]		ropen,
	[Tcreate]	rcreate,
	[Tread]		rread,
	[Twrite]	rwrite,
	[Tclunk]	rclunk,
	[Tremove]	rremove,
	[Tstat]		rstat,
	[Twstat]	rwstat,
};

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
	{ OS½,		"OS/2", },
	{ TSO,		"TSO", },
	{ NT,		"Windows_NT", },	/* DOS like interface */
	{ NT,		"WINDOWS_NT", },	/* Unix like interface */
	{ Unknown,	0 },
};

#define S2P(x) (((ulong)(x)) & 0xffffff)

char *nosuchfile = "file does not exist";

void
usage(void)
{
	fprint(2, "ftpfs [-/dqn] [-a passwd] [-m mountpoint] [-e ext] [-o os] [-r root] [net!]address\n");
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
	if (cpassword == 0)
		rlogin();
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
		fmtinstall('F', fcallconv); /* debugging */
		io();
		quit();
		break;
	default:
		close(p[0]);
		if(mount(p[1], mountpoint, MREPL|MCREATE, "") < 0)
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
	ff->node = 0;
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
	char *err;
	int n;

	kapid = kaproc();

	while(!dying){
		n = read(mfd, mdata, sizeof mdata);
		if(n <= 0)
			fatal("mount read");
		if(convM2S(mdata, &rhdr, n) == 0)
			continue;

		if(debug)
			fprint(2, "<-%F\n", &rhdr);/**/

		thdr.data = mdata + MAXMSG;
		if(!fcalls[rhdr.type])
			err = "bad fcall type";
		else
			err = (*fcalls[rhdr.type])(newfid(rhdr.fid));
		if(err){
			thdr.type = Rerror;
			strncpy(thdr.ename, err, ERRLEN);
		}else{
			thdr.type = rhdr.type + 1;
			thdr.fid = rhdr.fid;
		}
		thdr.tag = rhdr.tag;
		if(debug)
			fprint(2, "->%F\n", &thdr);/**/
		n = convS2M(&thdr, mdata);
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
rsession(Fid *f)
{
	USED(f);
	memset(thdr.authid, 0, sizeof(thdr.authid));
	memset(thdr.authdom, 0, sizeof(thdr.authdom));
	memset(thdr.chal, 0, sizeof(thdr.chal));
	return 0;
}

char*
rflush(Fid *f)
{
	USED(f);
	return 0;
}

char*
rattach(Fid *f)
{
	f->busy = 1;
	f->node = remroot;
	thdr.qid = f->node->d.qid;
	return 0;
}

char*
rclone(Fid *f)
{
	Fid *nf;

	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->node = f->node;
	return 0;
}

char*
rwalk(Fid *f)
{
	char *name;
	Node *np;

	if((f->node->d.qid.path & CHDIR) == 0)
		return "not a directory";

	name = rhdr.name;
	if(strcmp(name, ".") == 0){
		thdr.qid = f->node->d.qid;
		return 0;
	}
	if(strcmp(name, "..") == 0){
		f->node = f->node->parent;
		thdr.qid = f->node->d.qid;
		return 0;
	}
	if(strcmp(name, ".flush.ftpfs") == 0){
		/* hack to flush the cache */
		invalidate(f->node);
		readdir(f->node);
	}

	/* top level tops-20 names are special */
	if((os == Tops || os == VM || os == VMS) && f->node == remroot){
		np = newtopsdir(name);
		if(np){
			f->node = np;
			thdr.qid = f->node->d.qid;
			return 0;
		} else
			return nosuchfile;
	}

	f->node = extendpath(f->node, name);
	if(ISCACHED(f->node->parent)){
		/* the cache of the parent directory is good, believe it */
		if(!ISVALID(f->node))
			return nosuchfile;
		if(f->node->parent->chdirunknown || (f->node->d.mode & CHSYML))
			fixsymbolic(f->node);
	} else if(!ISVALID(f->node)){
		/* this isn't a valid node, try cd'ing to see if it's a dir */
		if(changedir(f->node) == 0){
			f->node->d.qid.path |= CHDIR;
			f->node->d.mode |= CHDIR;
		}else{
			f->node->d.qid.path &= ~CHDIR;
			f->node->d.mode &= ~CHDIR;
		}
	}

	thdr.qid = f->node->d.qid;
	return 0;
}

char *
rclwalk(Fid *f)
{
	Fid *nf;
	char *err;

	nf = newfid(rhdr.newfid);
	nf->busy = 1;
	nf->node = f->node;
	if(err = rwalk(nf))
		rclunk(nf);
	return err;
}

char *
ropen(Fid *f)
{
	int mode;

	mode = rhdr.mode;
	if(f->node->d.qid.path & CHDIR)
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
			if(f->node->d.qid.path & CHDIR){
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

	thdr.qid = f->node->d.qid;
	f->open = 1;
	f->node->opens++;
	return 0;
}

char*
rcreate(Fid *f)
{
	char *name;

	if((f->node->d.qid.path&CHDIR) == 0)
		return "not a directory";

	name = rhdr.name;
	f->node = extendpath(f->node, name);
	uncache(f->node);
	if(rhdr.perm & CHDIR){
		if(createdir(f->node) < 0)
			return "permission denied";
	} else
		filedirty(f->node);
	invalidate(f->node->parent);
	uncache(f->node->parent);

	thdr.qid = f->node->d.qid;
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

	thdr.count = 0;
	off = rhdr.offset;
	cnt = rhdr.count;
	if(cnt > MAXFDATA)
		cnt = MAXFDATA;

	if(f->node->d.qid.path & CHDIR){
		rv = 0;
		if((off % DIRLEN) != 0)
			return "bad offset";
		if((cnt < DIRLEN) != 0)
			return "bad length";
		n = off/DIRLEN;
		for(np = f->node->children; n && np; np = np->sibs)
			if(ISVALID(np))
				n--;
		if(n == 0){
			n = cnt/DIRLEN;
			for(; n && np; np = np->sibs){
				if(ISVALID(np)){
					if(np->d.mode & CHSYML)
						fixsymbolic(np);
					convD2M(&np->d, thdr.data + rv);
					rv += DIRLEN;
					n--;
				}
			}
		}
	} else {
		/* reread file if it's fallen out of the cache */
		if(!ISCACHED(f->node))
			if(readfile(f->node) < 0)
				return errstring;
		CACHED(f->node);
		rv = fileread(f->node, thdr.data, off, cnt);
		if(rv < 0)
			return errstring;
	}

	thdr.count = rv;
	return 0;
}

char*
rwrite(Fid *f)
{
	long off;
	int cnt;

	if(f->node->d.qid.path & CHDIR)
		return "directories are not writable";

	thdr.count = 0;
	off = rhdr.offset;
	cnt = rhdr.count;
	cnt = filewrite(f->node, rhdr.data, off, cnt);
	if(cnt < 0)
		return errstring;
	filedirty(f->node);
	thdr.count = cnt;
	return 0;
}

char *
rclunk(Fid *f)
{
	if(fileisdirty(f->node)){
		if(createfile(f->node) < 0)
			fprint(2, "ftpfs: couldn't create %s\n", f->node->d.name);
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
	if(CHDIR & f->node->d.qid.path){
		if(removedir(f->node) < 0)
			return errstring;
	} else {
		if(removefile(f->node) < 0)
			return errstring;
	}
	uncache(f->node->parent);
	uncache(f->node);
	INVALID(f->node);
	f->busy = 0;
	return 0;
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
	convD2M(&f->node->d, thdr.stat);
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
	char buf[8*1024], *s;

	dying = 1;

	va_start(arg, fmt);
	s = doprint(buf, buf + (sizeof(buf)-1) / sizeof(*buf), fmt, arg);
	va_end(arg);

	*s = 0;
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
	char *s;
	va_list arg;

	va_start(arg, fmt);
	s = doprint(errstring, errstring + (sizeof(errstring)-1) / sizeof(*errstring), fmt, arg);
	va_end(arg);
	*s = 0;
	return -1;
}

/*
 *  create a new node
 */
Node*
newnode(Node *parent, char *name)
{
	Node *np;
	static ulong path;

	np = mallocz(sizeof(Node), 1);
	if(np == 0)
		fatal("out of memory");
	safecpy(np->d.name, name, sizeof(np->d.name));
	np->d.atime = time(0);
	np->children = 0;
	np->longname = strdup(name);
	if(parent){
		np->parent = parent;
		np->sibs = parent->children;
		parent->children = np;
		np->depth = parent->depth + 1;
		np->d.dev = 0;		/* not stat'd */
	} else {
		/* the root node */
		np->parent = np;
		np->sibs = 0;
		np->depth = 0;
		np->d.dev = 1;
		strcpy(np->d.uid, "ken");
		strcpy(np->d.gid, "rob");
		np->d.mtime = np->d.atime;
	}
	np->fp = 0;
	np->d.qid.path = ++path;
	np->d.qid.vers = 0;

	return np;
}

/*
 *  walk one down the local mirror of the remote directory tree
 */
Node*
extendpath(Node *parent, char *elem)
{
	Node *np;

	for(np = parent->children; np; np = np->sibs)
		if(strcmp(np->d.name, elem) == 0)
			return np;

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
		np->d.dev = 0;
	}
}

/*
 *  make a top level tops-20 directory.  They are automaticly valid.
 */
Node*
newtopsdir(char *name)
{
	Node *np;

	np = extendpath(remroot, name);
	if(!ISVALID(np)){
		np->d.qid.path |= CHDIR;
		np->d.atime = time(0);
		np->d.mtime = np->d.atime;
		strcpy(np->d.uid, "who");
		strcpy(np->d.gid, "cares");
		np->d.mode = CHDIR|0777;
		np->d.length = 0;
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
		node->d.mode |= CHDIR;
		node->d.qid.path |= CHDIR;
	}
	node->d.mode &= ~CHSYML; 
}
