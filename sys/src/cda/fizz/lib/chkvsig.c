#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>
void f_minor(char *, ...);
void f_major(char *, ...);

void tweak(char * , int , int );
void addvsig(char *, int , Coord *, Pin *, Pin *);
static char *vinfo;
static long vworst;

void
vbest(char *s)
{
	vinfo = s;
	vworst = 0;
}

void
chkvsig(Vsig *v, int n)
{
	register Pin *p;
	register i;
	Signal *s;
	short bestvb;
	long best;

	if(v->name == 0){
		f_major("Special signal %d does not have a name", n);
		return;
	}
	symlook(v->name, S_VSIG, (void *)(v->signo+1));
	for(i = 0, p = v->pins; i < v->npins; i++, p++)
		blim(p->p);
	if(s = (Signal *)symlook(v->name, S_SIGNAL, (void *)0)){
		if(v->npins == 0){
			fprint(2, "Special signal %s has no pins to connect to\n", v->name);
			fprint(2, "Assume clip or pwd connection\n");
			s->type = VSIG|NOVPINS;
			return;
		}
		s->type = VSIG;
		nnprep(v->pins, v->npins);
		for(p = s->pins, i = 0; i < s->n; i++, p++){
			nn(p->p, &best, &bestvb);
			if(best){
				addvsig(v->name, bestvb, &s->coords[i], p, &v->pins[bestvb]);
				if(vinfo && (best > vworst)){
					vworst = best;
					sprint(vinfo, "%.2fi (%s.%d <--> %s.%d)",
						vworst/(float)INCH, s->coords[i].chip,
						s->coords[i].pin, v->name, bestvb);
				}
			} else {
				Chip *c;
				int pin;

				c = (Chip *)symlook(s->coords[i].chip, S_CHIP, (void *)0);
				pin = s->coords[i].pin-c->pmin;
				c->netted[pin] |= VBPIN;
				if(v->pins[bestvb].used)
					tweak(v->name, bestvb, c->pins[pin].drill);
;
				v->pins[bestvb].drill = c->pins[pin].drill;
			}
		}
	}
}

void
addvsig(char *vname, int vpin, Coord *c, Pin *p, Pin *pp)
{
	char buf[256];
	register Signal *s;

	sprint(buf, "%s%d", vname, vpin);
	if((s = (Signal *)symlook(buf, S_SIGNAL, (void *)0)) == 0){
		s = NEW(Signal);
		s->name = f_strdup(buf);
		(void)symlook(s->name, S_SIGNAL, (void *)s);
		s->n = 1;
		s->type = SUBVSIG;
		s->coords = (Coord *)lmalloc(MAXNET*(long)sizeof(Coord));
		s->pins = (Pin *)lmalloc(MAXNET*(long)sizeof(Pin));
		s->coords[0].chip = vname;
		s->coords[0].pin = vpin;
		s->pins[0] = *pp;
		pp->used = 1;
	}
	if(s->n >= MAXNET){
		f_major("too many (%d) elements in Signal %s", s->n, s->name);
		s->n /= 2;	/* cut down on repeat errors */
		return;
	}
	s->coords[s->n] = *c;
	s->pins[s->n] = *p;
	s->n++;
}

void
tweak(char * vname, int vpin, int drill)
{
	char buf[256];
	register Signal *s;

	sprint(buf, "%s%d", vname, vpin);
	if((s = (Signal *)symlook(buf, S_SIGNAL, (void *)0)) == 0){
		fprint(2, "vsig tweak: scrunted by '%s'\n", buf);
		exits("scrunted");
	}
	s->pins[0].drill = drill;
}
