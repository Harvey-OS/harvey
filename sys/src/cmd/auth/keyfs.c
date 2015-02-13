/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * keyfs
 */
#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <authsrv.h>
#include <fcall.h>
#include <bio.h>
#include <mp.h>
#include <libsec.h>
#include "authcmdlib.h"

#pragma	varargck	type	"W"	char*

int8_t authkey[8];

typedef struct Fid	Fid;
typedef struct User	User;

enum {
	Qroot,
	Quser,
	Qkey,
	Qsecret,
	Qlog,
	Qstatus,
	Qexpire,
	Qwarnings,
	Qmax,

	Nuser	= 512,
	MAXBAD	= 10,	/* max # of bad attempts before disabling the account */
	/* file must be randomly addressible, so names have fixed length */
	Namelen	= ANAMELEN,
};

enum {
	Sok,
	Sdisabled,
	Smax,
};

struct Fid {
	int	fid;
	uint32_t	qtype;
	User	*user;
	int	busy;
	Fid	*next;
};

struct User {
	int8_t	*name;
	int8_t	key[DESKEYLEN];
	int8_t	secret[SECRETLEN];
	uint32_t	expire;			/* 0 == never */
	uint8_t	status;
	uint32_t	bad;		/* # of consecutive bad authentication attempts */
	int	ref;
	int8_t	removed;
	uint8_t	warnings;
	int32_t	purgatory;		/* time purgatory ends */
	uint32_t	uniq;
	User	*link;
};

char	*qinfo[Qmax] = {
	[Qroot]		"keys",
	[Quser]		".",
	[Qkey]		"key",
	[Qsecret]	"secret",
	[Qlog]		"log",
	[Qexpire]	"expire",
	[Qstatus]	"status",
	[Qwarnings]	"warnings",
};

char	*status[Smax] = {
	[Sok]		"ok",
	[Sdisabled]	"disabled",
};

Fid	*fids;
User	*users[Nuser];
int8_t	*userkeys;
int	nuser;
uint32_t	uniq = 1;
Fcall	rhdr,
	thdr;
int	usepass;
int8_t	*warnarg;
uint8_t	mdata[8192 + IOHDRSZ];
int	messagesize = sizeof mdata;

int	readusers(void);
uint32_t	hash(int8_t*);
Fid	*findfid(int);
User	*finduser(int8_t*);
User	*installuser(int8_t*);
int	removeuser(User*);
void	insertuser(User*);
void	writeusers(void);
void	io(int, int);
void	*emalloc(uint32_t);
Qid	mkqid(User*, uint32_t);
int	dostat(User*, uint32_t, void*, int);
int	newkeys(void);
void	warning(void);
int	weirdfmt(Fmt *f);

int8_t	*Auth(Fid*), *Attach(Fid*), *Version(Fid*),
	*Flush(Fid*), *Walk(Fid*),
	*Open(Fid*), *Create(Fid*),
	*Read(Fid *), *Write(Fid*), *Clunk(Fid*),
	*Remove(Fid *), *Stat(Fid*), *Wstat(Fid*);
char 	*(*fcalls[])(Fid*) = {
	[Tattach]	Attach,
	[Tauth]	Auth,
	[Tclunk]	Clunk,
	[Tcreate]	Create,
	[Tflush]	Flush,
	[Topen]		Open,
	[Tread]		Read,
	[Tremove]	Remove,
	[Tstat]		Stat,
	[Tversion]	Version,
	[Twalk]		Walk,
	[Twrite]	Write,
	[Twstat]	Wstat,
};

