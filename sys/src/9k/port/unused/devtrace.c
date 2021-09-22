#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"netif.h"

#pragma profile 0

typedef struct Trace Trace;
/* This is a trace--a segment of memory to watch for entries and exits */
struct Trace {
	struct Trace *next;
	void *func;
	void *start;
	void *end;
	int enabled;
	char name[16];
};

enum {
	Qdir,
	Qctl,
	Qdata,
};

enum {
	TraceEntry = 1,
	TraceExit,
};

/* fix me make this programmable */
enum {
	defaultlogsize = 8192,
};

/* This represents a trace "hit" or event */
typedef struct Tracelog Tracelog;
struct Tracelog {
	uvlong ticks;
	int info;
	uintptr pc;
	/* these are different depending on type */
	uintptr dat[5];
	int machno;
};


static Rendez tracesleep;
static QLock traceslock;
/* this will contain as many entries as there are valid pc values */
static Trace **tracemap;
static Trace *traces; /* This stores all the traces */
static Lock loglk;
static Tracelog *tracelog = nil;
int traceactive = 0;
/* trace indices. These are just unsigned longs. You mask them
 * to get an index. This makes fifo empty/full etc. trivial.
 */
static uint pw = 0, pr = 0;
static int tracesactive = 0;
static int all = 0;
static int watching = 0;
static int slothits = 0;
static unsigned int traceinhits = 0;
static unsigned int newplfail = 0;
static unsigned long logsize = defaultlogsize, logmask = defaultlogsize - 1;

static int printsize = 0;	//The length of a line being printed

/* These are for observing a single process */
static int *pidwatch = nil;
static int numpids = 0;
static const PIDWATCHSIZE = 32; /* The number of PIDS that can be watched. Pretty arbitrary. */

int codesize = 0;

static uvlong lastestamp; /* last entry timestamp */
static uvlong lastxstamp; /* last exit timestamp */

/* Trace events can be either Entries or Exits */
static char eventname[] = {
	[TraceEntry] = 'E',
	[TraceExit] = 'X',
};

static Dirtab tracedir[]={
	".",		{Qdir, 0, QTDIR},	0,		DMDIR|0555,
	"tracectl",	{Qctl},		0,		0664,
	"trace",	{Qdata},	0,		0440,
};

char hex[] = "0123456789abcdef";

/* big-endian ... */
void
hex8(u32int l, char *c)
{
	int i;
	for(i = 2; i; i--){
		c[i-1] = hex[l&0xf];
		l >>= 4;
	}
}

void
hex16(u32int l, char *c)
{
	int i;
	for(i = 4; i; i--){
		c[i-1] = hex[l&0xf];
		l >>= 4;
	}
}

void
hex32(u32int l, char *c)
{
	int i;
	for(i = 8; i; i--){
		c[i-1] = hex[l&0xf];
		l >>= 4;
	}
}

void
hex64(u64int l, char *c)
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

static u64int
idx(u64int f)
{
	return f & logmask;
}

/*
 * Check if the given trace overlaps any others
 * Returns 1 if there is overlap, 0 if clear.
 */
int
overlapping(Trace *p)
{
	Trace *curr;

	for (curr = traces; curr != nil; curr = curr->next)
		if ((curr->start < p->start && p->start < curr->end) ||
		    (curr->start < p->end && p->end < curr->end))
			return 1;
	return 0;
}

/* Make sure a PC is valid and traced; if so, return its Trace */
/* if dopanic == 1, the kernel will panic on an invalid PC */
struct Trace **
traceslot(void *pc, int dopanic)
{
	int index;
	struct Trace **p;

	if (pc > etext) {
		if (dopanic)
			panic("Bad PC %p", pc);

		print("Invalid PC %p\n", pc);
		return nil;
	}
	index = (int)((uintptr)pc - KTZERO);
	if (index > codesize){
		if (dopanic) {
			panic("Bad PC %p", pc);
			while(1);
		}
		print("Invalid PC %p\n", pc);
		return nil;
	}
	p = &tracemap[index];
	if (tracemap[index])
		ainc(&slothits);
	return p;
}

/* Check if the given PC is traced and return a Trace if so */
struct Trace *
traced(void *pc, int dopanic)
{
	struct Trace **p;

	p = traceslot(pc, dopanic);
	if (p == nil)
		return nil;
	return *p;
}

/*
 * Return 1 if pid is being watched or no pids are being watched.
 * Return 0 if pids are being watched and the argument is not
 * among them.
 */
