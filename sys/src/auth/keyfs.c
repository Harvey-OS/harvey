#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <bio.h>
#include <auth.h>
#include "authsrv.h"

char authkey[DESKEYLEN];

typedef struct Fid	Fid;
typedef struct User	User;

enum{
	Qroot,
	Quser,
	Qkey,
	Qlog,
	Qstatus,
	Qexpire,
	Qishost,
	Qmax,

	Nuser	= 512,
	MAXBAD	= 50,			/* max number of bad attempts before diabling the account */
};

enum{
	Sok,
	Sdisabled,
	Smax,
};

struct Fid{
	int	fid;
	ulong	qtype;
	User	*user;
	int	busy;
	Fid	*next;
};

struct User{
	char	name[NAMELEN];
	char	key[DESKEYLEN];
	ulong	expire;			/* 0 == never */
	uchar	status;
	uchar	bad;			/* number of consecutive bad authentication attempts */
	int	ref;
	char	removed;
	uchar	ishost;
	ulong	uniq;
	User	*link;
};

char	*qinfo[Qmax] = {
	[Qroot]		"keys",
	[Quser]		".",
	[Qkey]		"key",
	[Qlog]		"log",
	[Qexpire]	"expire",
	[Qstatus]	"status",
	[Qishost]	"ishost",
};

char	*status[Smax] = {
	[Sok]		"ok",
	[Sdisabled]	"disabled",
};

Fid	*fids;
User	*users[Nuser];
char	*userkeys;
int	nuser;
int	nvkey;
int	cryptfd;
ulong	uniq = 1;
Fcall	rhdr,
	thdr;

int	readusers(void);
ulong	hash(char*);
Fid	*findfid(int);
User	*finduser(char*);
User	*installuser(char*);
int	removeuser(User*);
void	insertuser(User*);
void	writeusers(void);
void	io(int, int);
void	*emalloc(ulong);
Qid	mkqid(User*, ulong);
void	dostat(User*, ulong, void*);
int	dofcrypt(int, char*, char*, int);

char	*Auth(Fid*), *Attach(Fid*), *Nop(Fid*), *Session(Fid*),
	*Flush(Fid*), *Clone(Fid*), *Walk(Fid*),
	*Clwalk(Fid *), *Open(Fid*), *Create(Fid*),
	*Read(Fid *), *Write(Fid*), *Clunk(Fid*),
	*Remove(Fid *), *Stat(Fid*), *Wstat(Fid*);
char 	*(*fcalls[])(Fid*) = {
	[Tattach]	Attach,
	[Tauth]		Auth,
	[Tclone]	Clone,
	[Tclunk]	Clunk,
	[Tclwalk]	Clwalk,
	[Tcreate]	Create,
	[Tflush]	Flush,
	[Tnop]		Nop,
	[Topen]		Open,
	[Tread]		Read,
	[Tremove]	Remove,
	[Tsession]	Session,
	[Tstat]		Stat,
	[Twalk]		Walk,
	[Twrite]	Write,
	[Twstat]	Wstat,
};

void
main(int argc, char *argv[])
{
	char *mntpt;
	int p[2];

	mntpt = "/mnt/keys";
	nvkey = 1;
	cryptfd = -1;
	ARGBEGIN{
	case 'k':
		strncpy(authkey, ARGF(), DESKEYLEN);
		nvkey = 0;
		break;
	case 'd':
		nvkey = 0;
		cryptfd = open("/dev/crypt", ORDWR);
		if(cryptfd < 0)
			error("can't open /dev/crypt: %r\n");
		break;
	case 'm':
		mntpt = ARGF();
		break;
	}ARGEND
	argv0 = "keyfs";

	userkeys = "/adm/keys";
	if(argc > 0)
		userkeys = argv[0];
	readusers();
	if(pipe(p) < 0)
		error("can't make pipe: %r");
	switch(fork()){
	case 0:
		io(p[1], p[1]);
		exits(0);
	case -1:
		error("fork");
	default:
		if(mount(p[0], mntpt, MREPL|MCREATE, "", "") < 0)
			error("can't mount: %r");
		exits(0);
	}
}

