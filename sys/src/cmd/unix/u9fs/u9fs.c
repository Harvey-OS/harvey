/* already in plan9.h #include <sys/types.h> *//* for struct passwd, struct group, struct stat ... */
/* plan9.h is first to get the large file support definitions as early as possible */
#include <plan9.h>
#include <sys/stat.h>	/* for stat, umask */
#include <stdlib.h>	/* for malloc */
#include <string.h>	/* for strcpy, memmove */
#include <pwd.h>	/* for getpwnam, getpwuid */
#include <grp.h>	/* for getgrnam, getgrgid */
#include <unistd.h>	/* for gethostname, pread, pwrite, read, write */
#include <utime.h>	/* for utime */
#include <dirent.h>	/* for readdir */
#include <errno.h>	/* for errno */
#include <stdio.h>	/* for remove [sic] */
#include <fcntl.h>	/* for O_RDONLY, etc. */

#include <sys/socket.h>	/* various networking crud */
#include <netinet/in.h>
#include <netdb.h>

#include <fcall.h>
#include <oldfcall.h>
#include <u9fs.h>

/* #ifndef because can be given in makefile */
#ifndef DEFAULTLOG
#define DEFAULTLOG "/tmp/u9fs.log"
#endif

char *logfile = DEFAULTLOG;

#define S_ISSPECIAL(m) (S_ISCHR(m) || S_ISBLK(m) || S_ISFIFO(m))

enum {
	Tdot = 1,
	Tdotdot
};

enum {
	P9P1,
	P9P2000
};

typedef struct User User;
struct User {
	int id;
	gid_t defaultgid;
	char *name;
	char **mem;	/* group members */
	int nmem;
	User *next;
};

struct Fid {
	int fid;
	char *path;
	struct stat st;
	User *u;
	int omode;
	DIR *dir;
	int diroffset;
	int fd;
	struct dirent *dirent;
	int direof;
	Fid *next;
	Fid *prev;
	int auth;
	void *authmagic;
};

void*	emalloc(size_t);
void*	erealloc(void*, size_t);
char*	estrdup(char*);
char*	estrpath(char*, char*, int);
void	sysfatal(char*, ...);
int	okuser(char*);

void	rversion(Fcall*, Fcall*);
void	rauth(Fcall*, Fcall*);
void	rattach(Fcall*, Fcall*);
void	rflush(Fcall*, Fcall*);
void	rclone(Fcall*, Fcall*);
void	rwalk(Fcall*, Fcall*);
void	ropen(Fcall*, Fcall*);
void	rcreate(Fcall*, Fcall*);
void	rread(Fcall*, Fcall*);
void	rwrite(Fcall*, Fcall*);
void	rclunk(Fcall*, Fcall*);
void	rstat(Fcall*, Fcall*);
void	rwstat(Fcall*, Fcall*);
void	rclwalk(Fcall*, Fcall*);
void	rremove(Fcall*, Fcall*);

User*	uname2user(char*);
User*	gname2user(char*);
User*	uid2user(int);
User*	gid2user(int);

Fid*	newfid(int, char**);
Fid*	oldfidex(int, int, char**);
Fid*	oldfid(int, char**);
int	fidstat(Fid*, char**);
void	freefid(Fid*);

int	userchange(User*, char**);
int	userwalk(User*, char**, char*, Qid*, char**);
int	useropen(Fid*, int, char**);
int	usercreate(Fid*, char*, int, long, char**);
int	userremove(Fid*, char**);
int	userperm(User*, char*, int, int);
int	useringroup(User*, User*);

Qid	stat2qid(struct stat*);

void	getfcallold(int, Fcall*, int);
void	putfcallold(int, Fcall*);

char	Eauth[] =	"authentication failed";
char	Ebadfid[] =	"fid unknown or out of range";
char	Ebadoffset[] =	"bad offset in directory read";
char	Ebadusefid[] =	"bad use of fid";
char	Edirchange[] =	"wstat can't convert between files and directories";
char	Eexist[] =	"file or directory already exists";
char	Efidactive[] =	"fid already in use";
char	Enotdir[] =	"not a directory";
char	Enotingroup[] =	"not a member of proposed group";
char	Enotowner[] = 	"only owner can change group in wstat";
char	Eperm[] =	"permission denied";
char	Especial0[] =	"already attached without access to special files";
char	Especial1[] =	"already attached with access to special files";
char	Especial[] =	"no access to special file";
char	Etoolarge[] =	"i/o count too large";
char	Eunknowngroup[] = "unknown group";
char	Eunknownuser[] = "unknown user";
char	Ewstatbuffer[] = "bogus wstat buffer";

ulong	msize = IOHDRSZ+8192;
uchar*	rxbuf;
uchar*	txbuf;
void*	databuf;
int	connected;
int	devallowed;
char*	autharg;
char*	defaultuser;
char	hostname[256];
char	remotehostname[256];
int	chatty9p = 0;
int	network = 1;
int	old9p = -1;
int	authed;
User*	none;

Auth *authmethods[] = {	/* first is default */
	&authrhosts,
	&authp9any,
	&authnone,
};

Auth *auth;

/*
 * frogs: characters not valid in plan9
 * filenames, keep this list in sync with
 * /sys/src/9/port/chan.c:1656
 */
char isfrog[256]={
	/*NUL*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*BKS*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*DLE*/	1, 1, 1, 1, 1, 1, 1, 1,
	/*CAN*/	1, 1, 1, 1, 1, 1, 1, 1,
	['/']	1,
	[0x7f]	1,
};