int
watchingpid(int pid)
{
	int i;

	if (pidwatch[0] == 0)
		return 1;
	for (i = 0; i < numpids; i++)
		if (pidwatch[i] == pid)
			return 1;
	return 0;
}

/*
 * Remove a trace.
 */
void
removetrace(Trace *p)
{
	uchar *cp;
	struct Trace *prev, *curr;
	struct Trace **slot;

	slot = traceslot(p->start, 0);
	for(cp = p->start; cp <= p->end; slot++, cp++)
		*slot = nil;

	curr = traces;
	if (curr == p) {
		if (curr->next)
			traces = curr->next;
		else
			traces = nil;	// this seems to work fine
		free(curr);
		return;
	}

	prev = curr;
	curr = curr->next;
	do {
		if (curr == p) {
			prev->next = curr->next;
			return;
		}
		prev = curr;
		curr = curr->next;
	} while (curr != nil);
}

/* it is recommended that you call these with something sane. */
/* these next two functions assume you locked tracelock */

/* Turn on a trace */
void
traceon(struct Trace *p)
{
	uchar *cp;
	struct Trace **slot;

	slot = traceslot(p->start, 0);
	for(cp = p->start; cp <= p->end; slot++, cp++)
		*slot = p;
	p->enabled = 1;
	tracesactive++;
}

/* Turn off a trace */
void
traceoff(struct Trace  *p)
{
	uchar *cp;
	struct Trace **slot;

	slot = traceslot(p->start, 0);
	for(cp = p->start; cp <= p->end; slot++, cp++)
		*slot = nil;
	p->enabled = 0;
	tracesactive--;
}

/* Make a new tracelog (an event) */
/* can return NULL, meaning, no record for you */
static struct Tracelog *
newpl(void)
{
	uint index;

	index = ainc((int *)&pw);
	return &tracelog[idx(index)];
}

/* Called every time a (traced) function starts */
/* this is not really smp safe. FIX */
void
tracein(void* pc, uintptr a1, uintptr a2, uintptr a3, uintptr a4)
{
	struct Tracelog *pl;

	/* if we are here, tracing is active. Turn it off. */
	traceactive = 0;
	if (! traced(pc, 1)){
		traceactive = 1;
		return;
	}

	ainc((int *)&traceinhits);
	/* Continue if we are watching this pid or we're not watching any */
	if (!all)
		if (!up || !watchingpid(up->pid)){
			traceactive = 1;
			return;
		}

	pl = newpl();
	if (! pl) {
		ainc((int *)&newplfail);
		traceactive = 1;
		return;
	}

	cycles(&pl->ticks);

	pl->pc = (uintptr)pc;
	if (up)
		pl->dat[0] = up->pid;
	else
		pl->dat[0] = (unsigned long)-1;

	pl->dat[1] = a1;
	pl->dat[2] = a2;
	pl->dat[3] = a3;
	pl->dat[4] = a4;

	pl->info = TraceEntry;
	pl->machno = m->machno;
	traceactive = 1;
}

/* Called every time a traced function exits */
void
traceout(void* pc, uintptr retval)
{
	struct Tracelog *pl;

	/* if we are here, tracing is active. Turn it off. */
	traceactive = 0;
	if (! traced(pc, 1)){
		traceactive = 1;
		return;
	}

	if (!all)
		if (!up || !watchingpid(up->pid)){
			traceactive = 1;
			return;
		}

	pl = newpl();
	if (! pl){
		traceactive = 1;
		return;
	}

	cycles(&pl->ticks);

	pl->pc = (uintptr)pc;
	if (up)
		pl->dat[0] = up->pid;
	else
		pl->dat[0] = (unsigned long)-1;

	pl->dat[1] = retval;
	pl->dat[2] = 0;
	pl->dat[3] = 0;

	pl->info = TraceExit;
	pl->machno = m->machno;
	traceactive = 1;
}

/* Create a new trace with the given range */
static Trace *
mktrace(void *func, void *start, void *end)
{
	Trace *p;

	p = malloc(sizeof p[0]);
	p->func = func;
	p->start = start;
	p->end = end;
	return p;
}

/* Get rid of an old trace */
static void
freetrace(Trace *p)
{
	free(p);
}


static Chan*
traceattach(char *spec)
{
	return devattach('T', spec);
}

static Walkqid*
tracewalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, tracedir, nelem(tracedir), devgen);
}

