#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <libg.h>
#include <frame.h>
#include <layer.h>
#include "dat.h"
#include "fns.h"

typedef struct Rfile	Rfile;
typedef struct File	File;
typedef struct Dirtab	Dirtab;

struct Rfile			/* indexed by fid to find file */
{
	int	busy;
	int	open;
	int	fileno;
};

struct Dirtab
{
	char	*name;
	ulong	qid;
	ulong	perm;
	char	flag;
};

struct File
{
	int	busy;
	ulong	path;		/* == qid.path */
	int	slot;		/* == qid.vers */
	Dirtab	dir;
};

char	*errs[]=
{
	[Enoerr]	"no such error",
	[Eauth]		"authentication failed",
	[Enotdir]	"not a directory",
	[Eisdir]	"file is a directory",
	[Enonexist]	"file does not exist",
	[Eperm]		"permission denied",
	[Einuse]	"file in use",
	[Eio]		"i/o error",
	[Eslot]		"slot out of range",
	[Erbit]		"bad read from /dev/bitblt",
	[Erwin]		"bad read from /dev/window",
	[Esmall]	"buffer too small",
	[Eshutdown]	"window shut down",
	[Ebit]		"bad bitblt request",
	[Esbit]		"bad bitblt request from 8½ to system",
	[Ebitmap]	"unallocated bitmap",
	[Esubfont]	"unallocated subfont",
	[Efont]		"unallocated font",
	[Eunimpl]	"unimplemented operation",
	[Estring]	"malformed string",
	[Ebadrect]	"bad rectangle",
	[Enores]	"no resources",
	[Ebadctl]	"bad consctl request",
	[Epartial]	"offset in middle of rune",
	[Eoffend]	"offset beyond end of file",
	[Esfnotcached]	"subfont not cached",
	[Eshort]	"short read on /dev/bitblt",
	[Eballoc]	"out of bitmap memory",
	[Egreg]		"it's all greg's fault",
};

Rfile	*rfile;
File	*file;
int	nfile;
int	nfilealloc;
int	nrfilealloc;
char	rdata[MAXMSG+MAXFDATA+3];	/* room for rune overflow */
char	tdata[MAXMSG+MAXFDATA+3];
char	tbuf[MAXFDATA+3];
Fcall	rhdr;
Fcall	thdr;
int	sfd;		/* server end of pipe */
char	user[NAMELEN];

void	rflush(void);
void	rauth(void);
void	rattach(Rfile *);
void	rclone(Rfile *, int);
int	rwalk(Rfile *);
void	ropen(Rfile *);
void	rcreate(Rfile *);
void	rread(Rfile *);
void	rwrite(Rfile *);
void	rclunk(Rfile *, int);
void	rstat(Rfile *);
void	rwstat(Rfile *);
void	rerrstr(Rfile *);
void	ruserstr(Rfile *);
Dirtab	*dirlook(File *, int);
int	walive(Window *);
Window	*findwindow(Window *, ulong, int);
void	wfopen(Rfile*, ulong, ulong, Dirtab*);
int	doconsctl(char*, int, Window *);

int	okfid(int);
void	sendmsg(int);
long	getclock(void);

enum{
	Qdir=1,
	Qbitblt,
	Qcons,
	Qctl,
	Qmouse,
	Qnbmouse,
	Qsnarf,
	Qwindow,
	Qlabel,
	Qselect,
	Qwinds,
	Qwinid,
	Qtext,
	NQid
};

Dirtab dirtab[]=
{
	"bitblt",	{Qbitblt},	0600,		1,
	"cons",		{Qcons},	0600,		1,
	"consctl",	{Qctl},		0200,		1,
	"mouse",	{Qmouse},	0600,		1,
	"nbmouse",	{Qnbmouse},	0600,		1,
	"snarf",	{Qsnarf},	0600,		0,
	"window",	{Qwindow},	0600,		1,
	"label",	{Qlabel},	0600,		1,
	"select",	{Qselect},	0600,		1,
	"text",		{Qtext},	0400,		1,
	"windows",	{Qwinds|CHDIR},	CHDIR|0500,	0,
	"winid",	{Qwinid},	0400,		0,
	0,
};

Dirtab rootdir = 
{
	".",		{Qdir|CHDIR},	CHDIR|0500,	0,
};

Dirtab winddir =
{
	"windows",	{Qwinds|CHDIR},	CHDIR|0500,	0,
};

