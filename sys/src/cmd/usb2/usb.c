/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include "usb.h"


enum {
	/* fundamental constants */

	Uctries	= 4,	/* no. of tries for usbcmd */
	Ucdelay	= 50,	/* delay before retrying */

	/* request type */
	Rh2d	= 0<<7,		/* host to device */
	Rd2h	= 1<<7,		/* device to host */

	Rstd	= 0<<5,		/* types */
	Rclass	= 1<<5,
	Rvendor	= 2<<5,

	Rdevice	= 0,		/* recipients */
	Riface	= 1,
	Rendpt	= 2,		/* endpoint */
	Rother	= 3,

	/* standard requests */
	Rgetstatus	= 0,
	Rclearfeature	= 1,
	Rsetfeature	= 3,
	Rsetaddress	= 5,
	Rgetdesc	= 6, // set/get descriptor (addressed with type and index)
	Rsetdesc	= 7,
	Rgetconf	= 8, // set/get configuration (1 byte)
	Rsetconf	= 9,
	Rgetiface	= 10, // set/get interface(index) + alt setting(value) (1 byte)
	Rsetiface	= 11,
	Rsynchframe	= 12,

	Rgetcur	= 0x81,
	Rgetmin	= 0x82,
	Rgetmax	= 0x83,
	Rgetres	= 0x84,
	Rsetcur	= 0x01,
	Rsetmin	= 0x02,
	Rsetmax	= 0x03,
	Rsetres	= 0x04,

	/* dev classes */
	Clnone		= 0,		/* not in usb */
	Claudio		= 1,
	Clcomms		= 2,
	Clhid		= 3,
	Clprinter	= 7,
	Clstorage	= 8,
	Clhub		= 9,
	Cldata		= 10,

	/* standard descriptor sizes */
	Ddevicelen		= 18,
	Dconfiglen	= 9,
	Difacelen	= 9,
	Dendptlen		= 7,

	/* descriptor types */
	Ddevice		= 1,
	Dconfig		= 2,
	Dstring		= 3,
	Diface		= 4,
	Dendpt		= 5,
	Dreport		= 0x22,
	Dfunction	= 0x24,
	Dphysical	= 0x23,

	/* feature selectors */
	Fdevremotewakeup = 1,
	Fhalt 	= 0,

	/* device state */
	Detached = 0,
	Attached,
	Enabled,
	Assigned,
	Configured,

	/* endpoint direction */
	Ein = 0,
	Eout,
	Eboth,

	/* endpoint type */
	Econtrol = 0,
	Eiso = 1,
	Ebulk = 2,
	Eintr = 3,

	/* endpoint isotype */
	Eunknown = 0,
	Easync = 1,
	Eadapt = 2,
	Esync = 3,

	/* config attrib */
	Cbuspowered = 1<<7,
	Cselfpowered = 1<<6,
	Cremotewakeup = 1<<5,

	/* report types */
	Tmtype	= 3<<2,
	Tmitem	= 0xF0,
	Tmain	= 0<<2,
		Tinput	= 0x80,
		Toutput	= 0x90,
		Tfeature = 0xB0,
		Tcoll	= 0xA0,
		Tecoll	= 0xC0,
	 Tglobal	= 1<<2,
		Tusagepage = 0x00,
		Tlmin	= 0x10,
		Tlmax	= 0x20,
		Tpmin	= 0x30,
		Tpmax	= 0x40,
		Tunitexp	= 0x50,
		Tunit	= 0x60,
		Trepsize	= 0x70,
		TrepID	= 0x80,
		Trepcount = 0x90,
		Tpush	= 0xA0,
		Tpop	= 0xB0,
	 Tlocal	= 2<<2,
		Tusage	= 0x00,
		Tumin	= 0x10,
		Tumax	= 0x20,
		Tdindex	= 0x30,
		Tdmin	= 0x40,
		Tdmax	= 0x50,
		Tsindex	= 0x70,
		Tsmin	= 0x80,
		Tsmax	= 0x90,
		Tsetdelim = 0xA0,
	 Treserved	= 3<<2,
	 Tint32_t	= 0xFE,

};

