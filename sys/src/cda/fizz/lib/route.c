#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>
void f_major(char *, ...);
void f_minor(char *, ...);
void f_coords(char *, short *, Coord **);

static Keymap keys[] = {
	"name", NULLFN, 1,
	"alg", NULLFN, 2,
	"route", NULLFN, 3,
	"layout", NULLFN, 4,
	0
};
static Keymap algs[] = {
	"tsp", NULLFN, RTSP,
	"tspe", NULLFN, RTSPE,
	"mst", NULLFN, RMST,
	"default", NULLFN, RDEFAULT,
	"hand", NULLFN, RHAND,
	0
};

void
f_route(char *s)
{
	int nm;
	Signal *signal;

	BLANK(s);
	if(*s != '{'){
		fprint(2, "route needs a '{'\n");
		return;
	}
	while(s = f_inline()){
		BLANK(s);
		if(*s == '}') break;
		switch((int)f_keymap(keys, &s))
		{
		case 1:
			if(!(signal = (Signal *) symlook(s, S_SIGNAL, (void *)0)))
				f_minor("signal name not found for route '%s'", s);
			break;
		case 2:
			BLANK(s);
			signal->alg = (int)f_keymap(algs, &s);
			if(signal->alg == 0)
				f_minor("bad algorithm in signal route '%s'", s);
			if(*s) EOLN(s)
			break;
		case 3:
			if (signal)
				f_coords(s, &signal->nr, &signal->routes);
			break;
		case 4:
			if (signal){
				NUM(s, signal->nlayout)
				signal->layout = f_pins(s, 1, signal->nlayout);
			}
			break;
		default:
			f_minor("bad field in signal route '%s'", s);
			break;
		}
	}
}

static
int
in(Coord *c, int n, Coord *cc)
{
	while(n-- > 0){
		if((strcmp(c->chip, cc->chip) == 0) && (c->pin == cc->pin))
			return(1);
		cc++;
	}
	return(0);
}

void
chkroute(Signal *s)
{
	register i;

	if((s->type != NORMSIG) || (s->n <= 1))
		return;
	if((s->nw == 0) && (s->nr == 0)) return;
	if(s->alg == RTSPE){
		if(s->nr == 1){
			if(!in(s->routes, s->n, s->coords))
				f_major("%C not in net %s", s->routes[0], s->name);
		} else
			f_major("expected one (got %d coords) in route %s", s->n, s->name);
	} else
	if (!s->nw) {
		if(s->nr != s->n){
			f_major("%s: Route has %d coords, Net has %d", s->name, s->nr, s->n);
			return;
		}
		for(i = 0; i < s->n; i++)
			if(!in(s->routes+i, s->n, s->coords))
				f_major("%C not in net %s", s->routes[i], s->name);
		for(i = 0; i < s->n; i++)
			if(!in(s->coords+i, s->n, s->routes))
				f_major("%C not in route for %s", s->coords[i], s->name);
	}
}
