#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include "9p.h"
#include "impl.h"

enum {
	Tclwalk_walk = Tmax,
	Tclwalk_clone
};

static char Enowrite[] = "write prohibited";
static char Enowstat[] = "wstat prohibited";
static char Enoremove[] = "remove prohibited";
static char Enocreate[] = "create prohibited";
static char Ebotch[] = "9P protocol botch";
static char Ebadattach[] = "unknown specifier in attach";

int lib9p_chatty;
void (*endsrv)(void*);
static void
setfcallerror(Fcall *f, char *err)
{
	strncpy(f->ename, err, sizeof f->ename);
	f->ename[sizeof f->ename - 1] = '\0';
	f->type = Rerror;
}

static void
sendreply(Req *r, int fd)
{
	int n;
	char *buf;

	r->type++;
	r->ofcall.type = r->type;
	if(r->error)
		setfcallerror(&r->ofcall, r->error);

	if(r->ofcall.type == Rread)
		buf = emalloc(MAXMSG+MAXFDATA);
	else
		buf = emalloc(MAXMSG);
if(lib9p_chatty)
	fprint(2, "-> %F\n", &r->ofcall);
	switch(r->type){
	default:
		sysfatal("bad type %d in reply", r->ofcall.type);
		break;

	case Ropen:
	case Rnop:
	case Rflush:
	case Rattach:
	case Rsession:
	case Rclone:
	case Rwalk:
	case Rclwalk:
	case Rcreate:
	case Rread:
	case Rwrite:
	case Rclunk:
	case Rremove:
	case Rstat:
	case Rwstat:
	case Rerror:
		n = convS2M(&r->ofcall, buf);
		if(write(fd, buf, n) != n)
			sysfatal("write %d fails: %r", fd);
		free(buf);
	}
}

static char Eduptag[] = "duplicate tag";
static char Edupfid[] = "duplicate fid";
static char Eunknownfid[] = "unknown fid";
static char Eperm[] = "permission denied";
static char Enotfound[] = "file not found";

Req*
getreq(Reqpool *rpool, Fidpool *fpool, int fd)
{
	long n;
	char *buf;
	Fcall f;
	Req *r;

	n = MAXMSG+MAXFDATA;
	buf = mallocz(n, 1);
	if(buf == nil)
		return nil;

	if(getS(fd, buf, &f, &n) != nil){
		free(buf);
		return nil;
	}
	if((r=allocreq(rpool, f.tag)) == nil){	/* duplicate tag: cons up a Req */
		r = mallocz(sizeof *r, 1);
		if(r == nil){
			free(buf);
			return nil;
		}
		incref(&r->ref);
		r->tag = f.tag;
		r->fcall = f;
		r->error = Eduptag;
		r->buf = buf;
		r->responded = 0;
		return r;
	}

	r->responded = 0;
	r->buf = buf;
	r->fcall = f;
	r->ofcall = f;
	r->type = r->fcall.type;
	switch(r->type){
	case Tnop:
	case Tsession:
		break;

	case Tflush:
		r->oldreq = lookupreq(rpool, r->fcall.oldtag);
		if(r->oldreq == r)
			r->error = "you can't flush yourself";
		break;

	case Tattach:
		if((r->fid = allocfid(fpool, r->fcall.fid)) == nil)
			r->error = Edupfid;
		break;

	case Tclone:
	case Tclwalk:
		if((r->fid = lookupfid(fpool, r->fcall.fid)) == nil)
			r->error = Eunknownfid;
		else if(r->fid->omode != -1)
			r->error = "cannot clone open fid";
		else if((r->newfid = allocfid(fpool, r->fcall.newfid)) == nil)
			r->error = Edupfid;
		break;

	case Twalk:
	case Topen:
	case Tcreate:
	case Tread:
	case Twrite:
	case Tclunk:
	case Tremove:
	case Tstat:
	case Twstat:
		if((r->fid = lookupfid(fpool, r->fcall.fid)) == nil)
			r->error = Eunknownfid;
		break;
	}
if(lib9p_chatty)
	if(r->error)
		fprint(2, "<- %F: %s\n", &r->fcall, r->error);
	else	
		fprint(2, "<- %F\n", &r->fcall);

	return r;
}