typedef struct Usbdevdesc Usbdevdesc;
typedef struct Usbconfdesc Usbconfdesc;
typedef struct Usbstringdesc0 Usbstringdesc0;
typedef struct Usbstringdesc Usbstringdesc;
typedef struct Usbifacedesc Usbifacedesc;
typedef struct Usbendptdesc Usbendptdesc;

struct Usbdevdesc {
	uint8_t bLength;
	uint8_t bDescriptorType;
	//	Device Descriptor (0x01, Ddevice)
	uint8_t bcdUSB[2];
	//	USB Specification Number which device complies too.
	uint8_t bDeviceClass;
	/*	Class Code (Assigned by USB Org)
	 *	0x00: each interface specifies it’s own class code
	 *	0xFF, the class code is vendor specified.
	 *	Otherwise field is valid Class Code. */
	uint8_t bDeviceSubClass;
	//	Subclass Code (Assigned by USB Org)
	uint8_t bDeviceProtocol;
	//	Protocol Code (Assigned by USB Org)
	uint8_t bMaxPacketSize;
	//	Valid Sizes are 8, 16, 32, 64
	uint8_t idVendor[2];
	uint8_t idProduct[2];
	uint8_t bcDdeviceice[2];
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
};

struct Usbconfdesc {
	uint8_t bLength;
	uint8_t bDescriptorType;
	//	Configuration Descriptor (0x02, Dconfig)
	uint8_t wTotalLength[2];
	//	Total length in bytes of data returned
	uint8_t bNumInterfaces;
	//	Number of Interfaces
	uint8_t bConfigurationValue;
	//	Value to use as an argument to select this configuration
	uint8_t iConfiguration;
	//	Index of String Descriptor describing this configuration
	uint8_t bmAttributes;
	/*	D7 Reserved, set to 1. (USB 1.0 Bus Powered)
	 *	D6 Self Powered
	 *	D5 Remote Wakeup
	 *	D4..0 Reserved, set to 0. */
	uint8_t bMaxPower;
	//	Maximum Power Consumption in 2mA units
};

struct Usbstringdesc0 {
	uint8_t bLength;
	//	Size of Descriptor in Bytes
	uint8_t bDescriptorType;
	//	String Descriptor (0x03, Dstring)
	uint8_t wLANGID0[2];
	//	Supported Language Code Zero. bLength indicates how many wLANGIDs there are.
};

struct Usbstringdesc {
	uint8_t bLength;
	//	Size of Descriptor in Bytes
	uint8_t bDescriptorType;
	//	String Descriptor (0x03, Dstring)
	uint8_t bString[1];
	//	Unicode Encoded String
};

struct Usbifacedesc {
	uint8_t bLength;
	//	Size of Descriptor in Bytes (9 Bytes)
	uint8_t bDescriptorType;
	//	Interface Descriptor (0x04, Diface)
	uint8_t bInterfaceNumber;
	//	Number of Interface
	uint8_t bAlternateSetting;
	//	Value used to select alternative setting
	uint8_t bNumEndpoints;
	//	Number of Endpoints used for this interface
	uint8_t bInterfaceClass;
	//	Class Code (Assigned by USB Org)
	uint8_t bInterfaceSubClass;
	//	Subclass Code (Assigned by USB Org)
	uint8_t bInterfaceProtocol;
	//	Protocol Code (Assigned by USB Org)
	uint8_t iInterface;
	//	Index of String Descriptor Describing this interface
	uint8_t extra[256-9];
};

