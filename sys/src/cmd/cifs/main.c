#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <libsec.h>
#include <9p.h>
#include "cifs.h"

#define max(a,b)	(((a) > (b))? (a): (b))
#define min(a,b)	(((a) < (b))? (a): (b))

typedef struct Aux Aux;
struct Aux {
	Aux	*next;
	Aux	*prev;
	char	*path;		/* full path fo file */
	Share	*sp;		/* this share's info */
	long	expire;		/* expiration time of cache */
	long	off;		/* file pos of start of cache */
	long	end;		/* file pos of end of cache */
	char	*cache;
	int	fh;		/* file handle */
	int	sh;		/* search handle */
	long	srch;		/* find first's internal state */
};

extern int chatty9p;

int Checkcase = 1;		/* enforce case significance on filenames */
int Dfstout = 100;		/* timeout (in ms) for ping of dfs servers (assume they are local)  */
int Billtrog = 1;		/* enable file owner/group resolution */
int Attachpid;			/* pid of proc that attaches (ugh !) */
char *Debug = nil;		/* messages */
Qid Root;			/* root of remote system */
Share Ipc;			/* Share info of IPC$ share */
Session *Sess;			/* current session */
int Active = IDLE_TIME;		/* secs until next keepalive is sent */
static int Keeppid;		/* process ID of keepalive thread */
Share Shares[MAX_SHARES]; 	/* table of connected shares */
int Nshares = 0;		/* number of Shares connected */
Aux *Auxroot = nil;		/* linked list of Aux structs */
char *Host = nil;		/* host we are connected to */

static char *Ipcname = "IPC$";

#define ptype(x)	(((x) & 0xf))
#define pindex(x)	(((x) & 0xff0) >> 4)

void
setup(void)
{
	int fd;
	char buf[32];

	/*
	 * This is revolting but I cannot see any other way to get
	 * the pid of the server.  We need this as Windows doesn't
	 * drop the TCP connection when it closes a connection.
	 * Thus we keepalive() to detect when/if we are thrown off.
	 */
	Attachpid = getpid();

	snprint(buf, sizeof buf, "#p/%d/args", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		fprint(fd, "%s network", Host);
		close(fd);
	}
}

int
filetableinfo(Fmt *f)
{
	Aux *ap;
	char *type;

	if((ap = Auxroot) != nil)
		do{
			type = "walked";
			if(ap->sh != -1)
				type = "opendir";
			if(ap->fh != -1)
				type = "openfile";
			fmtprint(f, "%-9s %s\n", type, ap->path);
			ap = ap->next;
		}while(ap != Auxroot);
	return 0;
}

Qid
mkqid(char *s, int is_dir, long vers, int subtype, long path)
{
	Qid q;
	union {				/* align digest suitably */
		uchar	digest[SHA1dlen];
		uvlong	uvl;
	} u;

	sha1((uchar *)s, strlen(s), u.digest, nil);
	q.type = (is_dir)? QTDIR: 0;
	q.vers = vers;
	if(subtype){
		q.path = *((uvlong *)u.digest) & ~0xfffL;
		q.path |= ((path & 0xff) << 4);
		q.path |= (subtype & 0xf);
	}
	else
		q.path = *((uvlong *)u.digest) & ~0xfL;
	return q;
}

/*
 * used only for root dir and shares
 */
static void
V2D(Dir *d, Qid qid, char *name)
{
	memset(d, 0, sizeof(Dir));
	d->type = 'C';
	d->dev = 1;
	d->name = estrdup9p(name);
	d->uid = estrdup9p("bill");
	d->muid = estrdup9p("boyd");
	d->gid = estrdup9p("trog");
	d->mode = 0755 | DMDIR;
	d->atime = time(nil);
	d->mtime = d->atime;
	d->length = 0;
	d->qid = qid;
}

static void
I2D(Dir *d, Share *sp, char *path, FInfo *fi)
{
	char *name;

	if((name = strrchr(fi->name, '\\')) != nil)
		name++;
	else
		name = fi->name;
	d->name = estrdup9p(name);
	d->type = 'C';
	d->dev = sp->tid;
	d->uid = estrdup9p("bill");
	d->gid = estrdup9p("trog");
	d->muid = estrdup9p("boyd");
	d->atime = fi->accessed;
	d->mtime = fi->written;

	if(fi->attribs & ATTR_READONLY)
		d->mode = 0444;
	else
		d->mode = 0666;

	d->length = fi->size;
	d->qid = mkqid(path, fi->attribs & ATTR_DIRECTORY, fi->changed, 0, 0);

	if(fi->attribs & ATTR_DIRECTORY){
		d->length = 0;
		d->mode |= DMDIR|0111;
	}
}