void
getfcallnew(int fd, Fcall *fc, int have)
{
	int len;

	if(have > BIT32SZ)
		sysfatal("cannot happen");

	if(have < BIT32SZ && readn(fd, rxbuf+have, BIT32SZ-have) != BIT32SZ-have)
		sysfatal("couldn't read message");

	len = GBIT32(rxbuf);
	if(len <= BIT32SZ)
		sysfatal("bogus message");

	len -= BIT32SZ;
	if(readn(fd, rxbuf+BIT32SZ, len) != len)
		sysfatal("short message");

	if(convM2S(rxbuf, len+BIT32SZ, fc) != len+BIT32SZ)
		sysfatal("badly sized message type %d", rxbuf[0]);
}

void
getfcallold(int fd, Fcall *fc, int have)
{
	int len, n;

	if(have > 3)
		sysfatal("cannot happen");

	if(have < 3 && readn(fd, rxbuf, 3-have) != 3-have)
		sysfatal("couldn't read message");

	len = oldhdrsize(rxbuf[0]);
	if(len < 3)
		sysfatal("bad message %d", rxbuf[0]);
	if(len > 3 && readn(fd, rxbuf+3, len-3) != len-3)
		sysfatal("couldn't read message");

	n = iosize(rxbuf);
	if(readn(fd, rxbuf+len, n) != n)
		sysfatal("couldn't read message");
	len += n;

	if(convM2Sold(rxbuf, len, fc) != len)
		sysfatal("badly sized message type %d", rxbuf[0]);
}

void
putfcallnew(int wfd, Fcall *tx)
{
	uint n;

	if((n = convS2M(tx, txbuf, msize)) == 0)
		sysfatal("couldn't format message type %d", tx->type);
	if(write(wfd, txbuf, n) != n)
		sysfatal("couldn't send message");
}

void
putfcallold(int wfd, Fcall *tx)
{
	uint n;

	if((n = convS2Mold(tx, txbuf, msize)) == 0)
		sysfatal("couldn't format message type %d", tx->type);
	if(write(wfd, txbuf, n) != n)
		sysfatal("couldn't send message");
}

void
getfcall(int fd, Fcall *fc)
{
	if(old9p == 1){
		getfcallold(fd, fc, 0);
		return;
	}
	if(old9p == 0){
		getfcallnew(fd, fc, 0);
		return;
	}

	/* auto-detect */
	if(readn(fd, rxbuf, 3) != 3)
		sysfatal("couldn't read message");

	/* is it an old (9P1) message? */
	if(50 <= rxbuf[0] && rxbuf[0] <= 87 && (rxbuf[0]&1)==0 && GBIT16(rxbuf+1) == 0xFFFF){
		old9p = 1;
		getfcallold(fd, fc, 3);
		return;
	}

	getfcallnew(fd, fc, 3);
	old9p = 0;
}

void
seterror(Fcall *f, char *error)
{
	f->type = Rerror;
	f->ename = error ? error : "programmer error";
}

int
isowner(User *u, Fid *f)
{
	return u->id == f->st.st_uid;
}



void
serve(int rfd, int wfd)
{
	Fcall rx, tx;

	for(;;){
		getfcall(rfd, &rx);

		if(chatty9p)
			fprint(2, "<- %F\n", &rx);

		memset(&tx, 0, sizeof tx);
		tx.type = rx.type+1;
		tx.tag = rx.tag;
		switch(rx.type){
		case Tflush:
			break;
		case Tversion:
			rversion(&rx, &tx);
			break;
		case Tauth:
			rauth(&rx, &tx);
			break;
		case Tattach:
			rattach(&rx, &tx);
			break;
		case Twalk:
			rwalk(&rx, &tx);
			break;
		case Tstat:
			tx.stat = databuf;
			rstat(&rx, &tx);
			break;
		case Twstat:
			rwstat(&rx, &tx);
			break;
		case Topen:
			ropen(&rx, &tx);
			break;
		case Tcreate:
			rcreate(&rx, &tx);
			break;
		case Tread:
			tx.data = databuf;
			rread(&rx, &tx);
			break;
		case Twrite:
			rwrite(&rx, &tx);
			break;
		case Tclunk:
			rclunk(&rx, &tx);
			break;
		case Tremove:
			rremove(&rx, &tx);
			break;
		default:
			fprint(2, "unknown message %F\n", &rx);
			seterror(&tx, "bad message");
			break;
		}

		if(chatty9p)
			fprint(2, "-> %F\n", &tx);

		(old9p ? putfcallold : putfcallnew)(wfd, &tx);
	}
}

void
rversion(Fcall *rx, Fcall *tx)
{
	if(msize > rx->msize)
		msize = rx->msize;
	tx->msize = msize;
	if(strncmp(rx->version, "9P", 2) != 0)
		tx->version = "unknown";
	else
		tx->version = "9P2000";
}

void
rauth(Fcall *rx, Fcall *tx)
{
	char *e;

	if((e = auth->auth(rx, tx)) != nil)
		seterror(tx, e);
}