static char Ebadcount[] = "bad count";
static char Ebadoffset[] = "bad offset";

void
srv(Srv *srv, int fd)
{
	int o, p;
	Req *r;
	File *f, *of;

	fmtinstall('D', dirconv);
	fmtinstall('F', fcallconv);

	if(srv->fpool == nil)
		srv->fpool = allocfidpool();
	if(srv->rpool == nil)
		srv->rpool = allocreqpool();

	srv->rpool->srv = srv;
	srv->fd = fd;

	while(r = getreq(srv->rpool, srv->fpool, fd)){
		if(r->error){
			respond(r, r->error);
			continue;	
		}

		switch(r->type){
		default:
			abort();

		case Tnop:
			respond(r, nil);
			break;

		case Tsession:
			memset(r->ofcall.authid, 0, sizeof r->ofcall.authid);
			memset(r->ofcall.authdom, 0, sizeof r->ofcall.authdom);
			memset(r->ofcall.chal, 0, sizeof r->ofcall.chal);
			if(srv->session)
				srv->session(r, r->ofcall.authid,
					r->ofcall.authdom, r->ofcall.chal);
			else
				respond(r, nil);
			break;

		case Tflush:
			if(r->oldreq && srv->flush)
				srv->flush(r, r->oldreq);
			else
				respond(r, nil);
			break;

		case Tattach:
			strncpy(r->fid->uid, r->ofcall.uname, NAMELEN);
			r->fid->uid[NAMELEN-1] = '\0';
			if(srv->attach)
				srv->attach(r, r->fid, r->ofcall.aname, &r->ofcall.qid);
			else{		/* assume one tree, no auth */
				if(strcmp(r->ofcall.aname, "") != 0)
					r->error = Ebadattach;
				else{
					r->fid->file = srv->tree->root;
					r->ofcall.qid = r->fid->file->qid;
				}
				respond(r, nil);
			}
			break;

		case Tclwalk:
			r->type = Tclwalk_clone;
			/* fall through */
		case Tclone:
			if(r->fid->file)
				incref(&r->fid->file->ref);
			r->newfid->file = r->fid->file;

			strcpy(r->newfid->uid, r->fid->uid);
			r->newfid->qid = r->fid->qid;
			r->newfid->aux = r->fid->aux;

			if(srv->clone)
				srv->clone(r, r->fid, r->newfid);
			else
				respond(r, nil);
			break;
			
		case Twalk:
			/* if you change this, change the copy in respond Tclwalk_clone above */

			if(r->fid->file){
				/*
				 * We don't use the walk function if using file
				 * trees.  It's too much to expect clients to get 
				 * the reference counts right on error or when
				 * playing other funny games with the tree.
				 */
				if(f = fwalk(r->fid->file, r->fcall.name)) {
					of = r->fid->file;
					r->fid->file = f;
					fclose(of);
					r->ofcall.qid = f->qid;
					respond(r, nil);
				}else
					respond(r, Enotfound);
				break;
			}
			r->ofcall.qid = r->fid->qid;
			srv->walk(r, r->fid, r->fcall.name, &r->ofcall.qid);
			break;

		case Topen:
			if(r->fid->omode != -1){
				respond(r, Ebotch);
				break;
			}
			r->ofcall.qid = r->fid->qid;
			if(r->fid->file){
				switch(r->fcall.mode&3){
				case OREAD:	p = AREAD; break;
				case OWRITE:	p = AWRITE; break;
				case ORDWR:	p = AREAD|AWRITE; break;
				case OEXEC:	p = AEXEC; break;
				default:	p = ~0; assert(0);
				}
				if(r->fcall.mode & OTRUNC)
					p |= AWRITE;
				if(!hasperm(r->fid->file, r->fid->uid, p)){
					respond(r, Eperm);
					break;
				}
				if((r->fcall.mode & ORCLOSE)
				&& !hasperm(r->fid->file->parent, r->fid->uid, AWRITE)){
					respond(r, Eperm);
					break;
				}
				r->ofcall.qid = r->fid->file->qid;
			}
			if(srv->open)
				srv->open(r, r->fid, r->fcall.mode, &r->ofcall.qid);
			else
				respond(r, nil);
			break;

		case Tcreate:
			if(r->fid->omode != -1)
				r->error = Ebotch;
			else if(r->fid->file && !hasperm(r->fid->file, r->fid->uid, AWRITE))
				r->error = Eperm;
			else if(srv->create == nil)
				r->error = Enocreate;

			if(r->error)
				respond(r, r->error);
			else
				srv->create(r, r->fid, r->ofcall.name, r->ofcall.mode,
					r->ofcall.perm, &r->ofcall.qid);
			break;

		case Tread:
			r->rbuf = emalloc(MAXFDATA);
			r->ofcall.data = r->rbuf;
			o = r->fid->omode & OMASK;
			r->ofcall.count = r->fcall.count;
			if(r->ofcall.count > MAXFDATA)
				r->ofcall.count = MAXFDATA;
			if(o != OREAD && o != ORDWR && o != OEXEC)
				r->error = Ebotch;
			else if(r->ofcall.count < 0)
				r->error = Ebadcount;
			else if(r->ofcall.offset < 0 
				|| ((r->fid->qid.path&CHDIR) && r->fcall.offset%DIRLEN))
				r->error = Ebadoffset;
			else if((r->fid->qid.path&CHDIR) && r->fid->file){
				r->error = fdirread(r->fid->file, r->rbuf, 
							&r->ofcall.count, r->fcall.offset);
			}else{
				srv->read(r, r->fid, r->rbuf, &r->ofcall.count, r->fcall.offset);
				break;
			}
			respond(r, r->error);
			break;

		case Twrite:
			o = r->fid->omode & OMASK;
			if(o != OWRITE && o != ORDWR)
				r->error = Ebotch;
			else if(srv->write == nil)
				r->error = Enowrite;
			else{
				r->ofcall.count = r->fcall.count;
				srv->write(r, r->fid, r->fcall.data,
					&r->ofcall.count, r->fcall.offset);
				break;
			}
			respond(r, r->error);
			break;

		case Tclunk:
			if(srv->clunkaux)
				srv->clunkaux(r->fid);
			respond(r, nil);
			break;

		case Tremove:
			if(r->fid->file
			&& !hasperm(r->fid->file->parent, r->fid->uid, AWRITE)){
				respond(r, Eperm);
				break;
			}
			if(srv->remove == nil){
				if(r->fid->file)
					respond(r, nil);
				else
					respond(r, Enoremove);
				break;
			}
			srv->remove(r, r->fid);
			break;

		case Tstat:
			if(r->fid->file)
				r->d = r->fid->file->Dir;
			else if(srv->stat == nil){
				respond(r, "no stat");
				break;
			}

			if(srv->stat)
				srv->stat(r, r->fid, &r->d);
			else
				respond(r, nil);
			break;
			
		case Twstat:
			if(srv->wstat == nil)
				respond(r, nil);
			else{
				convM2D(r->fcall.stat, &r->d);
				srv->wstat(r, r->fid, &r->d);
			}
			break;
		}
	}
}

