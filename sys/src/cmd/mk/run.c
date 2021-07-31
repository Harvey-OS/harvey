#include	"mk.h"

typedef struct Event
{
	int pid;
	Job *job;
} Event;
static Event *events;
static int nevents, nrunning;
typedef struct Process
{
	int pid;
	int status;
	struct Process *b, *f;
} Process;
static Process *phead, *pfree;
static void sched(void);
static void pnew(int, int), pdelete(Process *);
static Envy *envy;
static int special;
#define	ENVQUANTA	10
static int envsize;
static int nextv;

static int pidslot(int);

int
Execl(char *p, char *a, ...)
{
	if (envy)
		exportenv(envy);
	exec(p, &a);
	return -1;
}

void
run(Job *j)
{
	Job *jj;

	if(jobs){
		for(jj = jobs; jj->next; jj = jj->next)
			;
		jj->next = j;
	} else 
		jobs = j;
	j->next = 0;
	/* this code also in waitup after parse redirect */
	if(nrunning < nproclimit)
		sched();
}

static void
sched(void)
{
	Job *j;
	Bufblock *buf;
	int slot, pip[2], pid;
	Node *n;

	if(jobs == 0){
		account();
		return;
	}
	j = jobs;
	jobs = j->next;
	if(DEBUG(D_EXEC))
		fprint(1, "firing up job for target %s\n", wtos(j->t));
	slot = nextslot();
	events[slot].job = j;
	dovars(j, slot);
	buf = newbuf();
	shprint(j->r->recipe, envy, buf);
	if(!tflag && (nflag || !(j->r->attr&QUIET)))
		Bwrite(&stdout, buf->start, (long)strlen(buf->start));
	freebuf(buf);
	if(nflag||tflag){
		for(n = j->n; n; n = n->next){
			if(tflag){
				if(!(n->flags&VIRTUAL))
					touch(n->name);
				else if(explain)
					Bprint(&stdout, "no touch of virtual '%s'\n", n->name);
			}
			n->time = time((long *)0);
			MADESET(n, MADE);
		}
	} else {
/*Bprint(&stdout, "recipe='%s'\n", j->r->recipe);/**/
		Bflush(&stdout);
		if(j->r->attr&RED){
			if(pipe(pip) < 0){
				perror("pipe");
				Exit();
			}
		}
		if((pid = rfork(RFPROC|RFFDG|RFENVG)) < 0){
			perror("mk rfork");
			Exit();
		}
		if(pid == 0){
			if(j->r->attr&RED){
				close(pip[0]);
				DUP2(pip[1], 1);
				close(pip[1]);
			}
			if(pipe(pip) < 0){
				perror("pipe-i");
				Exit();
			}
			if((pid = fork()) < 0){
				perror("mk fork");
				Exit();
			}
			if(pid != 0){
				close(pip[1]);
				DUP2(pip[0], 0);
				close(pip[0]);
				if(j->r->attr&NOMINUSE)

					Execl(shell, shellname, "-I", (char *)0);
				else
					Execl(shell, shellname, "-eI", (char *)0);
				perror(shell);
				NOCLOSEEXIT(1);
			} else {
				int k;
				char *s, *send;

				close(pip[0]);
				s = j->r->recipe;
				send = s+strlen(s);
				while(s < send){
					if((k = write(pip[1], s, send-s)) < 0)
						break;
					s += k;
				}
				NOCLOSEEXIT(0);
			}
		}
		account();
		nrunning++;
		if(j->r->attr&RED)
			close(pip[1]), j->fd = pip[0];
		else
			j->fd = -1;
		if(DEBUG(D_EXEC))
			fprint(1, "pid for target %s = %d\n", wtos(j->t), pid);
		events[slot].pid = pid;
	}
}

