#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

#include	"probe.h"

enum {
	Qdir,
	Qctl,
	Qdata,
};

enum {
	ProbeEntry = 1,
	ProbeExit
};

/* fix me make this programmable */
enum {
	defaultlogsize = 1024,
	printsize = 64,
};

typedef struct Probelog Probelog;
struct Probelog {
	uvlong ticks;
	/* yeah, waste a whole int on something stupid but ... */
	int info;
	ulong pc;
	/* these are different depending on type */
	long dat[4];
};

static Rendez probesleep;
static QLock probeslk;
static Probe *probes;
static Lock loglk;
static Probelog *probelog = nil;
/* probe indices. These are just unsigned longs. You mask them
 * to get an index. This makes fifo empty/full etc. trivial.
 */
static ulong pw = 0, pr = 0;
static int probesactive = 0;
static unsigned long logsize = defaultlogsize, logmask = defaultlogsize - 1;

static char eventname[] = {
	[ProbeEntry] = 'E',
	[ProbeExit] = 'X'
};

static Dirtab probedir[]={
	".",		{Qdir, 0, QTDIR},	0,		DMDIR|0555,
	"probectl",	{Qctl},		0,		0664,
	"probe",	{Qdata},	0,		0440,
};

char hex[] = {
	'0',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'A',
	'B',
	'C',
	'D',
	'E',
	'F',
};

/* big-endian ... */
void
hex32(ulong l, char *c)
{
	int i;
	for(i = 8; i; i--){
		c[i-1] = hex[l&0xf];
		l >>= 4;
	}
}

void
hex64(uvlong l, char *c)
{
	hex32(l>>32, c);
	hex32(l, &c[8]);
}
static int
lognonempty(void *)
{
	return pw - pr;
}

static int
logfull(void)
{
	return (pw - pr) >= logsize;
}

static ulong
idx(ulong f)
{
	return f & logmask;
}

/* can return NULL, meaning, no record for you */
static struct Probelog *
newpl(void)
{
	ulong index;

	if (logfull()){
		wakeup(&probesleep);
		return nil;
	}

	ilock(&loglk);
	index = pw++;
	iunlock(&loglk);

	return &probelog[idx(index)];

}

static void
probeentry(Probe *p)
{
	struct Probelog *pl;
//print("probeentry %p p %p func %p argp %p\n", &p, p, p->func, p->argp);
	pl = newpl();
	if (! pl)
		return;
	cycles(&pl->ticks);
	pl->pc = (ulong)p->func;
	pl->dat[0] = p->argp[0];
	pl->dat[1] = p->argp[1];
	pl->dat[2] = p->argp[2];
	pl->dat[3] = p->argp[3];
	pl->info = ProbeEntry;
}

static void
probeexit(Probe *p)
{
//print("probeexit %p p %p func %p argp %p\n", &p, p, p->func, p->argp);
	struct Probelog *pl;
	pl = newpl();
	if (! pl)
		return;
	cycles(&pl->ticks);
	pl->pc = (ulong)p->func;
	pl->dat[0] = p->rval;
	pl->info = ProbeExit;
}

static Chan*
probeattach(char *spec)
{
	return devattach('+', spec);
}

static Walkqid*
probewalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, probedir, nelem(probedir), devgen);
}

static long
probestat(Chan *c, uchar *db, long n)
{
	return devstat(c, db, n, probedir, nelem(probedir), devgen);
}

static Chan*
probeopen(Chan *c, int omode)
{
	/* if there is no probelog, allocate one. Open always fails
	  * if the basic alloc fails. You can resize it later.
	  */
	if (! probelog)
		probelog = malloc(sizeof(*probelog)*logsize);
	/* I guess malloc doesn't toss an error */
	if (! probelog)
		error("probelog malloc failed");

	c = devopen(c, omode, probedir, nelem(probedir), devgen);
	return c;
}

static void
probeclose(Chan *)
{
}

