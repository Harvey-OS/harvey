#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"

#define thdr	r->ifcall
#define rhdr	r->ofcall

extern int	errno;

static void
response(Req *r)
{
	char *err;

	if (errno) {
		err = xerrstr(errno);
		chat("%s\n", err);
		respond(r, err);
	} else {
		chat("OK\n");
		respond(r, nil);
	}
}

static void
rattach(Req *r)
{
	Xfs *xf;
	Xfile *root;

	chat("attach(fid=%d,uname=\"%s\",aname=\"%s\",afid=\"%d\")...",
		thdr.fid, thdr.uname, thdr.aname, thdr.afid);
	
	errno = 0;
	root = xfile(r->fid, Clean);
	if(!root){
		errno = Enomem;
		goto error;
	}
	root->xf = xf = getxfs(thdr.aname);
	if(!xf)
		goto error;
	
	/* now attach root inode */
	if( get_inode(root, EXT2_ROOT_INODE) < 0 )
		goto error;
	
	r->fid->qid.type = QTDIR;
	r->fid->qid.vers = 0;
	root->xf->rootqid = r->fid->qid;
	root->pinbr = EXT2_ROOT_INODE;
	root->root = 1;
	rhdr.qid = r->fid->qid;
	
error:
	response(r);
}
static char *
rclone(Fid *fid, Fid *newfid)
{
	Xfile *of = xfile(fid, Asis);
	Xfile *nf = xfile(newfid, Clean);

	chat("clone(fid=%d,newfid=%d)...", fid->fid, newfid->fid);
	errno = 0;
	if(!of)
		errno = Eio;
	else if(!nf)
		errno = Enomem;
	else{
		Xfile *next = nf->next;
		*nf = *of;
		nf->next = next;
		nf->fid = newfid->fid;
		nf->root = 0;
	}
	chat("%s\n", errno? xerrstr(errno) : "OK");
	return errno ? xerrstr(errno) : 0;
}
static char *
rwalk1(Fid *fid, char *name, Qid *qid)
{
	Xfile *f=xfile(fid, Asis);
	int nr, sinbr = 0;

	chat("walk1(fid=%d,name=\"%s\")...", fid->fid, name);
	errno = 0;
	if( !f ){
		chat("no xfile...");
		goto error;
	}
	if( !(fid->qid.type & QTDIR) ){
		chat("qid.type=0x%x...", fid->qid.type);
		goto error;
	}
	sinbr = f->pinbr;
	if( name == 0 || name[0] == 0 || !strcmp(name, ".") ){
		*qid = fid->qid;
		goto ok;
	}else if( !strcmp(name, "..") ){
		if( fid->qid.path == f->xf->rootqid.path ){
			chat("walkup from root...");
			*qid = fid->qid;
			goto ok;
		}
		if( get_inode(f, f->pinbr) < 0 )
			goto error;
		if( f->pinbr == EXT2_ROOT_INODE ){
			*qid = f->xf->rootqid;
			f->pinbr = EXT2_ROOT_INODE;
		} else {
			*qid = (Qid){f->pinbr,0,QTDIR};
			f->inbr = f->pinbr;
			if( (nr = get_file(f, "..")) < 0 )
				goto error;
			f->pinbr = nr;
		}
	}else{
		f->pinbr = f->inbr;
		if( (nr = get_file(f, name)) < 0 )
			goto error;
		if( get_inode(f, nr) < 0 )
			goto error;
		*qid = (Qid){nr,0,0};
		if( nr == EXT2_ROOT_INODE )
			*qid = f->xf->rootqid;
		else if( S_ISDIR(getmode(f)) )
			 qid->type = QTDIR;
		/*strcpy(f->name, thdr.name);*/
	}
ok:
	chat("OK\n");
	return 0;
error:
	f->pinbr = sinbr;
	chat("%s\n", xerrstr(Enonexist));
	return xerrstr(Enonexist);
}
static void
rstat(Req *r)
{
	Xfile *f=xfile(r->fid, Asis);

	chat("stat(fid=%d)...", thdr.fid);
	errno = 0;
	if( !f )
		errno = Eio;
	else{
		dostat(r->fid->qid, f, &r->d);
	}
	response(r);
}
static void
rwstat(Req *r)
{
	Xfile *f=xfile(r->fid, Asis);

	chat("wstat(fid=%d)...", thdr.fid);
	errno = 0;
	if( !f )
		errno = Eio;
	else
		dowstat(f, &r->d);
	response(r);	
}
static void
rread(Req *r)
{
	Xfile *f; 
	int nr;

	chat("read(fid=%d,offset=%lld,count=%d)...",
		thdr.fid, thdr.offset, thdr.count);
	errno = 0;
	if ( !(f=xfile(r->fid, Asis)) )
		goto error;
	if( r->fid->qid.type & QTDIR ){
		nr = readdir(f, r->rbuf, thdr.offset, thdr.count);
	}else
		nr = readfile(f, r->rbuf, thdr.offset, thdr.count);
	
	if(nr >= 0){
		rhdr.count = nr;
		chat("rcnt=%d...OK\n", nr);
		respond(r, nil);
		return;
	}
error:
	errno = Eio;
	response(r);
}
static void
rwrite(Req *r)
{
	Xfile *f; int nr;
	
	chat("write(fid=%d,offset=%lld,count=%d)...",
		thdr.fid, thdr.offset, thdr.count);

	errno = 0;
	if (!(f=xfile(r->fid, Asis)) ){
		errno = Eio;
		goto error;
	}
	if( !S_ISREG(getmode(f)) ){
		errno = Elink;
		goto error;
	}
	nr = writefile(f, thdr.data, thdr.offset, thdr.count);
	if(nr >= 0){	
		rhdr.count = nr;
		chat("rcnt=%d...OK\n", nr);
		respond(r, nil);
		return;
	}
	errno = Eio;
error:
	response(r);
}
static void
destroyfid(Fid *fid)
{
	chat("destroy(fid=%d)\n", fid->fid);
	xfile(fid, Clunk);
	/*syncbuf(xf);*/
}
static void
ropen(Req *r)
{
	Xfile *f;

	chat("open(fid=%d,mode=%d)...", thdr.fid, thdr.mode);

	errno = 0;
	f = xfile(r->fid, Asis);
	if( !f ){
		errno = Eio;
		goto error;
	}
	
	if(thdr.mode & OTRUNC){
		if( !S_ISREG(getmode(f)) ){
			errno = Eperm;
			goto error;
		}
		if(truncfile(f) < 0){
			goto error;
		}
	}
	chat("f->qid=0x%8.8lux...", r->fid->qid.path);
	rhdr.qid = r->fid->qid;
error:
	response(r);
}
static void
rcreate(Req *r)
{
	Xfile *f;
	int inr, perm;

	chat("create(fid=%d,name=\"%s\",perm=%uo,mode=%d)...",
		thdr.fid, thdr.name, thdr.perm, thdr.mode);

	errno = 0;
	if(strcmp(thdr.name, ".") == 0 || strcmp(thdr.name, "..") == 0){
		errno = Eperm;
		goto error;
	}
	f = xfile(r->fid, Asis);
	if( !f ){
		errno = Eio;
		goto error;
	}
	if( strlen(thdr.name) > EXT2_NAME_LEN ){
		chat("name too long ...");
		errno = Elongname;
		goto error;
	}

	/* create */
	errno = 0;
	if( thdr.perm & DMDIR ){
		perm = (thdr.perm & ~0777) | 
				(getmode(f) & thdr.perm & 0777);
		perm |= S_IFDIR;
		inr = create_dir(f, thdr.name, perm);
	}else{
		perm = (thdr.perm & (~0777|0111)) |
				(getmode(f) & thdr.perm & 0666);
		perm |= S_IFREG;
		inr = create_file(f, thdr.name, perm);
		
	}
	if( inr < 0 )
		goto error;

	/* fill with new inode */
	f->pinbr = f->inbr;
	if( get_inode(f, inr) < 0 ){
		errno = Eio;
		goto error;
	}
	r->fid->qid = (Qid){inr, 0, 0};
	if( S_ISDIR(getmode(f)) )
		r->fid->qid.type |= QTDIR;
	chat("f->qid=0x%8.8lux...", r->fid->qid.path);
	rhdr.qid = r->fid->qid;
error:
	response(r);
}
static void
rremove(Req *r)
{
	Xfile *f=xfile(r->fid, Asis);

	chat("remove(fid=%d) ...", thdr.fid);

	errno = 0;
	if(!f){
		errno = Eio;
		goto error;
	}

	/* check permission here !!!!*/

	unlink(f);

error:
	response(r);
}

Srv ext2srv = {
	.destroyfid =	destroyfid,
	.attach =	rattach,
	.stat =		rstat,
	.wstat =	rwstat,
	.clone =	rclone,
	.walk1 =	rwalk1,
	.open =		ropen,
	.read =		rread,
	.write =	rwrite,
	.create =	rcreate,
	.remove =	rremove,
};