static void
responderrstr(Req *r)
{
	char e[ERRMAX];

	*e = 0;
	rerrstr(e, sizeof e);
	respond(r, e);
}

static char *
newpath(char *path, char *name)
{
	char *p, *q;

	assert((p = strrchr(path, '/')) != nil);

	if(strcmp(name, "..") == 0){
		if(p == path)
			return estrdup9p("/");
		q = emalloc9p((p-path)+1);
		strecpy(q, q+(p-path)+1, path);
		return q;
	}
	if(strcmp(path, "/") == 0)
		return smprint("/%s", name);
	return smprint("%s/%s", path, name);
}

static int
dirgen(int slot, Dir *d, void *aux)
{
	long off;
	FInfo *fi;
	int rc, got;
	Aux *a = aux;
	char *npath;
	int numinf = numinfo();
	int slots = min(Sess->mtu, MTU) / sizeof(FInfo);

	if(strcmp(a->path, "/") == 0){
		if(slot < numinf){
			dirgeninfo(slot, d);
			return 0;
		} else
			slot -= numinf;

		if(slot >= Nshares)
			return -1;
		V2D(d, mkqid(Shares[slot].name, 1, 1, Pshare, slot),
			Shares[slot].name);
		return 0;
	}

	off = slot * sizeof(FInfo);
	if(off >= a->off && off < a->end && time(nil) < a->expire)
		goto from_cache;

	if(off == 0){
		fi = (FInfo *)a->cache;
		npath = smprint("%s/*", mapfile(a->path));
		a->sh = T2findfirst(Sess, a->sp, slots, npath, &got, &a->srch,
			(FInfo *)a->cache);
		free(npath);
		if(a->sh == -1)
			return -1;

		a->off = 0;
		a->end = got * sizeof(FInfo);

		if(got >= 2 && strcmp(fi[0].name, ".") == 0 &&
		    strcmp(fi[1].name, "..") == 0){
			a->end = (got - 2) * sizeof(FInfo);
			memmove(a->cache, a->cache + sizeof(FInfo)*2,
				a->end - a->off);
		}
	}

	while(off >= a->end && a->sh != -1){
		fi = (FInfo *)(a->cache + (a->end - a->off) - sizeof(FInfo));
		a->off = a->end;
		npath = smprint("%s/%s", mapfile(a->path), fi->name);
		rc = T2findnext(Sess, a->sp, slots, npath,
			&got, &a->srch, (FInfo *)a->cache, a->sh);
		free(npath);
		if(rc == -1 || got == 0)
			break;
		a->end = a->off + got * sizeof(FInfo);
	}
	a->expire = time(nil) + CACHETIME;

	if(got < slots){
		if(a->sh != -1)
			CIFSfindclose2(Sess, a->sp, a->sh);
		a->sh = -1;
	}

	if(off >= a->end)
		return -1;

from_cache:
	fi = (FInfo *)(a->cache + (off - a->off));
	npath = smprint("%s/%s", mapfile(a->path), fi->name);
	I2D(d, a->sp, npath, fi);
	if(Billtrog == 0)
		upd_names(Sess, a->sp, npath, d);
	free(npath);
	return 0;
}

static void
fsattach(Req *r)
{
	Aux *a;
	static int first = 1;
	char *spec = r->ifcall.aname;

	if(first)
		setup();

	if(spec && *spec){
		respond(r, "invalid attach specifier");
		return;
	}

	r->ofcall.qid = mkqid("/", 1, 1, Proot, 0);
	r->fid->qid = r->ofcall.qid;

	a = r->fid->aux = emalloc9p(sizeof(Aux));
	memset(a, 0, sizeof(Aux));
	a->path = estrdup9p("/");
	a->sp = nil;
	a->fh = -1;
	a->sh = -1;

	if(Auxroot){
		a->prev = Auxroot;
		a->next = Auxroot->next;
		Auxroot->next->prev = a;
		Auxroot->next = a;
	} else {
		Auxroot = a;
		a->next = a;
		a->prev = a;
	}
	respond(r, nil);
}