char *
Nop(Fid *f)
{
	USED(f);
	return 0;
}

char *
Session(Fid *f)
{
	USED(f);
	return 0;
}

char *
Flush(Fid *f)
{
	USED(f);
	return 0;
}

char *
Auth(Fid *f)
{
	if(f->busy)
		Clunk(f);
	f->user = 0;
	f->qtype = Qroot;
	f->busy = 1;
	strcpy(thdr.chal, "none");
	return 0;
}

char *
Attach(Fid *f)
{
	if(f->busy)
		Clunk(f);
	f->user = 0;
	f->qtype = Qroot;
	f->busy = 1;
	thdr.qid = mkqid(f->user, f->qtype);
	return 0;
}

char *
Clone(Fid *f)
{
	Fid *nf;

	if(!f->busy)
		return "permission denied";
	nf = findfid(rhdr.newfid);
	if(nf->busy)
		Clunk(nf);
	thdr.fid = rhdr.newfid;
	nf->busy = 1;
	nf->qtype = f->qtype;
	if(nf->user = f->user)
		nf->user->ref++;
	return 0;
}

char *
Walk(Fid *f)
{
	char *name;
	int i, max;

	if(!f->busy)
		return "permission denied";
	name = rhdr.name;
	switch(f->qtype){
	case Qroot:
		f->user = finduser(name);
		if(!f->user)
			return "file does not exist";
		f->user->ref++;
		f->qtype = Quser;
		break;
	case Quser:
		max = Qmax;
		if(!f->user->ishost)
			max = Qishost;
		for(i = Quser + 1; i < Qmax; i++)
			if(strcmp(name, qinfo[i]) == 0){
				f->qtype = i;
				break;
			}
		if(i == max)
			return "file not found";
		break;
	default:
		return "file is not a directory";
	}
	thdr.qid = mkqid(f->user, f->qtype);
	return 0;
}

char *
Clunk(Fid *f)
{
	f->busy = 0;
	if(f->user && --f->user->ref == 0 && f->user->removed)
		free(f->user);
	f->user = 0;
	return 0;
}

char *
Clwalk(Fid *f)
{
	Fid *nf;
	char *err;

	if(!f->busy)
		return "permission denied";
	nf = findfid(rhdr.newfid);
	thdr.fid = rhdr.newfid;
	if(nf->busy)
		Clunk(nf);
	nf->busy = 1;
	nf->qtype = f->qtype;
	if(nf->user = f->user)
		nf->user->ref++;
	if(err = Walk(nf))
		Clunk(nf);
	return err;
}

char *
Open(Fid *f)
{
	int mode;

	if(!f->busy)
		return "permission denied";
	mode = rhdr.mode;
	if(f->qtype == Quser && (mode & (OWRITE|OTRUNC)))
		return "user already exists";
	thdr.qid = mkqid(f->user, f->qtype);
	return 0;
}

char *
Create(Fid *f)
{
	char *name;
	long perm;

	if(!f->busy)
		return "permission denied";
	name = rhdr.name;
	if(f->user){
		if(strcmp(name, qinfo[Qishost]) != 0)
			return "permission denied";
		f->user->ishost = 1;
		f->qtype = Qishost;
	}else{
		perm = rhdr.perm;
		if(!(perm & CHDIR))
			return "permission denied";
		if(!memchr(name, '\0', NAMELEN))
			return "bad file name";
		if(finduser(name))
			return "user already exists";
		f->user = installuser(name);
		f->user->ref++;
		f->qtype = Quser;
	}
	thdr.qid = mkqid(f->user, f->qtype);
	writeusers();
	return 0;
}

