#include	"u.h"
#include	"lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#define	MAXALARM	10

Alarm	alarmtab[MAXALARM];

/*
 * Insert new into list after where
 */
void
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
void
delete(List **head, List *where, List *old)
{
	if(where == 0){
		*head = old->next;
		return;
	}
	where->next = old->next;
}

Alarm*
newalarm(void)
{
	int i;
	Alarm *a;

	for(i=0,a=alarmtab; i < nelem(alarmtab); i++,a++)
		if(a->busy==0 && a->f==0){
			a->f = 0;
			a->arg = 0;
			a->busy = 1;
			return a;
		}
	panic("newalarm");
	return 0;	/* not reached */
}

Alarm*
alarm(int ms, void (*f)(Alarm*), void *arg)
{
	Alarm *a, *w, *pw;
	ulong s;

	if(ms < 0)
		ms = 0;
	s = splhi();
	a = newalarm();
	a->dt = MS2TK(ms);
	a->f = f;
	a->arg = arg;
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
	splx(s);
	return a;
}

void
cancel(Alarm *a)
{
	a->f = 0;
}

void
alarminit(void)
{
}

#define NA 10		/* alarms per clock tick */
void
checkalarms(void)
{
	int i, n, s;
	Alarm *a;
	void (*f)(Alarm*);
	Alarm *alist[NA];

	s = splhi();
	a = m->alarm;
	if(a){
		for(n=0; a && a->dt<=0 && n<NA; n++){
			alist[n] = a;
			delete(&m->alarm, 0, a);
			a = m->alarm;
		}
		if(a)
			a->dt--;

		for(i = 0; i < n; i++){
			f = alist[i]->f;	/* avoid race with cancel */
			if(f)
				(*f)(alist[i]);
			alist[i]->busy = 0;
		}
	}
	splx(s);
}