struct Usbendptdesc {
	uint8_t bLength;
	//	Size of Descriptor in Bytes (7 bytes)
	uint8_t bDescriptorType;
	//	Endpoint Descriptor (0x05, Dendpt)
	uint8_t bEndpointAddress;
	/*	Endpoint Address
	 *	Bits 0..3b Endpoint Number.
	 *	Bits 4..6b Reserved. Set to Zero
	 *	Bits 7 Direction 0 = Out, 1 = In (Ignored for Control Endpoints) */
	uint8_t bmAttributes;
	/*	Bits 0..1 Transfer Type
	 *	00 = Control
	 *	01 = Isochronous
	 *	10 = Bulk
	 *	11 = Interrupt
	 *	Bits 2..7 are reserved. If Isochronous endpoint,
	 *	Bits 3..2 = Synchronisation Type (Iso Mode)
	 *	00 = No Synchonisation
	 *	01 = Asynchronous
	 *	10 = Adaptive
	 *	11 = Synchronous
	 *	Bits 5..4 = Usage Type (Iso Mode)
	 *	00 = Data Endpoint
	 *	01 = Feedback Endpoint
	 *	10 = Explicit Feedback Data Endpoint
	 *	11 = Reserved */
	uint8_t wMaxPacketSize[2];
	//	Maximum Packet Size this endpoint is capable of sending or receiving
	uint8_t bInterval;
	/*	Interval for polling endpoint data transfers. Value in frame counts.
	 *	Ignored for Bulk & Control Endpoints. Isochronous must equal 1,
	 *	may range from 1 to 255 for interrupt endpoints. */
};

static void
put16(uint8_t *buf, uint16_t val)
{
	buf[0] = val & 0xff;
	buf[1] = (val >> 8) & 0xff;
}

static uint16_t
get16(uint8_t *buf)
{
	uint16_t val;
	val = (uint16_t)buf[0] + ((uint16_t)buf[1] << 8);
	return val;
}

static int
cmdreq(int fd, int type, int req, int value, int index, uint8_t *data, int count)
{
	int ndata, n;
	uint8_t *wp;
	uint8_t buf[8];

	if(data == nil){
		wp = buf;
		ndata = 0;
	}else{
		ndata = count;
		wp = malloc(8+ndata);
	}
	wp[0] = type;
	wp[1] = req;
	put16(wp+2, value);
	put16(wp+4, index);
	put16(wp+6, count);
	if(data != nil)
		memmove(wp+8, data, ndata);

	n = write(fd, wp, 8+ndata);
	if(wp != buf)
		free(wp);
	if(n < 0)
		return -1;
	if(n != 8+ndata){
		fprint(2, "%s: cmd: short write: %d\n", argv0, n);
		return -1;
	}
	return n;
}

static int
cmdrep(int fd, void *buf, int nb)
{
	nb = read(fd, buf, nb);

	return nb;
}

static int
usbcmd(int fd, int type, int req, int value, int index, uint8_t *data, int count)
{
	int i, r, nerr;
	char err[64];

	/*
	 * Some devices do not respond to commands some times.
	 * Others even report errors but later work just fine. Retry.
	 */
	r = -1;
	*err = 0;
	for(i = nerr = 0; i < Uctries; i++){
		if(type & Rd2h)
			r = cmdreq(fd, type, req, value, index, nil, count);
		else
			r = cmdreq(fd, type, req, value, index, data, count);
		if(r > 0){
			if((type & Rd2h) == 0)
				break;
			r = cmdrep(fd, data, count);
			if(r > 0)
				break;
			if(r == 0)
				werrstr("no data from device");
		}
		nerr++;
		if(*err == 0)
			rerrstr(err, sizeof(err));
		sleep(Ucdelay);
	}
	if(r > 0 && i >= 2)
		fprint(2, "usbcmd required %d attempts (%s)\n", i, err);
	return r;
}

int
usbdescread(int fd, uint8_t *buf, int len, int desctype, int index)
{
	return usbcmd(fd, Rd2h|Rstd|Rdevice, Rgetdesc, desctype<<8|0, index, buf, len);
}

