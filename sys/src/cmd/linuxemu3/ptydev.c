#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

typedef struct Termios Termios;
typedef struct Winsize Winsize;
typedef struct Cbuf Cbuf;
typedef struct Tty Tty;
typedef struct Pty Pty;
typedef struct Ptyfile Ptyfile;

/* cflags */
enum {
	IGNBRK	= 01,
	BRKINT	= 02,
	IGNPAR	= 04,
	PARMRK	= 010,
	INPCK	= 020,
	ISTRIP	= 040,
	INLCR	= 0100,
	IGNCR	= 0200,
	ICRNL	= 0400,
	IUCLC	= 01000,
	IXON	= 02000,
	IXANY	= 04000,
	IXOFF	= 010000,
	IMAXBEL	= 020000,
	IUTF8	= 040000,
};

/* oflags */
enum {
	OPOST	= 01,
	OLCUC	= 02,
	ONLCR	= 04,
	OCRNL	= 010,
	ONOCR	= 020,
	ONLRET	= 040,
	OFILL	= 0100,
	OFDEL	= 0200,
	NLDLY	= 0400,
	NL0		= 0,
	NL1		= 0400,
	CRDLY	= 03000,
	CR0		= 0,
	CR1		= 01000,
	CR2		= 02000,
	CR3		= 03000,
	TABDLY	= 014000,
	TAB0	= 0,
	TAB1	= 04000,
	TAB2	= 010000,
	TAB3	= 014000,
	XTABS	= 014000,
	BSDLY	= 020000,
	BS0		= 0,
	BS1		= 020000,
	VTDLY	= 040000,
	VT0		= 0,
	VT1		= 040000,
	FFDLY	= 0100000,
	FF0		= 0,
	FF1		= 0100000,
};

/* cflags */
enum {
	CSIZE	= 060,
	CS5		= 0,
	CS6		= 020,
	CS7		= 040,
	CS8		= 060,
	CREAD	= 0200,
	CLOCAL	= 04000,
	HUPCL	= 02000,
};

/* lflags */
enum {
	ISIG		= 01,
	ICANON	= 02,
	XCASE	= 04,
	ECHO	= 010,
	ECHOE	= 020,
	ECHOK	= 040,
	ECHONL	= 0100,
	NOFLSH	= 0200,
	TOSTOP	= 0400,
	ECHOCTL	= 01000,
	ECHOPRT	= 02000,
	ECHOKE	= 04000,
	FLUSH0	= 010000,
	PENDIN	= 040000,
	IEXTEN	= 0100000,
};

/* cc */
enum {
	VINTR	= 0,
	VQUIT,
	VERASE,
	VKILL,
	VEOF,
	VTIME,
	VMIN,
	VSWTCH,
	VSTART,
	VSTOP,
	VSUSP,
	VEOL,
	VREPRINT,
	VDISCARD,
	VERASEW,
	VLNEXT,
	VEOL2,
	NCCS,
};

struct Termios
{
	int		iflag;		/* input modes */
	int		oflag;		/* output modes */
	int		cflag;		/* control modes */
	int		lflag;		/* local modes */
	uchar	cline;
	uchar	cc[NCCS];	/* control characters */
};

struct Winsize
{
	ushort	row;
	ushort	col;
	ushort	px;
	ushort	py;
};

struct Cbuf
{
	int	rp;
	int	wp;
	char	cb[256];
};

struct Tty
{
	Termios	t;
	Winsize	winsize;

	int		escaped;
	int		eol;

	int		pgid;

	Cbuf		wb;
	Cbuf		rb;
};

struct Pty
{
	Tty;

	int	id;
	int	closed;
	int	locked;

	struct {
		Uwaitq r;
		Uwaitq w;
	}	q[2];

	Ref;
	QLock;
};

struct Ptyfile
{
	Ufile;

	Pty	*pty;

	int	master;
};

static Pty *ptys[64];

int cbput(Cbuf *b, char c)
{
	int x;
	x = b->wp+1&(sizeof(b->cb)-1);
	if(x == b->rp)
		return -1;
	b->cb[b->wp] = c;
	b->wp = x;
	return 0;
}

int cbget(Cbuf *b)
{
	char c;
	if(b->rp == b->wp)
		return -1;
	c = b->cb[b->rp];
	b->rp = (b->rp + 1) & (sizeof(b->cb)-1);
	return c;
}

