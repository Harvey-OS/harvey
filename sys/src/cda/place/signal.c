#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#include "proto.h"
extern int errfd;

void
showsig(void)
{
	register Chip *c;

	b.ncursig = 0;
	if(b.chips){
		for(c = b.chips; !(c->flags&EOLIST); c++)
			if((c->flags&(SELECTED|WSIDE))
				== (scrn.selws ? (SELECTED|WSIDE) : SELECTED)){
					put1(SENDSIGS);
					putn(c->id);
					while(rcv());
			}
	}
}


void
showpin(void)
{
	register Chip *c;
	if(b.chips){
		for(c = b.chips; !(c->flags&EOLIST); c++)
			if((c->flags&(SELECTED|WSIDE))
				== (scrn.selws ? (SELECTED|WSIDE) : SELECTED))
				if(c->npins < 0) {
					cursorswitch(&blank);
					drawchip(c);
					c->npins *= -1;
					c->flags &= ~MAPPED;
					drawchip(c);
					cursorswitch((Cursor *) 0);
				}
	}
}

void
clrpin(void)
{
	Chip *c;
	if(b.chips){
		for(c = b.chips; !(c->flags&EOLIST); c++)
			if((c->flags&WSIDE)
				== (scrn.selws ? WSIDE : 0))
					if(c->npins > 0) {
						cursorswitch(&blank);
						drawchip(c);
						c->npins *= -1;
						c->flags &= ~MAPPED;
						drawchip(c);
						cursorswitch((Cursor *) 0);
					}
	}
}

void
do2sig(void)
{
	gdrawsig(b.cursig[0]);
}

static void
do1sig(NMitem *m)
{
	b.ncursig = 1;
	b.cursig[0] = m->data;
	m->data = (long) 12;
}

NMitem *
mssig(int i)
{
	register Signal *s = 0;
	static NMitem m;
	static char buf[64], nam[64];

	if(b.sigs){
		s = b.sigs+i;
		if(s->name == 0)
			s = 0;
	}
	m.data = s? i : -1;
	m.next = 0;
	m.hfn = do1sig;
	m.dfn = 0;
	m.bfn = 0;
	if(s)
	{
		sprint(nam, "%s\240", s->name+b.chipstr);
		m.text = nam;
		m.help = 0;
	} else
		m.text = 0;
	return(&m);
}

void
getsigs(short *sigs)
{
	register Signal *ss;

	while(*sigs >= 0){
		for(ss = b.sigs; ss->id != *sigs; ss++)
			if(ss->name == 0)
				panic("bad signal id");
		b.cursig[b.ncursig++] = ss-b.sigs;
		gdrawsig(ss-b.sigs);
		if(b.ncursig >= NCURSIG)
			b.ncursig--;
		sigs++;
	}
}