int
usbconfprint(int fd, Usbconfig *cp)
{
	int nwr, ntot;
	int i;

	ntot = 0;
	nwr = fprint(fd, "config %d iface %d alt %d\n", cp->config, cp->iface, cp->alt);
	if(nwr == -1)
		return -1;
	ntot += nwr;
	for(i = 0; i < cp->nendpt; i++){
		nwr = fprint(
			fd,
			"	endpt[%d]: addr %d type %d maxpkt %d pollival %d\n",
			i,
			cp->endpt[i].addr,
			cp->endpt[i].type,
			cp->endpt[i].maxpkt,
			cp->endpt[i].pollival
		);
		if(nwr == -1)
			return -1;
		ntot += nwr;
	}
	if(cp->nextra > 0){
		for(i = 0; i < cp->nextra; i++){
			nwr = fprint(fd, "%s0x%02x%s", i%16==0 ? "\t" : "", cp->extra[i], i < cp->nextra-1 ? ", " : "\n");
			if(nwr == -1)
				return -1;
			ntot += nwr;
		}
	}
	return ntot;
}

int
usbconfread(int fd, Usbconfig **confp)
{
	Usbconfig *confs;
	int nconfs, aconfs;
	uint8_t buf[1024];
	Usbdevdesc devdesc;
	Usbconfdesc confdesc;
	Usbifacedesc ifacedesc;
	Usbendptdesc endptdesc;

	int nrd;
	int cfg, ncfg;
	int ifi, niface;
	int epi;


	nrd = usbdescread(fd, buf, sizeof buf, Ddevice, 0);
	if(nrd == -1)
		sysfatal("readdesc Ddevice");
	memmove(&devdesc, buf, sizeof devdesc);

	nconfs = 0;
	aconfs = 4;
	confs = malloc(aconfs * sizeof confs[0]);

	ncfg = devdesc.bNumConfigurations;
	for(cfg = 0; cfg < ncfg; cfg++){
		int off = 0;
		nrd = usbdescread(fd, buf, sizeof buf, Dconfig, cfg);
		if(nrd == -1)
			sysfatal("readdesc Dconfig");

		if(off >= nrd || off+buf[off] > nrd){
			print("premature end of config data");
			goto done;
		}
		if(buf[off+1] != Dconfig)
			sysfatal("non-config descriptor %d\n", confdesc.bDescriptorType);

		memmove(&confdesc, buf+off, buf[off]);
		off += confdesc.bLength;
		niface = confdesc.bNumInterfaces;
		for(ifi = 0; ifi < niface; ifi++){
			Usbconfig *cp;
ifskip:
			if(off >= nrd || off+buf[off] > nrd){
				print("premature end of config data");
				goto done;
			}
			if(buf[off+1] != Diface){
				print("non-iface descriptor %d in config\n", buf[off+1]);
				off += buf[off];
				goto ifskip;
			}
			memmove(&ifacedesc, buf+off, buf[off]);
			if(nconfs == aconfs){
				aconfs += 4;
				confs = realloc(confs, aconfs * sizeof confs[0]);
			}
			cp = &confs[nconfs++];
			memset(cp, 0, sizeof cp[0]);
			cp->config = confdesc.bConfigurationValue;
			cp->iface = ifacedesc.bInterfaceNumber;
			cp->alt = ifacedesc.bAlternateSetting;
			cp->nendpt = ifacedesc.bNumEndpoints;
			if(cp->nendpt > nelem(cp->endpt)){
				print("out of endpoints in usbconfig (need %d have %d)\n", cp->nendpt, nelem(cp->endpt));
				cp->nendpt = nelem(cp->endpt);
			}

			off += ifacedesc.bLength;
			for(epi = 0; epi < cp->nendpt; epi++){
epskip:
				if(off >= nrd || off+buf[off] > nrd){
					print("premature end of config data\n");
					goto done;
				}
				if(buf[off+1] != Dendpt){
					if(cp->nextra+buf[off] <= sizeof cp->extra){
						memmove(cp->extra + cp->nextra, buf+off, buf[off]);
						cp->nextra += buf[off];
					} else {
						print("out of extra space in usbconfig, desc type 0x%02x\n", buf[off+1]);
					}
					off += buf[off];
					goto epskip;
				}
				memmove(&endptdesc, buf+off, buf[off]);
				off += endptdesc.bLength;
				cp->endpt[epi].addr = endptdesc.bEndpointAddress;
				cp->endpt[epi].type = endptdesc.bmAttributes;
				cp->endpt[epi].maxpkt = get16(endptdesc.wMaxPacketSize);
				cp->endpt[epi].pollival = endptdesc.bInterval;
			}
		}
	}
done:

	aconfs = nconfs;
	confs = realloc(confs, aconfs * sizeof confs[0]);
	*confp = confs;
	return nconfs;
}