void
rattach(Fcall *rx, Fcall *tx)
{
	char *e;
	Fid *fid;
	User *u;

	if(rx->aname == nil)
		rx->aname = "";

	if(strcmp(rx->aname, "device") == 0){
		if(connected && !devallowed){
			seterror(tx, Especial0);
			return;
		}
		devallowed = 1;
	}else{
		if(connected && devallowed){
			seterror(tx, Especial1);
			return;
		}
	}

	if(strcmp(rx->uname, "none") == 0){
		if(authed == 0){
			seterror(tx, Eauth);
			return;
		}
	} else {
		if((e = auth->attach(rx, tx)) != nil){
			seterror(tx, e);
			return;
		}
		authed++;
	}

	if((fid = newfid(rx->fid, &e)) == nil){
		seterror(tx, e);
		return;
	}
	fid->path = estrdup("/");
	if(fidstat(fid, &e) < 0){
		seterror(tx, e);
		freefid(fid);
		return;
	}

	if(defaultuser)
		rx->uname = defaultuser;

	if((u = uname2user(rx->uname)) == nil
	|| (!defaultuser && u->id == 0)){
		/* we don't know anyone named root... */
		seterror(tx, Eunknownuser);
		freefid(fid);
		return;
	}

	fid->u = u;
	tx->qid = stat2qid(&fid->st);
	return;
}

void
rwalk(Fcall *rx, Fcall *tx)
{
	int i;
	char *path, *e;
	Fid *fid, *nfid;

	e = nil;
	if((fid = oldfid(rx->fid, &e)) == nil){
		seterror(tx, e);
		return;
	}

	if(fid->omode != -1){
		seterror(tx, Ebadusefid);
		return;
	}

	if(fidstat(fid, &e) < 0){
		seterror(tx, e);
		return;
	}

	if(!S_ISDIR(fid->st.st_mode) && rx->nwname){
		seterror(tx, Enotdir);
		return;
	}

	nfid = nil;
	if(rx->newfid != rx->fid && (nfid = newfid(rx->newfid, &e)) == nil){
		seterror(tx, e);
		return;
	}

	path = estrdup(fid->path);
	e = nil;
	for(i=0; i<rx->nwname; i++)
		if(userwalk(fid->u, &path, rx->wname[i], &tx->wqid[i], &e) < 0)
			break;

	if(i == rx->nwname){		/* successful clone or walk */
		tx->nwqid = i;
		if(nfid){
			nfid->path = path;
			nfid->u = fid->u;
		}else{
			free(fid->path);
			fid->path = path;
		}
	}else{
		if(i > 0)		/* partial walk? */
			tx->nwqid = i;
		else
			seterror(tx, e);

		if(nfid)		/* clone implicit new fid */
			freefid(nfid);
		free(path);
	}
	return;
}

void
ropen(Fcall *rx, Fcall *tx)
{
	char *e;
	Fid *fid;

	if((fid = oldfid(rx->fid, &e)) == nil){
		seterror(tx, e);
		return;
	}

	if(fid->omode != -1){
		seterror(tx, Ebadusefid);
		return;
	}

	if(fidstat(fid, &e) < 0){
		seterror(tx, e);
		return;
	}

	if(!devallowed && S_ISSPECIAL(fid->st.st_mode)){
		seterror(tx, Especial);
		return;
	}

	if(useropen(fid, rx->mode, &e) < 0){
		seterror(tx, e);
		return;
	}

	tx->iounit = 0;
	tx->qid = stat2qid(&fid->st);
}

void
rcreate(Fcall *rx, Fcall *tx)
{
	char *e;
	Fid *fid;

	if((fid = oldfid(rx->fid, &e)) == nil){
		seterror(tx, e);
		return;
	}

	if(fid->omode != -1){
		seterror(tx, Ebadusefid);
		return;
	}

	if(fidstat(fid, &e) < 0){
		seterror(tx, e);
		return;
	}

	if(!S_ISDIR(fid->st.st_mode)){
		seterror(tx, Enotdir);
		return;
	}

	if(usercreate(fid, rx->name, rx->mode, rx->perm, &e) < 0){
		seterror(tx, e);
		return;
	}

	if(fidstat(fid, &e) < 0){
		seterror(tx, e);
		return;
	}

	tx->iounit = 0;
	tx->qid = stat2qid(&fid->st);
}

uchar
modebyte(struct stat *st)
{
	uchar b;

	b = 0;

	if(S_ISDIR(st->st_mode))
		b |= QTDIR;

	/* no way to test append-only */
	/* no real way to test exclusive use, but mark devices as such */
	if(S_ISSPECIAL(st->st_mode))
		b |= QTEXCL;

	return b;
}

ulong
plan9mode(struct stat *st)
{
	return ((ulong)modebyte(st)<<24) | (st->st_mode & 0777);
}

/* 
 * this is for chmod, so don't worry about S_IFDIR
 */
mode_t
unixmode(Dir *d)
{
	return (mode_t)(d->mode&0777);
}

Qid
stat2qid(struct stat *st)
{
	uchar *p, *ep, *q;
	Qid qid;

	/*
	 * For now, ignore the device number.
	 */
	qid.path = 0;
	p = (uchar*)&qid.path;
	ep = p+sizeof(qid.path);
	q = p+sizeof(ino_t);
	if(q > ep){
		fprint(2, "warning: inode number too big\n");
		q = ep;
	}
	memmove(p, &st->st_ino, q-p);
	q = q+sizeof(dev_t);
	if(q > ep){
/*
 *		fprint(2, "warning: inode number + device number too big %d+%d\n",
 *			sizeof(ino_t), sizeof(dev_t));
 */
		q = ep - sizeof(dev_t);
		if(q < p)
			fprint(2, "warning: device number too big by itself\n");
		else
			*(dev_t*)q ^= st->st_dev;
	}

	qid.vers = st->st_mtime ^ (st->st_size << 8);
	qid.type = modebyte(st);
	return qid;
}

