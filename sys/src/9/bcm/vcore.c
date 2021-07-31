#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

/*
 * Mailbox interface with videocore gpu
 */

#define	MAILBOX		(VIRTIO+0xB880)

typedef struct Prophdr Prophdr;
typedef struct Fbinfo Fbinfo;

enum {
	Read		= 0x00>>2,
	Write		= 0x00>>2,
	Peek		= 0x10>>2,
	Sender		= 0x14>>2,
	Status		= 0x18>>2,
		Full		= 1<<31,
		Empty		= 1<<30,
	Config		= 0x1C>>2,
	NRegs		= 0x20>>2,

	ChanMask	= 0xF,
	ChanProps	= 8,
	ChanFb		= 1,

	Req			= 0x0,
	RspOk		= 0x80000000,
	TagResp		= 1<<31,

	TagGetfwrev	= 0x00000001,
	TagGetmac	= 0x00010003,
	TagGetram	= 0x00010005,
	TagGetpower	= 0x00020001,
	TagSetpower	= 0x00028001,
		Powerwait	= 1<<1,
	TagGetclkspd= 0x00030002,
	TagFballoc	= 0x00040001,
	TagFbfree	= 0x00048001,
	TagFbblank	= 0x00040002,
	TagGetres	= 0x00040003,
	TagSetres	= 0x00048003,
	TagGetvres	= 0x00040004,
	TagSetvres	= 0x00048004,
	TagGetdepth	= 0x00040005,
	TagSetdepth	= 0x00048005,
	TagGetrgb	= 0x00044006,
	TagSetrgb	= 0x00048006,
};

struct Fbinfo {
	u32int	xres;
	u32int	yres;
	u32int	xresvirtual;
	u32int	yresvirtual;
	u32int	pitch;			/* returned by gpu */
	u32int	bpp;
	u32int	xoffset;
	u32int	yoffset;
	u32int	base;			/* returned by gpu */
	u32int	screensize;		/* returned by gpu */
};


struct Prophdr {
	u32int	len;
	u32int	req;
	u32int	tag;
	u32int	tagbuflen;
	u32int	taglen;
	u32int	data[1];
};

static void
vcwrite(uint chan, int val)
{
	u32int *r;

	r = (u32int*)MAILBOX + NRegs;
	val &= ~ChanMask;
	while(r[Status]&Full)
		;
	coherence();
	r[Write] = val | chan;
}

static int
vcread(uint chan)
{
	u32int *r;
	int x;

	r = (u32int*)MAILBOX;
	do{
		while(r[Status]&Empty)
			;
		coherence();
		x = r[Read];
	}while((x&ChanMask) != chan);
	return x & ~ChanMask;
}

/*
 * Property interface
 */

static int
vcreq(int tag, void *buf, int vallen, int rsplen)
{
	uintptr r;
	int n;
	Prophdr *prop;
	static uintptr base = BUSDRAM;

	if(rsplen < vallen)
		rsplen = vallen;
	rsplen = (rsplen+3) & ~3;
	prop = (Prophdr*)(VCBUFFER);
	n = sizeof(Prophdr) + rsplen + 8;
	memset(prop, 0, n);
	prop->len = n;
	prop->req = Req;
	prop->tag = tag;
	prop->tagbuflen = rsplen;
	prop->taglen = vallen;
	if(vallen > 0)
		memmove(prop->data, buf, vallen);
	cachedwbinvse(prop, prop->len);
	for(;;){
		vcwrite(ChanProps, PADDR(prop) + base);
		r = vcread(ChanProps);
		if(r == PADDR(prop) + base)
			break;
		if(base == 0)
			return -1;
		base = 0;
	}
	if(prop->req == RspOk && prop->tag == tag && prop->taglen & TagResp) {
		if((n = prop->taglen & ~TagResp) < rsplen)
			rsplen = n;
		memmove(buf, prop->data, rsplen);
	}else
		rsplen = -1;

	return rsplen;
}

/*
 * Framebuffer
 */

static int
fbdefault(int *width, int *height, int *depth)
{
	u32int buf[3];

	if(vcreq(TagGetres, &buf[0], 0, 2*4) != 2*4 ||
	   vcreq(TagGetdepth, &buf[2], 0, 4) != 4)
		return -1;
	*width = buf[0];
	*height = buf[1];
	*depth = buf[2];
	return 0;
}

void*
fbinit(int set, int *width, int *height, int *depth)
{
	Fbinfo *fi;
	uintptr va;

	if(!set)
		fbdefault(width, height, depth);
	/* Screen width must be a multiple of 16 */
	*width &= ~0xF;
	fi = (Fbinfo*)(VCBUFFER);
	memset(fi, 0, sizeof(*fi));
	fi->xres = fi->xresvirtual = *width;
	fi->yres = fi->yresvirtual = *height;
	fi->bpp = *depth;
	cachedwbinvse(fi, sizeof(*fi));
	vcwrite(ChanFb, DMAADDR(fi));
	if(vcread(ChanFb) != 0)
		return 0;
	va = mmukmap(FRAMEBUFFER, PADDR(fi->base), fi->screensize);
	if(va)
		memset((char*)va, 0x7F, fi->screensize);
	return (void*)va;
}

int
fbblank(int blank)
{
	u32int buf[1];

	buf[0] = blank;
	if(vcreq(TagFbblank, buf, sizeof buf, sizeof buf) != sizeof buf)
		return -1;
	return buf[0] & 1;
}

/*
 * Power management
 */
void
setpower(int dev, int on)
{
	u32int buf[2];

	buf[0] = dev;
	buf[1] = Powerwait | (on? 1: 0);
	vcreq(TagSetpower, buf, sizeof buf, sizeof buf);
}

int
getpower(int dev)
{
	u32int buf[2];

	buf[0] = dev;
	buf[1] = 0;
	if(vcreq(TagGetpower, buf, sizeof buf[0], sizeof buf) != sizeof buf)
		return -1;
	return buf[0] & 1;
}

/*
 * Get ethernet address (as hex string)
 *	 [not reentrant]
 */
char *
getethermac(void)
{
	uchar ea[8];
	char *p;
	int i;
	static char buf[16];

	memset(ea, 0, sizeof ea);
	vcreq(TagGetmac, ea, 0, sizeof ea);
	p = buf;
	for(i = 0; i < 6; i++)
		p += sprint(p, "%.2x", ea[i]);
	return buf;
}

/*
 * Get firmware revision
 */
uint
getfirmware(void)
{
	u32int buf[1];

	if(vcreq(TagGetfwrev, buf, 0, sizeof buf) != sizeof buf)
		return 0;
	return buf[0];
}

/*
 * Get ARM ram
 */
void
getramsize(Confmem *mem)
{
	u32int buf[2];

	if(vcreq(TagGetram, buf, 0, sizeof buf) != sizeof buf)
		return;
	mem->base = buf[0];
	mem->limit = buf[1];
}

/*
 * Get clock rate
 */
ulong
getclkrate(int clkid)
{
	u32int buf[2];

	buf[0] = clkid;
	if(vcreq(TagGetclkspd, buf, sizeof(buf[0]), sizeof(buf)) != sizeof buf)
		return 0;
	return buf[1];
}