void
respond(Req *r, char *error)
{
	File *f, *of;
	Srv *srv;

	assert(r->responded == 0);
	r->responded = 1;
	srv = r->pool->srv;

	switch(r->type){
	default:
		abort();

	case Tnop:
		break;

	case Tflush:
		freereq(r->oldreq);
		r->oldreq = nil;
		break;

	case Tattach:
		if(error)
			freefid(r->fid);
		else {
			r->fid->qid = r->ofcall.qid;
			closefid(r->fid);
		}
		r->fid = nil;
		break;

	case Tsession:
		break;

	case Tclone:
		closefid(r->fid);
		r->fid = nil;
		if(error)
			freefid(r->newfid);
		else
			closefid(r->newfid);
		r->newfid = nil;
		break;

	case Tclwalk:	/* can't happen */
		abort();

	/*
	 * In order to use the clone and walk functions rather
	 * than require a clwalk, the response here is an error
	 * if there was one, but otherwise we call the walk
	 * function, who will eventually call us again (next case).
	 */
	case Tclwalk_clone:
		closefid(r->fid);
		r->fid = nil;
		if(error){
			freefid(r->newfid);
			r->newfid = nil;
			break;
		}

		r->fid = r->newfid;
		r->newfid = nil;
		r->type = Tclwalk_walk;

		/* copy of case Twalk in srv below */
		if(r->fid->file){
			/*
			 * We don't use the walk function if using file
			 * trees.  It's too much to expect clients to get 
			 * the reference counts right on error or when
			 * playing other funny games with the tree.
			 */
			if(f = fwalk(r->fid->file, r->fcall.name)) {
				of = r->fid->file;
				r->fid->file = f;
				fclose(of);
				r->fid->qid = f->qid;
				r->ofcall.qid = r->fid->qid;
				respond(r, nil);
			}else
				respond(r, Enotfound);
			break;
		}
		r->ofcall.qid = r->fid->qid;
		srv->walk(r, r->fid, r->fcall.name, &r->ofcall.qid);
		break;

	case Tclwalk_walk:
		r->type = Tclwalk;
		if(error)
			freefid(r->fid);
		else{
			r->fid->qid = r->ofcall.qid;
			closefid(r->fid);
		}
		r->fid = nil;
		break;

	case Twalk:
		if(error == nil)
			r->fid->qid = r->ofcall.qid;
		closefid(r->fid);
		r->fid = nil;
		break;

	case Topen:
		if(error == nil){
			r->fid->omode = r->fcall.mode;
			r->fid->qid = r->ofcall.qid;
		}
		closefid(r->fid);
		r->fid = nil;
		break;

	case Tcreate:
		if(error == nil){
			r->fid->omode = r->fcall.mode;
			r->fid->qid = r->ofcall.qid;
		}
		closefid(r->fid);
		r->fid = nil;
		break;

	case Tread:
		closefid(r->fid);
		r->fid = nil;
		break;

	case Twrite:
		if(r->fid->file)
			r->fid->file->qid.vers++;
		closefid(r->fid);
		r->fid = nil;
		break;

	case Tclunk:
		freefid(r->fid);
		r->fid = nil;
		break;

	case Tremove:
		if(error == nil) {
			if(r->fid->file) {
				fremove(r->fid->file);
				r->fid->file = nil;
			}
		}
		if(srv->clunkaux)
			srv->clunkaux(r->fid);
		freefid(r->fid);
		r->fid = nil;
		break;

	case Tstat:
		if(error == nil)
			convD2M(&r->d, r->ofcall.stat);
		closefid(r->fid);
		r->fid = nil;
		break;

	case Twstat:
		closefid(r->fid);
		r->fid = nil;
		break;
	}

	assert(r->type != Tclwalk_clone);

	if(r->type == Tclwalk_walk)
		return;

	r->error = error;
	sendreply(r, r->pool->srv->fd);
	freereq(r);
}