void
io(void)
{
	long n;
	Rfile *rf;
	char *err;

	if(boxup){
		border(&screen, box, BOXWID, F&~D);	/* draw box */
		bflush();
	}
	n = sizeof(rdata);
	err = getS(sfd, rdata, &rhdr, &n);		/* read (may take a while) */
	if(boxup){
		border(&screen, box, BOXWID, F&~D);	/* undraw box */
		bflush();
	}
	if(err){
		fprint(2, "8½: format error %s\n", err);
		return;
	}		

/*BUG: check busy */
	if(rhdr.type!=Tauth && okfid(rhdr.fid)==0)
		return;
	rf = &rfile[rhdr.fid];
#ifdef DEBUG
	{
		static int inited;
		if(!inited){
			fmtinstall('F', fcallconv);
			inited++;
		}
		fprint(2, "msg: %F\n", &rhdr);
	}
#endif
	switch(rhdr.type){
	default:
		fprint(2, "8½: unknown type %d\n", rhdr.type);
		return;
	case Tflush:
		rflush();
		break;
	case Tauth:
		rauth();
		break;
	case Tattach:
		rattach(rf);
		break;
	case Tclone:
		rclone(rf, 0);
		break;
	case Twalk:
		rwalk(rf);
		break;
	case Tclwalk:
		rclone(rf, 1);
		break;
	case Topen:
		ropen(rf);
		break;
	case Tcreate:
		rcreate(rf);
		break;
	case Tread:
		rread(rf);
		break;
	case Twrite:
		rwrite(rf);
		break;
	case Tclunk:
		rclunk(rf, 0);
		break;
	case Tremove:
		rclunk(rf, Eperm);
		break;
	case Tstat:
		rstat(rf);
		break;
	case Twstat:
		rwstat(rf);
		break;
	}
}

int
newfile(void)
{
	int i;
	enum{ Delta=10 };

	for(i=0; i<nfile; i++)
		if(file[i].busy == 0)
			return i;
	if(nfilealloc == nfile){
		file = erealloc(file, (nfilealloc+Delta)*sizeof(File));
		memset(file+nfilealloc, 0, Delta*sizeof(File));
		nfilealloc += Delta;
	}
	return nfile++;
}

int
okfid(int fid)
{
	enum{ Delta=10 };

	if(fid < 0){
		fprint(2, "8½: negative fid %d\n", fid);
		return 0;
	}
	if(fid >= nrfilealloc){
		fid += Delta;
		rfile = erealloc(rfile, fid*sizeof(Rfile));
		memset(rfile+nrfilealloc, 0, (fid-nrfilealloc)*sizeof(Rfile));
		nrfilealloc = fid;
	}
	return 1;
}

void
rflush(void)
{
	termflush(rhdr.oldtag);
	sendmsg(0);
}

void
rauth(void)
{
	sendmsg(Eauth);
}

void
rattach(Rfile *rf)
{
	int f, s, err, pid;
	Rectangle r;
	char *n;
	File *fp;

	err = 0;
	if(strcmp(rhdr.aname, "slave") == 0){
		if(nfile == 0)
			newfile();
		f = 0;
		file[0].slot = 0;
	}else{
		if(rhdr.aname[0] == 'N'){
			n = rhdr.aname+1;
			pid = strtoul(n, &n, 0);
			r.min.x = strtol(n, &n, 0);
			r.min.y = strtol(n, &n, 0);
			r.max.x = strtol(n, &n, 0);
			r.max.y = strtol(n, &n, 0);
			if(cfd>=0 && okrect(r)){
				s = newterm(r, 0, pid);
				if(s == -1){
					err = Enores;
					goto send;
				}
			}else{
				err = Ebadrect;
				goto send;
			}
		}else{
			s = atoi(rhdr.aname);
			if(s<=0 || s>=(1<<16)){
				print("slot %d out of range\n", s);
				err = Eslot;
				goto send;
			}
		}
		f = newfile();
		file[f].slot = s;
	}
	rf->busy = 1;
	rf->open = 0;
	rf->fileno = f;
	fp = &file[f];
	fp->busy = 1;
	fp->path = CHDIR|Qdir;
	fp->dir = rootdir;
	thdr.qid.path = fp->path;
	thdr.qid.vers = fp->slot;
	strcpy(user, rhdr.uname);
	window[fp->slot].ref++;
    send:
	sendmsg(err);
}

