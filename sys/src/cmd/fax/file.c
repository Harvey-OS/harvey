#include <u.h>
#include <libc.h>
#include <bio.h>

#include "modem.h"

static long wd[5] = {
	1728, 2048, 2432, 1216, 864
};

void
setpageid(char *pageid, char *spool, long time, int pid, int pageno)
{
	sprint(pageid, "%s/%lud.%d.%3.3d", spool, time, pid, pageno);
}

int
createfaxfile(Modem *m, char *spool)
{
	setpageid(m->pageid, spool, m->time, m->pid, m->pageno);
	verbose("openfaxfile: %s", m->pageid);
	if((m->pagefd = create(m->pageid, OTRUNC|OWRITE, 0666)) < 0)
		return seterror(m, Esys);

	fprint(m->pagefd, "TYPE=ccitt-g31\n");
	fprint(m->pagefd, "WINDOW=0 0 %ld -1\n", wd[m->fdcs[2]]);
	if(m->valid & Vftsi)
		fprint(m->pagefd, "FTSI=%s\n", m->ftsi);
	fprint(m->pagefd, "FDCS=%lud,%lud,%lud,%lud,%lud,%lud,%lud,%lud\n",
		m->fdcs[0], m->fdcs[1], m->fdcs[2], m->fdcs[3],
		m->fdcs[4], m->fdcs[5], m->fdcs[6], m->fdcs[7]);
	fprint(m->pagefd, "\n");

	return Eok;
}

enum
{
	Gshdrsize=	0x40,
};

int
gsopen(Modem *m)
{
	int n;
	char bytes[Gshdrsize];

	/*
	 *  Is this gs output
	 */
	n = Bread(m->bp, bytes, Gshdrsize);
	if(n != Gshdrsize)
		return seterror(m, Esys);
	if(bytes[0]!='\0' || strcmp(bytes+1, "PC Research, Inc")!=0){
		Bseek(m->bp, 0, 0);
		return seterror(m, Esys);
	}

	m->valid |= Vtype;
	if(bytes[0x1d])
		m->vr = 1;
	else
		m->vr = 0;
	m->wd = 0;
	m->ln = 2;
	m->df = 0;
	return Eok;
}

int
picopen(Modem *m)
{
	char *p, *q;
	int i, x;

	/*
	 * Look at the header. Every page should have a valid type.
	 * The first page should have WINDOW.
	 */
	while(p = Brdline(m->bp, '\n')){
		if(Blinelen(m->bp) == 1)
			break;
		p[Blinelen(m->bp)-1] = 0;

		verbose("openfaxfile: %s", p);
		if(strcmp("TYPE=ccitt-g31", p) == 0)
			m->valid |= Vtype;
		/*
		else if(m->pageno == 1 && strncmp("PHONE=", p, 6) == 0){
			strcpy(m->number, p+6);
			m->valid |= Vphone;
		}
		 */
		else if(m->pageno == 1 && strncmp("WINDOW=", p, 7) == 0){
			p += 7;
			verbose("openfaxfile: WINDOW: %s", p);
			for(i = 0; i < 4; i++){
				x = strtol(p, &q, 10);
				if(i == 2)
					m->wd = x;
				if((p = q) == 0){
					Bterm(m->bp);
					return seterror(m, Eproto);
				}
			}
			for(i = 0; i < 5; i++){
				if(m->wd == wd[i]){
					m->wd = i;
					m->valid |= Vwd;
					break;
				}
			}
			if((m->valid & Vwd) == 0){
				Bterm(m->bp);
				return seterror(m, Eproto);
			}
		}
		else if(m->pageno == 1 && strncmp("FDCS=", p, 5) == 0){
			p += 5;
			m->df = m->vr = m->wd = 0;
			m->ln = 2;
			for(i = 0; i < 5; i++){
				x = strtol(p, &q, 10);
				switch(i){
				case 0:
					m->vr = x;
					break;
				case 3:
					m->ln = x;
					break;
				case 4:
					m->df = x;
					break;
				}
				if((p = q) == 0){
					Bterm(m->bp);
					return seterror(m, Eproto);
				}
				if(*p++ != ',')
					break;
			}
		}
	}

	verbose("openfaxfile: valid #%4.4ux", m->valid);
	if((m->valid & (Vtype|Vwd)) != (Vtype|Vwd)){
		Bterm(m->bp);
		return seterror(m, Eproto);
	}

	return Eok;
}

int
openfaxfile(Modem *m, char *file)
{
	if((m->bp = Bopen(file, OREAD)) == 0)
		return seterror(m, Esys);
	m->valid &= ~(Vtype);

	if(gsopen(m) == Eok)
		return Eok;
	return picopen(m);
}
