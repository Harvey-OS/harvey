#include	"lib9.h"
#include	"sys.h"
#include	"error.h"
#include	"keyboard.h"

enum
{
	Qdir,
	Qauth,
	Qauthcheck,
	Qauthent,
	Qcons,
	Qconsctl,
	Qcpunote,
	Qhostowner,
	Qkey,
	Qnull,
	Qsnarf,
	Qsysname,
	Quser
};

Dirtab contab[] =
{
	"authenticate",	{Qauth},	0,		0666,
	"authcheck",	{Qauthcheck},	0,		0666,
	"authenticator", {Qauthent},	0,		0666,
	"cons",		{Qcons},	0,		0666,
	"consctl",	{Qconsctl},	0,		0222,
	"cpunote",	{Qcpunote},	0,		0444,
	"hostowner",	{Qhostowner},	NAMELEN,	0664,
	"key",		{Qkey},		DESKEYLEN,	0622,
	"null",		{Qnull},	0,		0666,
	"snarf",	{Qsnarf},	0,		0666,
	"sysname",	{Qsysname},	0,		0444,
	"user",		{Quser},	NAMELEN,	0666,
};

Dirtab *snarftab = &contab[9];

#define Ncontab	(sizeof(contab)/sizeof(contab[0]))

void (*screenputs)(char*, int);

static Qlock locked;

Queue*	kbdq;			/* unprocessed console input */
static Queue*	lineq;			/* processed console input */
char	sysname[3*NAMELEN];
static struct
{
	Qlock	q;

	int	raw;		/* true if we shouldn't process input */
	int	ctl;		/* number of opens to the control file */
	int	x;		/* index into line */
	char	line[1024];	/* current input line */

	Rune	c;
	int	count;
} kbd;

void
coninit(void)
{
	canqlock(&locked);

	kbdq = qopen(1024, 0, 0, 0);
	if(kbdq == 0)
		panic("no memory");
	lineq = qopen(1024, 0, 0, 0);
	if(lineq == 0)
		panic("no memory");
}

Chan*
conattach(void *spec)
{
	return devattach('c', spec);
}

Chan*
conclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
conwalk(Chan *c, char *name)
{
	return devwalk(c, name, contab, Ncontab, devgen);
}

void
constat(Chan *c, char *db)
{
	devstat(c, db, contab, Ncontab, devgen);
}

Chan*
conopen(Chan *c, int omode)
{
	switch(c->qid.path & ~CHDIR){
	case Qdir:
		if(omode != OREAD)
			error(Eisdir);
		c->mode = omode;
		c->flag |= COPEN;
		c->offset = 0;
		return c;
	case Qconsctl:
		qlock(&kbd.q);
		kbd.ctl++;
		qunlock(&kbd.q);
		break;
	case Qsnarf:
		if(omode == ORDWR)
			error(Eperm);
		if(omode == OREAD)
			c->u.aux = strdup("");
		else
			c->u.aux = mallocz(SnarfSize);
		break;
	}

	return devopen(c, omode, contab, Ncontab, devgen);
}

void
concreate(Chan *c, char *name, int omode, ulong perm)
{
	error(Eperm);
}

void
conremove(Chan *c)
{
	error(Eperm);
}

void
conwstat(Chan *c, char *dp)
{
	error(Eperm);
}

void
conclose(Chan *c)
{
	switch(c->qid.path) {
	case Qconsctl:
		if(c->flag&COPEN) {
			qlock(&kbd.q);
			kbd.ctl++;
			kbd.raw = 0;
			qunlock(&kbd.q);
		}
	case Qauth:
	case Qauthcheck:
	case Qauthent:
		authclose(c);
		break;
	case Qsnarf:
		if(c->mode == OWRITE)
			clipwrite(c->u.aux, strlen(c->u.aux));
		free(c->u.aux);
		break;
	}
}