char *
enfrog(char *src)
{
	char *d, *dst;
	uchar *s;

	d = dst = emalloc(strlen(src)*3 + 1);
	for (s = (uchar *)src; *s; s++)
		if(isfrog[*s] || *s == '\\')
			d += sprintf(d, "\\%02x", *s);
		else
			*d++ = *s;
	*d = 0;
	return dst;
}

char *
defrog(char *s)
{
	char *d, *dst, buf[3];

	d = dst = emalloc(strlen(s) + 1);
	for(; *s; s++)
		if(*s == '\\' && strlen(s) >= 3){
			buf[0] = *++s;			/* skip \ */
			buf[1] = *++s;
			buf[2] = 0;
			*d++ = strtoul(buf, NULL, 16);
		} else
			*d++ = *s;
	*d = 0;
	return dst;
}

void
stat2dir(char *path, struct stat *st, Dir *d)
{
	User *u;
	char *q, *p, *npath;

	memset(d, 0, sizeof(*d));
	d->qid = stat2qid(st);
	d->mode = plan9mode(st);
	d->atime = st->st_atime;
	d->mtime = st->st_mtime;
	d->length = st->st_size;

	d->uid = (u = uid2user(st->st_uid)) ? u->name : "???";
	d->gid = (u = gid2user(st->st_gid)) ? u->name : "???";
	d->muid = "";

	if((q = strrchr(path, '/')) != nil)
		d->name = enfrog(q+1);
	else
		d->name = enfrog(path);
}

void
rread(Fcall *rx, Fcall *tx)
{
	char *e, *path;
	uchar *p, *ep;
	int n;
	Fid *fid;
	Dir d;
	struct stat st;

	if(rx->count > msize-IOHDRSZ){
		seterror(tx, Etoolarge);
		return;
	}

	if((fid = oldfidex(rx->fid, -1, &e)) == nil){
		seterror(tx, e);
		return;
	}

	if (fid->auth) {
		char *e;
		e = auth->read(rx, tx);
		if (e)
			seterror(tx, e);
		return;
	}

	if(fid->omode == -1 || (fid->omode&3) == OWRITE){
		seterror(tx, Ebadusefid);
		return;
	}

	if(fid->dir){
		if(rx->offset != fid->diroffset){
			if(rx->offset != 0){
				seterror(tx, Ebadoffset);
				return;
			}
			rewinddir(fid->dir);
			fid->diroffset = 0;
			fid->direof = 0;
		}
		if(fid->direof){
			tx->count = 0;
			return;
		}

		p = (uchar*)tx->data;
		ep = (uchar*)tx->data+rx->count;
		for(;;){
			if(p+BIT16SZ >= ep)
				break;
			if(fid->dirent == nil)	/* one entry cache for when convD2M fails */
				if((fid->dirent = readdir(fid->dir)) == nil){
					fid->direof = 1;
					break;
				}
			if(strcmp(fid->dirent->d_name, ".") == 0
			|| strcmp(fid->dirent->d_name, "..") == 0){
				fid->dirent = nil;
				continue;
			}
			path = estrpath(fid->path, fid->dirent->d_name, 0);
			memset(&st, 0, sizeof st);
			if(stat(path, &st) < 0){
				fprint(2, "dirread: stat(%s) failed: %s\n", path, strerror(errno));
				fid->dirent = nil;
				free(path);
				continue;
			}
			free(path);
			stat2dir(fid->dirent->d_name, &st, &d);
			if((n=(old9p ? convD2Mold : convD2M)(&d, p, ep-p)) <= BIT16SZ)
				break;
			p += n;
			fid->dirent = nil;
		}
		tx->count = p - (uchar*)tx->data;
		fid->diroffset += tx->count;
	}else{
		if((n = pread(fid->fd, tx->data, rx->count, rx->offset)) < 0){
			seterror(tx, strerror(errno));
			return;
		}
		tx->count = n;
	}
}

void
rwrite(Fcall *rx, Fcall *tx)
{
	char *e;
	Fid *fid;
	int n;

	if(rx->count > msize-IOHDRSZ){
		seterror(tx, Etoolarge);
		return;
	}

	if((fid = oldfidex(rx->fid, -1, &e)) == nil){
		seterror(tx, e);
		return;
	}

	if (fid->auth) {
		char *e;
		e = auth->write(rx, tx);
		if (e)
			seterror(tx, e);
		return;
	}

	if(fid->omode == -1 || (fid->omode&3) == OREAD || (fid->omode&3) == OEXEC){
		seterror(tx, Ebadusefid);
		return;
	}

	if((n = pwrite(fid->fd, rx->data, rx->count, rx->offset)) < 0){
		seterror(tx, strerror(errno));
		return;
	}
	tx->count = n;
}

void
rclunk(Fcall *rx, Fcall *tx)
{
	char *e;
	Fid *fid;

	if((fid = oldfidex(rx->fid, -1, &e)) == nil){
		seterror(tx, e);
		return;
	}
	if (fid->auth) {
		if (auth->clunk) {
			e = (*auth->clunk)(rx, tx);
			if (e) {
				seterror(tx, e);
				return;
			}
		}
	}
	else if(fid->omode != -1 && fid->omode&ORCLOSE)
		remove(fid->path);
	freefid(fid);
}

void
rremove(Fcall *rx, Fcall *tx)
{
	char *e;
	Fid *fid;

	if((fid = oldfid(rx->fid, &e)) == nil){
		seterror(tx, e);
		return;
	}
	if(userremove(fid, &e) < 0)
		seterror(tx, e);
	freefid(fid);
}