static long
tracestat(Chan *c, uchar *db, long n)
{
	return devstat(c, db, n, tracedir, nelem(tracedir), devgen);
}

static Chan*
traceopen(Chan *c, int omode)
{
	/*
	 * if there is no tracelog, allocate one. Open always fails
	 * if the basic alloc fails. You can resize it later.
	 */

	codesize = (uintptr)etext - (uintptr)KTZERO;
	if (! tracemap)
		//tracemap = malloc(sizeof(struct tracemap *)*codesize);
		tracemap = malloc(sizeof(struct Trace *)*codesize);
	if (! tracemap)
		error("tracemap malloc failed");
	if (! tracelog)
		tracelog = malloc(sizeof(*tracelog)*logsize);
	/* I guess malloc doesn't toss an error */
	if (! tracelog)
		error("tracelog malloc failed");
	if (! pidwatch)
		pidwatch = malloc(sizeof(int)*PIDWATCHSIZE);
	if (! pidwatch)
		error("pidwatch malloc failed");
	return devopen(c, omode, tracedir, nelem(tracedir), devgen);
}

static void
traceclose(Chan *)
{
}

/*
 * Reading from the device, either the data or control files.
 * The data reading involves deep rminnich magic so we don't have
 * to call print(), which is traced.
 */
static long
traceread(Chan *c, void *a, long n, vlong offset)
{
	char *buf;
	char *cp = a;
	struct Tracelog *pl;
	Trace *p;
	int i, j;
	int saveactive = traceactive;
	static QLock gate;

	traceactive = 0;
	if (waserror()) {
		traceactive = saveactive;
		nexterror();
	}

	if(c->qid.type == QTDIR) {
		long l = devdirread(c, a, n, tracedir, nelem(tracedir), devgen);

		poperror();
		traceactive = saveactive;
		return l;
	}

	switch((int) c->qid.path){
	default:
		error("traceread: bad qid");
	case Qctl:
		i = 0;
		qlock(&traceslock);
		buf = malloc(READSTR);
		i += snprint(buf + i, READSTR - i, "logsize %lud\n", logsize);
		for(p = traces; p != nil; p = p->next)
			i += snprint(buf + i, READSTR - i, "trace %p %p new %s\n",
				p->start, p->end, p->name);

		for(p = traces; p != nil; p = p->next)
			i += snprint(buf + i, READSTR - i, "#trace %p traced? %p\n",
				p->func, traced(p->func, 0));

		for(p = traces; p != nil; p = p->next)
			if (p->enabled)
				i += snprint(buf + i, READSTR - i, "trace %s on\n",
				p->name);
		i += snprint(buf + i, READSTR - i, "#tracehits %d, in queue %d\n",
				pw, pw-pr);
		i += snprint(buf + i, READSTR - i, "#tracelog %p\n", tracelog);
		i += snprint(buf + i, READSTR - i, "#traceactive %d\n", saveactive);
		i += snprint(buf + i, READSTR - i, "#slothits %d\n", slothits);
		i += snprint(buf + i, READSTR - i, "#traceinhits %d\n", traceinhits);
		for (j = 0; j < numpids - 1; j++)
			i += snprint(buf + i, READSTR - i, "watch %d\n", pidwatch[j]);
		snprint(buf + i, READSTR - i, "watch %d\n", pidwatch[numpids - 1]);
		n = readstr(offset, a, n, buf);
		free(buf);
		qunlock(&traceslock);
		break;
	case Qdata:
		// Set the printsize
		/* 32-bit E PCPCPCPC TIMETIMETIMETIME PID# CR XXARG1XX XXARG2XX XXARG3XX XXARG4XX\n */
		if (sizeof(uintptr) == 4)
			printsize = 73;		// 32-bit format
		else
			printsize = 121;	// must be 64-bit

		i = 0;
		while(lognonempty((void *)0)){
			int j;

			if (pw - pr > logsize)
				pr = pw - logsize;

			pl = tracelog + idx(pr);

			if ((i + printsize) > n)
				break;
			/* simple format */
			if (sizeof(uintptr) == 4) {
				cp[0] = eventname[pl->info];
				cp++;
				*cp++ = ' ';
				hex32((uint)pl->pc, cp);
				cp[8] = ' ';
				cp += 9;
				hex64(pl->ticks, cp);
				cp[16] = ' ';
				cp += 17;
				hex16(pl->dat[0], cp);
				cp += 4;
				cp[0] = ' ';
				cp++;
				hex8(pl->machno, cp);
				cp += 2;
				cp[0] = ' ';
				cp++;
				for(j = 1; j < 4; j++){
					hex32(pl->dat[j], cp);
					cp[8] = ' ';
					cp += 9;
				}
				/* adjust for extra skip above */
				cp--;
				*cp++ = '\n';
				pr++;
				i += printsize;
			} else  {
				cp[0] = eventname[pl->info];
				cp++;
				*cp++ = ' ';
				hex64((u64int)pl->pc, cp);
				cp[16] = ' ';
				cp += 17;
				hex64(pl->ticks, cp);
				cp[16] = ' ';
				cp += 17;
				hex32(pl->dat[0], cp);
				cp += 8;
				cp[0] = ' ';
				cp++;
				cp[0] = ' ';
				cp++;
				cp[0] = ' ';
				cp++;
				cp[0] = ' ';
				cp++;
				hex8(pl->machno, cp);
				cp += 4;
				for (j = 1; j < 5; j++) {
					hex64(pl->dat[j], cp);
					cp[16] = ' ';
					cp += 17;
				}
				cp--;
				*cp++ = '\n';
				pr++;
				i += printsize;
			}
		}
		n = i;
		break;
	}
	poperror();
	traceactive = saveactive;
	return n;
}