static char*
fsclone(Fid *ofid, Fid *fid)
{
	Aux *oa = ofid->aux;
	Aux *a = emalloc9p(sizeof(Aux));

	fid->aux = a;

	memset(a, 0, sizeof(Aux));
	a->sh = -1;
	a->fh = -1;
	a->sp = oa->sp;
	a->path = estrdup9p(oa->path);

	if(Auxroot){
		a->prev = Auxroot;
		a->next = Auxroot->next;
		Auxroot->next->prev = a;
		Auxroot->next = a;
	} else {
		Auxroot = a;
		a->next = a;
		a->prev = a;
	}
	return nil;
}

/*
 * for some weird reason T2queryall() returns share names
 * in lower case so we have to do an extra test against
 * our share table to validate filename case.
 *
 * on top of this here (snell & Wilcox) most of our
 * redirections point to a share of the same name,
 * but some do not, thus the tail of the filename
 * returned by T2queryall() is not the same as
 * the name we wanted.
 *
 * We work around this by not validating the names
 * or files which resolve to share names as they must
 * be correct, having been enforced in the dfs layer.
 */
static int
validfile(char *found, char *want, char *winpath, Share *sp)
{
	char *share;

	if(strcmp(want, "..") == 0)
		return 1;
	if(strcmp(winpath, "/") == 0){
		share = trimshare(sp->name);
		if(cistrcmp(want, share) == 0)
			return strcmp(want, share) == 0;
		/*
		 * OK, a DFS redirection points us from a directory XXX
		 * to a share named YYY.  There is no case checking we can
		 * do so we allow either case - it's all we can do.
		 */
		return 1;
	}
	if(cistrcmp(found, want) != 0)
		return 0;
	if(!Checkcase)
		return 1;
	if(strcmp(found, want) == 0)
		return 1;
	return 0;
}


static char*
fswalk1(Fid *fid, char *name, Qid *qid)
{
	FInfo fi;
	int rc, n, i;
	Aux *a = fid->aux;
	static char e[ERRMAX];
	char *p, *npath, *winpath;

	*e = 0;
	npath = newpath(a->path, name);
	if(strcmp(npath, "/") == 0){			/* root dir */
		*qid = mkqid("/", 1, 1, Proot, 0);
		free(a->path);
		a->path = npath;
		fid->qid = *qid;
		return nil;
	}

	if(strrchr(npath, '/') == npath){		/* top level dir */
		if((n = walkinfo(name)) != -1){		/* info file */
			*qid = mkqid(npath, 0, 1, Pinfo, n);
		}
		else {					/* volume name */
			for(i = 0; i < Nshares; i++){
				n = strlen(Shares[i].name);
				if(cistrncmp(npath+1, Shares[i].name, n) != 0)
					continue;
				if(Checkcase && strncmp(npath+1, Shares[i].name, n) != 0)
					continue;
				if(npath[n+1] != 0 && npath[n+1] != '/')
					continue;
				break;
			}
			if(i >= Nshares){
				free(npath);
				return "not found";
			}
			a->sp = Shares+i;
			*qid = mkqid(npath, 1, 1, Pshare, i);
		}
		free(a->path);
		a->path = npath;
		fid->qid = *qid;
		return nil;
	}

	/* must be a vanilla file or directory */
again:
	if(mapshare(npath, &a->sp) == -1){
		rerrstr(e, sizeof(e));
		free(npath);
		return e;
	}

	winpath = mapfile(npath);
	memset(&fi, 0, sizeof fi);
	if(Sess->caps & CAP_NT_SMBS)
		rc = T2queryall(Sess, a->sp, winpath, &fi);
	else
		rc = T2querystandard(Sess, a->sp, winpath, &fi);

	if(rc == -1){
		rerrstr(e, sizeof(e));
		free(npath);
		return e;
	}

	if((a->sp->options & SMB_SHARE_IS_IN_DFS) != 0 &&
	    (fi.attribs & ATTR_REPARSE) != 0){
		if(redirect(Sess, a->sp, npath) != -1)
			goto again;
	}

	if((p = strrchr(fi.name, '/')) == nil && (p = strrchr(fi.name, '\\')) == nil)
		p = fi.name;
	else
		p++;

	if(! validfile(p, name, winpath, a->sp)){
		free(npath);
		return "not found";

	}
	*qid = mkqid(npath, fi.attribs & ATTR_DIRECTORY, fi.changed, 0, 0);

	free(a->path);
	a->path = npath;
	fid->qid = *qid;
	return nil;
}

