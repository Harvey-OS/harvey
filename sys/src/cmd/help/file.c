#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <libg.h>
#include <frame.h>
#include "dat.h"
#include "fns.h"

#define	NFID	5000
#define	NFILE	1000

typedef struct Rfile	Rfile;
typedef struct File	File;
typedef struct Dirtab	Dirtab;

struct Rfile			/* indexed by fid to find file */
{
	int	busy;
	int	open;
	int	eof;
	File	*file;
};

struct File
{
	int	busy;
	ulong	path;		/* == qid.path */
	ulong	slot;		/* == qid.vers */
	Dirtab	*dir;
};

struct Dirtab
{
	char	*name;
	ulong	qid;
	ulong	perm;
};

char	*errname[]=
{
	[Ebadaddr]	"bad address in control message",
	[Ebadctl]	"bad control message",
	[Eclosed]	"page is closed",
	[Eio]		"i/o error",
	[Eisdir]	"file is a directory",
	[Enonexist]	"file does not exist",
	[Enopage]	"no context to create page",
	[Enotdir]	"not a directory",
	[Eperm]		"permission denied",
	[Eslot]		"slot out of range",
	[Egreg]		"it's all greg's fault",
};

Rfile	rfile[NFID];
File	file[NFILE];
char	rdata[MAXMSG+MAXFDATA+1];
char	tdata[MAXMSG+MAXFDATA+1];
Fcall	rhdr;
Fcall	thdr;
int	sfd;		/* server end of pipe */
char	user[NAMELEN];
Client	client[NSLOT];
char	*tmpstr;

void	rflush(void);
void	rattach(Rfile*);
void	rclone(Rfile*, int);
int	rwalk(Rfile*);
void	ropen(Rfile*);
void	rcreate(Rfile*);
void	rread(Rfile*);
void	rwrite(Rfile*);
void	rclunk(Rfile*, int);
void	rstat(Rfile*);
void	rwstat(Rfile*);
void	rerrstr(Rfile*);
void	ruserstr(Rfile*);

void	sendmsg(int);
long	getclock(void);

char *mname[]={
	[Tnop]		"Tnop",
	[Rnop]		"Rnop",
	[Tflush]	"Tflush",
	[Rflush]	"Rflush",
	[Terror]	"Terror",
	[Rerror]	"Rerror",
	[Tsession]	"Tsession",
	[Rsession]	"Rsession",
	[Tattach]	"Tattach",
	[Rattach]	"Rattach",
	[Tclone]	"Tclone",
	[Rclone]	"Rclone",
	[Twalk]		"Twalk",
	[Rwalk]		"Rwalk",
	[Topen]		"Topen",
	[Ropen]		"Ropen",
	[Tcreate]	"Tcreate",
	[Rcreate]	"Rcreate",
	[Tclunk]	"Tclunk",
	[Rclunk]	"Rclunk",
	[Tread]		"Tread",
	[Rread]		"Rread",
	[Twrite]	"Twrite",
	[Rwrite]	"Rwrite",
	[Tremove]	"Tremove",
	[Rremove]	"Rremove",
	[Tstat]		"Tstat",
	[Rstat]		"Rstat",
	[Twstat]	"Twstat",
	[Rwstat]	"Rwstat",
	0, 0, 0
};

/*
 * Layout of qid:
 * 	path bits 0-3	type of file (cons, index, etc.)
 *	path bits 4-31	page id
 *	vers		slot number
 */

enum{
	Qdir,
	Qcons,
	Qindex,
	Qconsctl,
	Qmouse,
	Qnew,
	Qdir1,
	Qctl,
	Qtag,
	Qbody,
	Qtagpos,
	Qtagsel,
	Qbodypos,
	Qbodysel,
	Qbodyapp,
};

#define	NQID		16
#define	QPAGESHIFT	4
#define	QID(qid)	((qid)&(NQID-1))
#define	QPAGE(qid)	(((qid)&~(CHDIR|(NQID-1)))>>QPAGESHIFT)

