#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "ctype.h"
#include "../port/error.h"
#include	"realtime.h"
#include	"../port/edf.h"

static char qsep[] = " \t\r\n{}";
static char tsep[] = " \t\r\n";
static uvlong fasthz;	

Task *
findtask(int taskno)
{
	List *l;
	Task *t;

	for (l = tasks.next; l; l = l->next)
		if ((t = l->i) && t->taskno == taskno)
			return t;
	return nil;
}

Resource *
resource(char *name, int add)
{
	Resource *r;
	List *l;

	for (l = resources.next; l; l = l->next)
		if ((r = l->i) && strcmp(r->name, name) == 0)
			return r;
	if (add <= 0)
		return nil;
	r = mallocz(sizeof(Resource), 1);
	if (r == nil)
		error("resource: malloc");
	kstrdup(&r->name, name);
	enlist(&resources, r);
	return r;
}

void
resourcefree(Resource *r)
{
	if (r == nil)
		return;
	if (decref(r))
		return;
	delist(&resources, r);
	assert(r->tasks.n == 0);
	free(r->name);
	free(r);
}

void
removetask(Task *t)
{
	Proc *p;
	Resource *r;
	List *l;

	edf->edfexpel(t);
	while (l = t->procs.next){
		p = l->i;
		assert(p);
		p->task = nil;
		DEBUG("taking proc %lud off task %d\n", p->pid, t->taskno);
		delist(&t->procs, p);
	}
	while (p = t->runq.head){
		/* put runnable procs on regular run queue */
		t->runq.head = p->rnext;
		DEBUG("putting proc %lud on runq\n", p->pid);
		ready(p);
		t->runq.n--;
	}
	t->runq.tail = nil;
	assert(t->runq.n == 0);
	while (l = t->csns.next){
		r = l->i;
		assert(r);
		if (delist(&r->tasks, t)) {
			DEBUG("freeing task %d from resource %s\n", t->taskno, r->name);
			taskfree(t);
		}
		if (delist(&t->csns, r)) {
			DEBUG("freeing resource %s\n", r->name);
			resourcefree(r);
		}
	}
	free(t->user);
	t->user = nil;
	t->state = EdfUnused;
	if (delist(&tasks, t)) {
		DEBUG("freeing task %d\n", t->taskno);
		taskfree(t);
	}
}

char*
parseresource(Head *h, CSN *parent, char *s)
{
	CSN *csn, *q;
	Resource *r;
	char *p, *e, c;
	Time T;
	int recursed;

	csn = nil;
	recursed = 1;
	while(*s != '\0' && *s != '}'){
		/* Start of a token */
		while(*s && utfrune(tsep, *s))
			*s++ = '\0';
		if (*s == '\0' || *s == '}')
			return s;	/* No token found */
		if (*s == '{'){
			/* Recursion */
			if (recursed)
				error("unexpected '{'");
			s = parseresource(h, csn, s+1);
			if (*s != '}')
				error("expected '}'");
			recursed = 1;
			*s++ = '\0';
		}else{
			p = s;
			/* Normal token */
			while(*s && utfrune(qsep, *s) == nil)
				s++;
			assert(s > p);
			c = *s;
			*s = '\0';
			if (strcmp(p, "R") == 0){
				if (csn == nil || csn->R != 0)
					error("unexpected R");
				csn->R = 1;
			}else if (isalpha(*p) || *p == '_'){
				r = resource(p, 1);
				assert(r);
				for (q = parent; q; q = q->p)
					if (q->i == r){
						resourcefree(r);
						error("nested resource");
					}
				csn = mallocz(sizeof(CSN), 1);
				csn->p = parent;
				csn->i = r;
				incref(r);
				DEBUG("Putting resource %s 0x%p in CSN 0x%p on list 0x%p\n",
					r->name, r, csn, h);
				putlist(h, csn);
				recursed = 0;
			}else{
				if (csn == nil || csn->C != 0)
					error("unexpected resource cost");
				if (e=parsetime(&T, p))
					error(e);
				csn->C = time2ticks(T);
			}
			*s = c;
		}
	}
	return s;
}