static void
fsstat(Req *r)
{
	int rc;
	FInfo fi;
	Aux *a = r->fid->aux;

	if(ptype(r->fid->qid.path) == Proot)
		V2D(&r->d, r->fid->qid, "");
	else if(ptype(r->fid->qid.path) == Pinfo)
		dirgeninfo(pindex(r->fid->qid.path), &r->d);
	else if(ptype(r->fid->qid.path) == Pshare)
		V2D(&r->d, r->fid->qid, a->path +1);
	else{
		memset(&fi, 0, sizeof fi);
		if(Sess->caps & CAP_NT_SMBS)
			rc = T2queryall(Sess, a->sp, mapfile(a->path), &fi);
		else
			rc = T2querystandard(Sess, a->sp, mapfile(a->path), &fi);
		if(rc == -1){
			responderrstr(r);
			return;
		}
		I2D(&r->d, a->sp, a->path, &fi);
		if(Billtrog == 0)
			upd_names(Sess, a->sp, mapfile(a->path), &r->d);
	}
	respond(r, nil);
}

static int
smbcreateopen(Aux *a, char *path, int mode, int perm, int is_create,
	int is_dir, FInfo *fip)
{
	int rc, action, attrs, access, result;

	if(is_create && is_dir){
		if(CIFScreatedirectory(Sess, a->sp, path) == -1)
			return -1;
		return 0;
	}

	if(mode & DMAPPEND) {
		werrstr("filesystem does not support DMAPPEND");
		return -1;
	}

	if(is_create)
		action = 0x12;
	else if(mode & OTRUNC)
		action = 0x02;
	else
		action = 0x01;

	if(perm & 0222)
		attrs = ATTR_NORMAL;
	else
		attrs = ATTR_NORMAL|ATTR_READONLY;

	switch (mode & OMASK){
	case OREAD:
		access = 0;
		break;
	case OWRITE:
		access = 1;
		break;
	case ORDWR:
		access = 2;
		break;
	case OEXEC:
		access = 3;
		break;
	default:
		werrstr("%d bad open mode", mode & OMASK);
		return -1;
		break;
	}

	if(mode & DMEXCL == 0)
		access |= 0x10;
	else
		access |= 0x40;

	if((a->fh = CIFS_SMB_opencreate(Sess, a->sp, path, access, attrs,
	    action, &result)) == -1)
		return -1;

	if(Sess->caps & CAP_NT_SMBS)
		rc = T2queryall(Sess, a->sp, mapfile(a->path), fip);
	else
		rc = T2querystandard(Sess, a->sp, mapfile(a->path), fip);
	if(rc == -1){
		fprint(2, "internal error: stat of newly open/created file failed\n");
		return -1;
	}

	if((mode & OEXCL) && (result & 0x8000) == 0){
		werrstr("%d bad open mode", mode & OMASK);
		return -1;
	}
	return 0;
}

/* Uncle Bill, you have a lot to answer for... */
static int
ntcreateopen(Aux *a, char *path, int mode, int perm, int is_create,
	int is_dir, FInfo *fip)
{
	int options, result, attrs, flags, access, action, share;

	if(mode & DMAPPEND){
		werrstr("CIFSopen, DMAPPEND not supported");
		return -1;
	}

	if(is_create){
		if(mode & OEXCL)
			action = FILE_OPEN;
		else if(mode & OTRUNC)
			action = FILE_CREATE;
		else
			action = FILE_OVERWRITE_IF;
	} else {
		if(mode & OTRUNC)
			action = FILE_OVERWRITE_IF;
		else
			action = FILE_OPEN_IF;
	}

	flags = 0;		/* FIXME: really not sure */

	if(mode & OEXCL)
		share = FILE_NO_SHARE;
	else
		share = FILE_SHARE_ALL;

	switch (mode & OMASK){
	case OREAD:
		access = GENERIC_READ;
		break;
	case OWRITE:
		access = GENERIC_WRITE;
		break;
	case ORDWR:
		access = GENERIC_ALL;
		break;
	case OEXEC:
		access = GENERIC_EXECUTE;
		break;
	default:
		werrstr("%d bad open mode", mode & OMASK);
		return -1;
		break;
	}

	if(is_dir){
		action = FILE_CREATE;
		options = FILE_DIRECTORY_FILE;
		if(perm & 0222)
			attrs = ATTR_DIRECTORY;
		else
			attrs = ATTR_DIRECTORY|ATTR_READONLY;
	} else {
		options = FILE_NON_DIRECTORY_FILE;
		if(perm & 0222)
			attrs = ATTR_NORMAL;
		else
			attrs = ATTR_NORMAL|ATTR_READONLY;
	}

	if(mode & ORCLOSE){
		options |= FILE_DELETE_ON_CLOSE;
		attrs |= ATTR_DELETE_ON_CLOSE;
	}

	if((a->fh = CIFS_NT_opencreate(Sess, a->sp, path, flags, options,
	    attrs, access, share, action, &result, fip)) == -1)
		return -1;

	if((mode & OEXCL) && (result & 0x8000) == 0){
		werrstr("%d bad open mode", mode & OMASK);
		return -1;
	}

	return 0;
}

