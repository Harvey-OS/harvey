#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "dat.h"
#include "fns.h"

extern Fcall	thdr;
extern Fcall	rhdr;
extern char	data[sizeof(Fcall)+MAXFDATA];
extern char	fdata[MAXFDATA];
extern int	errno;

void
rnop(void)
{
	chat("nop...");
}
void
rsession(void)
{
	memset(thdr.authid, 0, sizeof(thdr.authid));
	memset(thdr.authdom, 0, sizeof(thdr.authdom));
	memset(thdr.chal, 0, sizeof(thdr.chal));
	chat("session...");
}
void
rflush(void)
{
	chat("flush...");
}
void
rattach(void)
{
	Xfs *xf;
	Xfile *root;

	chat("attach(fid=%d,uname=\"%s\",aname=\"%s\",auth=\"%s\")...",
		thdr.fid, thdr.uname, thdr.aname, thdr.auth);
	
	root = xfile(thdr.fid, Clean);
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
	
	root->qid.path = CHDIR;
	root->qid.vers = 0;
	root->xf->rootqid = root->qid;
	root->pinbr = EXT2_ROOT_INODE;
	root->root = 1;
	rhdr.qid = root->qid;
	
	return;
error:
	if(root)
		xfile(thdr.fid, Clunk);
	return;
}
void
rclone(void)
{
	Xfile *of = xfile(thdr.fid, Asis);
	Xfile *nf = xfile(thdr.newfid, Clean);

	chat("clone(fid=%d,newfid=%d)...", thdr.fid, thdr.newfid);
	if(!of)
		errno = Eio;
	else if(!nf)
		errno = Enomem;
	else{
		Xfile *next = nf->next;
		*nf = *of;
		nf->next = next;
		nf->fid = thdr.newfid;
		nf->root = 0;
	}
}
void
rwalk(void)
{
	Xfile *f=xfile(thdr.fid, Asis);
	int nr, sinbr = 0;

	chat("walk(fid=%d,name=\"%s\")...", thdr.fid, thdr.name);
	if( !f ){
		chat("no xfile...");
		goto error;
	}
	if( !(f->qid.path & CHDIR) ){
		chat("qid.path=0x%x...", f->qid.path);
		goto error;
	}
	sinbr = f->pinbr;
	if( !strcmp(thdr.name, ".") ){
		rhdr.qid = f->qid;
		return;
	}else if( !strcmp(thdr.name, "..") ){
		if( f->qid.path == f->xf->rootqid.path ){
			chat("walkup from root...");
			rhdr.qid = f->qid;
			return;
		}
		if( get_inode(f, f->pinbr) < 0 )
			goto error;
		if( f->pinbr == EXT2_ROOT_INODE ){
			f->qid.path = f->xf->rootqid.path;
			f->pinbr = EXT2_ROOT_INODE;
		} else {
			f->qid.path = f->pinbr | CHDIR;
			f->inbr = f->pinbr;
			if( (nr = get_file(f, "..")) < 0 )
				goto error;
			f->pinbr = nr;
		}
	}else{
		f->pinbr = f->inbr;
		if( (nr = get_file(f, thdr.name)) < 0 )
			goto error;
		if( get_inode(f, nr) < 0 )
			goto error;
		f->qid.path = nr;
		if( nr == EXT2_ROOT_INODE )
			f->qid.path = f->xf->rootqid.path;
		else if( S_ISDIR(getmode(f)) )
			 f->qid.path |= CHDIR;
		/*strcpy(f->name, thdr.name);*/
	}
	rhdr.qid = f->qid;
	return;
error:
	f->pinbr = sinbr;
	f->qid.path = -1;
	f->qid.vers = -1;
	errno = Enonexist;
	return;
}
void
rstat(void)
{
	Dir dir;
	Xfile *f=xfile(thdr.fid, Asis);

	chat("stat(fid=%d)...", thdr.fid);
	if( !f )
		errno = Eio;
	else{
		dostat(f, &dir);
		convD2M(&dir, rhdr.stat);
	}
}
void
rwstat(void)
{
	Xfile *f=xfile(thdr.fid, Asis);

	chat("wstat(fid=%d)...", thdr.fid);
	if( !f )
		errno = Eio;
	else 
		dowstat(f, (Dir *)thdr.stat);	
	
}
void
rread(void)
{
	Xfile *f; 
	int r;

	chat("read(fid=%d,offset=%d,count=%d)...",
		thdr.fid, thdr.offset, thdr.count);
	if ( !(f=xfile(thdr.fid, Asis)) || !(f->flags&Oread) )
		goto error;
	if( f->qid.path & CHDIR ){
		thdr.count = (thdr.count / DIRLEN) * DIRLEN;
		if(thdr.count<DIRLEN || thdr.offset%DIRLEN){
			chat("count=%d,offset=%d,DIRLEN=%d...", 
				thdr.count, thdr.offset, DIRLEN);
			goto error;
		}
		r = readdir(f, fdata, thdr.offset, thdr.count);
	}else
		r = readfile(f, fdata, thdr.offset, thdr.count);
	
	if(r >= 0){
		rhdr.count = r;
		rhdr.data = fdata;
		chat("rcnt=%d...", r);
		return;
	}
error:
		errno = Eio;
	
}
void
rwrite(void)
{
	Xfile *f; int r;
	
	chat("write(fid=%d,offset=%d,count=%d)...",
		thdr.fid, thdr.offset, thdr.count);

	if (!(f=xfile(thdr.fid, Asis)) || !(f->flags&Owrite)){
		errno = Eio;
		return;
	}
	if( S_ISLNK(getmode(f)) ){
		errno = Elink;
		return;
	}
	r = writefile(f, thdr.data, thdr.offset, thdr.count);
	if(r >= 0){	
		rhdr.count = r;
		chat("rcnt=%d...", r);
	}
}
void
rclunk(void)
{
	/*Xfile *f = xfile(thdr.fid, Asis);*/
	/*Xfs *xf = f->xf;*/

	chat("clunk(fid=%d)...", thdr.fid);
	xfile(thdr.fid, Clunk);
	/*syncbuf(xf);*/
}
void
ropen(void)
{
	Xfile *f;
	int omode=0;

	chat("open(fid=%d,mode=%d)...", thdr.fid, thdr.mode);

	f = xfile(thdr.fid, Asis);
	if( !f || (f->flags&Omodes)){
		errno = Eio;
		return;
	}
	
	if(thdr.mode & ORCLOSE)
		omode |= Orclose;

	switch(thdr.mode & 7){
	case OREAD:
	case OEXEC:
		omode |= Oread;
		break;
	case ORDWR:
		omode |= Oread;
		/* fall through */
	case OWRITE:
		omode |= Owrite;
		break;
	default:
		errno = Eio;
		return;
	}
	if(thdr.mode & OTRUNC){
		if( (f->qid.path & CHDIR) || S_ISLNK(getmode(f)) ){
			errno = Eperm;
			return;
		}
		if(truncfile(f) < 0){
			return;
		}
	}
	f->flags |= omode;
	chat("f->qid=0x%8.8lux...", f->qid.path);
	rhdr.qid = f->qid;
}
void
rcreate(void)
{
	Xfile *f;
	int inr, perm, omode=0;

	chat("create(fid=%d,name=\"%s\",perm=%uo,mode=%d)...",
		thdr.fid, thdr.name, thdr.perm, thdr.mode);

	if(strcmp(thdr.name, ".") == 0 || strcmp(thdr.name, "..") == 0){
		errno = Eperm;
		return;
	}
	f = xfile(thdr.fid, Asis);
	if( !f ){
		errno = Eio;
		return;
	}
	if( f->flags&Omodes ){
		chat("create on open file...");
		errno = Eio;
		return;
	}
	if( !(f->qid.path & CHDIR) ){
		chat("create file in non-directory...");
		errno = Eperm;
		return;
	}
	if( strlen(thdr.name) > NAMELEN ){
		chat("name too long ...");
		errno = Elongname;
		return;
	}

	/* create */
	if( thdr.perm & CHDIR ){
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
		return;

	/* fill with new inode */
	f->pinbr = f->inbr;
	if( get_inode(f, inr) < 0 ){
		errno = Eio;
		return;
	}
	f->qid.path = inr;
	if( S_ISDIR(getmode(f)) )
		f->qid.path |= CHDIR;
	/* do open */
	if(thdr.mode & ORCLOSE)
		omode |= Orclose;
	switch(thdr.mode & 7){
	case OREAD:
	case OEXEC:
		omode |= Oread;
		break;
	case ORDWR:
		omode |= Oread;
		/* fall through */
	case OWRITE:
		omode |= Owrite;
		break;
	default:
		errno = Eio;
		return;
	}
	f->flags |= omode;
	chat("f->qid=0x%8.8lux...", f->qid.path);
	rhdr.qid = f->qid;

}
void
rremove(void)
{
	Xfile *f=xfile(thdr.fid, Asis);

	chat("remove(fid=%d,name=\"%s\") ...", thdr.fid, thdr.name);

	if(!f){
		errno = Eio;
		return;
	}

	/* check permission here !!!!*/

	unlink(f);

	/* do a clunk here as the protocol say */

	xfile(thdr.fid, Clunk);
}

