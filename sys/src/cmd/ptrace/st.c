#include "all.h"
/*
 * The main data structures are kept here.
 *
 * All events are converted into St (sched trace)
 * structures and kept allocated at graph[]
 * If input is from device, this array is sorted by time.
 * Most other data structures rely on indexes on graph[].
 *
 * All events for a process are linked together using the
 * St.pnext index.
 *
 * There is a Proc per process, with an array of indexes in graph[]
 * for events that changed state. These are usually log of the
 * size of the trace for the process.
 * 
 * For browsing, each proc is noted with the row used and
 * the first and last slots in Proc.state shown in the current view.
 * This narrows the events to be considered for a plot.
 *
 * Starting with a Proc.state[i], all following events can
 * be retrieved by following the St.pnext-linked list.
 *
 * As a further aid, the code plotting processes decorates
 * St's with the x for its start point, so that we can scan
 * a series of St's (or state[]s) and determine which one is
 * at the mouse.
 *
 */
typedef struct Name Name;
struct Name
{
	int pid;
	char name[10];
};

static int lno = 1;
static char *fname;
St **graph;
int ngraph;

static Name *name;
static int nname;

Proc *proc;
int nproc;

static char *etname[Nevent] = {
	[SAdmit] =	"Admit",
	[SRelease] =	"Release",
	[SEdf] =	"Edf",
	[SRun] =	"Run",
	[SReady] =	"Ready",
	[SSleep] =	"Sleep",
	[SYield] =	"Yield",
	[SSlice] =	"Slice",
	[SDeadline] =	"Deadline",
	[SExpel] =	"Expel",
	[SDead] =	"Dead",
	[SInts] =	"Ints",
	[SInte] =	"Inte",
	[STrap] =	"Trap",
	[SUser] = 	"User",
	[SName]	=	"Name",
};

/* Should be generated */
static char *ssnames[] =
{
	"Dead",
	"Moribund",
	"Ready",
	"Scheding",
	"Running",
	"Queueing",
	"QueueingR",
	"QueueingW",
	"Wakeme",
	"Broken",
	"Stopped",
	"Rendez",
	"Waitrelease",
};

static char*
ssname(int i)
{
	if(i >= 0 && i < nelem(ssnames))
		return ssnames[i];
	return "unknown";
}

static void
error(char* msg, ...)
{
	char	buf[500];
	va_list	arg;

	va_start(arg, msg);
	vseprint(buf, buf+sizeof(buf), msg, arg);
	va_end(arg);
	Bflush(bout);
	fprint(2, "%s: %s:%d: %s\n", argv0, fname, lno, msg);
}

int
Tfmt(Fmt *f)
{
	vlong t, i;
	static char *u[] = {"s", "m", "µ", "n"};
	static vlong d[] = {S(1), MS(1), US(1), 1};
	static char spc[3+2+1];

	t = va_arg(f->args, vlong);
	if((f->flags&FmtSharp) == 0)
		return fmtprint(f, "%011lld", t);

	if(spc[0] == 0)
		memset(spc, ' ', sizeof spc - 1);

	for(i = 0; i < nelem(u); i++){
		fmtprint(f, "%3lld%s ", t/d[i], u[i]);
		t %= d[i];
	}
	return 0;
}

/*
 * This tries to combine printing the state information for
 * the text file format so it keeps all the information and
 * is easy to parse, with a compact representation amenable
 * for an editor.
 * %#G is intended to print the state in the window.
 * %G is intended to print the full state for the file.
 * If the output of %G is changed, readgraph() must be updated
 * to parse it properly.
 * As of now it uses the (awk) fields 1, 2, 3, 4, NF-1, and NF.
 * all other fields are ignored for reading.
 */