void
rclone(Rfile *rf, int dowalk)
{
	Rfile *nrf;
	File *fp;
	int err=0;

	fp = &file[rf->fileno];
	if(window[fp->slot].deleted){
		err = Eshutdown;
		goto send;
	}
	if(okfid(rhdr.newfid) == 0){
		err = Egreg;
		goto send;
	}
	rf = &rfile[rhdr.fid];	/* okfid() might have reallocated rfile */
	nrf = &rfile[rhdr.newfid];
	if(nrf->busy){
		fprint(2, "8½: clone to used channel\n");
		err = Egreg;
		goto send;
	}
	nrf->busy = 1;
	nrf->open = 0;
	nrf->fileno = rf->fileno;
	window[fp->slot].ref++;
	if(dowalk){
		rhdr.fid = rhdr.newfid;
		if(rwalk(nrf)){
			window[fp->slot].ref--;
			nrf->busy = 0;
		}
		return;
	}
    send:
	sendmsg(err);
}

int
rwalk(Rfile *rf)
{
	int err;
	ulong qid, slot;
	char *name;
	File *fp;
	int j;
	Dirtab *dp;

	err = 0;
	qid = 0;
	fp = &file[rf->fileno];
	slot = fp->slot;
	if(window[fp->slot].deleted){
		err = Eshutdown;
		goto send;
	}
	name = rhdr.name;
	if((fp->path&CHDIR) == 0){
		err = Enotdir;
		goto send;
	}
	if(strcmp(name, ".") == 0){
		qid = fp->path;
		goto send;
	}
	if(strcmp(name, "..") == 0){
		if(fp->path==(Qwinds|CHDIR))
			qid = Qdir|CHDIR;
		else{
			qid = Qwinds|CHDIR;
			wfopen(rf, qid, slot, &winddir);
		}
		goto send;
	}
	for(j=0; ; j++){
		dp = dirlook(fp, j);
		if(dp->name==0)
			break;
		if(strcmp(name, dp->name) == 0){
			qid = dp->qid;
			wfopen(rf, qid, slot, dp);
			goto send;
		}
	}
	err = Enonexist;
    send:
	thdr.qid.path = qid;
	thdr.qid.vers = slot;
	sendmsg(err);
	return err;
}

void
ropen(Rfile *rf)
{
	int err, mode, trunc;
	File *fp;
	long perm;
	Window *w;
	ulong qtype;

	err = 0;
	fp = &file[rf->fileno];
	perm = fp->dir.perm;
	mode = rhdr.mode;
	trunc = 0;
	if(fp->path & CHDIR){
		if(mode)
			err = Eperm;
	}else{
		trunc = mode & OTRUNC;
		mode &= ~OTRUNC;
		if(mode<0 || mode>3){
			err = Eperm;
			goto send;
		}
		if(((mode==OREAD  || mode==ORDWR) && !(perm&0400))
		|| ((mode==OWRITE || mode==ORDWR) && !(perm&0200))
		|| ((mode==OEXEC) && !(perm&0100))
		|| (trunc && !(perm&0200))){
			err = Eperm;
			goto send;
		}
	}

	w = &window[fp->slot];
	if(w->deleted){
		err = Eshutdown;
		goto send;
	}
	qtype = fp->path;
	if((qtype&CHDIR)==0 && qtype>=NQid){
		int i;
		for(i=1; i<NSLOT; i++)
			if(walive(&window[i]) && atoi(window[i].wqid)==(qtype>>16))
				break;
		if(i>=NSLOT){
			err = Eshutdown;
			goto send;
		}
		w = &window[i];
		qtype &= 0xFFFF;
	}
	switch(qtype){
	case Qbitblt:
		if(w->bitopen){
			err = Einuse;
			goto send;
		}
		bitopen(w);
		break;

	case Qmouse:
	case Qnbmouse:
		if(w->mouseopen){
			err = Einuse;
			goto send;
		}
		w->mouseopen = 1;
		break;

	case Qsnarf:
		if(trunc)
			textdelete(&window[SNARF].text, 0, window[SNARF].text.n);
		break;

	case Qctl:
		w->ctlopen++;
		break;
	}
	rf->open = 1;

    send:
	thdr.qid.path = fp->path;
	thdr.qid.vers = fp->slot;
	sendmsg(err);
}