Dirtab dirtab0[] =
{
	".",		{CHDIR+Qdir},	CHDIR|0500,
	"cons",		{Qcons},	0200,
	"index",	{Qindex},	0400,
	"consctl",	{Qconsctl},	0200,
	"mouse",	{Qmouse},	0200,
	"new",		{CHDIR+Qnew},	CHDIR|0500,
	".",		{CHDIR+Qdir1},	CHDIR|0500,
	0,		0,	0,
};

Dirtab dirtab1[] =
{
	".",		{CHDIR+Qdir1},	CHDIR|0500,
	"ctl",		{Qctl},		0600,
	"tag",		{Qtag},		0400,
	"body",		{Qbody},		0400,
	"tagpos",	{Qtagpos},	0400,
	"tagsel",	{Qtagsel},	0400,
	"bodypos",	{Qbodypos},	0400,
	"bodysel",	{Qbodysel},	0400,
	"bodyapp",	{Qbodyapp},	0200,
	0,		0,		0,
};

void
io(void)
{
	long n;
	Rfile *rf;

	bflush();
	n = read(sfd, rdata, sizeof rdata);
	if(n <= 0)
		error("mount read");
	if(convM2S(rdata, &rhdr, n) == 0)
		error("format error");
	if(rhdr.fid<0 || NFID<=rhdr.fid)
		error("fid out of range");
	/*BUG: check busy */
	rf = &rfile[rhdr.fid];
/*	fprint(2, "msg: %d %s on %d\n", rhdr.type,
		mname[rhdr.type]? mname[rhdr.type] : "mystery", rhdr.fid); /**/
	switch(rhdr.type){
	default:
		error("type");
		break;
	case Tflush:
		rflush();
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

	for(i=0; i<NFILE; i++)
		if(file[i].busy == 0)
			return i;
	error("no free files");
	return -1;
}

void
rflush(void)		/* program is synchronous so flush is easy */
{
	sendmsg(0);
}

void
rattach(Rfile *rf)
{
	int f, s, err;

	err = 0;
	if(strcmp(rhdr.aname, "slave") == 0){
		f = 0;
		file[0].slot = 0;
	}else{
		s = atoi(rhdr.aname);
		if(s<=0 || s>=NSLOT){
			print("slot %d out of range\n", s);
			err = Eslot;
			goto send;
		}
		f = newfile();
		file[f].slot = s;
	}
	rf->busy = 1;
	rf->open = 0;
	rf->eof = 0;
	file[f].busy = 1;
	file[f].path = CHDIR;
	file[f].dir = &dirtab0[0];
	rf->file = &file[f];
	thdr.qid.path = rf->file->path;
	thdr.qid.vers = rf->file->slot;
	strcpy(user, rhdr.uname);
	client[rf->file->slot].ref++;
    send:
	sendmsg(err);
}

void
rclone(Rfile *rf, int dowalk)
{
	Rfile *nrf;
	int err=0;

	nrf = &rfile[rhdr.newfid];
	if(rhdr.newfid<0 || NFID<=rhdr.newfid)
		error("clone nfid out of range");
	if(nrf->busy)
		error("clone to used channel");
	nrf->busy = 1;
	nrf->open = 0;
	nrf->eof = 0;
	nrf->file = rf->file;
	client[nrf->file->slot].ref++;
	if(dowalk){
		rhdr.fid = rhdr.newfid;
		if(rwalk(nrf)){
			client[nrf->file->slot].ref--;
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
	int err, slot;
	long dqid, qid, pno;
	char *name;
	File *fp;
	Dirtab *tab;
	Page *p;
	int i, j, q;

	err = 0;
	qid = 0;
	slot = rf->file->slot;
	dqid = rf->file->path;
	pno = QPAGE(dqid)<<QPAGESHIFT;
	name = rhdr.name;
	if((dqid&CHDIR) == 0){
		err = Enotdir;
		goto send;
	}
	if(QID(dqid) == Qdir1){
		tab = dirtab1;
		goto Tablookup;
	}
	/* first, is it a directory-by-number? */
	q = atoi(name);
	if(q > 0){
		if(pagelookup(q)){
			qid = CHDIR|(q<<QPAGESHIFT)|Qdir1;
			tab = dirtab0;
			j = Qdir1;	/* distasteful! */
			goto Found;
		}
		err = Enonexist;
		goto send;
	}

	/* no: table lookup */
	tab = dirtab0;

    Tablookup:
	for(j=0; tab[j].name; j++){
		if(strcmp(name, tab[j].name) == 0){
			if(tab[j].qid == CHDIR+Qnew){
				/* make new page */
				if(curt)
					p = curt->page;
				else{
					p = page;
					if(p == 0){
						err = Enopage;
						goto send;
					}
				}
				if(p->parent == 0)
					while(p->down)
						p = p->down;
				q = pageadd(p->parent, 1)->id;
				qid = CHDIR|(q<<QPAGESHIFT)|Qdir1;
				tab = dirtab0;
				j = Qdir1;	/* distasteful! */
			}else
				qid = pno|tab[j].qid;
    Found:
			for(i=0,fp=file; i<NFILE; i++,fp++)
				if(fp->busy && fp->path==qid && fp->slot==rf->file->slot){
					rf->file = fp;
					goto send;
				}
			i = newfile();
			fp = &file[i];
			fp->busy = 1;
			fp->slot = rf->file->slot;
			fp->path = qid;
			fp->dir = &tab[j];
			rf->file = fp;
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

	err = 0;
	fp = rf->file;
	perm = fp->dir->perm;
	mode = rhdr.mode;
	if(fp->path & CHDIR){
		if(mode){
			err = Eperm;
			goto send;
		}
	}else{
		trunc = mode & OTRUNC;
		mode &= ~OTRUNC;
		if(mode<0 || mode>OEXEC){
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

	if(fp->slot > 0) switch(QID(fp->path)){
	case Qdir:
	case Qdir1:
	case Qcons:
	case Qindex:
		break;

	case Qconsctl:
	case Qmouse:
		err = Egreg;
		goto send;

	case Qctl:
	case Qtag:
	case Qbody:
	case Qtagpos:
	case Qtagsel:
	case Qbodypos:
	case Qbodysel:
	case Qbodyapp:
		break;

	default:
		err = Enonexist;
		goto send;
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

#define	NUMLEN	12

void
rread(Rfile *rf)
{
	int j, n, err, cnt;
	long off, clock, slot, qid, cn;
	Rune *r;
	ulong c0, c1, q0;
	Dir dir;
	File *fp;
	Dirtab *tab;
	Dirtab *dp;
	Client *c;
	String *s;
	Text *t;
	Page *p;
	static char buf[NQID*DIRLEN];

	n = 0;
	err = 0;
	off = rhdr.offset;
	cnt = rhdr.count;
	if(off < 0){
		err = Eio;
		goto send;
	}
	fp = rf->file;
	slot = fp->slot;
	c = &client[slot];
	if(fp->path & CHDIR){
		if(off%DIRLEN || cnt%DIRLEN){
			err = Eio;
			goto send;
		}
		clock = getclock();
		if(QID(fp->path) == Qdir1)
			tab = dirtab1;
		else{
			tab = dirtab0;
			if(off/DIRLEN+1 >= Qdir1){
				off -= Qdir-1;
				qid = CHDIR|Qdir1;
				goto Numeric;
			}
		}
		for(dp=&tab[off/DIRLEN+1],qid=dp->qid; dp->name && cnt>0; dp++){
			off = 0;	/* residual */
			qid = dp->qid;
			if(tab==dirtab0 && dp->qid==(CHDIR|Qdir1))
				break;
			memmove(dir.name, dp->name, NAMELEN);
			dir.qid.path = (QPAGE(fp->path)<<QPAGESHIFT)|qid;
			dir.qid.vers = slot;
			dir.mode = dp->perm;
			dir.length = 0;
			dir.hlength = 0;
			strcpy(dir.uid, user);
			strcpy(dir.gid, user);
			dir.atime = clock;
			dir.mtime = clock;
			convD2M(&dir, buf+n);
			n += DIRLEN;
			cnt -= DIRLEN;
		}
		/* do the numeric dirs */
		if(tab==dirtab0 && cnt>0){
	Numeric:
			for(j=1; j<pageid && cnt>0; j++){
				if(!pagelookup(j))
					continue;
				if(off > 0){
					/* count down */
					off -= DIRLEN;
					continue;
				}
				sprint(dir.name, "%d", j);
				dir.qid.path = (QPAGE(fp->path)<<QPAGESHIFT)|qid;
				dir.qid.vers = slot;
				dir.mode = dirtab0[Qdir1].perm;
				dir.length = 0;
				dir.hlength = 0;
				strcpy(dir.uid, user);
				strcpy(dir.gid, user);
				dir.atime = clock;
				dir.mtime = clock;
				convD2M(&dir, buf+n);
				n += DIRLEN;
				cnt -= DIRLEN;
			}
		}
		thdr.data = buf;
		goto send;
	}
	switch(QID(fp->path)){
	case Qindex:
		s = c->index;

	caseString:
		if(rf->eof)
			goto send;
		if(tmpstr)
			free(tmpstr);
		/* this is badly done */
		tmpstr = cstr(s->s, s->n);
		cn = strlen(tmpstr);
		if(off > cn){
			err = Eio;
			goto send;
		}
		n = cnt;
		if(off+n > cn)
			n = cn - off;
		if(n == 0)
			goto send;
		/* protect against cat'ing into itself */
		if(off+n == cn)
			rf->eof = 1;
		thdr.data = tmpstr+off;
		goto send;

	case Qtag:
	case Qbody:
		p = pagelookup(QPAGE(fp->path));
		if(p == 0){
	Closed:
			err = Eclosed;
			goto send;
		}
		if(QID(fp->path) == Qtag)
			s = &p->tag;
		else
			s = &p->body;
		goto caseString;

	case Qctl:
		p = pagelookup(QPAGE(fp->path));
		if(p == 0)
			goto Closed;
		j = sprint(buf, "%d", p->id);
		if(off > j){
			err = Eio;
			goto send;
		}
		n = cnt;
		if(off+n > j)
			n = j - off;
		if(n == 0)
			goto send;
		thdr.data = buf+off;
		goto send;

	case Qtagpos:
	case Qbodypos:
		p = pagelookup(QPAGE(fp->path));
		if(p == 0)
			goto Closed;
		if(off > 2*NUMLEN){
			err = Eio;
			goto send;
		}
		n = cnt;
		if(off+n > 2*NUMLEN)
			n = 2*NUMLEN - off;
		if(n == 0)
			goto send;
		if(QID(fp->path) == Qtagpos)
			t = &p->tag;
		else
			t = &p->body;
		/* convert from runes to chars in offset */
		c0 = 0;
		r = t->s;
		for(q0=0; q0<t->q0; q0++)
			c0 += runelen(*r++);
		c1 = c0;
		for(; q0<t->q1; q0++)
			c1 += runelen(*r++);
		sprint(buf, "%-11d %-11d ", c0, c1);
		thdr.data = buf+off;
		goto send;

	case Qtagsel:
	case Qbodysel:
		p = pagelookup(QPAGE(fp->path));
		if(p == 0)
			goto Closed;
		if(QID(fp->path) == Qtagsel)
			t = &p->tag;
		else
			t = &p->body;
		if(tmpstr)
			free(tmpstr);
		/* this is badly done */
		tmpstr = cstr(t->s+t->q0, t->q1-t->q0);
		cn = strlen(tmpstr);
		if(off > cn){
			err = Eio;
			goto send;
		}
		n = cnt;
		if(off+n > cn)
			n = cn - off;
		if(n == 0)
			goto send;
		thdr.data = tmpstr+off;
		goto send;

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
	int err, cnt;
	File *fp;
	Page *p;
	Rune *r;
	Text *t;
	char *d;
	Addr a;

	err = 0;
	cnt = rhdr.count;
	rhdr.data[cnt] = 0;
	fp = rf->file;
	if(fp->path & CHDIR){
		err = Eisdir;
		goto send;
	}
	if(fp->slot == 0)	/* slave i/o */
		switch(QID(fp->path)){
		case Qmouse:
			if(cnt != sizeof(mouse.data))
				error("bad count slave mouse");
			memmove(mouse.data, rhdr.data, sizeof(mouse.data));
			run(pmouse);
			goto send;

		case Qcons:
			if(cnt != 1)
				error("bad count slave kbd");
			kbdc = ((uchar*)rhdr.data)[0];
			run(pkbd);
			goto send;

		default:
			error("slave i/o unknown qid");
		}

	switch(QID(fp->path)){
	case Qcons:
		xtrnwrite(&client[fp->slot], rhdr.fid, (uchar*)rhdr.data, cnt);
		break;

	case Qbodyapp:
		if(cnt <= 0){
			cnt = 0;
			goto send;
		}
		p = pagelookup(QPAGE(fp->path));
		if(p == 0){
			cnt = 0;
			err = Ebadctl;
			goto send;
		}
		t = &p->body;
		r = tmprstr(rhdr.data);
		textinsert(t, r, t->n, 0, 1);
		free(r);
		break;

	case Qctl:
		if(cnt <= 0){
			cnt = 0;
			goto send;
		}
		p = pagelookup(QPAGE(fp->path));
		if(p == 0){
			cnt = 0;
			err = Ebadctl;
			goto send;
		}
		switch(rhdr.data[0]){
		default:
			cnt = 0;
			err = Ebadctl;
			goto send;

		case 'a':
			t = &p->tag;
		caseaA:
			d = strchr(rhdr.data+1, '\n');
			if(d==0){
				cnt = 0;
				err = Ebadaddr;
				goto send;
			}
			r = tmprstr(d+1);
			textinsert(t, r, t->n, 0, 1);
			free(r);
			break;

		case 'A':
			t = &p->body;
			goto caseaA;

		case 'd':
			t = &p->tag;
		casedD:
			r = tmprstr(rhdr.data+1);
			a = address(r, t);
			free(r);
			d = strchr(rhdr.data+1, '\n');
			if(a.valid==0 || d==0){
				cnt = 0;
				err = Ebadaddr;
				goto send;
			}
			t->q0 = a.q0;
			t->q1 = a.q1;
			cut(t, 0);
			break;

		case 'D':
			t = &p->body;
			goto casedD;

		case 'i':
			t = &p->tag;
		caseiI:
			r = tmprstr(rhdr.data+1);
			a = address(r, t);
			free(r);
			d = strchr(rhdr.data+1, '\n');
			if(a.valid==0 || d==0){
				cnt = 0;
				err = Ebadaddr;
				goto send;
			}
			r = tmprstr(d+1);
			textinsert(t, r, a.q0, 0, 1);
			free(r);
			break;

		case 'I':
			t = &p->body;
			goto caseiI;

		case 'u':
			p->mod = 0;
			unputtag(p, "Put!");
			break;

		}
		break;
	default:
		error("xtrn i/o unknown qid");
	}

    send:
	thdr.count = cnt;
	sendmsg(err);
}

void
rclunk(Rfile *rf, int err)
{
	Client *c;
	File *fp;

	fp = rf->file;
	if(rf->busy == 0)
		goto send;
	c = &client[fp->slot];
	if(--c->ref == 0){
		if(fp->slot == 0)
			error("slave died");
		xtrndelete(c);
	}
	if(rf->open) switch(QID(rf->file->path)){
	default:
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

	err = 0;
	fp = rf->file;
	if(QID(fp->path) == Qdir1)
		sprint(dir.name, "%d", QPAGE(fp->path));
	else
		memmove(dir.name, fp->dir->name, NAMELEN);
	dir.qid.path = fp->path;
	dir.qid.vers = fp->slot;
	dir.mode = fp->dir->perm;
	dir.length = 0;
	dir.hlength = 0;
	strcpy(dir.uid, user);
	strcpy(dir.gid, user);
	dir.atime = dir.mtime = getclock();
	convD2M(&dir, thdr.stat);
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
		strncpy(thdr.ename, errname[err], ERRLEN);
	}else{
		thdr.type = rhdr.type+1;
		thdr.fid = rhdr.fid;
	}
	thdr.tag = rhdr.tag;
	n = convS2M(&thdr, tdata);
	if(write(sfd, tdata, n)!=n)
		error("mount write");
}