int cbfill(Cbuf *b)
{
	return (b->wp - b->rp) & (sizeof(b->cb)-1);
}

void ttyinit(Tty *t)
{
	memset(&t->t, 0, sizeof(t->t));

	t->t.iflag = ICRNL;
	t->t.oflag = OPOST|ONLCR|NL0|CR0|TAB0|BS0|VT0|FF0;
	t->t.lflag = ICANON|IEXTEN|ECHO|ECHOE|ECHOK;

	if(current)
		t->pgid = current->pgid;
}

int ttywrite(Tty *t, char *buf, int len)
{
	char *p, *e;

	for(p=buf, e=buf+len; p<e; p++){
		char c;

		c = *p;
		if((t->t.oflag & OPOST) == 0) {
			if(cbput(&t->wb, c) < 0)
				break;
			continue;
		}
		switch(c) {
		case '\n':
			if(t->t.oflag & ONLCR) {
				if(cbput(&t->wb, '\r') < 0)
					goto done;
			}
			if(cbput(&t->wb, c) < 0)
				goto done;
			break;
			
		case '\t':
			if((t->t.oflag & TAB3) == TAB3) {
				int tab;

				tab = 8;
				while(tab--)
					cbput(&t->wb, ' ');
				break;
			}
			/* Fall Through */
		default:
			if(t->t.oflag & OLCUC)
				if(c >= 'a' && c <= 'z')
					c = 'A' + (c-'a');
			if(cbput(&t->wb, c) < 0)
				goto done;
		}
	}
done:
	return p-buf;
}

int ttycanread(Tty *t, int *n)
{
	int x;

	x = cbfill(&t->rb);
	if(t->t.lflag & ICANON){
		if(t->eol == 0)
			return 0;
	} else {
		if(x == 0)
			return 0;
	}
	if(n != nil)
		*n = x;
	return 1;
}

int ttyread(Tty *t, char *buf, int len)
{
	char *p, *e;

	if((t->t.lflag & ICANON) && t->eol == 0)
		return 0;

	for(p=buf, e=buf+len; p<e; p++){
		int c;

		if((c = cbget(&t->rb)) < 0)
			break;

		if(c==0 || c=='\n'){
			t->eol--;
			if(t->t.lflag & ICANON){
				if(c == 0)
					break;
				*p++ = c;
				break;
			}
		}

		*p = c;
	}
	return p-buf;
}


static void
echo(Tty *t, char c)
{
	if(t->t.lflag & ECHO) {
		switch(c) {
		case '\r':
			if(t->t.oflag & OCRNL) {
				cbput(&t->wb, '\n');
				break;
			}
			cbput(&t->wb, c);
			break;
		case '\n':
			if(t->t.oflag & ONLCR)
				cbput(&t->wb, '\r');
			cbput(&t->wb, '\n');
			break;
		case '\t':
			if((t->t.oflag & TAB3) == TAB3) {
				int tab;

				tab = 8;
				while(tab--)
					cbput(&t->wb, ' ');
				break;
			}
			/* Fall Through */
		default:
			cbput(&t->wb, c);
			break;
		}
	}
	else
	if(c == '\n' && (t->t.lflag&(ECHONL|ICANON)) == (ECHONL|ICANON)) {
		if(t->t.oflag & ONLCR)
			cbput(&t->wb, '\r');
		cbput(&t->wb, '\n');
	}
}

static int
bs(Tty *t)
{
	char c;
	int x;

	if(cbfill(&t->rb) == 0)
		return 0;
	x = (t->rb.wp-1)&(sizeof(t->rb.cb)-1);
	c = t->rb.cb[x];
	if(c == 0 || c == '\n')
		return 0;
	t->rb.wp = x;
	echo(t, '\b');
	if(t->t.lflag & ECHOE) {
		echo(t, ' ');
		echo(t, '\b');
	}
	return 1;
}