static void
fscreate(Req *r)
{
	FInfo fi;
	int rc, is_dir;
	char *npath;
	Aux *a = r->fid->aux;

	a->end = a->off = 0;
	a->cache = emalloc9p(max(Sess->mtu, MTU));

	is_dir = (r->ifcall.perm & DMDIR) == DMDIR;
	npath = smprint("%s/%s", a->path, r->ifcall.name);

	if(Sess->caps & CAP_NT_SMBS)
		rc = ntcreateopen(a, mapfile(npath), r->ifcall.mode,
			r->ifcall.perm, 1, is_dir, &fi);
	else
		rc = smbcreateopen(a, mapfile(npath), r->ifcall.mode,
			r->ifcall.perm, 1, is_dir, &fi);
	if(rc == -1){
		free(npath);
		responderrstr(r);
		return;
	}

	r->fid->qid = mkqid(npath, fi.attribs & ATTR_DIRECTORY, fi.changed, 0, 0);

	r->ofcall.qid = r->fid->qid;
	free(a->path);
	a->path = npath;

	respond(r, nil);
}

static void
fsopen(Req *r)
{
	int rc;
	FInfo fi;
	Aux *a = r->fid->aux;

	a->end = a->off = 0;
	a->cache = emalloc9p(max(Sess->mtu, MTU));

	if(ptype(r->fid->qid.path) == Pinfo){
		if(makeinfo(pindex(r->fid->qid.path)) != -1)
			respond(r, nil);
		else
			respond(r, "cannot generate info");
		return;
	}

	if(r->fid->qid.type & QTDIR){
		respond(r, nil);
		return;
	}

	if(Sess->caps & CAP_NT_SMBS)
		rc = ntcreateopen(a, mapfile(a->path), r->ifcall.mode, 0777,
			0, 0, &fi);
	else
		rc = smbcreateopen(a, mapfile(a->path), r->ifcall.mode, 0777,
			0, 0, &fi);
	if(rc == -1){
		responderrstr(r);
		return;
	}
	respond(r, nil);
}

static void
fswrite(Req *r)
{
	vlong n, m, got;
	Aux *a = r->fid->aux;
	vlong len = r->ifcall.count;
	vlong off = r->ifcall.offset;
	char *buf = r->ifcall.data;

	got = 0;
	n = Sess->mtu -OVERHEAD;
	do{
		if(len - got < n)
			n = len - got;
		m = CIFSwrite(Sess, a->sp, a->fh, off + got, buf + got, n);
		if(m != -1)
			got += m;
	} while(got < len && m >= n);

	r->ofcall.count = got;
	if(m == -1)
		responderrstr(r);
	else
		respond(r, nil);
}

static void
fsread(Req *r)
{
	vlong n, m, got;
	Aux *a = r->fid->aux;
	char *buf = r->ofcall.data;
	vlong len = r->ifcall.count;
	vlong off = r->ifcall.offset;

	if(ptype(r->fid->qid.path) == Pinfo){
		r->ofcall.count = readinfo(pindex(r->fid->qid.path), buf, len,
			off);
		respond(r, nil);
		return;
	}

	if(r->fid->qid.type & QTDIR){
		dirread9p(r, dirgen, a);
		respond(r, nil);
		return;
	}

	got = 0;
	n = Sess->mtu -OVERHEAD;
	do{
		if(len - got < n)
			n = len - got;
		m = CIFSread(Sess, a->sp, a->fh, off + got, buf + got, n, len);
		if(m != -1)
			got += m;
	} while(got < len && m >= n);

	r->ofcall.count = got;
	if(m == -1)
		responderrstr(r);
	else
		respond(r, nil);
}

static void
fsdestroyfid(Fid *f)
{
	Aux *a = f->aux;

	if(ptype(f->qid.path) == Pinfo)
		freeinfo(pindex(f->qid.path));
	f->omode = -1;
	if(! a)
		return;
	if(a->fh != -1)
		if(CIFSclose(Sess, a->sp, a->fh) == -1)
			fprint(2, "%s: close failed fh=%d %r\n", argv0, a->fh);
	if(a->sh != -1)
		if(CIFSfindclose2(Sess, a->sp, a->sh) == -1)
			fprint(2, "%s: findclose failed sh=%d %r\n",
				argv0, a->sh);
	if(a->path)
		free(a->path);
	if(a->cache)
		free(a->cache);

	if(a == Auxroot)
		Auxroot = a->next;
	a->prev->next = a->next;
	a->next->prev = a->prev;
	if(a->next == a->prev)
		Auxroot = nil;
	if(a)
		free(a);
}

