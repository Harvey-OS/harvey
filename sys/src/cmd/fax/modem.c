#include <u.h>
#include <libc.h>
#include <bio.h>

#include "modem.h"

typedef struct {
	char	*terse;
	char	*verbose;
	int	result;
	int	(*f)(Modem*);
} ResultCode;

static ResultCode results[] = {
	{ "0",	"OK",		Rok,		0, },
	{ "1",	"CONNECT",	Rconnect,	0, },
	{ "2",	"RING",		Rring,		0, },
	{ "3",	"NO CARRIER", 	Rfailure,	0, },
	{ "4",	"ERROR",	Rrerror,	0, },
	{ "5",	"CONNECT 1200",	Rconnect,	0, },
	{ "6",	"NO DIALTONE",	Rfailure,	0, },
	{ "7",	"BUSY",		Rfailure,	0, },
	{ "8",	"NO ANSWER",	Rfailure,	0, },
	{ "9",	"CONNECT 2400",	Rconnect,	0, },		/* MT1432BA */
	{ "10",	"CONNECT 2400",	Rconnect,	0, },		/* Hayes */
	{ "11",	"CONNECT 4800",	Rconnect,	0, },
	{ "12",	"CONNECT 9600",	Rconnect,	0, },
	{ "13",	"CONNECT 14400",Rconnect,	0, },
	{ "23",	"CONNECT 1275",	Rconnect,	0, },		/* MT1432BA */

	{ "-1",	"+FCON",	Rcontinue,	fcon, },
	{ "-1",	"+FTSI",	Rcontinue,	ftsi, },
	{ "-1",	"+FDCS",	Rcontinue,	fdcs, },
	{ "-1",	"+FCFR",	Rcontinue,	fcfr, },
	{ "-1",	"+FPTS",	Rcontinue,	fpts, },
	{ "-1",	"+FET",		Rcontinue,	fet, },
	{ "-1",	"+FHNG",	Rcontinue,	fhng, },

	{ 0 },
};

void
initmodem(Modem *m, int fd, int cfd, char *type, char *id)
{
	m->fd = fd;
	m->cfd = cfd;
	if(id == 0)
		id = "Plan 9";
	m->id = id;
	m->t = type;
}

int
rawmchar(Modem *m, char *p)
{
	Dir *d;
	int n;

	if(m->icount == 0)
		m->iptr = m->ibuf;

	if(m->icount){
		*p = *m->iptr++;
		m->icount--;
		return Eok;
	}

	m->iptr = m->ibuf;

	if((d = dirfstat(m->fd)) == nil){
		verbose("rawmchar: dirfstat: %r");
		return seterror(m, Esys);
	}
	n = d->length;
	free(d);
	if(n == 0)
		return Enoresponse;

	if(n > sizeof(m->ibuf)-1)
		n = sizeof(m->ibuf)-1;
	if((m->icount = read(m->fd, m->ibuf, n)) <= 0){
		verbose("rawmchar: read: %r");
		m->icount = 0;
		return seterror(m, Esys);
	}
	*p = *m->iptr++;
	m->icount--;

	return Eok;
}

int
getmchar(Modem *m, char *buf, long timeout)
{
	int r, t;

	timeout += time(0);
	while((t = time(0)) <= timeout){
		switch(r = rawmchar(m, buf)){

		case Eok:
			return Eok;

		case Enoresponse:
			sleep(100);
			continue;

		default:
			return r;
		}
	}
	verbose("getmchar: time %ud, timeout %ud", t, timeout);

	return seterror(m, Enoresponse);
}

int
putmchar(Modem *m, char *p)
{
	if(write(m->fd, p, 1) < 0)
		return seterror(m, Esys);
	return Eok;
}

/*
 *  lines terminate with cr-lf
 */
static int
getmline(Modem *m, char *buf, int len, long timeout)
{
	int r, t;
	char *e = buf+len-1;
	char last = 0;

	timeout += time(0);
	while((t = time(0)) <= timeout){
		switch(r = rawmchar(m, buf)){

		case Eok:
			/* ignore ^s ^q which are used for flow */
			if(*buf == '\021' || *buf == '\023')
				continue;
			if(*buf == '\n'){
				/* ignore nl if its not with a cr */
				if(last == '\r'){
					*buf = 0;
					return Eok;
				}
				continue;
			}
			last = *buf;
			if(*buf == '\r')
				continue;
			buf++;
			if(buf == e){
				*buf = 0;
				return Eok;
			}
			continue;

		case Enoresponse:
			sleep(100);
			continue;

		default:
			return r;
		}
	}
	verbose("getmline: time %ud, timeout %ud", t, timeout);

	return seterror(m, Enoresponse);
}

int
command(Modem *m, char *s)
{
	verbose("m->: %s", s);
	if(fprint(m->fd, "%s\r", s) < 0)
		return seterror(m, Esys);
	return Eok;
}

/*
 * Read till we see a message or we time out.
 * BUG: line lengths not checked;
 *	newlines
 */
int
response(Modem *m, int timeout)
{
	int r;
	ResultCode *rp;

	while(getmline(m, m->response, sizeof(m->response), timeout) == Eok){
		if(m->response[0] == 0)
			continue;
		verbose("<-m: %s", m->response);
		for(rp = results; rp->terse; rp++){
			if(strncmp(rp->verbose, m->response, strlen(rp->verbose)))
				continue;
			r = rp->result;
			if(rp->f && (r = (*rp->f)(m)) == Rcontinue)
				break;
			return r;
		}
	}

	m->response[0] = 0;
	return Rnoise;
}

void
xonoff(Modem *m, int i)
{
	char buf[8];

	sprint(buf, "x%d", i);
	i = strlen(buf);
	write(m->cfd, buf, i);
}