int ttywriteinput(Tty *t, char *buf, int len)
{
	char *p, *e;

	for(p=buf, e=buf+len; p<e; p++){
		char c;

		c = *p;

		if(t->t.iflag & ISTRIP)
			c &= 0177;

		if((t->t.iflag & IXON) && c == t->t.cc[VSTOP]) {
			p++;
			break;
		}

		switch(c) {
		case '\r':
			if(t->t.iflag & IGNCR)
				continue;
			if(t->t.iflag & ICRNL)
				c = '\n';
			break;
		case '\n':
			if(t->t.iflag&INLCR)
				c = '\r';
			break;
		}

		if(t->t.lflag & ISIG) {
			if(c == t->t.cc[VINTR]){
				if(t->pgid > 0)
					sys_kill(-t->pgid, SIGINT);
				continue;
			}
			if(c == t->t.cc[VQUIT]){
				if(t->pgid > 0)
					sys_kill(-t->pgid, SIGQUIT);
				continue;
			}
		}

		if((t->t.lflag & ICANON) && t->escaped == 0) {
			if(c == t->t.cc[VERASE]) {
				bs(t);
				continue;
			}
			if(c == t->t.cc[VKILL]) {
				while(bs(t))
					;
				if(t->t.lflag & ECHOK)
					echo(t, '\n');
				continue;
			}
		}

		if(t->escaped == 0 && (c == t->t.cc[VEOF] || c == '\n'))
			t->eol++;

		if((t->t.lflag & ICANON) == 0) {
			echo(t, c);
			cbput(&t->rb, c);
			continue;
		}

		if(t->escaped) 
			echo(t, '\b');

		if(c != t->t.cc[VEOF])
			echo(t, c);

		if(c != '\\') {
			if(c == t->t.cc[VEOF])
				c = 0;
			cbput(&t->rb, c);
			t->escaped = 0;
			continue;
		}
		if(t->escaped) {
			cbput(&t->rb, '\\');
			t->escaped = 0;
		}
		else
			t->escaped = 1;
	}

	return p-buf;
}

int ttycanreadoutput(Tty *t, int *n)
{
	int x;

	x = cbfill(&t->wb);
	if(n != nil)
		*n = x;
	return x > 0 ? 1 : 0;
}

int ttyreadoutput(Tty *t, char *buf, int len)
{
	char *p, *e;

	for(p=buf, e=buf+len; p<e; p++){
		int c;

		if((c = cbget(&t->wb)) < 0)
			break;
		*p = c;
	}
	return p-buf;
}

static int
pollpty(Ufile *f, void *tab)
{
	Ptyfile *p = (Ptyfile*)f;
	int err;
	int n;

	if(p->pty == nil)
		return 0;

	qlock(p->pty);
	if(p->master){
		pollwait(p, &p->pty->q[1].r, tab);
		n = ttycanreadoutput(p->pty, nil);
	} else {
		pollwait(p, &p->pty->q[0].r, tab);
		n = ttycanread(p->pty, nil);
	}
	err = POLLOUT;
	if(n){
		err |= POLLIN;
	} else if(p->master==0 && p->pty->closed){
		err |= (POLLIN | POLLHUP);
	}
	qunlock(p->pty);

	return err;
}

static int
readpty(Ufile *f, void *data, int len, vlong)
{
	int err;
	Ptyfile *p = (Ptyfile*)f;

	if(p->pty == nil)
		return -EPERM;
	qlock(p->pty);
	for(;;){
		if(p->master){
			err = ttycanreadoutput(p->pty, nil);
		} else {
			err = ttycanread(p->pty, nil);
		}
		if(err > 0){
			if(p->master){
				err = ttyreadoutput(p->pty, (char*)data, len);
			}else{
				err = ttyread(p->pty, (char*)data, len);
			}
		} else {
			if(p->master == 0 && p->pty->closed){
				err = -EIO;
			} else if(p->mode & O_NONBLOCK){	
				err = -EAGAIN;
			} else {
				if((err = sleepq(&p->pty->q[p->master].r, p->pty, 1)) == 0)
					continue;
			}
		}
		wakeq(&p->pty->q[!p->master].w, MAXPROC);
		break;
	}
	qunlock(p->pty);

	return err;
}

static int
writepty(Ufile *f, void *data, int len, vlong)
{
	Ptyfile *p = (Ptyfile*)f;
	int err;

	if(p->pty == nil)
		return -EPERM;
	if(len == 0)
		return len;

	qlock(p->pty);
	for(;;){
		if(p->pty->closed){
			err = -EIO;
			break;
		}
		if(p->master){
			err = ttywriteinput(p->pty, (char*)data, len);
		} else{
			err = ttywrite(p->pty, (char*)data, len);
		}
		if(err == 0){
			if((err = sleepq(&p->pty->q[p->master].w, p->pty, 1)) == 0)
				continue;
		} else {
			if(ttycanread(p->pty, nil))
				wakeq(&p->pty->q[0].r, MAXPROC);
			if(ttycanreadoutput(p->pty, nil))
				wakeq(&p->pty->q[1].r, MAXPROC);
		}
		break;
	}
	qunlock(p->pty);

	return err;
}