void
rcreate(Rfile *rf)
{
	USED(rf);
	sendmsg(Eperm);
}

void
rread(Rfile *rf)
{
	int i, l, n, j, err, cnt, wid;
	long off, clock;
	Dir dir;
	File *fp;
	Dirtab *dp;
	Window *w;
	Text *t;
	char *s;

	n = 0;
	err = 0;
	off = rhdr.offset;
	cnt = rhdr.count;
	fp = &file[rf->fileno];
	if((w = findwindow(&window[fp->slot], fp->path, 0)) == 0){
		err = Eshutdown;
		goto send;
	}
	if(fp->path & CHDIR){
		if(off%DIRLEN || cnt%DIRLEN){
			err = Eio;
			goto send;
		}
		clock = getclock();
		for(j=off/DIRLEN; cnt>0; j++){
			dp = dirlook(fp, j);
			if(dp->name==0)
				break;
			memmove(dir.name, dp->name, NAMELEN);
			dir.qid.path = dp->qid;
			dir.qid.vers = fp->slot;
			dir.mode = dp->perm;
			dir.length = 0;
			dir.hlength = 0;
			memmove(dir.uid, user, NAMELEN);
			memmove(dir.gid, user, NAMELEN);
			dir.atime = clock;
			dir.mtime = clock;
			convD2M(&dir, tbuf+n);
			n += DIRLEN;
			cnt -= DIRLEN;
		}
		thdr.data = tbuf;
		goto send;
	}
	switch(fp->path&0xFFFF){
	case Qbitblt:
		n = bitread(w, cnt, &err);
		thdr.data = bitbuf;
		break;

	case Qcons:
		termread(w, rhdr.tag, rhdr.fid, cnt);
		return;

	case Qmouse:
	case Qnbmouse:
		if(cnt < sizeof(mouse.data)){
			n = 0;
			err = Esmall;
			goto send;
		}
		termmouseread(w, rhdr.tag, rhdr.fid, Qmouse==(fp->path&0xFFFF));
		return;

	case Qselect:
		t = &w->text;
		i = w->q0;
		j = w->q1;
		goto retUTF;

	case Qsnarf:
		t = &window[SNARF].text;
		i = 0;
		j = t->n;

	retUTF:		/* just unpack from the beginning */
		l = 0;
		n = 0;
		for(; i<j; i++){
			wid = runetochar(tbuf+n, t->s+i);
			l += wid;
			if(l > rhdr.offset){
				if(n == 0){	/* maybe partial rune */
					n = l-rhdr.offset;
					memmove(tbuf, tbuf+(wid-n), 3);
				}else
					n += wid;
			}
			if(n >= cnt){
				n = cnt;
				break;
			}
		}
		thdr.data = tbuf;
		goto send;

	case Qtext:
		t = &w->text;
		i = 0;
		j = t->n;
		goto retUTF;

	case Qwindow:
		n = bitwinread(w, rhdr.offset, cnt);
		if(n < 0)
			err = Erwin;
		thdr.data = bitbuf;
		break;

	case Qwinid:
		s = w->wqid;
		goto rstring;

	case Qlabel:
		s = w->label;
		goto rstring;

	rstring:
		if(off > strlen(s))
			n = 0;
		else{
			thdr.data = s+off;
			n = strlen(thdr.data);
			if(n > cnt)
				n = cnt;
		}
		break;

	default:
		n = 0;
		err = Egreg;
		goto send;
	}

    send:
	thdr.count = n;
	sendmsg(err);
}