void
rstat(Fcall *rx, Fcall *tx)
{
	char *e;
	Fid *fid;
	Dir d;

	if((fid = oldfid(rx->fid, &e)) == nil){
		seterror(tx, e);
		return;
	}

	if(fidstat(fid, &e) < 0){
		seterror(tx, e);
		return;
	}

	stat2dir(fid->path, &fid->st, &d);
	if((tx->nstat=(old9p ? convD2Mold : convD2M)(&d, tx->stat, msize)) <= BIT16SZ)
		seterror(tx, "convD2M fails");
}

void
rwstat(Fcall *rx, Fcall *tx)
{
	char *e;
	char *p, *old, *new, *dir;
	gid_t gid;
	Dir d;
	Fid *fid;

	if((fid = oldfid(rx->fid, &e)) == nil){
		seterror(tx, e);
		return;
	}

	/*
	 * wstat is supposed to be atomic.
	 * we check all the things we can before trying anything.
	 * still, if we are told to truncate a file and rename it and only
	 * one works, we're screwed.  in such cases we leave things
	 * half broken and return an error.  it's hardly perfect.
	 */
	if((old9p ? convM2Dold : convM2D)(rx->stat, rx->nstat, &d, (char*)rx->stat) <= BIT16SZ){
		seterror(tx, Ewstatbuffer);
		return;
	}

	if(fidstat(fid, &e) < 0){
		seterror(tx, e);
		return;
	}

	/*
	 * The casting is necessary because d.mode is ulong and might,
	 * on some systems, be 64 bits.  We only want to compare the
	 * bottom 32 bits, since that's all that gets sent in the protocol.
	 * 
	 * Same situation for d.mtime and d.length (although that last check
	 * is admittedly superfluous, given the current lack of 128-bit machines).
	 */
	gid = (gid_t)-1;
	if(d.gid[0] != '\0'){
		User *g;

		g = gname2user(d.gid);
		if(g == nil){
			seterror(tx, Eunknowngroup);
			return;
		}
		gid = (gid_t)g->id;

		if(groupchange(fid->u, gid2user(gid), &e) < 0){
			seterror(tx, e);
			return;
		}		
	}

	if((u32int)d.mode != (u32int)~0 && (((d.mode&DMDIR)!=0) ^ (S_ISDIR(fid->st.st_mode)!=0))){
		seterror(tx, Edirchange);
		return;
	}

	if(strcmp(fid->path, "/") == 0){
		seterror(tx, "no wstat of root");
		return;
	}

	/*
	 * try things in increasing order of harm to the file.
	 * mtime should come after truncate so that if you
	 * do both the mtime actually takes effect, but i'd rather
	 * leave truncate until last.
	 * (see above comment about atomicity).
	 */
	if((u32int)d.mode != (u32int)~0 && chmod(fid->path, unixmode(&d)) < 0){
		if(chatty9p)
			fprint(2, "chmod(%s, 0%luo) failed\n", fid->path, unixmode(&d));
		seterror(tx, strerror(errno));
		return;
	}

	if((u32int)d.mtime != (u32int)~0){
		struct utimbuf t;

		t.actime = 0;
		t.modtime = d.mtime;
		if(utime(fid->path, &t) < 0){
			if(chatty9p)
				fprint(2, "utime(%s) failed\n", fid->path);
			seterror(tx, strerror(errno));
			return;
		}
	}

	if(gid != (gid_t)-1 && gid != fid->st.st_gid){
		if(chown(fid->path, (uid_t)-1, gid) < 0){
			if(chatty9p)
				fprint(2, "chgrp(%s, %d) failed\n", fid->path, gid);
			seterror(tx, strerror(errno));
			return;
		}
	}

	if(d.name[0]){
		old = fid->path;
		dir = estrdup(fid->path);
		if((p = strrchr(dir, '/')) > dir)
			*p = '\0';
		else{
			seterror(tx, "whoops: can't happen in u9fs");
			return;
		}
		new = estrpath(dir, d.name, 1);
		if(strcmp(old, new) != 0 && rename(old, new) < 0){
			if(chatty9p)
				fprint(2, "rename(%s, %s) failed\n", old, new);
			seterror(tx, strerror(errno));
			free(new);
			free(dir);
			return;
		}
		fid->path = new;
		free(old);
		free(dir);
	}

	if((u64int)d.length != (u64int)~0 && truncate(fid->path, d.length) < 0){
		fprint(2, "truncate(%s, %lld) failed\n", fid->path, d.length);
		seterror(tx, strerror(errno));
		return;
	}
}

/*
 * we keep a table by numeric id.  by name lookups happen infrequently
 * while by-number lookups happen once for every directory entry read
 * and every stat request.
 */
User *utab[64];
User *gtab[64];

User*
adduser(struct passwd *p)
{
	User *u;

	u = emalloc(sizeof(*u));
	u->id = p->pw_uid;
	u->name = estrdup(p->pw_name);
	u->next = utab[p->pw_uid%nelem(utab)];
	u->defaultgid = p->pw_gid;
	utab[p->pw_uid%nelem(utab)] = u;
	return u;
}

int
useringroup(User *u, User *g)
{
	int i;

	for(i=0; i<g->nmem; i++)
		if(strcmp(g->mem[i], u->name) == 0)
			return 1;

	/*
	 * Hack around common Unix problem that everyone has
	 * default group "user" but /etc/group lists no members.
	 */
	if(u->defaultgid == g->id)
		return 1;
	return 0;
}

