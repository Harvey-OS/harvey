#include	<u.h>
#include	<libc.h>
#include	"compat.h"
#include	"kbd.h"
#include	"error.h"

typedef struct Queue	Queue;
struct Queue
{
	QLock	qwait;
	Rendez	rwait;

	Lock	lock;
	int	notempty;
	char	buf[1024];
	char	*w;
	char	*r;
	char	*e;
};

Queue*	kbdq;			/* unprocessed console input */
Queue*	lineq;			/* processed console input */
Snarf	snarf = {
	.vers =	1
};

static struct
{
	QLock;
	int	raw;		/* true if we shouldn't process input */
	int	ctl;		/* number of opens to the control file */
	int	x;		/* index into line */
	char	line[1024];	/* current input line */
} kbd;

/*
 * cheapo fixed-length queues
 */
static void
qwrite(Queue *q, void *v, int n)
{
	char *buf, *next;
	int i;

	buf = v;
	lock(&q->lock);
	for(i = 0; i < n; i++){
		next = q->w+1;
		if(next >= q->e)
			next = q->buf;
		if(next == q->r)
			break;
		*q->w = buf[i];
		q->w = next;
	}
	q->notempty = 1;
	unlock(&q->lock);
	rendwakeup(&q->rwait);
}

static int
qcanread(void *vq)
{
	Queue *q;
	int ne;

	q = vq;
	lock(&q->lock);
	ne = q->notempty;
	unlock(&q->lock);
	return ne;
}

static int
qread(Queue *q, void *v, int n)
{
	char *a;
	int nn, notempty;

	if(n == 0)
		return 0;
	a = v;
	nn = 0;
	for(;;){
		lock(&q->lock);

		while(nn < n && q->r != q->w){
			a[nn++] = *q->r++;
			if(q->r >= q->e)
				q->r = q->buf;
		}

		notempty = q->notempty;
		q->notempty = q->r != q->w;
		unlock(&q->lock);
		if(notempty)
			break;

		/*
		 * wait for something to show up in the kbd buffer.
		 */
		qlock(&q->qwait);
		if(waserror()){
			qunlock(&q->qwait);
			nexterror();
		}
		rendsleep(&q->rwait, qcanread, q);
		qunlock(&q->qwait);
		poperror();
	}
	return nn;
}

static Queue *
mkqueue(void)
{
	Queue *q;

	q = smalloc(sizeof(Queue));
	q->r = q->buf;
	q->w = q->r;
	q->e = &q->buf[sizeof q->buf];
	q->notempty = 0;
	return q;
}

static void
echoscreen(char *buf, int n)
{
	char *e, *p;
	char ebuf[128];
	int x;

	p = ebuf;
	e = ebuf + sizeof(ebuf) - 4;
	while(n-- > 0){
		if(p >= e){
			screenputs(ebuf, p - ebuf);
			p = ebuf;
		}
		x = *buf++;
		if(x == 0x15){
			*p++ = '^';
			*p++ = 'U';
			*p++ = '\n';
		} else
			*p++ = x;
	}
	if(p != ebuf)
		screenputs(ebuf, p - ebuf);
}

/*
 *  Put character, possibly a rune, into read queue at interrupt time.
 *  Called at interrupt time to process a character.
 */
void
kbdputc(int ch)
{
	int n;
	char buf[3];
	Rune r;

	r = ch;
	n = runetochar(buf, &r);
	qwrite(kbdq, buf, n);
	if(!kbd.raw)
		echoscreen(buf, n);
}

static void
kbdputcinit(void)
{
	kbdq = mkqueue();
	lineq = mkqueue();
	kbd.raw = 0;
	kbd.ctl = 0;
	kbd.x = 0;
}

enum{
	Qdir,
	Qcons,
	Qconsctl,
	Qsnarf,
	Qwinname,
};

static Dirtab consdir[]={
	".",		{Qdir, 0, QTDIR},	0,		DMDIR|0555,
	"cons",		{Qcons},	0,		0660,
	"consctl",	{Qconsctl},	0,		0220,
	"snarf",	{Qsnarf},	0,		0600,
	"winname",	{Qwinname},	0,		0000,
};

static void
consinit(void)
{
	kbdputcinit();
}

static Chan*
consattach(char *spec)
{
	return devattach('c', spec);
}

static Walkqid*
conswalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name,nname, consdir, nelem(consdir), devgen);
}

static int
consstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, consdir, nelem(consdir), devgen);
}

static Chan*
consopen(Chan *c, int omode)
{
	c->aux = nil;
	c = devopen(c, omode, consdir, nelem(consdir), devgen);
	switch((ulong)c->qid.path){
	case Qconsctl:
		qlock(&kbd);
		kbd.ctl++;
		qunlock(&kbd);
		break;
	case Qsnarf:
		if((c->mode&3) == OWRITE || (c->mode&3) == ORDWR)
			c->aux = smalloc(sizeof(Snarf));
		break;
	}
	return c;
}