void
rwrite(Rfile *rf)
{
	int i, n, j, l, wid, err, cnt;
	File *fp;
	Window *w;
	Rune *tmp;
	Text *t;

	err = 0;
	cnt = rhdr.count;
	fp = &file[rf->fileno];
	if(fp->path & CHDIR){
		err = Eisdir;
		goto send;
	}
	if(fp->slot == 0)	/* slave i/o */
		switch(fp->path){
		case Qmouse:
			if(cnt != sizeof(mouse.data))
				error("bad count slave mouse");
			memmove(mouse.data, rhdr.data, sizeof(mouse.data));
			run(pmouse);
			goto send;

		case Qcons:
			if(cnt > 3)
				error("bad count slave kbd");
			/* toss characters if too many uneaten */
			if(kbdcnt+cnt <= NKBDC){
				memmove(kbdc+kbdcnt, rhdr.data, cnt);
				kbdcnt += cnt;
				run(pkbd);
			}
			goto send;

		default:
			error("slave i/o unknown qid");
		}
	if((w = findwindow(&window[fp->slot], fp->path, 0)) == 0){
		err = Eshutdown;
		goto send;
	}
	switch(fp->path&0xFFFF){
	case Qbitblt:
		cnt = bitwrite(w, rhdr.data, cnt, &err);
		break;

	case Qcons:
		termwrite(w, rhdr.tag, rhdr.fid, rhdr.data, cnt);
		return;

	case Qsnarf:
		w = &window[SNARF];
		t = &w->text;
		l = 0;
		/* find offset by counting from beginning */
		for(n=0; n<t->n; n++){
			if(l >= rhdr.offset)
				break;
			wid = runetochar(tbuf, &t->s[n]);
			l += wid;
		}
		if(n == t->n)
			l += w->nwpart;
		if(l != rhdr.offset){
			cnt = 0;
			err = Epartial;
			if(n == t->n)
				err = Eoffend;
			goto send;
		}
		if(rhdr.offset == 0)
			w->nwpart = 0;
		termwrite(w, 0, 0, rhdr.data, cnt);
		tmp = emalloc((cnt+w->nwpart+1)*sizeof(Rune));	/* partial rune plus NUL */
		for(i=0; (j=termwrune(w)) >= 0; i++)
			tmp[i] = j;
		/* a little funny: we delete from offset to end of snarf */
		textdelete(t, n, t->n);
		textinsert(&window[SNARF], t, tmp, i, n, 0);
		free(tmp);
		break;

	case Qlabel:
		if(cnt >= sizeof w->label)
			cnt = sizeof w->label-1;
		memmove(w->label, rhdr.data, cnt);
		w->label[cnt] = '\0';
		break;

	case Qctl:
		if(err = doconsctl(rhdr.data, cnt, w))
			cnt = 0;
		break;

	default:
		cnt = 0;
		err = Egreg;
		goto send;
	}
    send:
	thdr.count = cnt;
	sendmsg(err);
}

void
rclunk(Rfile *rf, int err)
{
	Window *w, *myw;
	File *fp;

	fp = &file[rf->fileno];
	if(rf->busy == 0)
		goto send;
	myw = &window[fp->slot];
	w = findwindow(myw, fp->path, 1);
	if(--myw->ref == 0)
		termdelete(myw);
	if(rf->open && w) switch(fp->path&0xFFFF){
	case Qbitblt:
		w->bitopen = 0;
		bitclose(w);
		break;

	case Qnbmouse:
	case Qmouse:
		w->mouseopen = 0;
		break;

	case Qwindow:
		bitwclose(w);
		break;

	case Qctl:
		if(w->slot>0 && --w->ctlopen==0){
			termunraw(w);
			termhold(w, 0);
		}
		break;
	}
	if(fp->slot == 0){	/* slave shutting down */
		close(cfd);
		cfd = -1;
	}
    send:
	rf->busy = 0;
	sendmsg(err);
}

void
rstat(Rfile *rf)
{
	Dir dir;
	int err;
	File *fp;

	fp = &file[rf->fileno];
	if(window[fp->slot].deleted){
		err = Eshutdown;
		goto send;
	}
	err = 0;
	memmove(dir.name, fp->dir.name, NAMELEN);
	dir.qid.path = fp->path;
	dir.qid.vers = fp->slot;
	dir.mode = fp->dir.perm;
	dir.length = 0;
	dir.hlength = 0;
	memmove(dir.uid, user, NAMELEN);
	memmove(dir.gid, user, NAMELEN);
	dir.atime = dir.mtime = getclock();
	convD2M(&dir, (char*)thdr.stat);
    send:
	sendmsg(err);
}

void
rwstat(Rfile *rf)
{
	USED(rf);
	sendmsg(Eperm);
}

long
getclock(void)
{
	char x[13];

	x[0] = 0;
	seek(clockfd, 0, 0);
	read(clockfd, x, 12);
	x[12] = 0;
	return atoi(x);
}

void
sendmsg(int err)
{
	int n;

	if(err){
		thdr.type = Rerror;
		strncpy(thdr.ename, errs[err], ERRLEN);
	}else{
		thdr.type = rhdr.type+1;
		thdr.fid = rhdr.fid;
	}
	thdr.tag = rhdr.tag;
	n = convS2M(&thdr, tdata);
	if(n < 3)
		fprint(2, "8½ Would have sent %d type %d\n", n, rhdr.type);
	if(write(sfd, tdata, n)!=n)
		error("mount write");
}