/*
 * Process commands sent to the ctl file.
 */
static long
tracewrite(Chan *c, void *a, long n, vlong)
{
	char *tok[6]; // changed this so "tracein" works with the new 4th arg
	char *ep, *s = nil;
	Trace *p, **pp, *foo;
	int ntok;
	int saveactive = traceactive;

	traceactive = 0;
	qlock(&traceslock);
	if(waserror()){
		qunlock(&traceslock);
		if(s != nil) free(s);
		traceactive = saveactive;
		nexterror();
	}
	switch((uintptr)c->qid.path){
	default:
		error("tracewrite: bad qid");
	case Qctl:
		s = malloc(n + 1);
		memmove(s, a, n);
		s[n] = 0;
		ntok = tokenize(s, tok, nelem(tok));
		if(strcmp(tok[0], "trace") == 0){ /* 'trace' ktextaddr 'on'|'off'|'mk'|'del' [name] */
			if(ntok < 3)
				error("devtrace: usage: 'trace' [ktextaddr|name] 'on'|'off'|'mk'|'del' [name]");
			for(pp = &traces; *pp != nil; pp = &(*pp)->next)
				if(strcmp(tok[1], (*pp)->name) == 0)
					break;
			p = *pp;
			if((ntok > 3) && strcmp(tok[3], "new") == 0){
				uintptr addr;
				void *start, *end, *func;

				if (ntok != 5) {
					error("devtrace: usage: trace <ktextstart> <ktextend> new <name>");
				}
				addr = (uintptr)strtoul(tok[1], &ep, 16);
				if (addr < KTZERO)
					addr |= KTZERO;
				func = start = (void *)addr;
				if(*ep) {
					error("devtrace: start address not in recognized format");
				}
				addr = (uintptr)strtoul(tok[2], &ep, 16);
				if (addr < KTZERO)
					addr |= KTZERO;
				end = (void *)addr;
				if(*ep) {
					error("devtrace: end address not in recognized format");
				}

				if (start > end || start > etext || end > etext)
					error("devtrace: invalid address range");

				/* What do we do here? start and end are weird *
				if((addr < (uintptr)start) || (addr > (uintptr)end)
					error("devtrace: address out of bounds");
				 */
				if(p) {
					error("devtrace: trace already exists");
				}
				p = mktrace(func, start, end);
				for (foo = traces; foo != nil; foo = foo->next)
					if (strcmp(tok[4], foo->name) == 0)
						error("devtrace: trace with that name already exists");

				if (!overlapping(p)) {
					p->next = traces;
					if(ntok < 5)
						snprint(p->name, sizeof p->name, "%p", func);
					else
						strncpy(p->name, tok[4], sizeof p->name);
					traces = p;
				} else
					error("devtrace: given range overlaps with existing trace");
			} else if(strcmp(tok[2], "remove") == 0){
				if (ntok != 3)
					error("devtrace: usage: trace <name> remove");
				if (p == nil) {
					error("devtrace: trace not found");
				}
				removetrace(p);
			} else if(strcmp(tok[2], "on") == 0){
				if (ntok != 3)
					error("devtrace: usage: trace <name> on");
				if(p == nil)
					error("devtrace: trace not found");
				if (! traced(p->func, 0))
					traceon(p);
			} else if(strcmp(tok[2], "off") == 0){
				if (ntok != 3)
					error("devtrace: usage: trace <name> off");
				if(p == nil)
					error("devtrace: trace not found");
				if(traced(p->func, 0))
					traceoff(p);
			}
		} else if(strcmp(tok[0], "query") == 0){
			/* See if addr is being traced */
			Trace* p;
			uintptr addr;

			if (ntok != 2) {
				error("devtrace: usage: query <addr>");
			}
			addr = (uintptr)strtoul(tok[1], &ep, 16);
			if (addr < KTZERO)
				addr |= KTZERO;
			p = traced((void *)addr, 0);
			if (p) {
				print("Probing is enabled\n");
			} else {
				print("Probing is disabled\n");
			}
		} else if(strcmp(tok[0], "size") == 0){
			int l, size;
			struct Tracelog *newtracelog;

			if (ntok != 2)
				error("devtrace: usage: size <exponent>");

			l = strtoul(tok[1], &ep, 0);
			if(*ep) {
				error("devtrace: size not in recognized format");
			}
			size = 1 << l;
			/* sort of foolish. Alloc new trace first, then free old. */
			/* and too bad if there are unread traces */
			newtracelog = mallocz(sizeof(*newtracelog)*size, 1);
			/* does malloc throw waserror? I don't know */
			if (newtracelog){
				free(tracelog);
				tracelog = newtracelog;
				logsize = size;
				logmask = size - 1;
				pr = pw = 0;
			} else
				error("devtrace: can't allocate that much");
		} else if (strcmp(tok[0], "testtracein") == 0) {
			/* Manually jump to a certain bit of traced code */
			uintptr pc, a1, a2, a3, a4;
			int x;

			if (ntok != 6)
				error("devtrace: usage: testtracein <pc> <arg1> <arg2> <arg3> <arg4>");

			pc = (uintptr)strtoul(tok[1], &ep, 16);
			if (pc < KTZERO)
				pc |= KTZERO;
			a1 = (uintptr)strtoul(tok[2], &ep, 16);
			a2 = (uintptr)strtoul(tok[3], &ep, 16);
			a3 = (uintptr)strtoul(tok[4], &ep, 16);
			a4 = (uintptr)strtoul(tok[5], &ep, 16);

			if (traced((void *)pc, 0)) {
				x = splhi();
				watching = 1;
				tracein((void *)pc, a1, a2, a3, a4);
				watching = 0;
				splx(x);
			}
		} else if (strcmp(tok[0], "watch") == 0) {
			/* Watch a certain PID */
			int pid;

			if (ntok != 2)
				error("devtrace: usage: watch [0|<PID>]");
			pid = atoi(tok[1]);
			if (pid == 0) {
				pidwatch = malloc(sizeof(int)*PIDWATCHSIZE);
				numpids = 0;
			} else if (pid < 0) {
				error("PID must be greater than zero.");
			} else if (numpids < PIDWATCHSIZE) {
				pidwatch[numpids] = pid;
				ainc(&numpids);
			} else
				error("pidwatch array full!");
		} else if (strcmp(tok[0], "start") == 0) {
			if (ntok != 1)
				error("devtrace: usage: start");
			saveactive = 1;
		} else if (strcmp(tok[0], "stop") == 0) {
			if (ntok != 1)
				error("devtrace: usage: stop");
			saveactive = 0;
			all = 0;
		} else if (strcmp(tok[0], "all") == 0) {
			if (ntok != 1)
				error("devtrace: usage: all");
			saveactive = 1;
			all = 1;
		} else
			error("devtrace:  usage: 'trace' [ktextaddr|name] "
				"'on'|'off'|'mk'|'del' [name] or:  'size' "
				"buffersize (power of 2)");
		free(s);
		break;
	}
	poperror();
	qunlock(&traceslock);
	traceactive = saveactive;
	return n;
}

Dev tracedevtab = {
	'T',
	"trace",
	devreset,
	devinit,
	devshutdown,
	traceattach,
	tracewalk,
	tracestat,
	traceopen,
	devcreate,
	traceclose,
	traceread,
	devbread,
	tracewrite,
	devbwrite,
	devremove,
	devwstat,
};
