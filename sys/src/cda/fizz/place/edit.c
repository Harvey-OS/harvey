#include	<u.h>
#include	<libc.h>
#include	<stdio.h>
#include	<cda/fizz.h>
#include	"host.h"
#include	"proto.h"

Point
Pt(int a, int b)
{
	Point ret;
	ret.x = a;
	ret.y = b;
	return(ret);
}

int
move(int *ns, Point *pp, int n, Point delta)
{
	Point op;
	int ret = 0;
	register Chip *cp;
	register i;

if (dbfp) fprintf(dbfp, "move by %d/%d\n", delta.x, delta.y);
	/* store old point for backup and unplace */
	for(i = 0; i < n; i++){
		cp = cofn(ns[i]);
		pp[i] = cp->pt;
		unplace(cp);
	}
	/* now try to move to new spots */
	for(i = 0; i < n; i++){
		cp = cofn(ns[i]);
		cp->flags &= ~UNPLACED;
		cp->pt = pp[i];
		fg_psub(&cp->pt, delta);
		place(cp);
		if(cp->flags&UNPLACED){
if (dbfp) fprintf(dbfp, "%s at %p failed\n", cp->name, cp->pt.x, cp->pt.y);
			ret = 1;
		}
	}
	/* if we buggered up; reset everyone! */
	if(ret){
		for(i = 0; i < n; i++){
			cp = cofn(ns[i]);
			if(!(cp->flags&UNPLACED))
				unplace(cp);
		}
		for(i = 0; i < n; i++)
			insert(ns[i], cp->flags&WSIDE, pp[i]);
	} else {
		for(i = 0; i < n; i++){
			sendchip(cofn(ns[i]), ns[i]);
			updsig(cofn(ns[i]));
		}
	}
	return(ret);
}

void
insert(int n, int side, Point p)
{
	register Chip *cp;

	cp = cofn(n);
 	if(side)
 		cp->flags |= WSIDE;
 	else
 		cp->flags &= ~WSIDE;
	cp->pt = p;
if (dbfp) fprintf(dbfp, "insert %s(%d) at %d/%d %s side r=%r\n", cp->name, n, cp->pt.x, cp->pt.y, (side?"wire":"comp"),
cp->r);
	cp->flags &= ~(UNPLACED|PLACE);
	place(cp);
	if(cp->flags&UNPLACED){
if (dbfp) fprintf(dbfp, "insert failed\n");
		put1(ERROR);
		putstr("insert errors");
		return;
	}
	sendchip(cp, n);
	updsig(cp);
}

int
findpinsig(Signal ** ss, Point p)
{
	int i, j;
	Pin *ppin;
	Signal *sig;
	while(*ss) {
		for(i = (*ss)->n, ppin = (*ss)->pins; i; --i, ++ppin)
			if((ppin->p.x == p.x) && (ppin->p.y == p.y)) {
if (dbfp) fprintf(dbfp, " %s\n", (*ss)->name);
				return((*ss)->num);
			}
		++ss;
	}
	for(j = 0; j < 6; ++j)
		if(b.v[j].name) {
			sig = (Signal *)symlook(b.v[j].name, S_SIGNAL, (void *) 0);
			if(sig)
				for(i = sig->n, ppin = sig->pins; i; --i, ++ppin)
					if((ppin->p.x == p.x) && (ppin->p.y == p.y)) {
if (dbfp) fprintf(dbfp, " %s\n", b.v[j].name);
						return(-1 - b.v[j].signo);
					}
		}
if (dbfp) fprintf(dbfp, " no con\n");
	return(-8);
}

void
sendpin(int n)
{
	register Chip *c;
	char buf[8];
	Signal **ss;

	sprint(buf, "%d", n);
	c = (Chip *)symlook(buf, S_CHID, (void *)0);
if (dbfp) fprintf(dbfp, "sendpin(%d=%s n=%d):", n, c->name, c->npins);
	ss = (Signal **)symlook(c->name, S_CSMAP, (void *)0);
	put1(PINS);
	putn(n);
	putn((int)c->npins);
	for(n = 0; n < c->npins; n++){
if (dbfp) fprintf(dbfp, " %p", c->pins[n].p.x, c->pins[n].p.y);
		putp(Pt(c->pins[n].p.x - c->r.min.x,  c->r.max.y - c->pins[n].p.y));
		putn(findpinsig(ss, c->pins[n].p));
	}
	flush();
if (dbfp) fprintf(dbfp, "\n");
}

void
sendsig(int n)
{
	register Signal *s;
	char buf[8];

	sprint(buf, "%d", n);
	s = (Signal *)symlook(buf, S_SID, (void *)0);
if (dbfp) fprintf(dbfp, "sendsig(%d=%s n=%d):", n, s->name, s->n);
	if(s->done){
if (dbfp) fprintf(dbfp, " done\n");
		put1(SIG_ND);
		putn(n);
		flush();
		return;
	}
	put1(SIG);
	putn(n);
	wrap(s);
	s->done = 1;
	for(n = 0; n < s->n; n++){
if (dbfp) fprintf(dbfp, " %d/%d", s->pins[n].p.x, s->pins[n].p.y);
		putp(s->pins[n].p);
	}
	flush();
if (dbfp) fprintf(dbfp, "\n");
}

void
sendchsig(int n)
{
	register Signal **ss;
	register Chip *c;
	char buf[8];

	sprint(buf, "%d", n);
	c = (Chip *)symlook(buf, S_CHID, (void *)0);
	ss = (Signal **)symlook(c->name, S_CSMAP, (void *)0);
if (dbfp) fprintf(dbfp, "chsig(%d):", n);
	put1(CHIPSIGS);
	while(*ss){
if (dbfp) fprintf(dbfp, " %d", (*ss)->num);
		putn((int)(*ss)->num);
		ss++;
	}
if (dbfp) fprintf(dbfp, "\n");
	putn(-1);
	flush();
}

void updssig(Chip *);

int
rotchip(int n)
{
	register Chip *cp;
	int ret = 0;
	Point pt;

	cp = cofn(n);
	pt = cp->pt;
	unplace(cp);
	cp->rotation = (cp->rotation+1)&3;
	cp->pt = pt;
	cp->flags &= ~UNPLACED;
	place(cp);
	if(cp->flags&UNPLACED){
		cp->flags &= ~UNPLACED;
		cp->rotation = (cp->rotation-1)&3;
		cp->pt = pt;
		place(cp);
		ret = 1;
	}
	updsig(cp);
	updssig(cp);
	sendpin(n);
	sendchip(cp, n);
	return(ret);
}

void
updsig(Chip *c)
{
	register Signal *s, **ss;
	register i;

	ss = (Signal **)symlook(c->name, S_CSMAP, (void *)0);
	while(s = *ss){
		s->done = 0;
		for(i = 0; i < s->n; i++)
			if(strcmp(s->coords[i].chip, c->name) == 0)
				s->pins[i].p = c->pins[s->coords[i].pin-c->pmin].p;
		ss++;
	}
	flush();
}

void
updssig(Chip *c)
{
	int j, i;
	Signal *s;
	for(j = 0; j < 6; ++j)
		if(b.v[j].name) {
			s = (Signal *)symlook(b.v[j].name, S_SIGNAL, (void *) 0);
			if(s) {
				s->done = 0;
				for(i = 0; i < s->n; i++)
					if(strcmp(s->coords[i].chip, c->name) == 0)
						s->pins[i].p = c->pins[s->coords[i].pin-c->pmin].p;
			}
		}
}