void
resourcetimes(Task *task, Head *h)
{
	CSN *c;
	Resource *r;
	TaskLink *p;
	Ticks C;

	for(c = (CSN*)h->next; c; c = (CSN*)c->next){
		c->t = task;
		if (c->p)
			C =  c->p->C;
		else
			C =  task->C;
		if (task->flags & BestEffort){
			if (c->C == 0)
				error("no cost specified for resource");
		}else if (c->C > C)
			error("resource uses too much time");
		else if (c->C == 0)
			c->C = C;
		r = c->i;
		assert(r);
		/* Put task on resource's list */
		if(p = (TaskLink*)onlist(&r->tasks, task)){
			if(c->R == 0)
				p->R = 0;
		}else{
			p = mallocz(sizeof(TaskLink), 1);
			p->i = task;
			p->R = c->R;
			putlist(&r->tasks, p);
			incref(task);
		}
	}
}

char *
seprintresources(char *p, char *e)
{
	List *l, *q;
	Resource *r;
	Task *t;

	for (l = resources.next; l; l = l->next){
		r = l->i;
		assert(r);
		p = seprint(p, e, "name=%s", r->name);
		if (r->tasks.n){
			p = seprint(p, e, " tasks='");
			for (q = r->tasks.next; q; q = q->next){
				if (q != r->tasks.next)
					p = seprint(p, e, " ");
				t = q->i;
				p = seprint(p, e, "%d", t->taskno);
			}
			p = seprint(p, e, "'");
		}
		if (r->Delta)
			p = seprint(p, e, " Δ=%T", ticks2time(r->Delta));
		else if (r->testDelta)
			p = seprint(p, e, " testΔ=%T", ticks2time(r->testDelta));
		p = seprint(p, e, "\n");
	}
	return p;
}

char *
seprintcsn(char *p, char *e, Head *h)
{
	CSN *c, *prevc, *parent;
	Resource *r;
	int opens;

	prevc = (CSN*)~0;
	parent = nil;
	opens = 0;
	for(c = (CSN*)h->next; c; c = (CSN*)c->next){
		if (c->p == prevc){
			/* previous is parent: recurse */
			parent = prevc;
			p = seprint(p, e, "{ ");
			opens++;
		}else if (c->p != parent) {
			/* unrecurse */
			parent = parent->p;
			p = seprint(p, e, "} ");
		}
		if (r = c->i)
			p = seprint(p, e, "%s ", r->name);
		else
			p = seprint(p, e, "orphan ");
		if (c->R)
			p = seprint(p, e, "R ");
		if (c->Delta)
			p = seprint(p, e, "Δ=%T ", ticks2time(c->Delta));
		if (c->testDelta)
			p = seprint(p, e, "tΔ=%T ", ticks2time(c->testDelta));
		if (c->C)
			p = seprint(p, e, "C=%T ", ticks2time(c->C));
		prevc = c;
	}
	while (opens--)
		p = seprint(p, e, "} ");
	return p;
}

char*
seprinttask(char *p, char *e, Task *t, Ticks now)
{
	vlong n;
	char c;

	assert(t);
	p = seprint(p, e, "{%s, D%U, Δ%U,T%U, C%U, S%U",
		edfstatename[t->state], t->D, t->Delta, t->T, t->C, t->S);
	n = t->r - now;
	if (n >= 0)
		c = ' ';
	else {
		n = -n;
		c = '-';
	}
	p = seprint(p, e, ", r%c%U", c, (uvlong)n);
	n = t->d - now;
	if (n >= 0)
		c = ' ';
	else {
		n = -n;
		c = '-';
	}
	p = seprint(p, e, ", d%c%U", c, (uvlong)n);
	n = t->t - now;
	if (n >= 0)
		c = ' ';
	else {
		n = -n;
		c = '-';
	}
	p = seprint(p, e, ", t%c%U}", c, (uvlong)n);
	return p;
}

char*
dumpq(char *p, char *e, Taskq *q, Ticks now)
{
	Task *t;

	t = q->head;
	for(;;){
		if (t == nil)
			return seprint(p, e, "\n");
		p = seprinttask(p, e, t, now);
		t = t->rnext;
		if (t)
			seprint(p, e, ", ");
	}
	return nil;
}