void
setsnarf(char *buf, int n, int *vers)
{
	int i;

	qlock(&snarf);
	snarf.vers++;
	if(vers)
		*vers = snarf.vers;	
	for(i = 0; i < nelem(consdir); i++){
		if(consdir[i].qid.type == Qsnarf){
			consdir[i].qid.vers = snarf.vers;
			break;
		}
	}
	free(snarf.buf);
	snarf.n = n;
	snarf.buf = buf;
	qunlock(&snarf);
}

static void
consclose(Chan *c)
{
	Snarf *t;

	switch((ulong)c->qid.path){
	/* last close of control file turns off raw */
	case Qconsctl:
		if(c->flag&COPEN){
			qlock(&kbd);
			if(--kbd.ctl == 0)
				kbd.raw = 0;
			qunlock(&kbd);
		}
		break;
	/* odd behavior but really ok: replace snarf buffer when /dev/snarf is closed */
	case Qsnarf:
		t = c->aux;
		if(t == nil)
			break;
		setsnarf(t->buf, t->n, 0);
		t->buf = nil;	/* setsnarf took it */
		free(t);
		c->aux = nil;
		break;
	}
}

static long
consread(Chan *c, void *buf, long n, vlong off)
{
	char ch;
	int	send;

	if(n <= 0)
		return n;
	switch((ulong)c->qid.path){
	case Qsnarf:
		qlock(&snarf);
		if(off < snarf.n){
			if(off + n > snarf.n)
				n = snarf.n - off;
			memmove(buf, snarf.buf+off, n);
		}else
			n = 0;
		qunlock(&snarf);
		return n;

	case Qdir:
		return devdirread(c, buf, n, consdir, nelem(consdir), devgen);

	case Qcons:
		qlock(&kbd);
		if(waserror()){
			qunlock(&kbd);
			nexterror();
		}
		while(!qcanread(lineq)){
			qread(kbdq, &ch, 1);
			send = 0;
			if(ch == 0){
				/* flush output on rawoff -> rawon */
				if(kbd.x > 0)
					send = !qcanread(kbdq);
			}else if(kbd.raw){
				kbd.line[kbd.x++] = ch;
				send = !qcanread(kbdq);
			}else{
				switch(ch){
				case '\b':
					if(kbd.x > 0)
						kbd.x--;
					break;
				case 0x15:	/* ^U */
					kbd.x = 0;
					break;
				case '\n':
				case 0x04:	/* ^D */
					send = 1;
				default:
					if(ch != 0x04)
						kbd.line[kbd.x++] = ch;
					break;
				}
			}
			if(send || kbd.x == sizeof kbd.line){
				qwrite(lineq, kbd.line, kbd.x);
				kbd.x = 0;
			}
		}
		n = qread(lineq, buf, n);
		qunlock(&kbd);
		poperror();
		return n;

	default:
		print("consread 0x%llux\n", c->qid.path);
		error(Egreg);
	}
	return -1;		/* never reached */
}

static long
conswrite(Chan *c, void *va, long n, vlong)
{
	Snarf *t;
	char buf[256], *a;
	char ch;

	switch((ulong)c->qid.path){
	case Qcons:
		screenputs(va, n);
		break;

	case Qconsctl:
		if(n >= sizeof(buf))
			n = sizeof(buf)-1;
		strncpy(buf, va, n);
		buf[n] = 0;
		for(a = buf; a;){
			if(strncmp(a, "rawon", 5) == 0){
				kbd.raw = 1;
				/* clumsy hack - wake up reader */
				ch = 0;
				qwrite(kbdq, &ch, 1);			
			} else if(strncmp(a, "rawoff", 6) == 0){
				kbd.raw = 0;
			}
			if(a = strchr(a, ' '))
				a++;
		}
		break;

	case Qsnarf:
		t = c->aux;
		/* always append only */
		if(t->n > MAXSNARF)	/* avoid thrashing when people cut huge text */
			error("snarf buffer too big");
		a = realloc(t->buf, t->n + n + 1);
		if(a == nil)
			error("snarf buffer too big");
		t->buf = a;
		memmove(t->buf+t->n, va, n);
		t->n += n;
		t->buf[t->n] = '\0';
		break;
	default:
		print("conswrite: 0x%llux\n", c->qid.path);
		error(Egreg);
	}
	return n;
}

Dev consdevtab = {
	'c',
	"cons",

	devreset,
	consinit,
	consattach,
	conswalk,
	consstat,
	consopen,
	devcreate,
	consclose,
	consread,
	devbread,
	conswrite,
	devbwrite,
	devremove,
	devwstat,
};