int
Gfmt(Fmt *f)
{
	St *g;
	vlong arg;
	u64int addr;
	extern char *scname[];
	extern int maxscval;

	g = va_arg(f->args, St*);
	if(f->flags&FmtSharp)
		fmtprint(f, " %s", etname[g->state]);
	else{
		fmtprint(f, "%T %6d %02d", g->time, g->pid, g->machno);
		fmtprint(f, " %-4s", (g->name && g->name[0]) ? g->name : "_");
		fmtprint(f, " %-6s", etname[g->state]);
	}
	switch(g->etype){
	case SSleep:
		arg = g->arg&0xFF;
		fmtprint(f, " %s pc %#ullx", ssname(arg), g->arg>>8);
		if(f->flags&FmtSharp)
			return 0;
		break;
	case STrap:
		arg = g->arg;
		addr = g->arg & STrapMask;
		if(arg&STrapRPF)
			fmtprint(f, " %-10s r %#ullx", "pfault", addr);
		else if(arg&STrapWPF)
			fmtprint(f, " %-10s w %#ullx", "pfault", addr);
		else if(arg&STrapSC){
			arg &= ~STrapSC;
			if(arg <= maxscval)
				fmtprint(f, " %-10s", scname[arg]);
			else
				fmtprint(f, " sc%ulld", arg);
		}else
			fmtprint(f, " %-10s", etname[g->etype]);
		if(f->flags&FmtSharp)
			return 0;
		break;
	default:
		fmtprint(f, " %-10s", etname[g->etype]);
	}

	if(f->flags&FmtSharp)
		return fmtprint(f, " %#ullx", g->arg);
	else
		return fmtprint(f, " %2d %#ullx", g->etype, g->arg);
}

static void
addname(St *g)
{
	int i;

	for(i = 0; i < nname; i++)
		if(name[i].pid == 0 || name[i].pid == g->pid)
			break;
	if(i == nname){
		if((nname%Incr) == 0){
			name = realloc(name, (nname+Incr)*sizeof name[0]);
			if(name == nil)
				sysfatal("malloc: %r");
		}
		nname++;
	}
	name[i].pid = g->pid;
	assert(sizeof g->arg <= sizeof name[i].name);
	memmove(name[i].name, &g->arg, sizeof g->arg);
	name[i].name[sizeof g->arg] = 0;
}

static char*
pidname(int pid)
{
	int i;

	for(i = 0; i < nname; i++)
		if(name[i].pid == pid)
			return name[i].name;
	return "_";
}

static St*
readdev(Biobuf *bin)
{
	uchar buf[PTsize], *p;
	long nr;
	uvlong x;
	St *g;

	nr = Bread(bin, buf, sizeof buf);
	if(nr == 0)
		return nil;
	if(nr < 0){
		error("read: %r");
		return nil;
	}
	if(nr != sizeof buf){
		error("wrong event size");
		return nil;
	}
	lno++;	/* count events instead of lines */

	g = mallocz(sizeof *g, 1);
	if(g == nil)
		sysfatal("no mem");
	g->pid = le32get(buf, &p);
	g->etype = le32get(p, &p);
	g->state = g->etype;
	g->machno = le32get(p, &p);
	x = le64get(p, &p);
	g->time = x;
	g->arg = le64get(p, &p);
	if(g->etype == SName)
		addname(g);
	g->name = pidname(g->pid);

	if(verb)
		fprint(2, "pid %ud etype %ud mach %ud time %lld arg %lld\n",
			g->pid, g->etype, g->machno, g->time, g->arg);
	return g;
}

static St*
readfile(Biobuf *bin)
{
	char *s;
	char *toks[18];
	int ntoks, et;
	St* g;

	while((s = Brdstr(bin, '\n', 1)) != nil){
		lno++;
		if(s[0] == '#')
			goto next;
		ntoks = tokenize(s, toks, nelem(toks));
		if(ntoks == 0)
			goto next;
		if(ntoks < 8){
			error("wrong event at %s\n", toks[0]);
			goto next;
		}
		g = mallocz(sizeof *g, 1);
		if(g == nil)
			sysfatal("no memory");
		g->time = (vlong)strtoull(toks[0], nil, 10);
		g->pid = strtoul(toks[1], nil, 0);
		g->machno = strtoul(toks[2], nil, 0);
		g->name = strdup(toks[3]);

		et = strtoul(toks[ntoks-2], 0, 0);
		if(et < 0 || et >= Nevent || etname[et] == nil){
			error("unknown state id %d\n", et);
			goto next;
		}
		g->etype = et;
		g->state = et;
		g->arg = strtoull(toks[ntoks-1], nil, 0);
		free(s);
		return g;
	next:
		free(s);
	}
	return nil;
}