int
rdonly(Session *s, Share *sp, char *path, int rdonly)
{
	int rc;
	FInfo fi;

	if(Sess->caps & CAP_NT_SMBS)
		rc = T2queryall(s, sp, path, &fi);
	else
		rc = T2querystandard(s, sp, path, &fi);
	if(rc == -1)
		return -1;

	if((rdonly && !(fi.attribs & ATTR_READONLY)) ||
	    (!rdonly && (fi.attribs & ATTR_READONLY))){
		fi.attribs &= ~ATTR_READONLY;
		fi.attribs |= rdonly? ATTR_READONLY: 0;
		rc = CIFSsetinfo(s, sp, path, &fi);
	}
	return rc;
}

static void
fsremove(Req *r)
{
	int try, rc;
	char e[ERRMAX];
	Aux *ap, *a = r->fid->aux;

	*e = 0;
	if(ptype(r->fid->qid.path) == Proot ||
	   ptype(r->fid->qid.path) == Pshare){
		respond(r, "illegal operation");
		return;
	}

	/* close all instences of this file/dir */
	if((ap = Auxroot) != nil)
		do{
			if(strcmp(ap->path, a->path) == 0){
				if(ap->sh != -1)
					CIFSfindclose2(Sess, ap->sp, ap->sh);
				ap->sh = -1;
				if(ap->fh != -1)
					CIFSclose(Sess, ap->sp, ap->fh);
				ap->fh = -1;
			}
			ap = ap->next;
		}while(ap != Auxroot);
	try = 0;
again:
	if(r->fid->qid.type & QTDIR)
		rc = CIFSdeletedirectory(Sess, a->sp, mapfile(a->path));
	else
		rc = CIFSdeletefile(Sess, a->sp, mapfile(a->path));

	rerrstr(e, sizeof(e));
	if(rc == -1 && try++ == 0 && strcmp(e, "permission denied") == 0 &&
	    rdonly(Sess, a->sp, mapfile(a->path), 0) == 0)
		goto again;
	if(rc == -1)
		responderrstr(r);
	else
		respond(r, nil);
}