User*
addgroup(struct group *g)
{
	User *u;
	char **p;
	int n;

	u = emalloc(sizeof(*u));
	n = 0;
	for(p=g->gr_mem; *p; p++)
		n++;
	u->mem = emalloc(sizeof(u->mem[0])*n);
	n = 0;
	for(p=g->gr_mem; *p; p++)
		u->mem[n++] = estrdup(*p);
	u->nmem = n;
	u->id = g->gr_gid;
	u->name = estrdup(g->gr_name);
	u->next = gtab[g->gr_gid%nelem(gtab)];
	gtab[g->gr_gid%nelem(gtab)] = u;
	return u;
}

User*
uname2user(char *name)
{
	int i;
	User *u;
	struct passwd *p;

	for(i=0; i<nelem(utab); i++)
		for(u=utab[i]; u; u=u->next)
			if(strcmp(u->name, name) == 0)
				return u;

	if((p = getpwnam(name)) == nil)
		return nil;
	return adduser(p);
}

User*
uid2user(int id)
{
	User *u;
	struct passwd *p;

	for(u=utab[id%nelem(utab)]; u; u=u->next)
		if(u->id == id)
			return u;

	if((p = getpwuid(id)) == nil)
		return nil;
	return adduser(p);
}

User*
gname2user(char *name)
{
	int i;
	User *u;
	struct group *g;

	for(i=0; i<nelem(gtab); i++)
		for(u=gtab[i]; u; u=u->next)
			if(strcmp(u->name, name) == 0)
				return u;

	if((g = getgrnam(name)) == nil)
		return nil;
	return addgroup(g);
}

User*
gid2user(int id)
{
	User *u;
	struct group *g;

	for(u=gtab[id%nelem(gtab)]; u; u=u->next)
		if(u->id == id)
			return u;

	if((g = getgrgid(id)) == nil)
		return nil;
	return addgroup(g);
}

void
sysfatal(char *fmt, ...)
{
	char buf[1024];
	va_list va, temp;

	va_start(va, fmt);
	va_copy(temp, va);
	doprint(buf, buf+sizeof buf, fmt, &temp);
	va_end(temp);
	va_end(va);
	fprint(2, "u9fs: %s\n", buf);
	fprint(2, "last unix error: %s\n", strerror(errno));
	exit(1);
}

void*
emalloc(size_t n)
{
	void *p;

	if(n == 0)
		n = 1;
	p = malloc(n);
	if(p == 0)
		sysfatal("malloc(%ld) fails", (long)n);
	memset(p, 0, n);
	return p;
}

void*
erealloc(void *p, size_t n)
{
	if(p == 0)
		p = malloc(n);
	else
		p = realloc(p, n);
	if(p == 0)
		sysfatal("realloc(..., %ld) fails", (long)n);
	return p;
}

char*
estrdup(char *p)
{
	p = strdup(p);
	if(p == 0)
		sysfatal("strdup(%.20s) fails", p);
	return p;
}

char*
estrpath(char *p, char *q, int frog)
{
	char *r, *s;

	if(strcmp(q, "..") == 0){
		r = estrdup(p);
		if((s = strrchr(r, '/')) && s > r)
			*s = '\0';
		else if(s == r)
			s[1] = '\0';
		return r;
	}

	if(frog)
		q = defrog(q);
	else
		q = strdup(q);
	r = emalloc(strlen(p)+1+strlen(q)+1);
	strcpy(r, p);
	if(r[0]=='\0' || r[strlen(r)-1] != '/')
		strcat(r, "/");
	strcat(r, q);
	free(q);
	return r;
}

Fid *fidtab[1];

Fid*
lookupfid(int fid)
{
	Fid *f;

	for(f=fidtab[fid%nelem(fidtab)]; f; f=f->next)
		if(f->fid == fid)
			return f;
	return nil;
}

Fid*
newfid(int fid, char **ep)
{
	Fid *f;

	if(lookupfid(fid) != nil){
		*ep = Efidactive;
		return nil;
	}

	f = emalloc(sizeof(*f));
	f->next = fidtab[fid%nelem(fidtab)];
	if(f->next)
		f->next->prev = f;
	fidtab[fid%nelem(fidtab)] = f;
	f->fid = fid;
	f->fd = -1;
	f->omode = -1;
	return f;
}

Fid*
newauthfid(int fid, void *magic, char **ep)
{
	Fid *af;
	af = newfid(fid, ep);
	if (af == nil)
		return nil;
	af->auth = 1;
	af->authmagic = magic;
	return af;
}

Fid*
oldfidex(int fid, int auth, char **ep)
{
	Fid *f;

	if((f = lookupfid(fid)) == nil){
		*ep = Ebadfid;
		return nil;
	}

	if (auth != -1 && f->auth != auth) {
		*ep = Ebadfid;
		return nil;
	}

	if (!f->auth) {
		if(userchange(f->u, ep) < 0)
			return nil;
	}

	return f;
}

Fid*
oldfid(int fid, char **ep)
{
	return oldfidex(fid, 0, ep);
}

Fid*
oldauthfid(int fid, void **magic, char **ep)
{
	Fid *af;
	af = oldfidex(fid, 1, ep);
	if (af == nil)
		return nil;
	*magic = af->authmagic;
	return af;
}

void
freefid(Fid *f)
{
	if(f->prev)
		f->prev->next = f->next;
	else
		fidtab[f->fid%nelem(fidtab)] = f->next;
	if(f->next)
		f->next->prev = f->prev;
	if(f->dir)
		closedir(f->dir);
	if(f->fd)
		close(f->fd);
	free(f->path);
	free(f);
}

