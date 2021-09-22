#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "usbfs.h"
#include "serial.h"
#include "acm.h"


/* Supported devices */
Cinfo acminfo[] = {
	{ NOKIAVid,	NOKIADidN95 },
	{ 0,		0 },
};

/*
 * Control capabilities from the class specific header.
 * The driver could use that to figure out which commands
 * are understood by the device (e.g. set control line,
 * set break, etc.)
 */
unsigned int ctrlcaps = 0;

/* ACM control interface number */
int acmctrl;


int
acmmatch(char *info)
{
	Cinfo *ip;
	char buf[50];

	for(ip = acminfo; ip->vid != 0; ip++){
		snprint(buf, sizeof buf, "vid %#06x did %#06x",
			ip->vid, ip->did);
		if(strstr(info, buf) != nil){
			dsprint(2, "serial: device supported by the ACM driver: %s\n", info);
			return 0;
		}
	}
	return -1;
}

char *
desc2str(Desc *desc)
{
	char buf[256], *s, *e;
	uchar dlen, *d;
	int i;

	dlen = desc->data.bLength;
	e = buf + sizeof(buf);
	s = seprint(buf, e, "%02ux", dlen);
	d = (uchar *)&desc->data;
	for (i = 1; i < dlen; i++)
		s = seprint(s, e, " %02ux", d[i]);
	return buf;
}

static int
parseuniondesc(Conf *conf, UnionDesc *udesc, int *epintr, int *epin, int *epout)
{
	int ctrl, data, i;
	Iface *inf, *cinf, *dinf;
	Ep *ep, *cep, *inep, *outep;

	cinf = dinf = nil;
	cep = inep = outep = nil;

	/* Obtain control and data interface ids */
	ctrl = udesc->bMasterInterface0;
	data = udesc->bSlaveInterface0;
	dprint(2, "serial: control interface %d, data interface %d\n", ctrl, data);

	if (ctrl == data){
		dprint(2, "serial: control and data interfaces must be separate\n");
		return -1;
	}

	/* Find control and data interface objects */
	for (i = 0; i < Niface; i++){
		inf = conf->iface[i];
		if (inf){
			if (inf->id == ctrl)
				cinf = inf;
			if (inf->id == data)
				dinf = inf;
		}
	}
	if (cinf == nil){
		dprint(2, "serial: invalid control interface id: %d\n", ctrl);
		return -1;
	}
	if (dinf == nil){
		dprint(2, "serial: invalid data interface id: %d\n", data);
		return -1;
	}

	/* Check interface classes */
	/* TODO: control and data interfaces are switched at some devices.
	 * The driver could handle that. */
	if (Class(cinf->csp) != ComClass){
		dprint(2, "serial: invalid control interface class: %02ulx\n", Class(cinf->csp));
		return -1;
	}
	if (Class(dinf->csp) != DataClass){
		dprint(2, "serial: invalid data interface class: %02ulx\n", Class(dinf->csp));
		return -1;
	}

	/* Find control interface endpoints */
	for (i = 0; i < Nep; i++){
		ep = cinf->ep[i];
		if (ep){
			if (cep == nil)
				cep = ep;
			else
				/* Weird. Ignore it. */
				dprint(2, "serial: multiple endpoints in the control interface, ignoring...\n");
		}
	}
	if (cep == nil){
		dprint(2, "serial: no endpoints in the control interface\n");
		return -1;
	}
	/* Find data interface endpoints */
	for (i = 0; i < Nep; i++){
		ep = dinf->ep[i];
		if (ep){
			if (inep == nil)
				inep = ep;
			else if (outep == nil)
				outep = ep;
			else
				/* Weird. Ignore it. */
				dprint(2, "serial: too many endpoints in the data interface, ignoring...\n");
		}
	}
	if (inep == nil || outep == nil){
		dprint(2, "serial: missing endpoints in the data interface\n");
		return -1;
	}

	/* Check endpoint types */
	/* TODO: input and output endpoints are switched at some devices.
	 * The driver could fix that. */
	if (cep->type != Eintr){
		dprint(2, "serial: invalid control endpoint type: %02x\n", cep->type);
		return -1;
	}
	if (inep->type != Ebulk){
		dprint(2, "serial: invalid input endpoint type: %02x\n", inep->type);
		return -1;
	}
	if (outep->type != Ebulk){
		dprint(2, "serial: invalid output endpoint type: %02x\n", outep->type);
		return -1;
	}
	if (inep->dir == Eout){
		dprint(2, "serial: invalid input endpoint direction\n");
		return -1;
	}
	if (outep->dir == Ein){
		dprint(2, "serial: invalid output endpoint direction\n");
		return -1;
	}
	/* Finally, we seem to have found the right interfaces */
	*epintr = cep->id;
	*epin = inep->id;
	*epout = outep->id;
	acmctrl = ctrl;
	return 0;
}