static void
fswstat(Req *r)
{
	int fh, result, rc;
	FInfo fi, tmpfi;
	char *p, *from, *npath;
	Aux *a = r->fid->aux;

	if(ptype(r->fid->qid.path) == Proot ||
	   ptype(r->fid->qid.path) == Pshare){
		respond(r, "illegal operation");
		return;
	}

	if((r->d.uid && r->d.uid[0]) || (r->d.gid && r->d.gid[0])){
		respond(r, "cannot change ownership");
		return;
	}

	/*
	 * get current info
	 */
	if(Sess->caps & CAP_NT_SMBS)
		rc = T2queryall(Sess, a->sp, mapfile(a->path), &fi);
	else
		rc = T2querystandard(Sess, a->sp, mapfile(a->path), &fi);
	if(rc == -1){
		werrstr("(query) - %r");
		responderrstr(r);
		return;
	}

	/*
	 * always clear the readonly attribute if set,
	 * before trying to set any other fields.
	 * wstat() fails if the file/dir is readonly
	 * and this function is so full of races - who cares about one more?
	 */
	rdonly(Sess, a->sp, mapfile(a->path), 0);

	/*
	 * rename - one piece of joy, renaming open files
	 * is legal (sharing permitting).
	 */
	if(r->d.name && r->d.name[0]){
		if((p = strrchr(a->path, '/')) == nil){
			respond(r, "illegal path");
			return;
		}
		npath = emalloc9p((p-a->path)+strlen(r->d.name)+2);
		strecpy(npath, npath+(p- a->path)+2, a->path);
		strcat(npath, r->d.name);

		from = estrdup9p(mapfile(a->path));
		if(CIFSrename(Sess, a->sp, from, mapfile(npath)) == -1){
			werrstr("(rename) - %r");
			responderrstr(r);
			free(npath);
			free(from);
			return;
		}
		free(from);
		free(a->path);
		a->path = npath;
	}

	/*
	 * set the files length, do this before setting
	 * the file times as open() will alter them
	 */
	if(~r->d.length){
		fi.size = r->d.length;

		if(Sess->caps & CAP_NT_SMBS){
			if((fh = CIFS_NT_opencreate(Sess, a->sp, mapfile(a->path),
			    0, FILE_NON_DIRECTORY_FILE,
	    		    ATTR_NORMAL, GENERIC_WRITE, FILE_SHARE_ALL,
			    FILE_OPEN_IF, &result, &tmpfi)) == -1){
				werrstr("(set length, open) - %r");
				responderrstr(r);
				return;
			}
			rc = T2setfilelength(Sess, a->sp, fh, &fi);
			CIFSclose(Sess, a->sp, fh);
			if(rc == -1){
				werrstr("(set length), set) - %r");
				responderrstr(r);
				return;
			}
		} else {
			if((fh = CIFS_SMB_opencreate(Sess, a->sp, mapfile(a->path),
			    1, ATTR_NORMAL, 1, &result)) == -1){
				werrstr("(set length, open) failed - %r");
				responderrstr(r);
				return;
			}
			rc = CIFSwrite(Sess, a->sp, fh, fi.size, 0, 0);
			CIFSclose(Sess, a->sp, fh);
			if(rc == -1){
				werrstr("(set length, write) - %r");
				responderrstr(r);
				return;
			}
		}
	}

	/*
	 * This doesn't appear to set length or
	 * attributes, no idea why, so I do those seperately
	 */
	if(~r->d.mtime || ~r->d.atime){
		if(~r->d.mtime)
			fi.written = r->d.mtime;
		if(~r->d.atime)
			fi.accessed = r->d.atime;
		if(T2setpathinfo(Sess, a->sp, mapfile(a->path), &fi) == -1){
			werrstr("(set path info) - %r");
			responderrstr(r);
			return;
		}
	}

	/*
	 * always update the readonly flag as
	 * we may have cleared it above.
	 */
	if(~r->d.mode){
		if(r->d.mode & 0222)
			fi.attribs &= ~ATTR_READONLY;
		else
			fi.attribs |= ATTR_READONLY;
	}
	if(rdonly(Sess, a->sp, mapfile(a->path), fi.attribs & ATTR_READONLY) == -1){
		werrstr("(set info) - %r");
		responderrstr(r);
		return;
	}

	/*
	 * Win95 has a broken write-behind cache for metadata
	 * on open files (writes go to the cache, reads bypass
	 * the cache), so we must flush the file.
	 */
	if(r->fid->omode != -1 && CIFSflush(Sess, a->sp, a->fh) == -1){
		werrstr("(flush) %r");
		responderrstr(r);
		return;
	}
	respond(r, nil);
}

static void
fsend(Srv *srv)
{
	int i;
	USED(srv);

	for(i = 0; i < Nshares; i++)
		CIFStreedisconnect(Sess, Shares+i);
	CIFSlogoff(Sess);
	postnote(PNPROC, Keeppid, "die");
}

Srv fs = {
	.destroyfid =	fsdestroyfid,
	.attach=	fsattach,
	.open=		fsopen,
	.create=	fscreate,
	.read=		fsread,
	.write=		fswrite,
	.remove=	fsremove,
	.stat=		fsstat,
	.wstat=		fswstat,
	.clone= 	fsclone,
	.walk1= 	fswalk1,
	.end=		fsend,
};

void
usage(void)
{
	fprint(2, "usage: %s [-d name] [-Dvb] [-a auth-method] [-s srvname] "
		"[-n called-name] [-k factotum-params] [-m mntpnt] "
		"host [share...]\n", argv0);
	exits("usage");
}

/*
 * SMBecho looks like the function to use for keepalives,
 * sadly the echo packet does not seem to reload the
 * idle timer in Microsoft's servers.  Instead we use
 * "get file system size" on each share until we get one that succeeds.
 */
static void
keepalive(void)
{
	char buf[32];
	uvlong tot, fre;
	int fd, i, slot, rc;

	snprint(buf, sizeof buf, "#p/%d/args", getpid());
	if((fd = open(buf, OWRITE)) >= 0){
		fprint(fd, "%s keepalive", Host);
		close(fd);
	}

	rc = 0;
	slot = 0;
	do{
		sleep(6000);
		if(Active-- != 0)
			continue;
		for(i = 0; i < Nshares; i++){
			if((rc = T2fssizeinfo(Sess, &Shares[slot], &tot, &fre)) == 0)
				break;
			if(++slot >= Nshares)
				slot = 0;
		}
	}while(rc != -1);
	postnote(PNPROC, Attachpid, "die");
}