static int
closepty(Ufile *f)
{
	Ptyfile *p = (Ptyfile*)f;

	if(p->pty == nil)
		return 0;

	qlock(p->pty);
	if(p->master)
		p->pty->closed = 1;
	if(!decref(p->pty)){
		ptys[p->pty->id] = nil;
		qunlock(p->pty);
		free(p->pty);
	} else {
		wakeq(&p->pty->q[0].r, MAXPROC);
		wakeq(&p->pty->q[0].w, MAXPROC);
		wakeq(&p->pty->q[1].r, MAXPROC);
		wakeq(&p->pty->q[1].w, MAXPROC);
		qunlock(p->pty);
	}
	return 0;
}

static int
changetty(Ptyfile *tty)
{
	Ufile *old;

	if(old = gettty()){
		putfile(old);
		return (old == tty) ? 0 : -EPERM;
	}
	tty->pty->pgid = current->pgid;
	settty(tty);
	return 0;
}

static int
ioctlpty(Ufile *f, int cmd, void *arg)
{
	Ptyfile *p = (Ptyfile*)f;
	int err, pid;

	if(p->pty == nil)
		return -ENOTTY;

	trace("ioctlpty(%s, %lux, %p)", p->path, (ulong)cmd, arg);

	err = 0;
	qlock(p->pty);
	switch(cmd){
	default:
		trace("ioctlpty: unknown: 0x%x", cmd);
		err = -ENOTTY;
		break;

	case 0x5401:	/* TCGETS */
		memmove(arg, &p->pty->t, sizeof(Termios));
		break;

	case 0x5402:	/* TCSETS */
	case 0x5403:	/* TCSETSW */
	case 0x5404:	/* TCSETSF */
		memmove(&p->pty->t, arg, sizeof(Termios));
		break;

	case 0x5422:	// TIOCNOTTY
		if((f = gettty()) && (f != p)){
			putfile(f);
			err = -ENOTTY;
			break;
		}
		settty(nil);
		break;

	case 0x540E:	// TIOCSCTTY
		err = changetty(p);
		break;

	case 0x540F:	// TIOCGPGRP
		*(int*)arg = p->pty->pgid;
		break;

	case 0x5410:	// TIOCSPGRP
		p->pty->pgid = *(int*)arg;
		break;

	case 0x5413:	// TIOCGWINSZ
		memmove(arg, &p->pty->winsize, sizeof(Winsize));
		break;

	case 0x5414:	// TIOCSWINSZ
		if(memcmp(&p->pty->winsize, arg, sizeof(Winsize)) == 0)
			break;
		memmove(&p->pty->winsize, arg, sizeof(Winsize));
		if((pid = p->pty->pgid) > 0){
			qunlock(p->pty);

			sys_kill(-pid, SIGWINCH);
			return 0;
		}
		break;
	case 0x40045431:	// TIOCSPTLCK
		if(p->master)
			p->pty->locked = *(int*)arg;
		break;

	case 0x80045430:
		*(int*)arg = p->pty->id;
		break;

	case 0x541B:
		if(arg == nil)
			break;
		if(p->master){
			ttycanreadoutput(p->pty, &err);
		} else {
			ttycanread(p->pty, &err);
		}
		if(err < 0){
			*((int*)arg) = 0;
			break;
		}
		*((int*)arg) = err;
		err = 0;
		break;		
	}
	qunlock(p->pty);

	return err;
}

