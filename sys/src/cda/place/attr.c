#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include	"term.h"

NMitem *
mattrfn(int i)
{
	register Chip *c;
	register j;
	char at[64];
	static NMitem m;
	static char buf[256];

	if(b.chips)
		for(c = b.chips, j = 0; !(c->flags&EOLIST); c++)
			if(((c->flags&(SELECTED|WSIDE)) == (scrn.selws ? (SELECTED|WSIDE) : SELECTED)) && (i == j++))
				goto found;
	c = 0;
found:
	m.data = 0;
	m.next = 0;
	m.hfn = 0;
	m.dfn = 0;
	m.bfn = 0;
	if(c)
	{
		at[0] = 0;
		at[1] = 0;
		switch(c->flags&3)
		{
		case SOFT:	 strcat(at, ",soft"); break;
		case HARD:	 strcat(at, ",hard"); break;
		case NOMOVE:	 strcat(at, ",nomove"); break;
		}
		if(c->flags&IGPINS) strcat(at, ",igpin");
		if(c->flags&IGRECT) strcat(at, ",igrect");
		if(c->flags&IGNAME) strcat(at, ",igname");
		sprint(buf, "%s\240%s", c->name+b.chipstr, at+1);
		m.text = buf;
		m.help = c->type+b.chipstr;
	} else
		m.text = 0;
	return(&m);
}
