#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

Alarm*	alarmtab;
Talarm	talarm;

/*
 * Insert new into list after where
 */
static void
insert(Alarm **head, Alarm *where, Alarm *new)
{
	if(where == 0){
		new->next = *head;
		*head = new;
	}else{
		new->next = where->next;
		where->next = new;
	}
		
}

/*
 * Delete old from list.  where->next is known to be old.
 */
static void
delete(Alarm **head, Alarm *where, Alarm *old)
{
	if(where == 0){
		*head = old->next;
		return;
	}
	where->next = old->next;
}

Alarm*
alarm(int ms, void (*f)(Alarm*, void*), void *arg)
{
	Alarm *a, *w, *pw;
	ulong s;

	if(ms < 0)
		ms = 0;
	a = newalarm();
	a->dt = MS2TK(ms);
	a->f = f;
	a->arg = arg;
	s = splhi();
	lock(&m->alarmlock);
	pw = 0;
	for(w=m->alarm; w; pw=w, w=w->next){
		if(w->dt <= a->dt){
			a->dt -= w->dt;
			continue;
		}
		w->dt -= a->dt;
		break;
	}
	insert(&m->alarm, pw, a);
	unlock(&m->alarmlock);
	splx(s);
	return a;
}

void
cancel(Alarm *a)
{
	a->f = 0;
}

Alarm*
newalarm(void)
{
	int i;
	Alarm *a;

	for(i=0,a=alarmtab; i<conf.nalarm; i++,a++)
		if(a->busy==0 && a->f==0 && canlock(a)){
			if(a->busy){
				unlock(a);
				continue;
			}
			a->f = 0;
			a->arg = 0;
			a->busy = 1;
			unlock(a);
			return a;
		}
	panic("newalarm");
	return 0;
}

void
alarminit(void)
{
	extern char etext[];

	cons.minpc = KTZERO;
	cons.maxpc = (ulong)etext;
	cons.nprofbuf = (cons.maxpc-cons.minpc) >> LRES;
	cons.profbuf = ialloc(cons.nprofbuf*sizeof(cons.profbuf[0]), 0);

	alarmtab = ialloc(conf.nalarm*sizeof(Alarm), 0);
}

static
struct
{
	int	nfilter;
	Filter*	filters[1000];
	int	time;
	int	index;
	int	cons;
} f;

void
dofilter(Filter *ft, int c1, int c2, int c3)
{
	int i;

	i = f.nfilter;
	if(i >= nelem(f.filters)) {
		print("dofilter: too many filters\n");
		return;
	}
	ft->c1 = c1;
	ft->c2 = c2;
	ft->c3 = c3;
	f.filters[i] = ft;
	f.nfilter = i+1;
}

void
checkalarms(void)
{
	User *p;
	ulong now;

	if(talarm.list == 0 || canlock(&talarm) == 0)
		return;

	now = MACHP(0)->ticks;
	for(;;) {
		p = talarm.list;
		if(p == 0)
			break;

		if(p->twhen == 0) {
			talarm.list = p->tlink;
			p->trend = 0;
			continue;
		}
		if(now < p->twhen)
			break;
		wakeup(p->trend);
		talarm.list = p->tlink;
		p->trend = 0;
	}

	unlock(&talarm);
}

void
processfilt(int n)
{
	int i;
	Filter *ft;
	ulong c0, c1;

	n += f.index;
	if(n > f.nfilter)
		n = f.nfilter;

	for(i=f.index; i<n; i++) {
		ft = f.filters[i];
		c0 = ft->count;
		c1 = c0 - ft->oldcount;
		ft->oldcount = c0;
		if(ft->c2)
			ft->filter = famd(ft->filter, c1, ft->c1, ft->c2);
	}
	f.index = n;
}

void
clock(ulong n, ulong pc)
{
	int i;
	Alarm *a;
	void (*fn)(Alarm*, void*);
	User *p;

	clockreload(n);
	m->ticks++;

	if(cons.profile) {
		cons.profbuf[0] += TK2MS(1);
		if(cons.minpc<=pc && pc<cons.maxpc){
			pc -= cons.minpc;
			pc >>= LRES;
			cons.profbuf[pc] += TK2MS(1);
		} else
			cons.profbuf[1] += TK2MS(1);
	}

	lights(Lreal, (m->ticks>>6)&1);
	if(m->machno == 0){
		if(f.cons >= 0) {
			(*consputc)(f.cons);
			f.cons = -1;
		}
		p = m->proc;
		if(p == 0)
			p = m->intrp;
		if(p) {
			p->time[0].count += TK2MS(1);
			p->time[1].count += TK2MS(1);
			p->time[2].count += TK2MS(1);
		}
		for(i=1; i<conf.nmach; i++){
			if(active.machs & (1<<i)){
				p = MACHP(i)->proc;
				if(p && p != m->intrp) {
					p->time[0].count += TK2MS(1);
					p->time[1].count += TK2MS(1);
					p->time[2].count += TK2MS(1);
				}
			}
		}
		m->intrp = 0;

		f.time += TK2MS(1);
		processfilt(TK2SEC(f.nfilter));
		while(f.time >= 1000) {
			f.time -= 1000;
			processfilt(f.nfilter-f.index);
			f.index = 0;
		}
	}

	if(active.exiting && active.machs&(1<<m->machno)){
		print("someone's exiting\n");
		exit();
	}

	checkalarms();

	if(canlock(&m->alarmlock)){
		if(m->alarm){
			a = m->alarm;
			a->dt--;
			while(a && a->dt<=0){
				fn = a->f;	/* avoid race with cancel */
				if(fn)
					(*fn)(a, a->arg);
				delete(&m->alarm, 0, a);
				a->busy = 0;
				a = m->alarm;
			}
		}
		unlock(&m->alarmlock);
	}
}

void
consstart(int c)
{
	f.cons = c;
}
