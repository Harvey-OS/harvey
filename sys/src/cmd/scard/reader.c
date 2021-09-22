#include <u.h>
#include <libc.h>
#include "icc.h"
#include "nfc.h"
#include "reader.h"
#include "mifare.h"



/* funny unlock extra msg, dunno where this comes from */
uchar esc[5] = { 0xff, 0x00, 0x48, 0x00, 0x00}; /*get the fware version */
uchar esc2[5] = { 0x80, 0x14, 0x04, 0x00, 0x06}; /*get the serial 0 */
uchar esc3[5] = { 0x80, 0x14, 0x00, 0x00, 0x08}; /*get the serial 1 */

enum{
	Maxfwarelen = 32
};

char *
ttaggetfware(int fd)
{
	char *s;
	int res, n;
	Cmdicc c;
	CmdiccR *cr;

	n = 0;
	cr = nil;

	s = mallocz(Maxfwarelen+1, 1);
	if(s == nil)
		return nil;

	memset(&c, 0, sizeof(Cmdicc));
	res = unpackcmdicc(esc, 5, &c);	
	if(res < 0){
		diprint(2, "ttag: unpack error\n");
		goto Err;
	}
	c.isspecial = 1;
	cr = iccrpc(fd, &c,res);
	if(cr == nil){
		diprint(2, "ttag: rpc error %r\n");
		goto Err;
	}

	if(cr->ndataf > 0){
		memmove(s+n, cr->dataf, cr->ndataf);
		n += cr->ndataf;
	}
	free(cr);

	memset(&c, 0, sizeof(Cmdicc));
	res = unpackcmdicc(esc2, 5, &c);	
	if(res < 0){
		diprint(2, "ttag: unpack error\n");
		return s;
	}
	cr = iccrpc(fd, &c, res);
	if(cr == nil){
		diprint(2, "ttag: rpc error %r\n");
		goto Err;
	}

	if(cr->ndataf > 0){
		memmove(s+n, cr->dataf, cr->ndataf);
		n += cr->ndataf;
	}
	free(cr);

	memset(&c, 0, sizeof(Cmdicc));
	res = unpackcmdicc(esc3, 5, &c);	
	if(res < 0){
		diprint(2, "ttag: unpack error\n");
		return s;
	}
	cr = iccrpc(fd, &c, res);
	if(cr == nil){
		diprint(2, "ttag: rpc error %r\n");
		goto Err;
	}
	if(cr->ndataf > 0){
		memmove(s+n, cr->dataf, cr->ndataf);
	}
	free(cr);
	return s;
Err:
	free(s);
	free(cr);
	return nil;
}


uchar mleds[9] = { 0xff, 0x00, 0x40, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00};
int
setleds(int fd, uchar leds, uchar t1, uchar t2, uchar n, uchar buzz)
{
	uchar le[9];

	memmove(le, mleds, 9);
	le[3] = leds;
	le[4] = 4;
	le[5] = t1;
	le[6] = t2;
	le[7] = n;
	le[8] = buzz;

	return procedraw(fd, le, 9);
}

uchar manton[] = { 0xff, 0x00, 0x00, 0x00, 0x04, 0xd4,  0x32, 0x01, 0x01};
uchar mantoff[] = { 0xff, 0x00, 0x00, 0x00, 0x04, 0xd4,  0x32, 0x01, 0x00};
int
antenna(int fd, int ison)
{	
	return procedraw(fd, ison?manton:mantoff, 9);

}

uchar mautopoll[32] = { 0xff, 0x00, 0x00, 0x00, 0x05, 0xd4, 0x60, 0x01, 0x01, 0x10};
int
autopoll(int fd)
{	
	int res;
	Cmdicc c;
	CmdiccR *cr;

	memset(&c, 0, sizeof(Cmdicc));
	res = unpackcmdicc(mautopoll, 10, &c);	
	if(res < 0){
		werrstr("ttag:  raw unpack\n");
		return -1;
	}
	cr = iccrpc(fd, &c, res);
	if(cr == nil){
		werrstr("ttag: raw rpc error %r\n");
		return -1;
	}
	if(cr->sw2 <= 0x5) /* shortcut... here I get a short ATQ I don't bother getting it*/
		res = 0;
	else
		res = cr->sw2;
	free(cr);
	return res;
}

/* special exchange data is this only for mifare?? */
uchar mxchg[7] = { 0xff, 0x00, 0x00, 0x00, 0x05, 0xd4, 0x40};
CmdiccR *
xchangedata(int fd, uchar *b, int bsz)
{	
	int res;
	Cmdicc c;
	CmdiccR *cr;
	uchar *buf;

	buf = mallocz(bsz+7, 1);
	memmove(buf, mxchg, 7);
	memmove(buf+7, b, bsz);
	

	memset(&c, 0, sizeof(Cmdicc));
	res = unpackcmdicc(buf, 7+bsz, &c);	
	if(res < 0){
		free(buf);
		werrstr("ttag:  raw unpack\n");
		return nil;
	}
	cr = iccrpc(fd, &c, res);
	if(cr == nil){
		free(buf);
		werrstr("ttag: raw rpc error %r\n");
		return nil;
	}
	else{
		free(buf);
		return cr;
	}
}

/* 	
 *	BUG: ? cla = 0xff is invalid, the rest is get response
 *	special for this reader?
*/
CmdiccR *
readtag(int fd, int sz)
{
	return getresponse(fd, sz, 0xff);
}

int
tagrdread(int fd, uchar *buf, long nbytes, vlong offset, Atq *a)
{
	int istag;
	int i, nr, blnr, bloff, ntot, nblkbytes;
	CmdiccR *cr;
	int ntagatq;
	char *s;

	 /* BUG, nfc card dependant (whole function) */

	ntot = 0;
	if(offset >=  4*Multrablksz)
		return 0;
	else if(nbytes + offset > 4*Multrablksz)
		nbytes =  4*Multrablksz - offset;
	
	antenna(fd, AON);

	ntagatq = autopoll(fd);
	if(ntagatq < 0){
		werrstr("autopoll error, %d", ntagatq);
		goto Error;
	}

	/* BUG autentication goes here */
	if(ntagatq){
		istag = 1;
		diprint(2, "TAG detected\n");
	}
	else{
		werrstr("tagrd, mifare, no tag\n");
		goto Error;
	}

	if(istag){
		if(ntagatq)
			cr = readtag(fd, ntagatq); /* Read the ATQ */
		else
			goto Error;
		if( cr == nil )
			goto Error;
		
		if(a != nil){
			unpackatq(cr->dataf, cr->ndataf, a);
			s = atq2str(a);
			diprint(2, "tagrd: %s\n", s);
		}
		sleep(1000);
		free(cr);
		for(i = 0; i<4; i++){
			blnr = offset / Multrablksz;
			if(offset > Multrablksz * (blnr + 1))
				bloff = 0;
			else
				bloff = offset % Multrablksz;
			if(nbytes != 0){
				if(Multrablksz - bloff < nbytes)
					nblkbytes =  Multrablksz - bloff;
				else
					nblkbytes =  nbytes;
				

				nr = mifareblkread(fd, buf+ntot, nblkbytes, bloff, blnr);
				if(nr < 0){
					ntot = -1;
					break;
				}
				ntot += nr;
				nbytes -= nr;
				offset += nr;
				
			}
		}
	}
	antenna(fd, AOFF);
	return ntot;
	Error:
		diprint(2, "rpc error\n");
	return -1;
}
