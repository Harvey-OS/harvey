#include <u.h>
#include <libc.h>
#include	<cda/fizz.h>

void f_minor(char *, ...);
void f_major(char *, ...);
void f_coords(char *, short *, Coord **);

static Keymap keys[] = {
	"name", NULLFN, 1,
	"width", NULLFN, 2,
	"layout", NULLFN, 3,
	0
};
void
f_layout(char *s)
{
	int n, nm, loop, nc;
	int gotsomething = 0;
	Signal *signal = 0;
	Layout lay;

	BLANK(s);
	if(*s != '{'){
		fprint(2, "layout needs a '{'\n");
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
			NUM(s, lay.width)
			f_coords(s, &lay.ncoords, &lay.coords);
			gotsomething = 1;
			break;
		case 3:
			NUM(s, lay.npins)
			lay.pins = f_pins(s, 1, lay.npins);
			gotsomething = 1;
			break;
		default:
			f_minor("Layout: unknown field: '%s'", s);
			break;
		}
	}
	if(gotsomething){
		if(signal == 0){
			f_minor("nameless layout");
			return;
		}
		signal->layout = (Layout *)lmalloc(sizeof(Layout));
		if(signal->layout)
			signal->layout[0] = lay;
		else
			f_minor("out of psace in layout for '%s'", signal->name);
	}
}
