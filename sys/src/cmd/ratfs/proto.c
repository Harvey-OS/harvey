#include "ratfs.h"

/*
 *	9P protocol interface
 */

enum {
	RELOAD = 0,		/* commands written to ctl file */
	RDEBUG,
	RNODEBUG,
	RNONE,
};

static void	rflush(Fcall*),		rnop(Fcall*),
		rauth(Fcall*),	rattach(Fcall*),
		rclone(Fcall*),		rwalk(Fcall*),
		rclwalk(Fcall*),	ropen(Fcall*),
		rcreate(Fcall*),	rread(Fcall*),
		rwrite(Fcall*),		rclunk(Fcall*),
		rremove(Fcall*),	rstat(Fcall*),
		rwstat(Fcall*),	rversion(Fcall*);

static	Fid*	newfid(int);
static	void	reply(Fcall*, char*);

static	void 	(*fcalls[])(Fcall*) = {
	[Tversion]	rversion,
	[Tflush]	rflush,
	[Tauth]	rauth,
	[Tattach]	rattach,
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


static	Keyword cmds[] = {
	"reload",		RELOAD,
	"debug",		RDEBUG,
	"nodebug",		RNODEBUG,
	0,			RNONE,
};

/*
 *	Main protocol loop
 */
void
io(void)
{
	Fcall	rhdr;
	int n;
	
	for(;;){
		n = read9pmsg(srvfd, rbuf, sizeof rbuf-1);
		if(n <= 0)
			fatal("mount read");
		if(convM2S(rbuf, n, &rhdr) == 0){
			if(debugfd >= 0)
				fprint(2, "%s: malformed message\n", argv0);
			continue;
		}
		
		if(debugfd >= 0)
			fprint(debugfd, "<-%F\n", &rhdr);/**/

		if(!fcalls[rhdr.type])
			reply(&rhdr, "bad fcall type");
		else
			(*fcalls[rhdr.type])(&rhdr);
	}
}

/*
 *	write a protocol reply to the client
 */
static void
reply(Fcall *r, char *error)
{
	int n;

	if(error == nil)
		r->type++;
	else {
		r->type = Rerror;
		r->ename = error;
	}
	if(debugfd >= 0)
		fprint(debugfd, "->%F\n", r);/**/
	n = convS2M(r, rbuf, sizeof rbuf);
	if(n == 0)
		sysfatal("convS2M: %r");
	if(write(srvfd, rbuf, n) < 0)
		sysfatal("reply: %r");
}


/*
 *  lookup a fid. if not found, create a new one.
 */

static Fid*
newfid(int fid)
{
	Fid *f, *ff;

	static Fid *fids;

	ff = 0;
	for(f = fids; f; f = f->next){
		if(f->fid == fid){
			if(!f->busy)
				f->node = 0;
			return f;
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

static void
rversion(Fcall *f)
{
	f->version = "9P2000";
	if(f->msize > MAXRPC)
		f->msize = MAXRPC;
	reply(f, 0);
}

static void
rauth(Fcall *f)
{
	reply(f, "ratfs: authentication not required");
}

static void
rflush(Fcall *f)
{
	reply(f, 0);
}

static void
rattach(Fcall *f)
{
	Fid *fidp;
	Dir *d;

	if((d=dirstat(conffile)) != nil && d->mtime > lastconftime)
		getconf();
	free(d);
	if((d=dirstat(ctlfile)) != nil && d->mtime > lastctltime)
		reload();
	free(d);
	cleantrusted();

	fidp = newfid(f->fid);
	fidp->busy = 1;
	fidp->node = root;
	fidp->name = root->d.name;
	fidp->uid = atom(f->uname);
	f->qid = root->d.qid;
	reply(f,0);
}

static void
rclone(Fcall *f)
{
	Fid *fidp, *nf;

	fidp = newfid(f->fid);
	if(fidp->node && fidp->node->d.type == Dummynode){
		reply(f, "can't clone an address");
		return;
	}
	nf = newfid(f->newfid);
	nf->busy = 1;
	nf->node = fidp->node;
	nf->uid = fidp->uid;
	nf->name = fidp->name;
	if(debugfd >= 0)
		printfid(nf);
	reply(f,0);
}

static void
rwalk(Fcall *f)
{
	int i, j;
	Fcall r;
	Fid *fidp, *nf;
	char *err;

	fidp = newfid(f->fid);
	if(fidp->node && fidp->node->d.type == Dummynode){
		reply(f, "can't walk an address node");
		return;
	}
	if(f->fid == f->newfid)
		nf = fidp;
	else{
		nf = newfid(f->newfid);
		nf->busy = 1;
		nf->node = fidp->node;
		nf->uid = fidp->uid;
		nf->name = fidp->name;
		if(debugfd >= 0)
			printfid(nf);
	}

	err = nil;
	for(i=0; i<f->nwname; i++){
		err = walk(f->wname[i], nf);
		if(err)
			break;
		r.wqid[i] = nf->node->d.qid;
	}
	

	if(i < f->nwname && f->fid != f->newfid){
		nf->busy = 0;
		nf->node = 0;
		nf->name = 0;
		nf->uid = 0;
	}
	if(i > 0 && i < f->nwname && f->fid == f->newfid){
		/*
		 * try to put things back;
		 * we never get this sort of call from the kernel
		 */
		for(j=0; j<i; j++)
			walk("..", nf);
	}
	memmove(f->wqid, r.wqid, sizeof f->wqid);
	f->nwqid = i;
	if(err && i==0)
		reply(f, err);
	else
		reply(f, 0);
}

/*
 *	We don't have to do full permission checking because most files
 *	have restricted semantics:
 *		The ctl file is only writable
 *		All others, including directories, are only readable
 */
static void
ropen(Fcall *f)
{
	Fid *fidp;
	int mode;

	fidp = newfid(f->fid);

	if(debugfd >= 0)
		printfid(fidp);

	mode = f->mode&(OREAD|OWRITE|ORDWR);
	if(fidp->node->d.type == Ctlfile) {
		if(mode != OWRITE) {	
			reply(f, "permission denied");
			return;
		}
	} else
	if (mode != OREAD) {
		reply(f, "permission denied or operation not supported");
		return;
	}

	f->qid = fidp->node->d.qid;
	fidp->open = 1;
	reply(f, 0);
}

static int
permitted(Fid *fp, Node *np, int mask)
{
	int mode;

	mode = np->d.mode;
	return (fp->uid==np->d.uid && (mode&(mask<<6)))
		|| (fp->uid==np->d.gid && (mode&(mask<<3)))
		|| (mode&mask);
}

/*
 *	creates are only allowed in the "trusted" subdirectory
 *	we also assume that the groupid == the uid
 */
static void
rcreate(Fcall *f)
{
	Fid *fidp;
	Node *np;

	fidp = newfid(f->fid);
	np = fidp->node;
	if((np->d.mode&DMDIR) == 0){
		reply(f, "not a directory");
		return;
	}

	if(!permitted(fidp, np, AWRITE)) {
		reply(f, "permission denied");
		return;
	}

	/* Ignore the supplied mode and force it to be non-writable */

	np = newnode(np, f->name, Trustedtemp, 0444, trustedqid++);
	if(trustedqid >= Qaddrfile)			/* wrap QIDs */
		trustedqid = Qtrustedfile;
	cidrparse(&np->ip, f->name);
	f->qid = np->d.qid;
	np->d.uid = fidp->uid;
	np->d.gid = np->d.uid;
	np->d.muid = np->d.muid;
	fidp->node = np;
	fidp->open = 1;
	reply(f, 0);
	return;
}

/*
 *	only directories can be read.  everthing else returns EOF.
 */
static void
rread(Fcall *f)
{
	long cnt;
	Fid *fidp;

	cnt = f->count;
	f->count = 0;
	fidp = newfid(f->fid);
	f->data = (char*)rbuf+IOHDRSZ;
	if(fidp->open == 0) {
		reply(f, "file not open");
		return;
	}
	if ((fidp->node->d.mode&DMDIR) == 0){
		reply(f, 0);				/*EOF*/
		return;
	}
	if(cnt > MAXRPC)
		cnt = MAXRPC;

	if(f->offset == 0)
		fidp->dirindex = 0;

	switch(fidp->node->d.type) {
	case Directory:
	case Addrdir:
	case Trusted:
		f->count = dread(fidp, cnt);
		break;
	case IPaddr:
	case Acctaddr:
		f->count = hread(fidp, cnt);
		break;
	default:
		reply(f, "can't read this type of file");
		return;
	}
	reply(f, 0);
}


/*
 * 	only the 'ctl' file in the top level directory is writable
 */

static void
rwrite(Fcall *f)
{
	Fid *fidp;
	int n;
	char *err, *argv[10];

	fidp = newfid(f->fid);
	if(fidp->node->d.mode & DMDIR){
		reply(f, "directories are not writable");
		return;
	}
	if(fidp->open == 0) {
		reply(f, "file not open");
		return;
	}

	if (!permitted(fidp, fidp->node, AWRITE)) {
		reply(f, "permission denied");
		return;
	}

	f->data[f->count] = 0;			/* the extra byte in rbuf leaves room */
	n = tokenize(f->data, argv, 10);
	err = 0;
	switch(findkey(argv[0], cmds)){
	case RELOAD:
		getconf();
		reload();
		break;
	case RDEBUG:
		if(n > 1){
			debugfd = create(argv[1], OWRITE, 0666);
			if(debugfd < 0)
				err = "create failed";
		} else
			debugfd = 2;
		break;
	case RNODEBUG:
		if(debugfd >= 0)
			close(debugfd);
		debugfd = -1;
		break;
	default:
		err = "unknown command";
		break;
	}
	reply(f, err);
}

static void
rclunk(Fcall *f)
{
	Fid *fidp;

	fidp = newfid(f->fid);
	fidp->open = 0;
	fidp->busy = 0;
	fidp->node = 0;
	fidp->name = 0;
	fidp->uid = 0;
	reply(f, 0);
}

/*
 *  no files or directories are removable; this becomes clunk;
 */
static void
rremove(Fcall *f)
{
	Fid *fidp;
	Node *dir, *np;

	fidp = newfid(f->fid);
	
	/*
	 * only trusted temporary files can be removed
	 * and only by their owner.
	 */
	if(fidp->node->d.type != Trustedtemp){
		reply(f, "can't be removed");
		return;
	}
	if(fidp->uid != fidp->node->d.uid){
		reply(f, "permission denied");
		return;
	}
	dir = fidp->node->parent;
	for(np = dir->children; np; np = np->sibs)
		if(np->sibs == fidp->node)
			break;
	if(np)
		np->sibs = fidp->node->sibs;
	else
		dir->children = fidp->node->sibs;
	dir->count--;
	free(fidp->node);
	fidp->node = 0;
	fidp->open = 0;
	fidp->busy = 0;
	fidp->name = 0;
	fidp->uid = 0;
	reply(f, 0);
}

static void
rstat(Fcall *f)
{
	Fid *fidp;

	fidp = newfid(f->fid);
	if (fidp->node->d.type == Dummynode)
		dummy.d.name = fidp->name;
	f->stat = (uchar*)rbuf+4+1+2+2;	/* knows about stat(5) */
	f->nstat = convD2M(&fidp->node->d, f->stat, MAXRPC);
	if(f->nstat <= BIT16SZ)
		reply(f, "ratfs: convD2M");
	else
		reply(f, 0);
	return;
}

static void
rwstat(Fcall *f)
{
	reply(f, "wstat not implemented");
}

