#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"

#include "dat.h"
#include "fns.h"

Hub*
roothub(int ctlrno)
{
	Hub *h;
	char name[100];

	h = emallocz(sizeof(Hub), 1);
	h->isroot = 1;
	h->ctlrno = ctlrno;
	h->nport = 2;			/* BUG */
	h->port = emallocz(h->nport*sizeof(Port), 1);

	sprint(name, "/dev/usb%d/port", ctlrno);
	if((h->portfd = open(name, ORDWR)) < 0){
		werrstr("open %s: %r", name);
		free(h);
		return nil;
	}

	h->dev0 = opendev(ctlrno, 0);
	h->d = h->dev0;
	incref(h->d);
	return h;
}

Hub*
newhub(Hub *parent, Device *d)
{
	Port *p;
	Hub *h;
	DHub *dd;
	byte buf[128], *PortPwrCtrlMask;
	int nr, nport, nmap, i, offset, mask;

	h = emallocz(sizeof(Hub), 1);
	h->d = d;
	h->ctlrno = parent->ctlrno;
	h->dev0 = parent->dev0;

	if (setupreq(d->ep[0], RD2H|Rclass|Rdevice, GET_DESCRIPTOR, HUB<<8|0,
	    0, DHUBLEN) < 0 ||
	   (nr = setupreply(d->ep[0], buf, sizeof(buf))) < DHUBLEN) {
		fprint(2, "usbd: error reading hub descriptor\n");
		free(h);
		return nil;
	}
	pdesc(d, -1, -1, buf, nr);
	dd = (DHub*)buf;
	nport = dd->bNbrPorts;
	nmap = 1 + nport/8;
	if(nr < 7 + 2*nmap) {
		fprint(2, "usbd: hub descriptor too small\n");
		free(h);
		return nil;
	}

	h->nport = nport;
	h->port = emallocz(nport*sizeof(Port), 1);
	h->pwrms = dd->bPwrOn2PwrGood*2;
	h->maxcurrent = dd->bHubContrCurrent;
	h->pwrmode = dd->wHubCharacteristics[0] & 3;
	h->compound = (dd->wHubCharacteristics[0] & (1<<2))!=0;

	PortPwrCtrlMask = dd->DeviceRemovable + nmap;
	for(i = 1; i <= nport; i++) {
		p = &h->port[i-1];
		offset = i/8;
		mask = 1<<(i%8);
		p->removable = (dd->DeviceRemovable[offset] & mask) != 0;
		p->pwrctl = (PortPwrCtrlMask[offset] & mask) != 0;
	}
	incref(d);
	incref(h->dev0);
	return h;
}

void
freehub(Hub *h)
{
	int i;
	Port *p;

	if(h == nil)
		return;
	for(i = 1; i <= h->nport; i++) {
		p = &h->port[i-1];
		freehub(p->hub);
		closedev(p->d);
	}
	free(h->port);
	if(h->isroot)
		close(h->portfd);
	else
		closedev(h->d);
	closedev(h->dev0);
	free(h);
}

int
Hfmt(Fmt *f)
{
	Hub *h;

	h = va_arg(f->args, Hub*);
	return fmtprint(f, "usb%d/%d", h->ctlrno, h->d->id);
}

static void
hubfeature(Hub *h, int port, int feature, int on)
{
	int cmd;

	cmd = CLEAR_FEATURE;
	if(on)
		cmd = SET_FEATURE;
	setup0(h->d, RH2D|Rclass|Rother, cmd, feature, port, 0);
}

void
portenable(Hub *h, int port, int on)
{
	if(h->isroot){
		if(fprint(h->portfd, "%s %d", on? "enable": "disable", port) < 0)
			sysfatal("usbd: portenable: write error: %r");
		return;
	}
	if(port == 0)
		return;
	hubfeature(h, port, PORT_ENABLE, on);
}

void
portreset(Hub *h, int port)
{
	if(h->isroot) {
		if(fprint(h->portfd, "reset %d", port) < 0)
			sysfatal("usbd: portreset: write error: %r");
		sleep(100);
		return;
	}
	if(port == 0)
		return;
	hubfeature(h, port, PORT_RESET, 1);
}

void
portpower(Hub *h, int port, int on)
{
	if(h->isroot) {
		/* no power control */
		return;
	}
	if(port == 0)
		return;
	hubfeature(h, port, PORT_POWER, on);
}

static struct
{
	int	bit;
	char	*name;
}
statustab[] =
{
	{ 1<<PORT_SUSPEND,		"suspend", },
	{ 1<<PORT_RESET,		"reset", },
	{ 1<<PORT_LOW_SPEED,		"lowspeed", },
	{ 1<<PORT_ENABLE,		"enable", },
	{ 1<<PORT_CONNECTION,		"present", },
};

int
portstatus(Hub *h, int port)
{
	int x;
	Endpt *e;
	byte buf[4];
	int n, nf, i, j;
	char *status, *q, *qe, *field[20];

	if(h->isroot) {
		seek(h->portfd, 0, 0);
		status = malloc(8192);
		n = read(h->portfd, status, 8192);
		if (n <= 0)
			sysfatal("usbd: can't read usb port status: %r");
		status[n] = '\0';
		q = status;
		for(;;) {
			qe = strchr(q, '\n');
			if(qe == nil)
				sysfatal("usbd: port %H.%d not found", h, port);
			*qe = '\0';
			nf = tokenize(q, field, nelem(field));
			if(nf < 2)
				sysfatal("Ill-formed port status: %s", q);
			if(strtol(field[0], nil, 0) == port)
				break;
			q = qe+1;
		}
		x = 0;
		for(i = 2; i < nf; i++) {
			for(j = 0; j < nelem(statustab); j++) {
				if(strcmp(field[i], statustab[j].name) == 0) {
					x |= statustab[j].bit;
					break;
				}
			}
		}
		free(status);
		return x;
	}
	e = h->d->ep[0];
	if (setupreq(e, RD2H|Rclass|Rother, GET_STATUS, 0, port, sizeof buf) < 0
	  || setupreply(e, buf, sizeof(buf)) < sizeof(buf)) {
		if (debug)
			sysfatal("usbd: error reading hub status %H.%d", h, port);
		return 0;
	}
	return GET2(buf);
}
