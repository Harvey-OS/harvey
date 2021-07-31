#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "term.h"
#include "proto.h"


long wraplen;
static improving = 0;
extern void chstat(NMitem *);

void
improveinfo(int n)
{
	if(improving = n < 0)
		n = -n;
	wraplen = n;
}

NMitem *
mimpfn(int i)
{
	static NMitem m;
	static char buf[64], nam[64];

	m.next = 0;
	m.data = 0;
	m.hfn = 0;
	m.dfn = 0;
	m.bfn = 0;
	m.text = nam;
	m.help = buf;
	buf[0] = 0;
	switch(i)
	{
	case 0:
		sprint(nam, "wraplen=%ld", wraplen);
		break;	
	case 1:
		if(improving){
			strcpy(nam, "improving");
			strcpy(buf, "improver running");
		} else {
			strcpy(nam, "improve 1");
			strcpy(buf, "improve selected chips (1 iter)");
			m.hfn = chstat;
			m.data = IMPROVE1;
		}
		break;
	case 2:
		if(improving){
			strcpy(nam, "stop improving");
			strcpy(buf, "stop improving soon");
		} else {
			strcpy(nam, "improve n");
			strcpy(buf, "improve selected chips (inf iter)");
			m.hfn = chstat;
			m.data = IMPROVEN;
		}
		break;
	case 3:
		if(improving){
			strcpy(nam, "stop improving");
			strcpy(buf, "stop improving soon");
		} else {
			strcpy(nam, "improve inf");
			strcpy(buf, "improve selected chips (inf iter)");
			m.hfn = chstat;
			m.data = IMPROVEF;
		}
		break;
	default:
		m.text = 0;
		break;
	}
	return(&m);
}
