#include <u.h>
#include <libc.h>
#include <bio.h>

#include "modem.h"

int
initfaxmodem(Modem *m)
{
	m->fax = 1;
	m->phase = 'A';
	m->valid = 0;

	return Eok;
}

static int
parameters(long a[], char *s)
{
	char *p;
	int i;

	i = 0;
	if((p = strchr(s, ':')) == 0)
		return 0;
	p++;
	while(s = strchr(p, ',')){
		a[i++] = strtol(p, 0, 10);
		p = s+1;
	}
	if(p)
		a[i++] = strtol(p, 0, 10);

	return i;
}

int
fcon(Modem *m)
{
	verbose("fcon: %s", m->response);
	if(m->fax == 0 || m->phase != 'A')
		return Rrerror;
	m->phase = 'B';
	return Rcontinue;
}

int
ftsi(Modem *m)
{
	char *p, *q;

	verbose("ftsi: %s", m->response);
	if((p = strchr(m->response, '"')) == 0 || (q = strrchr(p+1, '"')) == 0)
		return Rrerror;
	while(*++p == ' ')
		;
	*q = 0;
	if((m->valid &  Vftsi) == 0){
		strncpy(m->ftsi, p, sizeof(m->ftsi)-1);
		m->valid |= Vftsi;
	}
	return Rcontinue;
}

int
fdcs(Modem *m)
{
	verbose("fdcs: %s", m->response);
	parameters(m->fdcs, m->response);
	m->valid |= Vfdcs;
	return Rcontinue;
}

int
fcfr(Modem *m)
{
	verbose("fcfr: %s", m->response);
	if(m->fax == 0)
		return Rrerror;
	/* ???? */
	return Rcontinue;
}

int
fpts(Modem *m)
{
	verbose("fpts: %s", m->response);
	if(m->fax == 0)
		return Rrerror;
	parameters(m->fpts, m->response);
	m->valid |= Vfpts;
	return Rcontinue;
}

int
fet(Modem *m)
{
	char *p;

	verbose("fet: %s", m->response);
	if(m->fax == 0 || (p = strchr(m->response, ':')) == 0)
		return Rrerror;
	m->fet = strtol(p+1, 0, 10);
	m->valid |= Vfet;
	return Rcontinue;
}

int
fhng(Modem *m)
{
	char *p;

	verbose("fhng: %s", m->response);
	if(m->fax == 0 || (p = strchr(m->response, ':')) == 0)
		return Rrerror;
	m->fhng = strtol(p+1, 0, 10);
	m->valid |= Vfhng;
	return Rhangup;
}