static uvlong
uvmuldiv(uvlong x, ulong num, ulong den)
{
	/* multiply, then divide, avoiding overflow */
	uvlong hi;

	hi = (x & 0xffffffff00000000LL) >> 32;
	x &= 0xffffffffLL;
	hi *= num;
	return (x*num + (hi%den << 32)) / den + (hi/den << 32);
}

Time
ticks2time(Ticks ticks)
{
	if (ticks == Infinity)
		return Infinity;
	assert(ticks >= 0);
	if (fasthz == 0)
		fastticks(&fasthz);
	return uvmuldiv(ticks, Onesecond, fasthz);
}

Ticks
time2ticks(Time time)
{
	if (time == Infinity)
		return Infinity;
	assert(time >= 0);
	if (fasthz == 0)
		fastticks(&fasthz);
	return uvmuldiv(time, fasthz, Onesecond);
}

char *
parsetime(Time *rt, char *s)
{
	uvlong ticks;
	ulong l;
	char *e, *p;
	static int p10[] = {100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10, 1};

	if (s == nil)
		return("missing value");
	ticks=strtoul(s, &e, 10);
	if (*e == '.'){
		p = e+1;
		l = strtoul(p, &e, 10);
		if(e-p > nelem(p10))
			return "too many digits after decimal point";
		if(e-p == 0)
			return "ill-formed number";
		l *= p10[e-p-1];
	}else
		l = 0;
	if (*e){
		if(strcmp(e, "s") == 0)
			ticks = 1000000000 * ticks + l;
		else if (strcmp(e, "ms") == 0)
			ticks = 1000000 * ticks + l/1000;
		else if (strcmp(e, "µs") == 0 || strcmp(e, "us") == 0)
			ticks = 1000 * ticks + l/1000000;
		else if (strcmp(e, "ns") != 0)
			return "unrecognized unit";
	}
	*rt = ticks;
	return nil;
}

int
timeconv(Fmt *f)
{
	char buf[128], *sign;
	Time t;
	Ticks ticks;

	buf[0] = 0;
	switch(f->r) {
	case 'U':
		ticks = va_arg(f->args, Ticks);
		t = ticks2time(ticks);
		break;
	case 'T':		// Time in nanoseconds
		t = va_arg(f->args, Time);
		break;
	default:
		return fmtstrcpy(f, "(timeconv)");
	}
	if (t == Infinity)
		return fmtstrcpy(f, "∞");
	if (t < 0) {
		sign = "-";
		t = -t;
	}
	else
		sign = "";
	t += 1;
	if (t > Onesecond)
		sprint(buf, "%s%d.%.3ds", sign, (int)(t / Onesecond), (int)(t % Onesecond)/1000000);
	else if (t > Onemillisecond)
		sprint(buf, "%s%d.%.3dms", sign, (int)(t / Onemillisecond), (int)(t % Onemillisecond)/1000);
	else if (t > Onemicrosecond)
		sprint(buf, "%s%d.%.3dµs", sign, (int)(t / Onemicrosecond), (int)(t % Onemicrosecond));
	else
		sprint(buf, "%s%dns", sign, (int)t);
	return fmtstrcpy(f, buf);
}

int
putlist(Head *h, List *l)
{
	List **pp, *p;

	for(pp = &h->next; p = *pp; pp = &p->next)
		;
	l->next = nil;
	*pp = l;
	h->n++;
	return 1;
}

List *
onlist(Head *h, void *i)
{
	List *l;

	for (l = h->next; l; l = l->next)
		if (l->i == i){
			/* already on the list */
			return l;
		}
	return nil;
}

int
enlist(Head *h, void *i)
{
	List *l;

	if(onlist(h, i))
		return 0;
	l = malloc(sizeof(List));
	if(l == nil)
		panic("malloc in enlist");
	l->i = i;
	return putlist(h, l);
}

int
delist(Head *h, void *i)
{
	List *l, **p;

	for (p = &h->next; l = *p; p = &l->next)
		if (l->i == i){
			*p = l->next;
			free(l);
			h->n--;
			return 1;
		}
	return 0;
}

void *
findlist(Head *h, void *i)
{
	List *l;

	for (l = h->next; l; l = l->next)
		if (l->i == i)
			return i;
	return nil;
}