int
fidstat(Fid *fid, char **ep)
{
	if(stat(fid->path, &fid->st) < 0){
		fprint(2, "fidstat(%s) failed\n", fid->path);
		if(ep)
			*ep = strerror(errno);
		return -1;
	}
	if(S_ISDIR(fid->st.st_mode))
		fid->st.st_size = 0;
	return 0;
}

int
userchange(User *u, char **ep)
{
	if(defaultuser)
		return 0;

	if(setreuid(0, 0) < 0){
		fprint(2, "setreuid(0, 0) failed\n");
		*ep = "cannot setuid back to root";
		return -1;
	}

	/*
	 * Initgroups does not appear to be SUSV standard.
	 * But it exists on SGI and on Linux, which makes me
	 * think it's standard enough.  We have to do something
	 * like this, and the closest other function I can find is
	 * setgroups (which initgroups eventually calls).
	 * Setgroups is the same as far as standardization though,
	 * so we're stuck using a non-SUSV call.  Sigh.
	 */
	if(initgroups(u->name, u->defaultgid) < 0)
		fprint(2, "initgroups(%s) failed: %s\n", u->name, strerror(errno));

	if(setreuid(-1, u->id) < 0){
		fprint(2, "setreuid(-1, %s) failed\n", u->name);
		*ep = strerror(errno);
		return -1;
	}

	return 0;
}

/*
 * We do our own checking here, then switch to root temporarily
 * to set our gid.  In a perfect world, you'd be allowed to set your
 * egid to any of the supplemental groups of your euid, but this
 * is not the case on Linux 2.2.14 (and perhaps others).
 *
 * This is a race, of course, but it's a race against processes
 * that can edit the group lists.  If you can do that, you can
 * change your own group without our help.
 */
int
groupchange(User *u, User *g, char **ep)
{
	if(g == nil)
		return -1;
	if(!useringroup(u, g)){
		if(chatty9p)
			fprint(2, "%s not in group %s\n", u->name, g->name);
		*ep = Enotingroup;
		return -1;
	}

	setreuid(0,0);
	if(setregid(-1, g->id) < 0){
		fprint(2, "setegid(%s/%d) failed in groupchange\n", g->name, g->id);
		*ep = strerror(errno);
		return -1;
	}
	if(userchange(u, ep) < 0)
		return -1;

	return 0;
}


/*
 * An attempt to enforce permissions by looking at the 
 * file system.  Separation of checking permission and
 * actually performing the action is a terrible idea, of 
 * course, so we use setreuid for most of the permission
 * enforcement.  This is here only so we can give errors
 * on open(ORCLOSE) in some cases.
 */
int
userperm(User *u, char *path, int type, int need)
{
	char *p, *q;
	int i, have;
	struct stat st;
	User *g;

	switch(type){
	default:
		fprint(2, "bad type %d in userperm\n", type);
		return -1;
	case Tdot:
		if(stat(path, &st) < 0){
			fprint(2, "userperm: stat(%s) failed\n", path);
			return -1;
		}
		break;
	case Tdotdot:
		p = estrdup(path);
		if((q = strrchr(p, '/'))==nil){
			fprint(2, "userperm(%s, ..): bad path\n", p);
			free(p);
			return -1;
		}
		if(q > p)
			*q = '\0';
		else
			*(q+1) = '\0';
		if(stat(p, &st) < 0){
			fprint(2, "userperm: stat(%s) (dotdot of %s) failed\n",
				p, path);
			free(p);
			return -1;
		}
		free(p);
		break;
	}

	if(u == none){
		fprint(2, "userperm: none wants %d in 0%luo\n", need, st.st_mode);
		have = st.st_mode&7;
		if((have&need)==need)
			return 0;
		return -1;
	}
	have = st.st_mode&7;
	if((uid_t)u->id == st.st_uid)
		have |= (st.st_mode>>6)&7;
	if((have&need)==need)
		return 0;
	if(((have|((st.st_mode>>3)&7))&need) != need)	/* group won't help */
		return -1;
	g = gid2user(st.st_gid);
	for(i=0; i<g->nmem; i++){
		if(strcmp(g->mem[i], u->name) == 0){
			have |= (st.st_mode>>3)&7;
			break;
		}
	}
	if((have&need)==need)
		return 0;
	return -1;
}

int
userwalk(User *u, char **path, char *elem, Qid *qid, char **ep)
{
	char *npath;
	struct stat st;

	npath = estrpath(*path, elem, 1);
	if(stat(npath, &st) < 0){
		free(npath);
		*ep = strerror(errno);
		return -1;
	}
	*qid = stat2qid(&st);
	free(*path);
	*path = npath;
	return 0;
}

int
useropen(Fid *fid, int omode, char **ep)
{
	int a, o;

	/*
	 * Check this anyway, to try to head off problems later.
	 */
	if((omode&ORCLOSE) && userperm(fid->u, fid->path, Tdotdot, W_OK) < 0){
		*ep = Eperm;
		return -1;
	}

	switch(omode&3){
	default:
		*ep = "programmer error";
		return -1;
	case OREAD:
		a = R_OK;
		o = O_RDONLY;
		break;
	case ORDWR:
		a = R_OK|W_OK;
		o = O_RDWR;
		break;
	case OWRITE:
		a = W_OK;
		o = O_WRONLY;
		break;
	case OEXEC:
		a = X_OK;
		o = O_RDONLY;
		break;
	}
	if(omode & OTRUNC){
		a |= W_OK;
		o |= O_TRUNC;
	}

	if(S_ISDIR(fid->st.st_mode)){
		if(a != R_OK){
			fprint(2, "attempt by %s to open dir %d\n", fid->u->name, omode);
			*ep = Eperm;
			return -1;
		}
		if((fid->dir = opendir(fid->path)) == nil){
			*ep = strerror(errno);
			return -1;
		}
	}else{
		/*
		 * This is wrong because access used the real uid
		 * and not the effective uid.  Let the open sort it out.
		 *
		if(access(fid->path, a) < 0){
			*ep = strerror(errno);
			return -1;
		}
		 *
		 */
		if((fid->fd = open(fid->path, o)) < 0){
			*ep = strerror(errno);
			return -1;
		}
	}
	fid->omode = omode;
	return 0;
}