static void
addgraph(St *g)
{
	if((ngraph%Incr) == 0){
		graph = realloc(graph, (ngraph+Incr)*sizeof graph[0]);
		if(graph == nil)
			sysfatal("malloc: %r");
	}
	graph[ngraph++] = g;
}

static int
graphcmp(void *e1, void *e2)
{
	St **s1, **s2;
	vlong d;

	s1 = e1;
	s2 = e2;
	d = (*s1)->time - (*s2)->time;
	if(d < 0)
		return -1;
	if(d > 0)
		return 1;
	return 0;
}

void
readall(char *f, int isdev)
{
	Biobuf *bin;
	St *g;
	St *(*rd)(Biobuf*);
	vlong t0;
	int i;

	fname = f;
	bin = Bopen(f, OREAD);
	if(bin == nil)
		sysfatal("%s: Bopen: %r", f);

	if(isdev)
		rd = readdev;
	else
		rd = readfile;

	while((g = rd(bin)) != nil)
		addgraph(g);
	Bterm(bin);

	if(ngraph == 0)
		return;

	if(isdev)
		qsort(graph, ngraph, sizeof graph[0], graphcmp);

	t0 = graph[0]->time;
	for(i = 0; i < ngraph; i++)
		graph[i]->time -= t0;
}

static Proc*
getproc(St *g)
{
	int i;

	for(i = 0; i < nproc; i++)
		if(proc[i].pid == g->pid)
			break;
	if(i == nproc){
		if((nproc%Incr) == 0){
			proc = realloc(proc, (nproc+Incr)*sizeof proc[0]);
			if(proc == nil)
				sysfatal("malloc: %r");
		}
		memset(&proc[nproc], 0, sizeof proc[nproc]);
		nproc++;
	}
	proc[i].pid = g->pid;
	return &proc[i];
}

St*
pgraph(Proc *p, int i)
{
	int id;

	if(i < 0 || i >= p->nstate)
		return nil;
	id = p->state[i];
	if(id >= ngraph)
		return nil;
	return graph[id];
}

static void
appstate(Proc *p, int i)
{
		if((p->nstate%Incr) == 0){
			p->state = realloc(p->state, (p->nstate+Incr)*sizeof p->state[0]);
			if(p->state == nil)
				sysfatal("malloc: %r");
		}
		p->state[p->nstate++] = i;
}

void
makeprocs(void)
{
	Proc *p, *lastp;
	St *g, *sg;
	int i, j;

	lastp = nil;
	for(i = 0; i < ngraph; i++){
		g = graph[i];
		if(lastp != nil && lastp->pid == g->pid)
			p = lastp;
		else
			p = getproc(g);

		if(p->pnextp != nil)
			*p->pnextp = i;
		p->pnextp = &g->pnext;

		switch(g->etype){
		case SReady:
		case SRun:
		case SSleep:
		case SDead:
			appstate(p, i);
			break;
		default:
			if(p->nstate == 0){
				appstate(p, i);
				g->state= SRun;
			}else{
				sg = graph[p->state[p->nstate-1]];
				g->state = sg->state;
			}
		}
		lastp = p;
	}

	if(0)
	for(i = 0; i < nproc; i++){
		p = &proc[i];
		Bprint(bout, "proc pid %d:\n", p->pid);
		for(j = 0; j < p->nstate; j++)
			Bprint(bout, "%G\n", graph[p->state[j]]);
		Bprint(bout, "\n\n");
	}
}

