#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

long nnet;
void f_minor(char *, ...);
void f_major(char *, ...);

void
f_net(char *s)
{
	int nm, loop;
	int nc;
	int toomany = 0;
	register Coord *c;
	Signal *ss;
	char *os;

	BLANK(s);
	os = s;
	NAME(s);
	NUM(s, nc);
	if(nc < 1){
		f_major("net %s has %d (<1) points", os, nc);
		nc = 1;
	}
	if((Signal *)symlook(os, S_SIGNAL, (void *)0))
		f_major("redefinition of Net %s", os);
	ss = NEW(Signal);
	ss->name = f_strdup(os);
	ss->type = NORMSIG;	/* it will get set later if need be */
	ss->length = 0;
	ss->layout = 0;
	ss->n = nc;
	/* one more for outlier in tsp */
	ss->coords = c = (Coord *)f_malloc((nc+1)*(long)sizeof(Coord));
	symlook(ss->name, S_SIGNAL, (void *)ss);
	if(loop = *s == '{')
		s = f_inline();
	do {
		if(*s == '}') break;
		if(nc <= 0){
			if(toomany == 0){
				toomany = 1;
				f_major("unexpected (>%d) points for signal %s", ss->n, ss->name);
			}
			nc = ss->n;
			c = ss->coords;
		}
		BLANK(s);
		os = s;
		NAME(s);
		c->chip = f_strdup(os);
		NUM(s, c->pin);
		if(*s) {
			os = s;
			NAME(s);
			c->pinname = f_strdup(os);
		}
		if(*s) EOLN(s);
		nc--;
		c++;
	} while(loop && (s = f_inline()));
}

void
putnet(Signal *s)
{
	register Coord *co;
	register Pin *p;
	register Chip *c;
	int i, signo;

	for(i = 0; i < 6; i++)
		if(f_b->v[i].name && (strcmp(f_b->v[i].name, s->name) == 0))
			break;
	signo = i;
	s->pins = (Pin *)lmalloc((s->n+1)*(long)sizeof(Pin));
	for(co = s->coords, p = s->pins, i = 0; i < s->n; i++, co++){
		c = (Chip *)symlook(co->chip, S_CHIP, (void *)0);
		if(c == 0){
			f_major("signal %s refers to unknown chip %s", s->name, co->chip);
			continue;
		}
		if(c->netted) {
		    if (c->netted[co->pin-c->pmin]){
			int osig;

			osig = c->netted[co->pin-c->pmin]&SIGNUM;
			if(osig >= 7)
				osig -= 6;
			if((signo > 5) || (osig != signo))
				f_major("%s.%d on two nets (2nd is %s)",
					co->chip, co->pin, s->name);
			if(c->netted[co->pin-c->pmin]&NETTED){
				memcpy((char *)co, (char *)(co+1), sizeof(Coord)*(s->n-i-1));
				co--;
				continue;
			} else
				c->netted[co->pin-c->pmin] |= NETTED;
		    } else
			c->netted[co->pin-c->pmin] = NETTED|signo|(signo<6)?VBNET:0;
		    *p++ = c->pins[co->pin-c->pmin];
		}
	}
	s->n = p-s->pins;
}

void
chknet(Signal *s)
{
	if((s->type == NORMSIG) && (s->n > MAXNET))
		f_major("net %s too large (%d > %d)", s->name, s->n, MAXNET);
	nnet++;
}