long
conread(Chan *c, void *va, long count, ulong offset)
{
	int n, ch, eol;
	char *cbuf = va;

	if(c->qid.path & CHDIR)
		return devdirread(c, va, count, contab, Ncontab, devgen);

	switch(c->qid.path) {
	default:
		error(Egreg);
	case Qauth:
		return authread(c, cbuf, count);

	case Qauthent:
		return authentread(c, cbuf, count);

	case Qcons:
		qlock(&kbd.q);
		if(waserror()){
			qunlock(&kbd.q);
			nexterror();
		}
		if(kbd.raw) {
			if(qcanread(lineq))
				n = qread(lineq, va, count);
			else {
				/* read as much as possible */
				do{
					n = qread(kbdq, cbuf, count);
					count -= n;
					cbuf += n;
				} while(count>0 && qcanread(kbdq));
				n = cbuf-(char*)va;
			}
		} else {
			while(!qcanread(lineq)) {
				qread(kbdq, &kbd.line[kbd.x], 1);
				ch = kbd.line[kbd.x];
				if(kbd.raw){
					qiwrite(lineq, kbd.line, kbd.x+1);
					kbd.x = 0;
					continue;
				}
				eol = 0;
				switch(ch) {
				case '\b':
					if(kbd.x)
						kbd.x--;
					break;
				case 0x15:
					kbd.x = 0;
					break;
				case '\n':
				case 0x04:
					eol = 1;
				default:
					kbd.line[kbd.x++] = ch;
					break;
				}
				if(kbd.x == sizeof(kbd.line) || eol){
					if(ch == 0x04)
						kbd.x--;
					qwrite(lineq, kbd.line, kbd.x);
					kbd.x = 0;
				}
			}
			n = qread(lineq, va, count);
		}
		qunlock(&kbd.q);
		poperror();
		return n;
	case Qcpunote:
		/*
		 * We don't need to actually do anything here, on
		 * the assumption that any drawterm users are going
		 * to be running a window system (rio) that will
		 * take care of interrupts for us.  The notes are
		 * only necessary for cpu windows, not cpu window systems.
		 */
		qlock(&locked);
		return 0;
	case Qhostowner:
		return readstr(offset, va, count, eve);
	case Qkey:
		return keyread(va, count, offset);
	case Qnull:
		return 0;
	case Qsysname:
		return readstr(offset, va, count, sysname);
	case Quser:
		return readstr(offset, va, count, up->user);
	case Qsnarf: 
		if(offset == 0) {
			free(c->u.aux);
			c->u.aux = clipread();
		}
		if(c->u.aux == nil)
			return 0;
		return readstr(offset, va, count, c->u.aux);
	}
}

long
conwrite(Chan *c, void *va, long count, ulong offset)
{
	char buf[32];
	char *a = va;

	if(c->qid.path & CHDIR)
		error(Eperm);

	switch(c->qid.path) {
	default:
		error(Egreg);
	case Qauth:
		return authwrite(c, a, count);

	case Qauthcheck:
		return authcheck(c, a, count);

	case Qauthent:
		return authentwrite(c, a, count);

	case Qcons:
		if(screenputs){
			screenputs(a, count);
			return count;
		}else
			return write(1, va, count);
		
	case Qconsctl:
		if(count >= sizeof(buf))
			count = sizeof(buf)-1;
		strncpy(buf, va, count);
		buf[count] = 0;
		if(strncmp(buf, "rawon", 5) == 0) {
			kbd.raw = 1;
			return count;
		}
		else
		if(strncmp(buf, "rawoff", 6) == 0) {
			kbd.raw = 0;
			return count;
		}
		error(Ebadctl);
	case Qhostowner:
		return hostownerwrite(a, count);
	case Qkey:
		return keywrite(a, count);
	case Qnull:
		return count;
	case Qsnarf:
		if(offset+count >= SnarfSize)
			error(Etoobig);
		snarftab->qid.vers++;
		memmove((uchar*)(c->u.aux)+offset, va, count);
		return count;
	}
}

int
nrand(int n)
{
	static ulong randn;

	randn = randn*1103515245 + 12345 + ticks();
	return (randn>>16) % n;
}

static void
echo(Rune r, char *buf, int n)
{
	static int ctrlt;

	/*
	 * ^t hack BUG
	 */
	if(ctrlt == 2){
		ctrlt = 0;
		switch(r){
		case 0x14:
			break;	/* pass it on */
		case 'r':
			exits(0);
			break;
		}
	}
	else if(r == 0x14){
		ctrlt++;
		return;
	}
	ctrlt = 0;
	if(kbd.raw)
		return;

	/*
	 *  finally, the actual echoing
	 */
	if(r == 0x15){
		buf = "^U\n";
		n = 3;
	}
	screenputs(buf, n);
}

/*
 *  Put character, possibly a rune, into read queue at interrupt time.
 *  Called at interrupt time to process a character.
 */
static
void
_kbdputc(int c)
{
	Rune r;
	char buf[UTFmax];
	int n;
	
	r = c;
	n = runetochar(buf, &r);
	if(n == 0)
		return;
	echo(r, buf, n);
	kbd.c = r;
	qproduce(kbdq, buf, n);
}

/* _kbdputc, but with compose translation */
void
kbdputc(int c)
{
	int i;
	static int collecting, nk;
	static Rune kc[5];

	if(c == Kalt){
		collecting = 1;
		nk = 0;
		return;
	}
	
	if(!collecting){
		_kbdputc(c);
		return;
	}
	
	kc[nk++] = c;
	c = latin1(kc, nk);
	if(c < -1)	/* need more keystrokes */
		return;
	if(c != -1)	/* valid sequence */
		_kbdputc(c);
	else
		for(i=0; i<nk; i++)
			_kbdputc(kc[i]);
	nk = 0;
	collecting = 0;
}