int
acmfindports(Serial *ser)
{
	int epintr, epin, epout, i;
	Conf *conf;
	Desc **extra, *desc;
	UnionDesc *udesc;

	epintr = epin = epout = -1;
	udesc = nil;

	/* CDC ACM devices always have an interrupt endpoints */
	ser->hasepintr = 1;

	/* Current configuration */
	conf = ser->dev->usb->conf[0];

	/* Extra descriptors (i.e., none of device, configuration, interface or endpoint) */
	extra = ser->dev->usb->ddesc;

	/* Find a union interface */
	for(i = 0; i < Nddesc; i++){
		desc = extra[i];
		if(desc == nil)
			break;
		ddprint(2, "serial: parsing descriptor: %s\n", desc2str(desc));
		if(desc->conf != conf){
			/* Do not process descriptors that belong to alternative configurations */
			break;
		}
		if(desc->data.bDescriptorType != Dfunction){
			/* Weird descriptor. Skip it. */
			dprint(2, "serial: skipping descriptor type %02x\n", desc->data.bDescriptorType);
			continue;
		}
		/* Byte 0 is the descriptor subtype */
		switch (desc->data.bbytes[0]){
		case HeaderType:
			ddprint(2, "serial: skipping header descriptor\n");
			break;
		case ACMType:
			dprint(2, "serial: found an ACM type descriptor\n");
			ctrlcaps = desc->data.bbytes[1];
			break;
		case UnionType:
			/* This is the descriptor we are looking for! */
			udesc = (UnionDesc *)&desc->data;
			dprint(2, "serial: found a union descriptor\n");
			if (parseuniondesc(conf, udesc, &epintr, &epin, &epout) == 0){
				/* Found the endpoints. Exit the loop. */
				i = Nddesc;
			}
			break;
		default:
			/* Unsupported descriptor */
			ddprint(2, "serial: ignoring descriptor: unknown subtype %02ux\n",
					desc->data.bbytes[0]);
			break;
		}
	}
	if(!udesc){
		dprint(2, "serial: no union descriptor found\n");
		return -1;
	}
	if(epin == -1 || epout == -1 || epintr == -1)
		return -1; /* Something went wrong... */

	dprint(2, "serial: found endpoints: intr %d, in %d, out %d\n",
			epintr, epin, epout);

	if (!addport(ser, epin, epout, epintr)){
		return -1;
	}
	return 0;
}

/* 
 *The reason to have this function is that data must be read
 * in chunks of at least 64 bytes 
 */
static int
acmwait4data(Serialport *p, uchar *buf, int count)
{
	int dfd;
	int rcount;

	dfd = p->epin->dfd;
	
	if(p->ndata == 0){
		/* There is no data in the internal buffer */
		if(count >= DataBufSz){
			/* Standard read directly into the caller's buffer */
			dprint(2, "serial: acmwait4data:  reading %d bytes\n", count);
			qunlock(p->s);
			rcount = read(dfd, buf, count);
			qlock(p->s);
			return rcount;
		}else{
			/* Read data into the internal buffer */
			dprint(2, "serial: acmwait4data: reading %d bytes into buffer\n", DataBufSz);
			qunlock(p->s);
			rcount = read(dfd, p->data, DataBufSz);
			qlock(p->s);
			if (rcount <= 0)
				return rcount;
			p->ndata = rcount;
			p->pdata = p->data;
		}
	}
	/* Data available in the buffer */
	assert(p->ndata > 0);
	if(p->ndata <= count){
		/* Pass all the data to the caller */
		dprint(2, "serial: acmwait4data: copying all %d bytes from buffer\n", p->ndata);
		memcpy(buf, p->pdata, p->ndata);
		rcount    = p->ndata;
		p->pdata += p->ndata;
		p->ndata  = 0;
	}else{
		/* Pass only as much data as the caller asked for */
		dprint(2, "serial: acmwait4data: copying %d bytes from buffer\n", count);
		memcpy(buf, p->pdata, count);
		p->ndata -= count;
		p->pdata += count;
		rcount = count;
	}
	return rcount;
}

static int
acmreset(Serialport *p)
{
	p->ndata = 0;
	return 0;
}

