#include <u.h>
#include <libc.h>
#include <thread.h>
#include <bio.h>
#include "usb.h"

int usbdebug;

static char *edir[] = {"in", "out", "inout"};
static char *etype[] = {"ctl", "iso", "bulk", "intr"};
static char* cnames[] =
{
	"none", "audio", "comms", "hid", "",
	"", "", "printer", "storage", "hub", "data"
};
static char* devstates[] =
{
	"detached", "attached", "enabled", "assigned", "configured"
};

char*
classname(int c)
{
	static char buf[30];

	if(c >= 0 && c < nelem(cnames))
		return cnames[c];
	else{
		seprint(buf, buf+30, "%d", c);
		return buf;
	}
}

char *
hexstr(void *a, int n)
{
	int i;
	char *dbuff, *s, *e;
	uchar *b;

	b = a;
	dbuff = s = emallocz(1024, 0);
	*s = 0;
	e = s + 1024;
	for(i = 0; i < n; i++)
		s = seprint(s, e, " %.2ux", b[i]);
	if(s == e)
		fprint(2, "%s: usb/lib: hexdump: bug: small buffer\n", argv0);
	return dbuff;
}

static char *
seprintiface(char *s, char *e, Iface *i)
{
	int	j;
	Altc	*a;
	Ep	*ep;
	char	*eds, *ets;

	s = seprint(s, e, "\t\tiface csp %s.%uld.%uld\n",
		classname(Class(i->csp)), Subclass(i->csp), Proto(i->csp));
	for(j = 0; j < Naltc; j++){
		a=i->altc[j];
		if(a == nil)
			break;
		s = seprint(s, e, "\t\t  alt %d attr %d ival %d",
			j, a->attrib, a->interval);
		if(a->aux != nil)
			s = seprint(s, e, " devspec %p\n", a->aux);
		else
			s = seprint(s, e, "\n");
	}
	for(j = 0; j < Nep; j++){
		ep = i->ep[j];
		if(ep == nil)
			break;
		eds = ets = "";
		if(ep->dir <= nelem(edir))
			eds = edir[ep->dir];
		if(ep->type <= nelem(etype))
			ets = etype[ep->type];
		s = seprint(s, e, "\t\t  ep id %d addr %d dir %s type %s"
			" itype %d maxpkt %d ntds %d\n",
			ep->id, ep->addr, eds, ets, ep->isotype,
			ep->maxpkt, ep->ntds);
	}
	return s;
}

static char*
seprintconf(char *s, char *e, Usbdev *d, int ci)
{
	int i;
	Conf *c;
	char *hd;

	c = d->conf[ci];
	s = seprint(s, e, "\tconf: cval %d attrib %x %d mA\n",
		c->cval, c->attrib, c->milliamps);
	for(i = 0; i < Niface; i++)
		if(c->iface[i] == nil)
			break;
		else
			s = seprintiface(s, e, c->iface[i]);
	for(i = 0; i < Nddesc; i++)
		if(d->ddesc[i] == nil)
			break;
		else if(d->ddesc[i]->conf == c){
			hd = hexstr((uchar*)&d->ddesc[i]->data,
				d->ddesc[i]->data.bLength);
			s = seprint(s, e, "\t\tdev desc %x[%d]: %s\n",
				d->ddesc[i]->data.bDescriptorType,
				d->ddesc[i]->data.bLength, hd);
			free(hd);
		}
	return s;
}

int
Ufmt(Fmt *f)
{
	int i;
	Dev *d;
	Usbdev *ud;
	char buf[1024];
	char *s, *e;

	s = buf;
	e = buf+sizeof(buf);
	d = va_arg(f->args, Dev*);
	if(d == nil)
		return fmtprint(f, "<nildev>\n");
	s = seprint(s, e, "%s", d->dir);
	ud = d->usb;
	if(ud == nil)
		return fmtprint(f, "%s %ld refs\n", buf, d->ref);
	s = seprint(s, e, " csp %s.%uld.%uld",
		classname(Class(ud->csp)), Subclass(ud->csp), Proto(ud->csp));
	s = seprint(s, e, " vid %#ux did %#ux", ud->vid, ud->did);
	s = seprint(s, e, " refs %ld\n", d->ref);
	s = seprint(s, e, "\t%s %s %s\n", ud->vendor, ud->product, ud->serial);
	for(i = 0; i < Nconf; i++){
		if(ud->conf[i] == nil)
			break;
		else
			s = seprintconf(s, e, ud, i);
	}
	return fmtprint(f, "%s", buf);
}

char*
estrdup(char *s)
{
	char *d;

	d = strdup(s);
	if(d == nil)
		sysfatal("strdup: %r");
	setmalloctag(d, getcallerpc(&s));
	return d;
}

void*
emallocz(ulong size, int zero)
{
	void *x;

	x = malloc(size);
	if(x == nil)
		sysfatal("malloc: %r");
	if(zero)
		memset(x, 0, size);
	setmalloctag(x, getcallerpc(&size));
	return x;
}

