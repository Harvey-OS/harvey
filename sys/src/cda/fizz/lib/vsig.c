#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

void f_minor(char *, ...);
void f_major(char *, ...);

static Keymap keys[] = {
	"name", NULLFN, 1,
	"pins", NULLFN, 2,
	0
};
void
f_vsig(char *s)
{
	int n, nm, loop, nc;
	Pin *p, *pp;
	register Vsig *v;

	BLANK(s);
	NUM(s, n);
	if((n < 0) || (n > 5)){
		f_major("bad Vsig number %d - changed it to 0", n);
		n = 0;
	}
	v = &f_b->v[n];
	if(loop = *s == '{')
		s = f_inline();
	do {
		if(*s == '}') break;
		switch((int)f_keymap(keys, &s))
		{
		case 1:
			v->name = f_strdup(s);
			break;
		case 2:
			NUM(s, nc)
			if(p = f_pins(s, 1, nc)){
				if(v->pins == 0)
					v->npins = nc, v->pins = p;
				else {
					pp = (Pin *)f_malloc(sizeof(Pin)*(long)(nc+v->npins));
					memcpy((char *)pp, (char *)v->pins, sizeof(Pin)*(long)v->npins);
					memcpy((char *)&pp[v->npins], (char *)p, sizeof(Pin)*(long)nc);
					v->pins = pp;
					v->npins += nc;
				}
			}
			break;
		default:
			f_minor("Vsig: unknown field: '%s'", s);
			break;
		}
	} while(loop && (s = f_inline()));
}
