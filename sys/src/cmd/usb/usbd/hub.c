#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbproto.h"

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
	byte buf[256], *PortPwrCtrlMask;
	int nr, nport, nmap, i, offset, mask;

	h = emallocz(sizeof(Hub), 1);
	h->d = d;
	h->ctlrno = parent->ctlrno;
	h->dev0 = parent->dev0;

	nr = setupreq(d, RD2H|Rclass|Rdevice, GET_DESCRIPTOR, (0<<8)|0, 0, buf, sizeof(buf));
	if(nr < 0) {
		werrstr("newhub: %r");
Error:
		free(h);
		return nil;
	}
	if(nr < DHUBLEN) {
		werrstr("newhub: short device descriptor (%d < %d)", nr, DHUBLEN);
		goto Error;
	}
	dd = (DHub*)buf;
	nport = dd->bNbrPorts;
	nmap = 1 + nport/8;
	if(nr < 7 + 2*nmap) {
		werrstr("newhub: hub descriptor too small");
		goto Error;
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

static int
hubfeature(Hub *h, int port, int feature, int on)
{
	int cmd;

	if(on)
		cmd = SET_FEATURE;
	else
		cmd = CLEAR_FEATURE;
	return setupreq(h->d, Rclass|Rother, cmd, feature, port, nil, 0);
}

int
portenable(Hub *h, int port, int on)
{
	if(h->isroot)
		return fprint(h->portfd, "%s %d", on? "enable": "disable", port);
	return hubfeature(h, port, PORT_ENABLE, on);
}

int
portreset(Hub *h, int port)
{
	if(h->isroot) {
		if(fprint(h->portfd, "reset %d", port) < 0)
			return -1;
		sleep(100);
		return 0;
	}
	return hubfeature(h, port, PORT_RESET, 1);
}

int
portpower(Hub *h, int port, int on)
{
	if(h->isroot) {
		/* no power control */
		return 0;
	}
	return hubfeature(h, port, PORT_POWER, on);
}

static struct
{
	int	bit;
	char	*name;
}
statustab[] =
{
	{ 1<<PORT_SUSPEND,		"suspend", },
	{ 1<<PORT_RESET,			"reset", },
	{ 1<<PORT_LOW_SPEED,		"lowspeed", },
	{ 1<<PORT_ENABLE,			"enable", },
	{ 1<<PORT_CONNECTION,	"present", },
};

int
portstatus(Hub *h, int port)
{
	int x;
	byte buf[4];
	int n, nf, i, j;
	char *status, *q, *qe, *field[20];

	if(h->isroot) {
		seek(h->portfd, 0, 0);
		status = malloc(8192);
		n = read(h->portfd, status, 8192);
		if(n <= 0) {
			werrstr("portstatus read: %r");
			free(status);
			return -1;
		}
		status[n] = '\0';
		q = status;
		for(;;) {
			qe = strchr(q, '\n');
			if(qe == nil)
				sysfatal("usbd: port %H.%d not found", h, port);
			*qe = '\0';
			nf = tokenize(q, field, sizeof field);
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
	if(setupreq(h->d, RD2H|Rclass|Rother, GET_STATUS, 0, port, buf, sizeof(buf)) < sizeof(buf)) {
		werrstr("portstatus: setupreq: %r");
		return -1;
	}
	return GET4(buf);
}