static void
usage(void)
{
	fprint(2, "usage: %s [-p] [-m mtpt] [-w warn] [keyfile]\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *mntpt;
	int p[2];

	fmtinstall('W', weirdfmt);
	mntpt = "/mnt/keys";
	ARGBEGIN{
	case 'm':
		mntpt = EARGF(usage());
		break;
	case 'p':
		usepass = 1;
		break;
	case 'w':
		warnarg = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND
	argv0 = "keyfs";

	userkeys = "/adm/keys";
	if(argc > 1)
		usage();
	if(argc == 1)
		userkeys = argv[0];

	if(pipe(p) < 0)
		error("can't make pipe: %r");

	if(usepass) {
		getpass(authkey, nil, 0, 0);
	} else {
		if(!getauthkey(authkey))
			print("keyfs: warning: can't read NVRAM\n");
	}

	switch(rfork(RFPROC|RFNAMEG|RFNOTEG|RFNOWAIT|RFENVG|RFFDG)){
	case 0:
		close(p[0]);
		io(p[1], p[1]);
		exits(0);
	case -1:
		error("fork");
	default:
		close(p[1]);
		if(mount(p[0], -1, mntpt, MREPL|MCREATE, "") < 0)
			error("can't mount: %r");
		exits(0);
	}
}

int8_t *
Flush(Fid *f)
{
	USED(f);
	return 0;
}

int8_t *
Auth(Fid *)
{
	return "keyfs: authentication not required";
}

int8_t *
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

int8_t*
Version(Fid*)
{
	Fid *f;

	for(f = fids; f; f = f->next)
		if(f->busy)
			Clunk(f);
	if(rhdr.msize > sizeof mdata)
		thdr.msize = sizeof mdata;
	else
		thdr.msize = rhdr.msize;
	messagesize = thdr.msize;
	if(strncmp(rhdr.version, "9P2000", 6) != 0)
		return "bad 9P version";
	thdr.version = "9P2000";
	return 0;
}

int8_t *
Walk(Fid *f)
{
	int8_t *name, *err;
	int i, j, max;
	Fid *nf;
	uint32_t qtype;
	User *user;

	if(!f->busy)
		return "walk of unused fid";
	nf = nil;
	qtype = f->qtype;
	user = f->user;
	if(rhdr.fid != rhdr.newfid){
		nf = findfid(rhdr.newfid);
		if(nf->busy)
			return "fid in use";
		f = nf;	/* walk f */
	}

	err = nil;
	i = 0;
	if(rhdr.nwname > 0){
		for(; i<rhdr.nwname; i++){
			if(i >= MAXWELEM){
				err = "too many path name elements";
				break;
			}
			name = rhdr.wname[i];
			switch(qtype){
			case Qroot:
				if(strcmp(name, "..") == 0)
					goto Accept;
				user = finduser(name);
				if(!user)
					goto Out;
				qtype = Quser;

			Accept:
				thdr.wqid[i] = mkqid(user, qtype);
				break;

			case Quser:
				if(strcmp(name, "..") == 0) {
					qtype = Qroot;
					user = 0;
					goto Accept;
				}
				max = Qmax;
				for(j = Quser + 1; j < Qmax; j++)
					if(strcmp(name, qinfo[j]) == 0){
						qtype = j;
						break;
					}
				if(j < max)
					goto Accept;
				goto Out;

			default:
				err = "file is not a directory";
				goto Out;
			}
		}
	    Out:
		if(i < rhdr.nwname && err == nil)
			err = "file not found";
	}

	if(err != nil){
		return err;
	}

	/* if we cloned and then completed the walk, update new fid */
	if(rhdr.fid != rhdr.newfid && i == rhdr.nwname){
		nf->busy = 1;
		nf->qtype = qtype;
		if(nf->user = user)
			nf->user->ref++;
	}else if(nf == nil && rhdr.nwname > 0){	/* walk without clone (rare) */
		Clunk(f);
		f->busy = 1;
		f->qtype = qtype;
		if(f->user = user)
			f->user->ref++;
	}

	thdr.nwqid = i;
	return 0;
}

int8_t *
Clunk(Fid *f)
{
	f->busy = 0;
	if(f->user && --f->user->ref == 0 && f->user->removed) {
		free(f->user->name);
		free(f->user);
	}
	f->user = 0;
	return 0;
}

int8_t *
Open(Fid *f)
{
	int mode;

	if(!f->busy)
		return "open of unused fid";
	mode = rhdr.mode;
	if(f->qtype == Quser && (mode & (OWRITE|OTRUNC)))
		return "user already exists";
	thdr.qid = mkqid(f->user, f->qtype);
	thdr.iounit = messagesize - IOHDRSZ;
	return 0;
}

int8_t *
Create(Fid *f)
{
	int8_t *name;
	int32_t perm;

	if(!f->busy)
		return "create of unused fid";
	name = rhdr.name;
	if(f->user){
		return "permission denied";
	}else{
		perm = rhdr.perm;
		if(!(perm & DMDIR))
			return "permission denied";
		if(strcmp(name, "") == 0)
			return "empty file name";
		if(strlen(name) >= Namelen)
			return "file name too long";
		if(finduser(name))
			return "user already exists";
		f->user = installuser(name);
		f->user->ref++;
		f->qtype = Quser;
	}
	thdr.qid = mkqid(f->user, f->qtype);
	thdr.iounit = messagesize - IOHDRSZ;
	writeusers();
	return 0;
}

int8_t *
Read(Fid *f)
{
	User *u;
	int8_t *data;
	uint32_t off, n, m;
	int i, j, max;

	if(!f->busy)
		return "read of unused fid";
	n = rhdr.count;
	off = rhdr.offset;
	thdr.count = 0;
	data = thdr.data;
	switch(f->qtype){
	case Qroot:
		j = 0;
		for(i = 0; i < Nuser; i++)
			for(u = users[i]; u; j += m, u = u->link){
				m = dostat(u, Quser, data, n);
				if(m <= BIT16SZ)
					break;
				if(j < off)
					continue;
				data += m;
				n -= m;
			}
		thdr.count = data - thdr.data;
		return 0;
	case Quser:
		max = Qmax;
		max -= Quser + 1;
		j = 0;
		for(i = 0; i < max; j += m, i++){
			m = dostat(f->user, i + Quser + 1, data, n);
			if(m <= BIT16SZ)
				break;
			if(j < off)
				continue;
			data += m;
			n -= m;
		}
		thdr.count = data - thdr.data;
		return 0;
	case Qkey:
		if(f->user->status != Sok)
			return "user disabled";
		if(f->user->purgatory > time(0))
			return "user in purgatory";
		if(f->user->expire != 0 && f->user->expire < time(0))
			return "user expired";
		if(off != 0)
			return 0;
		if(n > DESKEYLEN)
			n = DESKEYLEN;
		memmove(thdr.data, f->user->key, n);
		thdr.count = n;
		return 0;
	case Qsecret:
		if(f->user->status != Sok)
			return "user disabled";
		if(f->user->purgatory > time(0))
			return "user in purgatory";
		if(f->user->expire != 0 && f->user->expire < time(0))
			return "user expired";
		if(off != 0)
			return 0;
		if(n > strlen(f->user->secret))
			n = strlen(f->user->secret);
		memmove(thdr.data, f->user->secret, n);
		thdr.count = n;
		return 0;
	case Qstatus:
		if(off != 0){
			thdr.count = 0;
			return 0;
		}
		if(f->user->status == Sok && f->user->expire && f->user->expire < time(0))
			sprint(thdr.data, "expired\n");
		else
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
		sprint(data, "%lud\n", f->user->bad);
		if(n > strlen(data))
			n = strlen(data);
		thdr.count = n;
		return 0;
	case Qwarnings:
		if(off != 0){
			thdr.count = 0;
			return 0;
		}
		sprint(data, "%ud\n", f->user->warnings);
		if(n > strlen(data))
			n = strlen(data);
		thdr.count = n;
		return 0;
	default:
		return "permission denied: unknown qid";
	}
}

int8_t *
Write(Fid *f)
{
	int8_t *data, *p;
	uint32_t n, expire;
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
	case Qsecret:
		if(n >= SECRETLEN)
			return "garbled write data";
		memmove(f->user->secret, data, n);
		f->user->secret[n] = 0;
		thdr.count = n;
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
		f->user->warnings = 0;
		thdr.count = n;
		break;
	case Qlog:
		data[n] = '\0';
		if(strcmp(data, "good") == 0)
			f->user->bad = 0;
		else
			f->user->bad++;
		if(f->user->bad && ((f->user->bad)%MAXBAD) == 0)
			f->user->purgatory = time(0) + f->user->bad;
		return 0;
	case Qwarnings:
		data[n] = '\0';
		f->user->warnings = strtoul(data, 0, 10);
		thdr.count = n;
		break;
	case Qroot:
	case Quser:
	default:
		return "permission denied";
	}
	writeusers();
	return 0;
}

int8_t *
Remove(Fid *f)
{
	if(!f->busy)
		return "permission denied";
	if(f->qtype == Qwarnings)
		f->user->warnings = 0;
	else if(f->qtype == Quser)
		removeuser(f->user);
	else {
		Clunk(f);
		return "permission denied";
	}
	Clunk(f);
	writeusers();
	return 0;
}

int8_t *
Stat(Fid *f)
{
	static uint8_t statbuf[1024];

	if(!f->busy)
		return "stat on unattached fid";
	thdr.nstat = dostat(f->user, f->qtype, statbuf, sizeof statbuf);
	if(thdr.nstat <= BIT16SZ)
		return "stat buffer too small";
	thdr.stat = statbuf;
	return 0;
}

int8_t *
Wstat(Fid *f)
{
	Dir d;
	int n;
	int8_t buf[1024];

	if(!f->busy || f->qtype != Quser)
		return "permission denied";
	if(rhdr.nstat > sizeof buf)
		return "wstat buffer too big";
	if(convM2D(rhdr.stat, rhdr.nstat, &d, buf) == 0)
		return "bad stat buffer";
	n = strlen(d.name);
	if(n == 0 || n >= Namelen)
		return "bad user name";
	if(finduser(d.name))
		return "user already exists";
	if(!removeuser(f->user))
		return "user previously removed";
	free(f->user->name);
	f->user->name = strdup(d.name);
	if(f->user->name == nil)
		error("wstat: malloc failed: %r");
	insertuser(f->user);
	writeusers();
	return 0;
}

Qid
mkqid(User *u, uint32_t qtype)
{
	Qid q;

	q.vers = 0;
	q.path = qtype;
	if(u)
		q.path |= u->uniq * 0x100;
	if(qtype == Quser || qtype == Qroot)
		q.type = QTDIR;
	else
		q.type = QTFILE;
	return q;
}

int
dostat(User *user, uint32_t qtype, void *p, int n)
{
	Dir d;

	if(qtype == Quser)
		d.name = user->name;
	else
		d.name = qinfo[qtype];
	d.uid = d.gid = d.muid = "auth";
	d.qid = mkqid(user, qtype);
	if(d.qid.type & QTDIR)
		d.mode = 0777|DMDIR;
	else
		d.mode = 0666;
	d.atime = d.mtime = time(0);
	d.length = 0;
	return convD2M(&d, p, n);
}

int
passline(Biobuf *b, void *vbuf)
{
	int8_t *buf = vbuf;

	if(Bread(b, buf, KEYDBLEN) != KEYDBLEN)
		return 0;
	decrypt(authkey, buf, KEYDBLEN);
	buf[Namelen-1] = '\0';
	return 1;
}

void
randombytes(uint8_t *p, int len)
{
	int i, fd;

	fd = open("/dev/random", OREAD);
	if(fd < 0){
		fprint(2, "keyfs: can't open /dev/random, using rand()\n");
		srand(time(0));
		for(i = 0; i < len; i++)
			p[i] = rand();
		return;
	}
	read(fd, p, len);
	close(fd);
}

void
oldCBCencrypt(int8_t *key7, uint8_t *p, int len)
{
	uint8_t ivec[8];
	uint8_t key[8];
	DESstate s;

	memset(ivec, 0, 8);
	des56to64((uint8_t*)key7, key);
	setupDESstate(&s, key, ivec);
	desCBCencrypt((uint8_t*)p, len, &s);
}

void
oldCBCdecrypt(int8_t *key7, uint8_t *p, int len)
{
	uint8_t ivec[8];
	uint8_t key[8];
	DESstate s;

	memset(ivec, 0, 8);
	des56to64((uint8_t*)key7, key);
	setupDESstate(&s, key, ivec);
	desCBCdecrypt((uint8_t*)p, len, &s);

}

void
writeusers(void)
{
	int fd, i, nu;
	User *u;
	uint8_t *p, *buf;
	uint32_t expire;

	/* count users */
	nu = 0;
	for(i = 0; i < Nuser; i++)
		for(u = users[i]; u; u = u->link)
			nu++;

	/* pack into buffer */
	buf = malloc(KEYDBOFF + nu*KEYDBLEN);
	if(buf == 0){
		fprint(2, "keyfs: can't write keys file, out of memory\n");
		return;
	}
	p = buf;
	randombytes(p, KEYDBOFF);
	p += KEYDBOFF;
	for(i = 0; i < Nuser; i++)
		for(u = users[i]; u; u = u->link){
			strncpy((int8_t*)p, u->name, Namelen);
			p += Namelen;
			memmove(p, u->key, DESKEYLEN);
			p += DESKEYLEN;
			*p++ = u->status;
			*p++ = u->warnings;
			expire = u->expire;
			*p++ = expire;
			*p++ = expire >> 8;
			*p++ = expire >> 16;
			*p++ = expire >> 24;
			memmove(p, u->secret, SECRETLEN);
			p += SECRETLEN;
		}

	/* encrypt */
	oldCBCencrypt(authkey, buf, p - buf);

	/* write file */
	fd = create(userkeys, OWRITE, 0660);
	if(fd < 0){
		free(buf);
		fprint(2, "keyfs: can't write keys file\n");
		return;
	}
	if(write(fd, buf, p - buf) != (p - buf))
		fprint(2, "keyfs: can't write keys file\n");

	free(buf);
	close(fd);
}

int
weirdfmt(Fmt *f)
{
	int8_t *s, *p, *ep, buf[ANAMELEN*4 + 1];
	int i, n;
	Rune r;

	s = va_arg(f->args, int8_t*);
	p = buf;
	ep = buf + sizeof buf;
	for(i = 0; i < ANAMELEN; i += n){
		n = chartorune(&r, s + i);
		if(r == Runeerror)
			p = seprint(p, ep, "[%.2x]", buf[i]);
		else if(isascii(r) && iscntrl(r))
			p = seprint(p, ep, "[%.2x]", r);
		else if(r == ' ' || r == '/')
			p = seprint(p, ep, "[%c]", r);
		else
			p = seprint(p, ep, "%C", r);
	}
	return fmtstrcpy(f, buf);
}

int
userok(int8_t *user, int nu)
{
	int i, n, rv;
	Rune r;
	int8_t buf[ANAMELEN+1];

	memset(buf, 0, sizeof buf);
	memmove(buf, user, ANAMELEN);

	if(buf[ANAMELEN-1] != 0){
		fprint(2, "keyfs: %d: no termination: %W\n", nu, buf);
		return -1;
	}

	rv = 0;
	for(i = 0; buf[i]; i += n){
		n = chartorune(&r, buf+i);
		if(r == Runeerror){
//			fprint(2, "keyfs: name %W bad rune byte %d\n", buf, i);
			rv = -1;
		} else if(isascii(r) && iscntrl(r) || r == ' ' || r == '/'){
//			fprint(2, "keyfs: name %W bad char %C\n", buf, r);
			rv = -1;
		}
	}

	if(i == 0){
		fprint(2, "keyfs: %d: nil name\n", nu);
		return -1;
	}
	if(rv == -1)
		fprint(2, "keyfs: %d: bad syntax: %W\n", nu, buf);
	return rv;
}

int
readusers(void)
{
	int fd, i, n, nu;
	uint8_t *p, *buf, *ep;
	User *u;
	Dir *d;

	/* read file into an array */
	fd = open(userkeys, OREAD);
	if(fd < 0)
		return 0;
	d = dirfstat(fd);
	if(d == nil){
		close(fd);
		return 0;
	}
	buf = malloc(d->length);
	if(buf == 0){
		close(fd);
		free(d);
		return 0;
	}
	n = readn(fd, buf, d->length);
	close(fd);
	free(d);
	if(n != d->length){
		free(buf);
		return 0;
	}

	/* decrypt */
	n -= n % KEYDBLEN;
	oldCBCdecrypt(authkey, buf, n);

	/* unpack */
	nu = 0;
	for(i = KEYDBOFF; i < n; i += KEYDBLEN){
		ep = buf + i;
		if(userok((int8_t*)ep, i/KEYDBLEN) < 0)
			continue;
		u = finduser((int8_t*)ep);
		if(u == 0)
			u = installuser((int8_t*)ep);
		memmove(u->key, ep + Namelen, DESKEYLEN);
		p = ep + Namelen + DESKEYLEN;
		u->status = *p++;
		u->warnings = *p++;
		if(u->status >= Smax)
			fprint(2, "keyfs: warning: bad status in key file\n");
		u->expire = p[0] + (p[1]<<8) + (p[2]<<16) + (p[3]<<24);
		p += 4;
		memmove(u->secret, p, SECRETLEN);
		u->secret[SECRETLEN-1] = 0;
		nu++;
	}
	free(buf);

	print("%d keys read\n", nu);
	return 1;
}

User *
installuser(int8_t *name)
{
	User *u;
	int h;

	h = hash(name);
	u = emalloc(sizeof *u);
	u->name = strdup(name);
	if(u->name == nil)
		error("malloc failed: %r");
	u->removed = 0;
	u->ref = 0;
	u->purgatory = 0;
	u->expire = 0;
	u->status = Sok;
	u->bad = 0;
	u->warnings = 0;
	u->uniq = uniq++;
	u->link = users[h];
	users[h] = u;
	return u;
}

User *
finduser(int8_t *name)
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
	int8_t *name;

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

uint32_t
hash(int8_t *s)
{
	uint32_t h;

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
	int8_t *err;
	int n;
	int32_t now, lastwarning;

	/* after restart, let the system settle for 5 mins before warning */
	lastwarning = time(0) - 24*60*60 + 5*60;

	for(;;){
		n = read9pmsg(in, mdata, messagesize);
		if(n == 0)
			continue;
		if(n < 0)
			error("mount read %d", n);
		if(convM2S(mdata, n, &rhdr) == 0)
			continue;

		if(newkeys())
			readusers();

		thdr.data = (int8_t*)mdata + IOHDRSZ;
		thdr.fid = rhdr.fid;
		if(!fcalls[rhdr.type])
			err = "fcall request";
		else
			err = (*fcalls[rhdr.type])(findfid(rhdr.fid));
		thdr.tag = rhdr.tag;
		thdr.type = rhdr.type+1;
		if(err){
			thdr.type = Rerror;
			thdr.ename = err;
		}
		n = convS2M(&thdr, mdata, messagesize);
		if(write(out, mdata, n) != n)
			error("mount write");

		now = time(0);
		if(warnarg && (now - lastwarning > 24*60*60)){
			syslog(0, "auth", "keyfs starting warnings: %lux %lux",
				now, lastwarning);
			warning();
			lastwarning = now;
		}
	}
}

int
newkeys(void)
{
	Dir *d;
	static int32_t ftime;

	d = dirstat(userkeys);
	if(d == nil)
		return 0;
	if(d->mtime > ftime){
		ftime = d->mtime;
		free(d);
		return 1;
	}
	free(d);
	return 0;
}

void *
emalloc(uint32_t n)
{
	void *p;

	if(p = malloc(n))
		return p;
	error("out of memory");
	return 0;		/* not reached */
}

void
warning(void)
{
	int i;
	int8_t buf[64];

	snprint(buf, sizeof buf, "-%s", warnarg);
	switch(rfork(RFPROC|RFNAMEG|RFNOTEG|RFNOWAIT|RFENVG|RFFDG)){
	case 0:
		i = open("/sys/log/auth", OWRITE);
		if(i >= 0){
			dup(i, 2);
			seek(2, 0, 2);
			close(i);
		}
		execl("/bin/auth/warning", "warning", warnarg, nil);
		error("can't exec warning");
	}
}