int
usbopen(int fd, Usbconfig *cp, int epi, int *ctlp)
{
	char *p, buf[1024];
	int epaddr, eptype, isread;
	int ctl, epctl, epdata;
	int n;

	ctl = -1;
	epctl = -1;
	epdata = -1;
	epaddr = cp->endpt[epi].addr&7;
	eptype = cp->endpt[epi].type&3;
	isread = (cp->endpt[epi].addr&128) != 0;

	if(usbcmd(fd, Rh2d|Rstd|Rdevice, Rsetconf, cp->config, 0, nil, 0) == -1){
		fprint(2, "usbopen: Rsetconf fail\n");
		goto fail;
	}

	if(usbcmd(fd, Rh2d|Rstd|Riface, Rsetiface, cp->alt, cp->iface, nil, 0) == -1){
		fprint(2, "usbopen: Rsetiface fail\n");
		goto fail;
	}

	fd2path(fd, buf, sizeof buf);
	if((p = strrchr(buf, '/')) == nil){
		fprint(2, "usbopen: no '.' in data path '%s'\n", buf);
		goto fail;
	}
	strcpy(p, "/ctl");
	if((ctl = open(buf, ORDWR)) == -1){
		fprint(2, "usbopen: open '%s' failed: %r\n", buf);
		goto fail;
	}

	n = snprint(buf, sizeof buf, "new %d %d %s", epaddr, eptype, isread ? "r" : "w");
	if(write(ctl, buf, n) != n){
		fprint(2, "usbopen: write failed '%s': %r\n", buf);
		//goto fail;
	}

	fd2path(fd, buf, sizeof buf);
	if((p = strrchr(buf, '.')) == nil){
		fprint(2, "usbopen: no '.' in ctl path '%s'\n", buf);
		goto fail;
	}
	snprint(p, sizeof buf - (p-buf), ".%d/ctl", epaddr);
	if((epctl = open(buf, ORDWR)) == -1){
		fprint(2, "usbopen: open '%s': %r\n", buf);
		goto fail;
	}
	n = snprint(buf, sizeof buf, "maxpkt %d", cp->endpt[epi].maxpkt);
	if(write(epctl, buf, n) != n){
		fprint(2, "usbopen: '%s': %r\n", buf);
		goto fail;
	}
	if(eptype == Eintr || eptype == Eiso){
		n = snprint(buf, sizeof buf, "pollival %d", cp->endpt[epi].pollival);
		if(write(epctl, buf, n) != n){
			fprint(2, "usbopen: pollival failed\n");
			goto fail;
		}
	}


	fd2path(fd, buf, sizeof buf);
	if((p = strrchr(buf, '.')) == nil){
		fprint(2, "usbopen: no '.' in ctl path '%s'\n", buf);
		goto fail;
	}
	snprint(p, sizeof buf - (p-buf), ".%d/data", epaddr);
	if((epdata = open(buf, isread ? OREAD : OWRITE)) == -1){
		fprint(2, "usbopen: open '%s': %r\n", buf);
		goto fail;
	}

	if(ctlp != nil)
		*ctlp = epctl;
	else
		close(epctl);
	if(ctl != -1)
		close(ctl);
	return epdata;

fail:
	if(ctl != -1)
		close(ctl);
	if(epctl != -1)
		close(epctl);
	if(epdata != -1)
		close(epdata);
	return -1;
}
