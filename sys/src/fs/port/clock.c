#include	"all.h"
#include	"mem.h"
#include	"io.h"
#include	"ureg.h"

Alarm	*alarmtab;

/*
 * Insert new into list after where
 */
static void
insert(List **head, List *where, List *new)
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
delete(List **head, List *where, List *old)
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
	a->dt = ms/MS2HZ;
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
	Filter*	filters[100];
	int	time;
	int	duart;
} f;

void
dofilter(Filter *ft)
{
	int i;

	i = f.nfilter;
	if(i >= sizeof(f.filters)/sizeof(f.filters[0])) {
		print("dofilter: too many filters\n");
		return;
	}
	f.filters[i] = ft;
	f.nfilter = i+1;
}

void
clock(ulong n, ulong pc)
{
	int i;
	Alarm *a;
	void (*fn)(Alarm*, void*);
	User *p;
	Filter *ft;
	ulong c0, c1;

	clockreload(n);
	m->ticks++;

	if(cons.profile) {
		cons.profbuf[0] += MS2HZ;
		if(cons.minpc<=pc && pc<cons.maxpc){
			pc -= cons.minpc;
			pc >>= LRES;
			cons.profbuf[pc] += MS2HZ;
		} else
			cons.profbuf[1] += MS2HZ;
	}

	lights(Lreal, (m->ticks>>6)&1);
	if(m->machno == 0){
		if(f.duart >= 0) {
			duartxmit(f.duart);
			f.duart = -1;
		}
		p = m->proc;
		if(p == 0)
			p = m->intrp;
		if(p)
			p->time.count += MS2HZ;
		for(i=1; i<conf.nmach; i++){
			if(active.machs & (1<<i)){
				p = MACHP(i)->proc;
				if(p && p != m->intrp)
					p->time.count += MS2HZ;
			}
		}
		m->intrp = 0;

		f.time += MS2HZ;
		while(f.time >= 1000) {
			f.time -= 1000;
			for(i=0; i<f.nfilter; i++) {
				ft = f.filters[i];
				c0 = ft->count;
				c1 = c0 - ft->oldcount;
				ft->oldcount = c0;
				ft->filter[0] = famd(ft->filter[0], c1, 59, 60);
				ft->filter[1] = famd(ft->filter[1], c1, 599, 600);
				ft->filter[2] = famd(ft->filter[2], c1, 5999, 6000);
			}
		}
	}
	if(active.exiting && active.machs&(1<<m->machno)){
		print("someone's exiting\n");
		exit();
	}
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
duartstart(int c)
{
	f.duart = c;
}