static long
proberead(Chan *c, void *a, long n, vlong offset)
{
	char *buf;
	char *cp = a;
	struct Probelog *pl;
	Probe *p;
	int i;
	static QLock gate;
	if(c->qid.type == QTDIR)
		return devdirread(c, a, n, probedir, nelem(probedir), devgen);
	switch((ulong)c->qid.path){
	default:
		error("proberead: bad qid");
	case Qctl:
		buf = malloc(READSTR);
		i = 0;
		qlock(&probeslk);
		i += snprint(buf + i, READSTR - i, "logsize %lud\n", logsize);
		for(p = probes; p != nil; p = p->next)
			i += snprint(buf + i, READSTR - i, "probe %p new %s\n",
				p->func, p->name);

		for(p = probes; p != nil; p = p->next)
			if (p->enabled)
				i += snprint(buf + i, READSTR - i, "probe %s on\n",
				p->name);
		i += snprint(buf + i, READSTR - i, "#probehits %lud, in queue %lud\n",
				pw, pw-pr);
		snprint(buf + i, READSTR - i, "#probelog %p\n", probelog);
		qunlock(&probeslk);
		n = readstr(offset, a, n, buf);
		free(buf);
		break;
	case Qdata:
		qlock(&gate);
		if(waserror()){
			qunlock(&gate);
			nexterror();
		}
		while(!lognonempty(nil))
			tsleep(&probesleep, lognonempty, nil, 5000);
		i = 0;
		while(lognonempty((void *)0)){
			int j;
			pl = probelog + idx(pr);

			if ((i + printsize) >= n)
				break;
			/* simple format */
			cp[0] = eventname[pl->info];
			cp ++;
			*cp++ = ' ';
			hex32(pl->pc, cp);
			cp[8] = ' ';
			cp += 9;
			hex64(pl->ticks, cp);
			cp[16] = ' ';
			cp += 17;
			for(j = 0; j < 4; j++){
				hex32(pl->dat[j], cp);
				cp[8] = ' ';
				cp += 9;
			}
			/* adjust for extra skip above */
			cp--;
			*cp++ = '\n';
			pr++;
			i += printsize;
		}
		poperror();
		qunlock(&gate);
		n = i;
		break;
	}
	return n;
}

static long
probewrite(Chan *c, void *a, long n, vlong)
{
	char *tok[5];
	char *ep, *s = nil;
	Probe *p, **pp;
	int ntok;

	qlock(&probeslk);
	if(waserror()){
		qunlock(&probeslk);
		if(s != nil) free(s);
		nexterror();
	}
	switch((ulong)c->qid.path){
	default:
		error("proberead: bad qid");
	case Qctl:
		s = malloc(n + 1);
		memmove(s, a, n);
		s[n] = 0;
		ntok = tokenize(s, tok, nelem(tok));
		if(!strcmp(tok[0], "probe")){	/* 'probe' ktextaddr 'on'|'off'|'mk'|'del' [name] */
			if(ntok < 3)
				error("devprobe: usage: 'probe' [ktextaddr|name] 'on'|'off'|'mk'|'del' [name]");
			for(pp = &probes; *pp != nil; pp = &(*pp)->next)
				if(!strcmp(tok[1], (*pp)->name))
					break;
			p = *pp;
			if(!strcmp(tok[2], "new")){
				ulong addr;
				void *func;
				addr = strtoul(tok[1], &ep, 0);
				func = (void*)addr;
				if(*ep)
					error("devprobe: address not in recognized format");
			//	if(addr < ((ulong) start) || addr > ((ulong) end))
			//		error("devprobe: address out of bounds");
				if(p != nil)
					error("devprobe: %#p already has probe");
				p = mkprobe(func, probeentry, probeexit);
				p->next = probes;
				if(ntok < 4)
					snprint(p->name, sizeof p->name, "%p", func);
				else
					strncpy(p->name, tok[3], sizeof p->name);
				probes = p;
			} else if(!strcmp(tok[2], "on")){
				if(p == nil)
					error("devprobe: probe not found");
				if(!p->enabled)
					probeinstall(p);
print("probeinstall in devprobe\n");
				probesactive++;
			} else if(!strcmp(tok[2], "off")){
				if(p == nil)
					error("devprobe: probe not found");
				if(p->enabled)
					probeuninstall(p);
				probesactive--;
			} else if(!strcmp(tok[2], "del")){
				if(p == nil)
					error("devprobe: probe not found");
				if(p->enabled)
					probeuninstall(p);
				probesactive--;
				*pp = p->next;
				freeprobe(p);
			} else if(!strcmp(tok[2], "mv")){
				if(p == nil)
					error("devprobe: probe not found");
				if(ntok < 4)
					error("devprobe: rename without new name?");
				strncpy(p->name, tok[3], sizeof p->name);
			}
		} else if(!strcmp(tok[0], "size")){
			int l, size;
			struct Probelog *newprobelog;
			l = strtoul(tok[1], &ep, 0);
			if(*ep)
				error("devprobe: size not in recognized format");
			size = 1 << l;
			/* sort of foolish. Alloc new probe first, then free old. */
			/* and too bad if there are unread probes */
			newprobelog = malloc(sizeof(*newprobelog)*size);
			/* does malloc throw waserror? I don't know */
			free(probelog);
			probelog = newprobelog;
			logsize = size;
			pr = pw = 0;
		} else {
			error("devprobe:  usage: 'probe' [ktextaddr|name] 'on'|'off'|'mk'|'del' [name] or:  'size' buffersize (power of 2)");
		}
		free(s);
		break;
	}
	poperror();
	qunlock(&probeslk);
	return n;
}

Dev probedevtab = {
	'+',
	"probe",
	devreset,
	devinit,
	devshutdown,
	probeattach,
	probewalk,
	probestat,
	probeopen,
	devcreate,
	probeclose,
	proberead,
	devbread,
	probewrite,
	devbwrite,
	devremove,
	devwstat,
};