static void
ding(void *u, char *msg)
{
	USED(u);
	if(strstr(msg, "alarm") != nil)
		noted(NCONT);
	noted(NDFLT);
}

void
dmpkey(char *s, void *v, int n)
{
	int i;
	unsigned char *p = (unsigned char *)v;

	print("%s", s);
	for(i = 0; i < n; i++)
		print("%02ux ", *p++);
	print("\n");
}

void
main(int argc, char **argv)
{
	int i, n;
	long svrtime;
	char windom[64], cname[64];
	char *method, *sysname, *keyp, *mtpt, *svs;
	static char *sh[1024];

	*cname = 0;
	keyp = "";
	method = nil;
	strcpy(windom, "unknown");
	mtpt = svs = nil;

	notify(ding);

	ARGBEGIN{
	case 'a':
		method = EARGF(autherr());
		break;
	case 'b':
		Billtrog ^= 1;
		break;
	case 'D':
		chatty9p++;
		break;
	case 'd':
		Debug = EARGF(usage());
		break;
	case 'i':
		Checkcase = 0;
		break;
	case 'k':
		keyp = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 'n':
		strncpy(cname, EARGF(usage()), sizeof(cname));
		cname[sizeof(cname) - 1] = 0;
		break;
	case 's':
		svs = EARGF(usage());
		break;
	case 't':
		Dfstout = atoi(EARGF(usage()));
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc < 1)
		usage();

	Host = argv[0];

	if(mtpt == nil && svs == nil)
		mtpt = smprint("/n/%s", Host);

	if((sysname = getenv("sysname")) == nil)
		sysname = "unknown";

	if(*cname && (Sess = cifsdial(Host, cname, sysname)) != nil)
		goto connected;

	if(calledname(Host, cname) == 0 &&
	    (Sess = cifsdial(Host, cname, sysname)) != nil)
		goto connected;

	strcpy(cname, Host);
	if((Sess = cifsdial(Host, Host, sysname)) != nil ||
	   (Sess = cifsdial(Host, "*SMBSERVER", sysname)) != nil)
		goto connected;

	sysfatal("%s - cannot dial, %r\n", Host);
connected:
	if(CIFSnegotiate(Sess, &svrtime, windom, sizeof windom, cname, sizeof cname) == -1)
		sysfatal("%s - cannot negioate common protocol, %r\n", Host);

#ifndef DEBUG_MAC
	Sess->secmode &= ~SECMODE_SIGN_ENABLED;
#endif

	Sess->auth = getauth(method, windom, keyp, Sess->secmode, Sess->chal,
		Sess->challen);

	if(CIFSsession(Sess) < 0)
		sysfatal("session authentication failed, %r\n");

	Sess->slip = svrtime - time(nil);
	Sess->cname = estrdup9p(cname);

	if(CIFStreeconnect(Sess, cname, Ipcname, &Ipc) == -1)
		fprint(2, "%s, %r - can't connect\n", Ipcname);

	Nshares = 0;
	if(argc == 1){
		Share *sip;

		if((n = RAPshareenum(Sess, &Ipc, &sip)) < 1)
			sysfatal("can't enumerate shares: %r - specify share "
				"names on command line\n");

		for(i = 0; i < n; i++){
#ifdef NO_HIDDEN_SHARES
			int l = strlen(sip[i].name);

			if(l > 1 && sip[i].name[l-1] == '$'){
				free(sip[i].name);
				continue;
			}
#endif
			memcpy(Shares+Nshares, sip+i, sizeof(Share));
			if(CIFStreeconnect(Sess, Sess->cname,
			    Shares[Nshares].name, Shares+Nshares) == -1){
				free(Shares[Nshares].name);
				continue;
			}
			Nshares++;
		}
		free(sip);
	} else
		for(i = 1; i < argc; i++){
			if(CIFStreeconnect(Sess, Sess->cname, argv[i],
			    Shares+Nshares) == -1){
				fprint(2, "%s: %s  %q - can't connect to share"
					", %r\n", argv0, Host, argv[i]);
				continue;
			}
			Shares[Nshares].name = strlwr(estrdup9p(argv[i]));
			Nshares++;
		}

	if(Nshares == 0)
		fprint(2, "no available shares\n");

	if((Keeppid = rfork(RFPROC|RFMEM|RFNOTEG|RFFDG|RFNAMEG)) == 0){
		keepalive();
		exits(nil);
	}
	postmountsrv(&fs, svs, mtpt, MREPL|MCREATE);
	exits(nil);
}
