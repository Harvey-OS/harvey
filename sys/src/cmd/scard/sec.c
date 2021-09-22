#include <u.h>
#include <libc.h>
#include <ip.h>
#include "icc.h"
#include "tlv.h"
#include "file.h"
#include "sec.h"

/* put data and others, similar */
CmdiccR *
getchall(int fd, ushort p1, uchar chan, int howmany)
{
	int rpcsz;
	Cmdicc c;
	CmdiccR *cr;

	rpcsz = 5;	/* cla ins p1 p2 ne */
	memset(&c, 0, sizeof(Cmdicc));

	c.cla = chan2cla(chan);
	c.ins = InsGetChall;
	c.p1 = p1;
	c.p2 = 0x00;
	c.ne = howmany;

	cr = iccrpc(fd, &c, rpcsz);	
	if(cr == nil){
		werrstr("getdata:  %r\n");
		return nil;
	}
	return cr;
}

/*	put data and others, similar
 *	for ACOS p1 and p2 are kt and kc indexes...
 */
CmdiccR *
extauth(int fd, ushort p1, ushort p2, uchar chan, uchar *data, uchar dlen)
{
	int rpcsz;
	Cmdicc c;
	CmdiccR *cr;
	int n;

	rpcsz = 5 + dlen;	/* cla ins p1 p2 ne + data*/
	memset(&c, 0, sizeof(Cmdicc));

	c.cla = chan2cla(chan);
	c.ins = InsExtAuth;
	c.p1 = p1;
	c.p2 = p2;
	c.nc = dlen;
	c.cdata = data;

	cr = iccrpc(fd, &c, rpcsz);	
	if(cr == nil){
		werrstr("extauth:  %r\n");
		return nil;
	}
	if(cr->sw1 != Sw1More){
		free(cr);
		werrstr("extauth:  unexpected response %#2.2ux\n", cr->sw1);
		return nil;
	}
	n = cr->sw2;
	free(cr);
	cr = getresponse(fd, n, 0);
	return cr;
}

CmdiccR *
crdenc(int fd, uchar p1, uchar p2, uchar *b, uchar blen, uchar chan)
{
	int rpcsz;
	Cmdicc c;
	CmdiccR *cr;

	rpcsz = 5+blen;	/* cla ins p1 p2 p3 blen nc ne */
	memset(&c, 0, sizeof(Cmdicc));
	/* TODO: for the moment, no chaining? */

	c.cla = 0x80 | chan2cla(chan);
//	c.ne = blen;
	c.ins = InsEncrypt;
	c.p1 = p1;
	c.p2 = p2;
	c.cdata = c.cdatapack;
	c.nc = blen;
	memmove(c.cdata, b, blen);
	cr = iccrpc(fd, &c, rpcsz);	/* TODO issue additional get response?? */
	if(cr == nil){
		werrstr("getdata:  %r\n");
		return nil;
	}
	return cr;
}

/* TODO: ACOS only for the moment. */
CmdiccR *
verify(int fd, ushort cla, uchar pid, uchar *pin, uchar plen)
{
	int rpcsz;
	Cmdicc c;
	CmdiccR *cr;

	rpcsz = 5+plen;	/* cla ins p1 p2 ne */
	memset(&c, 0, sizeof(Cmdicc));

	c.cla = cla;
	c.ins = InsVerify;
	c.p1 = 0;
	c.p2 = pid;
	c.nc = plen;
	c.cdata = c.cdatapack;
	memmove(c.cdata, pin, plen);

	cr = iccrpc(fd, &c, rpcsz);	
	if(cr == nil){
		werrstr("getdata:  %r\n");
		return nil;
	}
	return cr;
}