int
usercreate(Fid *fid, char *elem, int omode, long perm, char **ep)
{
	int o, m;
	char *opath, *npath;
	struct stat st, parent;

	if(stat(fid->path, &parent) < 0){
		*ep = strerror(errno);
		return -1;
	}

	/*
	 * Change group so that created file has expected group
	 * by Plan 9 semantics.  If that fails, might as well go
	 * with the user's default group.
	 */
	if(groupchange(fid->u, gid2user(parent.st_gid), ep) < 0
	&& groupchange(fid->u, gid2user(fid->u->defaultgid), ep) < 0)
		return -1;

	m = (perm & DMDIR) ? 0777 : 0666;
	perm = perm & (~m | (fid->st.st_mode & m));

	npath = estrpath(fid->path, elem, 1);
	if(perm & DMDIR){
		if((omode&~ORCLOSE) != OREAD){
			*ep = Eperm;
			free(npath);
			return -1;
		}
		if(stat(npath, &st) >= 0 || errno != ENOENT){
			*ep = Eexist;
			free(npath);
			return -1;
		}
		/* race */
		if(mkdir(npath, perm&0777) < 0){
			*ep = strerror(errno);
			free(npath);
			return -1;
		}
		if((fid->dir = opendir(npath)) == nil){
			*ep = strerror(errno);
			remove(npath);		/* race */
			free(npath);
			return -1;
		}
	}else{
		o = O_CREAT|O_EXCL;
		switch(omode&3){
		default:
			*ep = "programmer error";
			return -1;
		case OREAD:
		case OEXEC:
			o |= O_RDONLY;
			break;
		case ORDWR:
			o |= O_RDWR;
			break;
		case OWRITE:
			o |= O_WRONLY;
			break;
		}
		if(omode & OTRUNC)
			o |= O_TRUNC;
		if((fid->fd = open(npath, o, perm&0777)) < 0){
			if(chatty9p)
				fprint(2, "create(%s, 0x%x, 0%o) failed\n", npath, o, perm&0777);
			*ep = strerror(errno);
			free(npath);
			return -1;
		}
	}

	opath = fid->path;
	fid->path = npath;
	if(fidstat(fid, ep) < 0){
		fprint(2, "stat after create on %s failed\n", npath);
		remove(npath);	/* race */
		free(npath);
		fid->path = opath;
		if(fid->fd >= 0){
			close(fid->fd);
			fid->fd = -1;
		}else{
			closedir(fid->dir);
			fid->dir = nil;
		}
		return -1;
	}
	fid->omode = omode;
	free(opath);
	return 0;
}

int
userremove(Fid *fid, char **ep)
{
	if(remove(fid->path) < 0){
		*ep = strerror(errno);
		return -1;
	}
	return 0;
}

void
usage(void)
{
	fprint(2, "usage: u9fs [-Dnz] [-a authmethod] [-m msize] [-u user] [root]\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	char *authtype;
	int i;
	int fd;
	int logflag;

	auth = authmethods[0];
	logflag = O_WRONLY|O_APPEND|O_CREAT;
	ARGBEGIN{
	case 'D':
		chatty9p = 1;
		break;
	case 'a':
		authtype = EARGF(usage());
		auth = nil;
		for(i=0; i<nelem(authmethods); i++)
			if(strcmp(authmethods[i]->name, authtype)==0)
				auth = authmethods[i];
		if(auth == nil)
			sysfatal("unknown auth type '%s'", authtype);
		break;
	case 'A':
		autharg = EARGF(usage());
		break;
	case 'l':
		logfile = EARGF(usage());
		break;
	case 'm':
		msize = strtol(EARGF(usage()), 0, 0);
		break;
	case 'n':
		network = 0;
		break;
	case 'u':
		defaultuser = EARGF(usage());
		break;
	case 'z':
		logflag |= O_TRUNC;
	}ARGEND

	if(argc > 1)
		usage();

	fd = open(logfile, logflag, 0666);
	if(fd < 0)
		sysfatal("cannot open log '%s'", logfile);

	if(dup2(fd, 2) < 0)
		sysfatal("cannot dup fd onto stderr");
	fprint(2, "u9fs\nkill %d\n", (int)getpid());

	fmtinstall('F', fcallconv);
	fmtinstall('D', dirconv);
	fmtinstall('M', dirmodeconv);

	rxbuf = emalloc(msize);
	txbuf = emalloc(msize);
	databuf = emalloc(msize);

	if(auth->init)
		auth->init();

	if(network)
		getremotehostname(remotehostname, sizeof remotehostname);

	if(gethostname(hostname, sizeof hostname) < 0)
		strcpy(hostname, "gnot");

	umask(0);

	if(argc == 1)
		if(chroot(argv[0]) < 0)
			sysfatal("chroot '%s' failed", argv[0]);

	none = uname2user("none");

	serve(0, 1);
	return 0;
}