char *
Read(Fid *f)
{
	User *u;
	char *data;
	ulong off, n;
	int i, j, max;

	if(!f->busy)
		return "permission denied";
	n = rhdr.count;
	off = rhdr.offset;
	thdr.count = 0;
	data = thdr.data;
	switch(f->qtype){
	case Qroot:
		if(off % DIRLEN || n % DIRLEN)
			return "unaligned directory read";
		off /= DIRLEN;
		n /= DIRLEN;
		j = 0;
		for(i = 0; i < Nuser; i++)
			for(u = users[i]; u; j++, u = u->link){
				if(j < off)
					continue;
				if(j - off >= n)
					break;
				dostat(u, Quser, data);
				data += DIRLEN;
			}
		thdr.count = data - thdr.data;
		return 0;
	case Quser:
		if(off % DIRLEN || n % DIRLEN)
			return "unaligned directory read";
		off /= DIRLEN;
		n /= DIRLEN;
		max = Qmax;
		if(!f->user->ishost)
			max = Qishost;
		max -= Quser + 1;
		for(i = 0; i < max; i++){
			if(i < off)
				continue;
			if(i - off >= n)
				break;
			dostat(f->user, i - off + Quser + 1, data);
			data += DIRLEN;
		}
		thdr.count = data - thdr.data;
		return 0;
	case Qkey:
		if(f->user->status != Sok)
			return "user disabled";
		if(f->user->expire != 0 && f->user->expire < time(0))
			return "user expired";
		if(off != 0)
			return 0;
		if(n > DESKEYLEN)
			n = DESKEYLEN;
		memmove(thdr.data, f->user->key, n);
		thdr.count = n;
		return 0;
	case Qstatus:
		if(off != 0){
			thdr.count = 0;
			return 0;
		}
		sprint(thdr.data, "%s\n", status[f->user->status]);
		thdr.count = strlen(thdr.data);
		return 0;
	case Qexpire:
		if(off != 0){
			thdr.count = 0;
			return 0;
		}
		if(!f->user->expire)
			strcpy(data, "never\n");
		else
			sprint(data, "%lud\n", f->user->expire);
		if(n > strlen(data))
			n = strlen(data);
		thdr.count = n;
		return 0;
	case Qlog:
		if(off != 0){
			thdr.count = 0;
			return 0;
		}
		sprint(data, "%d\n", f->user->bad);
		if(n > strlen(data))
			n = strlen(data);
		thdr.count = n;
		return 0;
	case Qishost:
		thdr.count = 0;
		return 0;
	default:
		return "permission denied";
	}
}

char *
Write(Fid *f)
{
	char *data, *p;
	ulong n, expire;
	int i;

	if(!f->busy)
		return "permission denied";
	n = rhdr.count;
	data = rhdr.data;
	switch(f->qtype){
	case Qkey:
		if(n != DESKEYLEN)
			return "garbled write data";
		memmove(f->user->key, data, DESKEYLEN);
		thdr.count = DESKEYLEN;
		break;
	case Qstatus:
		data[n] = '\0';
		if(p = strchr(data, '\n'))
			*p = '\0';
		for(i = 0; i < Smax; i++)
			if(strcmp(data, status[i]) == 0){
				f->user->status = i;
				break;
			}
		if(i == Smax)
			return "unknown status";
		f->user->bad = 0;
		thdr.count = n;
		break;
	case Qexpire:
		data[n] = '\0';
		if(p = strchr(data, '\n'))
			*p = '\0';
		else
			p = &data[n];
		if(strcmp(data, "never") == 0)
			expire = 0;
		else{
			expire = strtoul(data, &data, 10);
			if(data != p)
				return "bad expiration date";
		}
		f->user->expire = expire;
		thdr.count = n;
		break;
	case Qlog:
		data[n] = '\0';
		if(strcmp(data, "good") == 0)
			f->user->bad = 0;
		else
			f->user->bad++;
		if(f->user->bad >= MAXBAD){
			f->user->status = Sdisabled;
			break;
		}
		return 0;
	case Qishost:
	case Qroot:
	case Quser:
	default:
		return "permission denied";
	}
	writeusers();
	return 0;
}

