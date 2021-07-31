#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"


void
nullfn(void)
{
}

void
setgrid(NMitem *m)
{
	draw();

	if(scrn.grid == m->data)
		m->data = (long)nullfn;
	else {
		scrn.grid = m->data;
		scrn.br.origin.x = (scrn.br.origin.x+scrn.grid-1)/scrn.grid;
		scrn.br.origin.x *= scrn.grid;
		scrn.br.origin.y = (scrn.br.origin.y+scrn.grid-1)/scrn.grid;
		scrn.br.origin.y *= scrn.grid;
		m->data = (long)draw;
	}
}

NMitem *
mgridn(int i)
{
	static NMitem m;
	static char buf[64], nam[64];

	if(i == 0)
		i = 1;
	else
		i = 5*i;
	m.data = i;
	m.next = 0;
	m.hfn = setgrid;
	m.dfn = 0;
	m.bfn = 0;
	if(i <= 250)
	{
		sprint(nam, "%s%d\240", i == scrn.grid? "*":" ", i);
		m.text = nam;
		m.help = "grid resolution *.01in";
	} else
		m.text = 0;
	return(&m);
}
