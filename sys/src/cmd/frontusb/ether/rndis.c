#include <u.h>
#include <libc.h>
#include <thread.h>

#include "usb.h"
#include "dat.h"

static uchar minit[24] = {
	2,  0, 0, 0, /* type = 2 (init) */
	24, 0, 0, 0, /* len = 24 */
	0,  0, 0, 0, /* rid = 1 */
	1,  0, 0, 0, /* vmajor = 1 */
	1,  0, 0, 0, /* vminor = 1 */
	0,  0, 0, 0, /* max xfer */
};
static uchar mgetmac[28] = {
	4,  0, 0, 0, /* type = 4 (query) */
	28, 0, 0, 0, /* len = 28 */
	0,  0, 0, 0, /* rid = 2 */
	1,  1, 1, 1, /* oid = get "permanent address" */
	0,  0, 0, 0, /* buflen = 0 */
	0,  0, 0, 0, /* bufoff = 0 */
	0,  0, 0, 0, /* reserved = 0 */
};
static uchar mfilter[32] = {
	5,  0, 0, 0, /* type = 5 (set) */
	32, 0, 0, 0, /* len = 32 */
	0,  0, 0, 0, /* rid = 3 */
	14, 1, 1, 0, /* oid = "current filter" */
	4,  0, 0, 0, /* buflen = 4 */
	20, 0, 0, 0, /* bufoff = 20 (8+20=28) */
	0,  0, 0, 0, /* reserved = 0 */
	12, 0, 0, 0, /* filter = all multicast + broadcast */
};

static int
rndisout(Dev *d, int id, uchar *msg, int sz)
{
	return usbcmd(d, Rh2d|Rclass|Riface, Rgetstatus, 0, id, msg, sz);
}

static int
rndisin(Dev *d, int id, uchar *buf, int sz)
{
	int r, status;
	for(;;){
		if((r = usbcmd(d, Rd2h|Rclass|Riface, Rclearfeature, 0, id, buf, sz)) >= 16){
			if((status = GET4(buf+12)) != 0){
				werrstr("status 0x%02x", status);
				r = -1;
			}else if(GET4(buf) == 7) /* ignore status messages */
				continue;
		}else if(r > 0){
			werrstr("short recv: %d", r);
			r = -1;
		}
		break;
	}
	return r;
}

static int
rndisreceive(Dev *ep)
{
	Block *b;
	int n, len;
	int doff, dlen;

	b = allocb(Maxpkt);
	if((n = read(ep->dfd, b->rp, b->lim - b->base)) >= 0){
		if(n < 44)
			werrstr("short packet: %d bytes", n);
		else if(GET4(b->rp) != 1)
			werrstr("not a network packet: type 0x%08ux", GET4(b->wp));
		else{
			doff = GET4(b->rp+8);
			dlen = GET4(b->rp+12);
			if((len = GET4(b->rp+4)) != n || 8+doff+dlen > len || dlen < ETHERHDRSIZE)
				werrstr("bad packet: doff %d, dlen %d, len %d", doff, dlen, len);
			else{
				b->rp += 8 + doff;
				b->wp = b->rp + dlen;

				etheriq(b);
				return 0;
			}
		}
	}

	freeb(b);
	return -1;
}

static void
rndistransmit(Dev *ep, Block *b)
{
	int n;

	n = BLEN(b);
	b->rp -= 44;
	PUT4(b->rp, 1);      /* type = 1 (packet) */
	PUT4(b->rp+4, 44+n); /* len */
	PUT4(b->rp+8, 44-8); /* data offset */
	PUT4(b->rp+12, n);   /* data length */
	memset(b->rp+16, 0, 7*4);
	write(ep->dfd, b->rp, 44+n);
	freeb(b);
}

int
rndisinit(Dev *d)
{
	uchar res[128];
	int r, i, off, sz;
	ulong csp;
	Ep *ep;

	r = 0;
	for(i = 0; i < nelem(d->usb->ep); i++){
		if((ep = d->usb->ep[i]) == nil)
			continue;
		csp = ep->iface->csp;
		// ff0202 is canonical CSP per Linux kernel; 301e0 used by Nexus 5
		if(csp == 0xff0202 || csp == 0x0301e0 || csp == 0x0104ef)
			r = 1;
	}
	if(!r){
		werrstr("no rndis found");
		return -1;
	}

	/* initialize */
	PUT4(minit+20, 1580); /* max xfer = 1580 */
	if(rndisout(d, 0, minit, sizeof(minit)) < 0)
		werrstr("init: %r");
	else if((r = rndisin(d, 0, res, sizeof(res))) < 0)
		werrstr("init: %r");
	else if(GET4(res) != 0x80000002 || r < 52)
		werrstr("not an init response: type 0x%08ux, len %d", GET4(res), r);
	/* check the type */
	else if((r = GET4(res+24)) != 1)
		werrstr("not a connectionless device: %d", r);
	else if((r = GET4(res+28)) != 0)
		werrstr("not a 802.3 device: %d", r);
	else{
		/* get mac address */
		if(rndisout(d, 0, mgetmac, sizeof(mgetmac)) < 0)
			werrstr("send getmac: %r");
		else if((r = rndisin(d, 0, res, sizeof(res))) < 0)
			werrstr("recv getmac: %r");
		else if(GET4(res) != 0x80000004 || r < 24)
			werrstr("not a query response: type 0x%08ux, len %d", GET4(res), r);
		else {
			sz = GET4(res+16);
			off = GET4(res+20);
			if(8+off+sz > r || sz != Eaddrlen)
				werrstr("invalid mac: off %d, sz %d, len %d", off, sz, r);
			else{
				memcpy(macaddr, res+8+off, Eaddrlen);
				/* set the filter */
				if(rndisout(d, 0, mfilter, sizeof(mfilter)) < 0)
					werrstr("send filter: %r");
				else if(rndisin(d, 0, res, sizeof(res)) < 0)
					werrstr("recv filter: %r");
				else if(GET4(res) != 0x80000005)
					werrstr("not a filter response: type 0x%08ux", GET4(res));
				else{
					epreceive = rndisreceive;
					eptransmit = rndistransmit;
					return 0;
				}
			}
		}
	}

	return -1;
}