char *
Remove(Fid *f)
{
	if(!f->busy)
		return "permission denied";
	if(f->qtype == Qishost)
		f->user->ishost = 0;
	else if(f->qtype == Quser)
		removeuser(f->user);
	else
		return "permission denied";
	Clunk(f);
	writeusers();
	return 0;
}

char *
Stat(Fid *f)
{
	if(!f->busy)
		return "stat on unattached fid";
	dostat(f->user, f->qtype, thdr.stat);
	return 0;
}

char *
Wstat(Fid *f)
{
	Dir d;

	if(!f->busy || f->qtype != Quser)
		return "permission denied";
	if(convM2D(rhdr.stat, &d) == 0)
		return "bad stat buffer";
	if(!memchr(d.name, '\0', NAMELEN))
		return "bad user name";
	if(finduser(d.name))
		return "user already exists";
	if(!removeuser(f->user))
		return "user previously removed";
	strncpy(f->user->name, d.name, NAMELEN);
	insertuser(f->user);
	writeusers();
	return 0;
}

Qid
mkqid(User *u, ulong qtype)
{
	Qid q;

	q.vers = 0;
	q.path = qtype;
	if(u)
		q.path |= u->uniq * 0x100;
	if(qtype == Quser || qtype == Qroot)
		q.path |= CHDIR;
	return q;
}

void
dostat(User *user, ulong qtype, void *p)
{
	Dir d;

	if(qtype == Quser)
		strncpy(d.name, user->name, NAMELEN);
	else
		strncpy(d.name, qinfo[qtype], NAMELEN);
	strncpy(d.uid, "auth", NAMELEN);
	strncpy(d.gid, "auth", NAMELEN);
	d.qid = mkqid(user, qtype);
	if(d.qid.path & CHDIR)
		d.mode = 0777|CHDIR;
	else
		d.mode = 0666;
	d.atime = d.mtime = time(0);
	d.length = d.hlength = 0;
	convD2M(&d, p);
}

int
passline(Biobuf *b, void *vbuf)
{
	char *buf = vbuf;

	if(Bread(b, buf, KEYDBLEN) != KEYDBLEN)
		return 0;
	if(cryptfd < 0)
		decrypt(authkey, buf, KEYDBLEN);
	else
		dofcrypt(cryptfd, "D", buf, KEYDBLEN);
	buf[NAMELEN-1] = '\0';
	return 1;
}

void
writeusers(void)
{
	User *u;
	Biobuf *b;
	char buf[KEYDBLEN], *p;
	ulong expire;
	int i;

	b = Bopen(userkeys, OWRITE);
	if(!b){
		fprint(2, "keyfs: can't write keys file\n");
		return;
	}
	for(i = 0; i < Nuser; i++)
		for(u = users[i]; u; u = u->link){
			strncpy(buf, u->name, NAMELEN);
			memmove(buf+NAMELEN, u->key, DESKEYLEN);
			p = buf + NAMELEN + DESKEYLEN;
			*p++ = u->status;
			*p++ = u->ishost;
			expire = u->expire;
			*p++ = expire;
			*p++ = expire >> 8;
			*p++ = expire >> 16;
			*p = expire >> 24;
			if(cryptfd < 0)
				encrypt(authkey, buf, KEYDBLEN);
			else
				dofcrypt(cryptfd, "E", buf, KEYDBLEN);
			Bwrite(b, buf, sizeof buf);
		}
	Bclose(b);
}

