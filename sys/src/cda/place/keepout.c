#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#include "proto.h"


static void
kpall(NMitem *m)
{
	register i;

	for(i = 0; i < 6; i++)
		if(b.keepoutmap[i] > 0)
			b.keepoutmap[i] = -b.keepoutmap[i];
	m->data = (long)draw;
}

static void
kpnone(NMitem *m)
{
	register i;

	for(i = 0; i < 6; i++)
		if(b.keepoutmap[i] < 0)
			b.keepoutmap[i] = -b.keepoutmap[i];
	m->data = (long)draw;
}

static int kpwho;

void
kpredraw(void)
{
	if(b.keepoutmap[kpwho] < 0){
		drawkeepout(kpwho);
		b.keepoutmap[kpwho] = -b.keepoutmap[kpwho];
	} else {
		b.keepoutmap[kpwho] = -b.keepoutmap[kpwho];
		drawkeepout(kpwho);
	}
}

static void
kpone(NMitem *m)
{
	kpwho = m->data;
	m->data = (long) 16;
}

NMitem *
mkeepoutfn(int i)
{
	register n;
	static NMitem m;
	static char buf[64], nam[64];

	m.next = 0;
	m.hfn = 0;
	m.dfn = 0;
	m.bfn = 0;
	m.text = nam;
	if(b.nkeepouts == 0){
		if(i)
			m.text = 0;
		else {
			strcpy(nam, "no keepouts");
		}
		return(&m);
	}
	m.help = buf;
	switch(i -= 1)
	{
	case -1:
		strcpy(nam, "all");
		strcpy(buf, "show all keepouts XORed");
		m.hfn = kpall;
		break;
	case 0:
		strcpy(nam, "none");
		strcpy(buf, "undraw all keepouts");
		m.hfn = kpnone;
		break;
	default:
		for(n = 0; n < 6; n++)
			if(b.keepoutmap[n])
				if(--i == 0)
					break;
		if(i == 0){
			sprint(nam, "%csig %d\240", b.keepoutmap[n]<0? '*':' ', n);
			sprint(buf, "%s show special signal %d", b.keepoutmap[n]<0? "don't":"do", n);
			m.hfn = kpone;
			m.data = n;
		} else
			m.text = 0;
		break;
	}
	return(&m);
}