int
waitup(int echildok, int *retstatus)
{
	int pid;
	int slot;
	Symtab *s;
	Word *w;
	Job *j;
	char buf[64];
	Bufblock *bp;
	int uarg = 0;
	int done;
	Node *n;
	Waitmsg wm;
	Process *p;
	extern int errno, runerrs;

	/* first check against the proces slist */
	if(retstatus)
		for(p = phead; p; p = p->f)
			if(p->pid == *retstatus){
				*retstatus = p->status;
				pdelete(p);
				return(-1);
			}
again:		/* rogue processes */
	if((pid = wait(&wm)) < 0){
		if(echildok > 0)
			return(1);
		else {
			fprint(2, "mk: (waitup %d) ", echildok);
			perror("mk wait");
			Exit();
		}
	}
	if(DEBUG(D_EXEC))
		fprint(1, "waitup got pid=%d, status=%s\n", pid, wm.msg);
	if(retstatus && (pid == *retstatus)){
		*retstatus = wm.msg[0]? 1:0;
		return(-1);
	}
	slot = pidslot(pid);
	if(slot < 0){
		if(DEBUG(D_EXEC))
			fprint(2, "mk: wait returned unexpected process %d\n", pid);
		pnew(pid, wm.msg[0]? 1:0);
		goto again;
	}
	j = events[slot].job;
	account();
	nrunning--;
	events[slot].pid = -1;
	if(wm.msg[0]){
		dovars(j, slot);
		bp = newbuf();
		shprint(j->r->recipe, envy, bp);
		front(bp->start);
		fprint(2, "mk: %s: exit status=%s", bp->start, wm.msg);
		freebuf(bp);
#ifdef UNIX
		status &= 0xFF;
		if(status&0x7F)
			fprint(2, " signal=%d", status&0x7F);
		if(status&0x80)
			fprint(2, ", core dumped");
#endif
		for(n = j->n, done = 0; n; n = n->next)
			if(n->flags&DELETE){
				if(done++ == 0)
					fprint(2, ", deleting");
				fprint(2, " '%s'", n->name);
			}
		fprint(2, "\n");
		for(n = j->n, done = 0; n; n = n->next)
			if(n->flags&DELETE){
				if(done++ == 0)
					/*Fflush(2)*/;
				delete(n->name);
			}
		if(kflag){
			runerrs++;
			uarg = 1;
		} else {
			jobs = 0;
			Exit();
		}
	}
	if(j->fd >= 0){
		sprint(buf, "process %d", pid);
		parse(buf, j->fd, 0, 0);
		execinit();	/* reread environ */
		nproc();
		while(jobs && (nrunning < nproclimit))
			sched();
	}
	for(w = j->t; w; w = w->next){
		if((s = symlook(w->s, S_NODE, (char *)0)) == 0)
			continue;	/* not interested in this node */
		update(uarg, (Node *)s->value);
	}
	if(nrunning < nproclimit)
		sched();
	return(0);
}

enum {
	TARGET,
	STEM,
	PREREQ,
	PID,
	NPROC,
	NEWPREREQ,
	ALLTARGET,
	STEM0,
	STEM1,
	STEM2,
	STEM3,
	STEM4,
	STEM5,
	STEM6,
	STEM7,
	STEM8,
	STEM9,
};

struct Myenv {
	char *name;
	Word w;
} myenv[] =
{
	[TARGET]	"target",	{"",0},		/* really sleazy */
	[STEM]		"stem",		{"",0},
	[PREREQ]	"prereq",	{"",0},
	[PID]		"pid",		{"",0},
	[NPROC]		"nproc",	{"",0},
	[NEWPREREQ]	"newprereq",	{"",0},
	[ALLTARGET]	"alltarget",	{"",0},
	[STEM0]		"stem0",	{"",0},	/* retain order of rest */
	[STEM1]		"stem1",	{"",0},
	[STEM2]		"stem2",	{"",0},
	[STEM3]		"stem3",	{"",0},
	[STEM4]		"stem4",	{"",0},
	[STEM5]		"stem5",	{"",0},
	[STEM6]		"stem6",	{"",0},
	[STEM7]		"stem7",	{"",0},
	[STEM8]		"stem8",	{"",0},
	[STEM9]		"stem9",	{"",0},
			0,		{0,0},
};

void
execinit(void)
{
	struct Myenv *mp;

	nextv = 0;		/* fill env from beginning */
	vardump();
	special = nextv-1;	/* pointer to last original env*/
	for (mp = myenv; mp->name; mp++)
		symlook(mp->name, S_WESET, "");
}

int
internalvar(char *name, Word *w)
{
	struct Myenv *mp;

	for (mp = myenv; mp->name; mp++)
		if (strcmp(name, mp->name) == 0) {
			mp->w.s = w->s;
			mp->w.next = w->next;
			return 1;
		}
	return 0;
}

void
dovars(Job *j, int slot)
{
	int c;
	char *s;
	struct Myenv *mp;
	int i;
	char buf[20];

	nextv = special;
	envinsert(myenv[TARGET].name, j->t);
	envinsert(myenv[STEM].name, &myenv[STEM].w);
	if(j->r->attr&REGEXP) {			/* memory leak */
		if (j->match[1].sp && j->match[1].ep) {
			s = j->match[1].ep+1;
			c = *s;
			myenv[STEM].w.s = strdup(j->match[1].sp);
			*s = c;
		} else
			myenv[STEM].w.s = "";
	} else
		myenv[STEM].w.s = j->stem;
	envinsert(myenv[PREREQ].name, j->p);
	sprint(buf, "%d", getpid());
	myenv[PID].w.s = strdup(buf);		/* memory leak */
	envinsert(myenv[PID].name, &myenv[PID].w);
	sprint(buf, "%d", slot);
	myenv[NPROC].w.s = strdup(buf);		/* memory leak */
	envinsert(myenv[NPROC].name, &myenv[NPROC].w);
	envinsert(myenv[NEWPREREQ].name, j->np);
	envinsert(myenv[ALLTARGET].name, j->at);
	mp = &myenv[STEM0];		/* careful - assumed order*/
	for(i = 0; i <= 9; i++){
		if((j->r->attr&REGEXP) && j->match[i].sp && j->match[i].ep) {
			s = j->match[i].ep;
			c = *s;
			*s = 0;
			mp->w.s = strdup(j->match[i].sp);	/*leak*/
			*s = c;
			envinsert(mp->name, &mp->w);
		} else 
			envinsert(mp->name, 0);
		mp++;
	}
	envinsert(0, 0);
}