int
readusers(void)
{
	Biobuf *b;
	User *u;
	char buf[KEYDBLEN];
	uchar *p;
	int nu;

	if(nvkey){
		nvkey = 0;
		if(!getauthkey(authkey))
			print("keyfs: warning: bad nvram checksum\n");
	}

	nu = 0;
	b = Bopen(userkeys, OREAD);
	if(b){
		while(passline(b, buf)){
			u = finduser(buf);
			if(u == 0)
				u = installuser(buf);
			memmove(u->key, buf+NAMELEN, DESKEYLEN);
			p = (uchar*)buf + NAMELEN + DESKEYLEN;
			u->status = *p++;
			u->ishost = *p++;
			if(u->status >= Smax)
				fprint(2, "keyfs: warning: bad status in key file\n");
			u->expire = p[0] + (p[1]<<8) + (p[2]<<16) + (p[3]<<24);
			nu++;
		}
		Bclose(b);
	}
	print("%d keys read\n", nu);
	return b != 0;
}

User *
installuser(char *name)
{
	User *u;
	int h;

	h = hash(name);
	u = emalloc(sizeof *u);
	strncpy(u->name, name, NAMELEN);
	u->removed = 0;
	u->ref = 0;
	u->expire = 0;
	u->status = Sok;
	u->bad = 0;
	u->ishost = 0;
	u->uniq = uniq++;
	u->link = users[h];
	users[h] = u;
	return u;
}

User *
finduser(char *name)
{
	User *u;

	for(u = users[hash(name)]; u; u = u->link)
		if(strcmp(name, u->name) == 0)
			return u;
	return 0;
}

int
removeuser(User *user)
{
	User *u, **last;
	char *name;

	user->removed = 1;
	name = user->name;
	last = &users[hash(name)];
	for(u = *last; u; u = *last){
		if(strcmp(name, u->name) == 0){
			*last = u->link;
			return 1;
		}
		last = &u->link;
	}
	return 0;
}

void
insertuser(User *user)
{
	int h;

	user->removed = 0;
	h = hash(user->name);
	user->link = users[h];
	users[h] = user;
}

ulong
hash(char *s)
{
	ulong h;

	h = 0;
	while(*s)
		h = (h << 1) ^ *s++;
	return h % Nuser;
}

Fid *
findfid(int fid)
{
	Fid *f, *ff;

	ff = 0;
	for(f = fids; f; f = f->next)
		if(f->fid == fid)
			return f;
		else if(!ff && !f->busy)
			ff = f;
	if(ff){
		ff->fid = fid;
		return ff;
	}
	f = emalloc(sizeof *f);
	f->fid = fid;
	f->busy = 0;
	f->user = 0;
	f->next = fids;
	fids = f;
	return f;
}

void
io(int in, int out)
{
	char mdata[MAXFDATA + MAXMSG], *err;
	int n;

	for(;;){
		n = read(in, mdata, sizeof mdata);
		if(n == 0)
			continue;
		if(n < 0)
			error("mount read %d", n);
		if(convM2S(mdata, &rhdr, n) == 0)
			continue;

		thdr.data = mdata + MAXMSG;
		thdr.fid = rhdr.fid;
		if(!fcalls[rhdr.type])
			err = "fcall request";
		else		
			err = (*fcalls[rhdr.type])(findfid(rhdr.fid));
		thdr.tag = rhdr.tag;
		thdr.type = rhdr.type+1;
		if(err){
			thdr.type = Rerror;
			strncpy(thdr.ename, err, ERRLEN);
		}
		n = convS2M(&thdr, mdata);
		if(write(out, mdata, n) != n)
			error("mount write");
	}
}

void *
emalloc(ulong n)
{
	void *p;

	if(p = malloc(n))
		return p;
	error("out of memory");
	return 0;		/* not reached */
}

/*
 * encrypt/decrypt data using a crypt device
 */
int
dofcrypt(int fd, char *how, char *s, int n)
{
	
	if(write(fd, how, 1) < 1)
		return -1;
	seek(fd, 0, 0);
	if(write(fd, s, n) < n)
		return -1;
	seek(fd, 0, 0);
	if(read(fd, s, n) < n)
		return -1;
	return n;
}