static int
openpty(char *path, int mode, int perm, Ufile **pf)
{
	Pty *pty;
	Ptyfile *p;
	int id;
	int master;

	USED(perm);

	if(strcmp("/dev/tty", path)==0){
		if(*pf = gettty())
			return 0;
		return -ENOTTY;
	} else if(strcmp("/dev/pts", path)==0){
		pty = nil;
		master = -1;
	} else if(strcmp("/dev/ptmx", path)==0){
		master = 1;
		for(id=0; id<nelem(ptys); id++){
			if(ptys[id] == nil)
				break;
		}
		if(id == nelem(ptys))
			return -EBUSY;

		pty = kmallocz(sizeof(*pty), 1);
		pty->ref = 1;

		ttyinit(pty);

		ptys[pty->id = id] = pty;
	} else {
		master = 0;
		if(strncmp("/dev/pts/", path, 9) != 0)
			return -ENOENT;
		id = atoi(path + 9);
		if(id < 0 || id >= nelem(ptys))
			return -ENOENT;
		if((pty = ptys[id]) == nil)
			return -ENOENT;

		qlock(pty);
		if(pty->closed || pty->locked){
			qunlock(pty);
			return -EIO;
		}
		incref(pty);
		qunlock(pty);
	}

	p = kmallocz(sizeof(*p), 1);
	p->dev = PTYDEV;
	p->ref = 1;
	p->fd = -1;
	p->mode = mode;
	p->path = kstrdup(path);
	p->pty = pty;
	p->master = master;

	if(!master && !(mode & O_NOCTTY))
		changetty(p);

	*pf = p;

	return 0;
}

static int
readdirpty(Ufile *f, Udirent **pd)
{
	Ptyfile *p = (Ptyfile*)f;
	int i, n;

	*pd = nil;
	if(p->pty != nil)
		return -EPERM;
	n = 0;	
	for(i=0; i<nelem(ptys); i++){
		char buf[12];

		if(ptys[i] == nil)
			continue;
		snprint(buf, sizeof(buf), "%d", i);
		if((*pd = newdirent(f->path, buf, S_IFCHR | 0666)) == nil)
			break;
		pd = &((*pd)->next);
		n++;
	}
	return n;
}

static int
fstatpty(Ufile *f, Ustat *s)
{
	Ptyfile *p = (Ptyfile*)f;

	if(p->pty != nil){
		s->mode = 0666 | S_IFCHR;
		if(p->master){
			s->rdev = 5<<8 | 2;
		} else {
			s->rdev = 3<<8;
		}
	} else {
		s->mode = 0777 | S_IFDIR;
		s->rdev = 0;
	}
	s->ino = hashpath(p->path);
	s->dev = 0;
	s->uid = current->uid;
	s->gid = current->gid;
	s->size = 0;
	s->atime = s->mtime = s->ctime = boottime/1000000000LL;
	return 0;
};

static int
statpty(char *path, int, Ustat *s)
{
	if(strcmp("/dev/tty", path)==0){
		s->mode = 0666 | S_IFCHR;
	} else if(strcmp("/dev/ptmx", path)==0){
		s->mode = 0666 | S_IFCHR;
		s->rdev = 5<<8 | 2;
	} else if(strcmp("/dev/pts", path)==0){
		s->mode = 0777 | S_IFDIR;
	} else if(strncmp("/dev/pts/", path, 9)==0){
		int id;

		id = atoi(path + 9);
		if(id < 0 || id >= nelem(ptys))
			return -ENOENT;
		if(ptys[id] == nil)
			return -ENOENT;

		s->mode = 0666 | S_IFCHR;
		s->rdev = 3<<8;
	} else {
		return -ENOENT;
	}
	s->ino = hashpath(path);
	s->uid = current->uid;
	s->gid = current->gid;
	s->size = 0;
	s->atime = s->mtime = s->ctime = boottime/1000000000LL;
	return 0;
}

static int
chmodpty(char *path, int mode)
{
	USED(path);
	USED(mode);

	return 0;
}

static int
chownpty(char *path, int uid, int gid, int link)
{
	USED(path);
	USED(uid);
	USED(gid);
	USED(link);

	return 0;
}

static int
fchmodpty(Ufile *f, int mode)
{
	USED(f);
	USED(mode);

	return 0;
}

static int
fchownpty(Ufile *f, int uid, int gid)
{
	USED(f);
	USED(uid);
	USED(gid);

	return 0;
}

static Udev ptydev = 
{
	.open = openpty,
	.read = readpty,
	.write = writepty,
	.poll = pollpty,
	.close = closepty,
	.readdir = readdirpty,
	.ioctl = ioctlpty,
	.fstat = fstatpty,
	.stat = statpty,
	.fchmod = fchmodpty,
	.fchown = fchownpty,
	.chmod = chmodpty,
	.chown = chownpty,
};

void ptydevinit(void)
{
	devtab[PTYDEV] = &ptydev;
	fsmount(&ptydev, "/dev/pts");
	fsmount(&ptydev, "/dev/ptmx");
	fsmount(&ptydev, "/dev/tty");
}