void
sendread(int err, int tag, int fid, char *buf, int cnt)
{
	int n;

	if(err){
		thdr.type = Rerror;
		strncpy(thdr.ename, errs[err], ERRLEN);
	}else{
		thdr.type = Tread+1;
		thdr.fid = fid;
		thdr.data = buf;
		thdr.count = cnt;
	}
	thdr.tag = tag;
	n = convS2M(&thdr, tdata);
	if(write(sfd, tdata, n)!=n)
		error("mount write");
}

void
sendwrite(int err, int tag, int fid, int cnt)
{
	int n;

	if(err){
		thdr.type = Rerror;
		strncpy(thdr.ename, errs[err], ERRLEN);
	}else{
		thdr.type = Twrite+1;
		thdr.fid = fid;
		thdr.count = cnt;
	}
	thdr.tag = tag;
	n = convS2M(&thdr, tdata);
	if(write(sfd, tdata, n)!=n)
		error("mount write");
}

Dirtab *
dirlook(File *fp, int nskip)
{
	static Dirtab dend = { 0 };
	static Dirtab dir;
	int i, j;

	if(fp->path==(Qdir|CHDIR)){		/* the main 8½ directory */
		if(nskip>=sizeof(dirtab)/sizeof(Dirtab))
			return &dend;
		return &dirtab[nskip];
	}else if(fp->path==(Qwinds|CHDIR)){	/* the windows subdirectory */
		for(i=1,j=0; i<NSLOT; i++){
			if(!walive(&window[i]))
				continue;
			if(j==nskip){
				dir = (Dirtab){window[i].wqid,
				 (NQid+atoi(window[i].wqid))|CHDIR,CHDIR|0500,0};
				return &dir;
			}
			j++;
		}
	}else{	/* the contents of a windows subdirectory */
		wqid = (fp->path&~CHDIR)-NQid;
		for(i=1; i<NSLOT; i++)
			if(walive(&window[i]) && wqid==atoi(window[i].wqid))
				break;
		if(i>=NSLOT)
			goto end;
		for(i=j=0; dirtab[i].name; i++){
			if(dirtab[i].flag==0)
				continue;
			if(j==nskip){
				dir = (Dirtab){dirtab[i].name,dirtab[i].qid+(wqid<<16),0600, 1};
				return &dir;
			}
			j++;
		}
	}
   end:
	return &dend;
}

int
walive(Window *w)
{
	if(w->busy && !w->deleted)
		return 1;
	return 0;
}

Window *
findwindow(Window *w, ulong wqid, int delOK)
{
	int i;

	if(wqid&CHDIR || wqid<=NQid){
		if(w->deleted && !delOK)
			return 0;
		return w;
	}
	for(i=1; i<NSLOT; i++){
		Window *w1 = &window[i];
		if(w1->busy && atoi(w1->wqid)==(wqid>>16))
			if(delOK || !w1->deleted)
				break;
	}
	if(i >= NSLOT)
		return 0;
	return &window[i];
}

void
wfopen(Rfile *rf, ulong qid, ulong slot, Dirtab *dp)
{
	File *fp, *nfp;
	int i;
	
	for(fp=file,i=0; i<nfile; i++,fp++)
		if(fp->busy && fp->path==qid && fp->slot==slot){
			rf->fileno = i;
			return;
		}
	rf->fileno = newfile();
	nfp = &file[rf->fileno];
	nfp->busy = 1;
	nfp->path = qid;
	nfp->slot = slot;
	nfp->dir = *dp;
}

int
doconsctl(char *bp, int cnt, Window *w)
{
	char buf[16];
	int err;

	err = 0;
	if(cnt>sizeof buf -1)
		cnt = sizeof buf -1;
	memmove(buf, bp, cnt);
	buf[cnt] = 0;
	if(strncmp(buf, "holdon", 6) == 0)
		termhold(w, 1);
	else if(strncmp(buf, "holdoff", 7) == 0)
		termhold(w, 0);
	else if(strncmp(buf, "rawon", 5) == 0)
		termraw(w);
	else if(strncmp(buf, "rawoff", 6) == 0)
		termunraw(w);
	else
		err = Ebadctl;
	run(w->p);
	return err;
}