/*
 *  read a message from fd and convert it.
 *  ignore 0-length messages.
 */
char *
getS(int fd, char *buf, Fcall *f, long *lp)
{
	long m, n;
	int i;
	char *errstr;

	errstr = "EOF";
	n = 0;
	for(i = 0; i < 3; i++){
		n = read(fd, buf, *lp);
		if(n == 0){
			continue;
		}
		if(n < 0)
			return "read error";
		m = convM2S(buf, f, n);
		if(m == 0){
			errstr = "bad type";
			continue;
		}
		*lp = m;
		return 0;
	}
	*lp = n;
	return errstr;
}

void
_postfd(char *name, int pfd)
{
	char buf[2*NAMELEN];
	int fd;

	snprint(buf, sizeof buf, "/srv/%s", name);

	fd = create(buf, OWRITE|ORCLOSE, 0600);
	if(fd == -1)
		sysfatal("post %s: %r", name);
	fprint(fd, "%d", pfd);
}

void
postmountsrv(Srv *s, char *name, char *mtpt, int flag)
{
	int fd[2];
	if(pipe(fd) < 0)
		sysfatal("pipe");
	if(name)
		_postfd(name, fd[0]);
	switch(rfork(RFPROC|RFFDG|RFNOTEG|RFNAMEG|RFMEM)){
	case -1:
		sysfatal("fork");
	case 0:
		close(fd[0]);
		srv(s, fd[1]);
		if(endsrv)
			endsrv(nil);
		_exits(0);
	default:
		if(mtpt)
			if(mount(fd[0], mtpt, flag, "") == -1)
				fprint(2, "mount %s: %r\n", mtpt);
	}
}