void
nproc(void)
{
	Symtab *sym;
	Word *w;

	if(sym = symlook("NPROC", S_VAR, (char *)0)) {
		w = (Word *) sym->value;
		if (w && w->s && w->s[0])
			nproclimit = atoi(w->s);
	}
	if(nproclimit < 1)
		nproclimit = 1;
	if(DEBUG(D_EXEC))
		fprint(1, "nprocs = %d\n", nproclimit);
	if(nproclimit > nevents){
		if(nevents)
			events = (Event *)Realloc((char *)events, nproclimit*sizeof(Event));
		else
			events = (Event *)Malloc(nproclimit*sizeof(Event));
		while(nevents < nproclimit)
			events[nevents++].pid = 0;
	}
}

int
nextslot(void)
{
	int i;

	for(i = 0; i < nproclimit; i++)
		if(events[i].pid <= 0) return i;
	assert("out of slots!!", 0);
	return 0;	/* cyntax */
}

int
pidslot(int pid)
{
	int i;

	for(i = 0; i < nevents; i++)
		if(events[i].pid == pid) return(i);
	if(DEBUG(D_EXEC))
		fprint(2, "mk: wait returned unexpected process %d\n", pid);
	return(-1);
}


static void
pnew(int pid, int status)
{
	Process *p;

	if(pfree){
		p = pfree;
		pfree = p->f;
	} else
		p = (Process *)Malloc(sizeof(Process));
	p->pid = pid;
	p->status = status;
	p->f = phead;
	phead = p;
	if(p->f)
		p->f->b = p;
	p->b = 0;
}

static void
pdelete(Process *p)
{
	if(p->f)
		p->f->b = p->b;
	if(p->b)
		p->b->f = p->f;
	else
		phead = p->f;
	p->f = pfree;
	pfree = p;
}

static long tslot[1000];
static long tick;

void
account(void)
{
	long t;

	time(&t);
	if(tick)
		tslot[nrunning] += (t-tick);
	tick = t;
}

void
killchildren(char *msg)
{
	Process *p;
	char buf[64];
	int fd;

	for(p = phead; p; p = p->f){
		sprint(buf, "/proc/%d/notepg", p->pid);
		fd = open(buf, OWRITE);
		if(fd >= 0){
			write(fd, msg, strlen(msg));
			close(fd);
		}
	}
}
/*
 *	execute a shell command capturing the output into the buffer.
 */
void			
rcexec(char *cstart, char *cend, Bufblock *buf)
{
	int childin[2], childout[2], pid;
	int tot, n;

	Bflush(&stdout);
	if(pipe(childin) < 0){
		SYNERR(-1); perror("pipe1");
		Exit();
	}
	if(pipe(childout) < 0){
		SYNERR(-1); perror("pipe2");
		Exit();
	}
	if((pid = rfork(RFPROC|RFFDG|RFENVG)) < 0){
		SYNERR(-1); perror("fork");
		Exit();
	}
	if(pid){	/* parent */
		close(childin[0]);
		close(childout[1]);
		if(cstart < cend)
			write(childin[1], cstart, cend-cstart+1);
		close(childin[1]);
		tot = n = 0;
		do {
			tot += n;
			buf->current += n;
			if (buf->current >= buf->end)
				growbuf(buf);
		} while((n = read(childout[0], buf->current, buf->end-buf->current)) > 0);
		if (tot && buf->current[-1] == '\n')
			buf->current--;
		close(childout[0]);
	} else {
		DUP2(childin[0], 0);
		DUP2(childout[1], 1);
		close(childin[0]);
		close(childin[1]);
		close(childout[0]);
		close(childout[1]);
		if(Execl(shell, shellname, "-I", (char *)0) < 0)
				perror(shell);
		NOCLOSEEXIT(1);
	}
}

void
envinsert(char *name, Word *value)
{
	if (nextv >= envsize) {
		envsize += ENVQUANTA;
		envy = (Envy *) Realloc((char *) envy, envsize*sizeof(Envy));
	}
	envy[nextv].name = name;
	envy[nextv++].values = value;
}