/* This function is unused */
static int
acmseteps(Serialport *p)
{
	char *s;

	dprint(2, "serial: maxpkt in: %d\n", p->epin->maxpkt);
	s = smprint("maxpkt %d", p->epin->maxpkt);
	devctl(p->epin, s);
	free(s);

	dprint(2, "serial: maxpkt out: %d\n", p->epout->maxpkt);
	s = smprint("maxpkt %d", p->epout->maxpkt);
	devctl(p->epout, s);
	free(s);

	dprint(2, "serial: maxpkt intr: %d\n", p->epintr->maxpkt);
	s = smprint("maxpkt %d", p->epintr->maxpkt);
	devctl(p->epintr, s);
	free(s);
	return 0;
}

/*
 * Functions for ACM control messages.
 */
static int
acmctrlmsg(Serialport *p, int req, int val, void *buf, int len)
{
	int result = usbcmd(p->s->dev, Rclass | Riface, 
			req, val, acmctrl, buf, len);
	dprint(2, "serial: acm control msg: req=%x val=%x len=%d result=%d\n",
			req, val, len, result);
	return result < 0 ? result : 0;
}

/*
 * From the Linux driver:
 * devices aren't required to support these requests.
 * the cdc acm descriptor tells whether they do...
 */
static int
acmsetctrline(Serialport *p, int state){
	return acmctrlmsg(p, ReqSetCtrlLineState, state, nil, 0);
}

static int
acmsetcoding(Serialport *p, LineCoding *lc)
{
	return acmctrlmsg(p, ReqSetLineCoding, 0, lc, sizeof(LineCoding));
}

static int
acmsendbreak(Serialport *p, int state)
{
	return acmctrlmsg(p, ReqSendBreak, state, nil, 0);
}

static int
acmsetbreak(Serialport *p, int state)
{
	return acmsendbreak(p, state ? 0xffff : 0);
}

static int
acmsendlines(Serialport *p)
{
	int res;

	if (p->rts)
		p->ctlstate |= CtrlRTS;
	else
		p->ctlstate &= ~CtrlRTS;
	if (p->dtr)
		p->ctlstate |= CtrlDTR;
	else
		p->ctlstate &= ~CtrlDTR;

	dsprint(2, "serial: sendlines: %x\n", p->ctlstate);
	res = acmsetctrline(p, p->ctlstate);
	dsprint(2, "serial: sendlines result: %d\n", res);
	return 0;
}

static int
acmsetparam(Serialport *p)
{
	LineCoding lc;
	int res;

	PUT4(lc.dwDTERate, p->baud);

	switch (p->stop){
	case 1:
		lc.bCharFormat = StopBits1;
		break;
	case 2:
		lc.bCharFormat = StopBits2;
		break;
	case 15:
		lc.bCharFormat = StopBits15;
		break;
	default:
		dsprint(2, "serial: invalid number of stop bits: %d\n", p->stop);
		return -1;
	}
	lc.bParityType = p->parity;
	lc.bDataBits = p->bits;

	/*
	 * The Linux driver does this, but I'm not sure if that's necessary...
	if (p->baud == 0){
		// hang up 
		if (p->dtr){
			p->dtr = 0;
			acmsendlines(p);
		}
	}else{
		if (!p->dtr){
			p->dtr++;
			acmsendlines(p);
		}
	}
	*/

	dprint(2, "serial: setparam: %d %d %d %d\n", GET4(lc.dwDTERate),
			lc.bCharFormat, lc.bParityType, lc.bDataBits);
	res = acmsetcoding(p, &lc);
	dsprint(2, "serial: setparam result: %d\n", res);
	return 0;
}

static int
acminit(Serialport *p)
{
	p->ndata = 0;

	p->rts = 0;
	p->dtr = 0;
	acmsendlines(p);

	p->baud = 9600;
	p->stop = 1;
	p->parity = NoParity;
	p->bits = 8;
	acmsetparam(p);

	/*
	p->ctrlstate |= CtrlRTS | CtrlDTR;
	if (acmsetctrline(p, p->ctlstate) < 0){
		if (ctrlcaps & CapLine){
			dprint(2, "serial: unable to set control line\n");
			return -1;
		}
	} 
	*/
	return 0;
}

Serialops acmops = {
	.findports	= acmfindports,
//	.seteps		= acmseteps,  /* unused */
	.init		= acminit,
	.reset		= acmreset,
	.wait4data	= acmwait4data,
	.setbreak	= acmsetbreak,
	.sendlines	= acmsendlines,
	.setparam	= acmsetparam,
};


