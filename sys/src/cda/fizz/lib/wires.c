#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

void f_minor(char *, ...);
void f_major(char *, ...);

static Keymap keys[] = {
	"level", NULLFN, 1,
	"net", NULLFN, 2,
	0
};

#define MAXINFLECT 100

void
f_wires(char *s)
{
	int loop, x, y, nm, ninf;
	Signal *ss;
	Wire *w, *last_wire;
	Pin *p;
	Pin inflect[MAXINFLECT];
	char *os, *netname;

	BLANK(s);
	if(*s != '{'){
		f_minor("wires needs a '{'\n");
		return;
	}
	while(s = f_inline()){
		BLANK(s)
		if(*s == '}') break;
		switch((int)f_keymap(keys, &s))
		{
		case 1:
			BLANK(s)
			os = s;
			NAME(s)
			BLANK(s)
			fprint(2, "Layer %s\n", os);
			break;
		case 2:
			BLANK(s)
			os = s;
			NAME(s)
			netname = f_strdup(os);
			BLANK(s)
			if(*s != '{')
				break;
			if(!(ss = (Signal *)symlook(netname, S_SIGNAL, (void *)0))){
				f_minor("signal %s not found", os);
			}
			s = f_inline();
			ninf = 0;
			do {
				if(*s == '}') break;
				if (ninf == MAXINFLECT) {
					f_major("too many inflection point (%d)", MAXINFLECT);
					break;
				}
				COORD(s, inflect[ninf].p.x, inflect[ninf++].p.y);
			} while (s = f_inline());
			w = (ss -> wires) ? ss -> wires : (Wire *) 0;
			ss->wires = (Wire *) f_malloc(sizeof(Wire));
			p = (Pin *)f_malloc((ninf+1)*(long)sizeof(Pin));
			ss->wires->ninf = ninf;
			ss->wires->next = w;
			ss->wires->inflections = p;
			ss->nw++;
			if (w) w->previous = ss->wires;
			memcpy(p, inflect, (ninf)*(long)sizeof(Pin));
			break;
		default:
			f_minor("Wires: unknown field: '%s'\n", s);
			break;
		}
	} while(loop && (s = f_inline()));
}
