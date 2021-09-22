#include <u.h>
#include <libc.h>
#include "icc.h"

enum{
	Maxpayload = 64,
};

typedef struct Errcode Errcode;
struct Errcode{
	ushort sw;
	char	errstr[128];
};

/* 	BUG this constants go with the constants in icc.h C<<8|K
 *	there are better ways to do this probably
 */
Errcode iccerrs[48] = {
	{0x6200, "no information given"},
	/* 0x6202, to 0x6280, triggering by the card */
	{0x6281, "part of returned data may be corrupted"},
	{0x6282, "end of file or record reached before reading ne bytes"},
	{0x6283, "selected file deactivated"},
	{0x6284, "file control information not formatted accordingly "},
	{0x6285, "selected file in termination state"},
	{0x6286, "no input data available from a sensor on the card"},
	{0x6300, "no information given"},
	{0x6381, "file filled up by the last write"},
	/* 0x63c0, counter from 0 to 15 encoded by last nibble */
	{0x6400, "execution error"},
	{0x6401, "immediate response required by the card"},
	/* 0x6402 to 0x6480, triggering by the card */
	{0x6500, "no information given"},
	{0x6581, "memory failure"},
	{0x6800, "no information given"},
	{0x6881, "logical channel not supported"},
	{0x6882, "secure messaging not supported"},
	{0x6883, "last command of the chain expected"},
	{0x6884, "command chaining not supported"},
	{0x6900, "no information given"},
	{0x6981, "command incompatible with file structure"},
	{0x6982, "security status not satisfied"},
	{0x6983, "authentication method blocked"},
	{0x6984, "reference data not usable"},
	{0x6985, "conditions of use not satisfied"},
	{0x6986, "command not allowed (no current ef)"},
	{0x6987, "expected secure messaging data objects missing"},
	{0x6988, "incorrect secure messaging data objects"},
	{0x6a00, "no information given"},
	{0x6a80, "incorrect parameters in the command data field"},
	{0x6a81, "function not supported"},
	{0x6a82, "file or application not found"},
	{0x6a83, "record not found"},
	{0x6a84, "not enough memory space in the file"},
	{0x6a85, "nc inconsistent with tlv structure"},
	{0x6a86, "incorrect parameters p1-p2"},
	{0x6a87, "nc inconsistent with parameters p1-p2"},
	{0x6a88, "referenced data or reference data not found (exact meaning depending on the command)"},
	{0x6a89, "file already exists"},
	{0x6a8a, "df name already exists"},
	{0x0, ""},

};

/* translation of all codes, errors or not errors different than ok */
Errcode codes[] = {
	{Sw1More,	"more bytes waiting"},
	{Sw1RomCh,	"state of non volatile memory changed"},
	{Sw1RomUch,	"state of non volatile memory unchanged"},
	{Sw1SecIssue,	"security related issues"},
	{Sw1ExRomUch,	"state of non volatile memory unchanged"},
	{Sw1ExRomCh,	"state of non volatile memory changed"},
	{Sw1ExSecIssue,	"security issues"},
	{Sw1Badlen,	"bad length"},
	{Sw1ClaFunUns,	"class function unsupported"},
	{Sw1PermDen,	"command not allowed"},
	{Sw1BadParam	,"wrong parameters p1-p2"},
	{Sw1BadParam1,	"bad parameters p1-p2"},
	{Sw1BadLe,	"wrong le, how many in sw2"},	
	{Sw1BadInstr,	"instruction code not supported or invalid"},
	{Sw1ClaUns,	"class not supported"},
	{Sw1UnknErr,	"unknown error"},
	{0x0, ""},
};

int
unpackcmdicc(uchar *b, int bsz, Cmdicc *c)
{
	uchar *s;

	s = b;
	if(bsz < 4){
		diprint(2, "icc: pack bsz: %d < 3\n", bsz);
		return -1;
	}
	c->cla = *b++;
	c->ins = *b++;
	c->p1 = *b++;
	c->p2 = *b++;
	c->nc = 0;
	c->ne = 0;
	if(bsz == 4){
		c->nc = 0;
		return bsz;
	}
	if(bsz > 5)	/* there is data */
		if(c->isext){
			if(*b++ != 0)
				diprint(2, "icc: warning, strange extendedlc[0]!= 0\n");
			c->nc = (b[0]<<8) + b[1];
			b += 2;
		}	
		else{
			c->nc = *b++;
			if(c->nc == 0)
				diprint(2, "icc: warning, strange short lc[0]== 0\n");
		}
	c->cdata = nil;
	if(c->nc != 0){
		if(c->nc > Maxcmd || b - s > bsz){
			diprint(2, "icc: bad size, nc: %lud, bsz: %d\n", c->nc, bsz); 
			return -1;
		}
		c->cdata = c->cdatapack;
		memmove(c->cdata, b, c->nc);
		b += c->nc; 
	}

	if(b - s == bsz){
		c->ne = 0;
		return b - s;
	}
	
	if(c->isext){
		if(*b++ != 0)
			diprint(2, "icc: warning, strange extended le[0]!= 0\n");
		c->ne = (b[0]<<8) + b[1];
		b += 2;
		if(c->ne == 0)
			c->ne = 0x10000;
	}
	else{
		c->ne = *b++;
		if(c->ne == 0)
			c->ne = 0x100;
	}
	
	return b - s;
	
}

int
packcmdicc(uchar *b, int bsz, Cmdicc *c)
{
	uchar *s;

	s = b;
	if(bsz < 4){
		diprint(2, "icc: unpack bsz: %d < 3\n", bsz);
		return -1;
	}
	*b++ = c->cla;
	*b++ = c->ins;
	*b++ = c->p1;
	*b++ = c->p2;
	if(bsz == 4)
		return b - s;

	if(bsz > 5)	/* there is data */
		if(c->isext){
			b[0] = c->nc >> 8;
			b[1] = c->nc & 0xff;
			b += 2;
		}
		else{
			*b++ = c->nc;
		}

		if(c->nc != 0 && c->cdata != nil){
			if(c->nc > Maxcmd || (b - s) > bsz){
				diprint(2, "icc: bad size, nc: %lud, bsz: %d\n", c->nc, bsz); 
				return -1;
			}
			memmove(b, c->cdata, c->nc);
			b += c->nc; 
		}

	if(b - s == bsz)
		return b - s;
	
	if(c->isext){
		b[0] = c->ne >> 8;
		b[1] = c->ne & 0xff;
		if(c->ne == 0x10000)
			*b++ = 0;
		else
			*b++ = c->ne;
	}
	else{
		if(c->ne == 0x100)
			*b++ = 0;
		else
			*b++ = c->ne;
	}
	
	return b - s;
	
}


int
unpackcmdiccR(CmdiccR *c, uchar *b, int bsz)
{
	uchar *s;

	s = b;
	if(bsz < 2){
		diprint(2, "icc: strangeresponse\n");
		return -1;
	}
	
	
	c->ndataf = bsz;
	if(!c->isspecial)
		c->ndataf -= 2;

	c->dataf = c->datafpack;
	memmove(c->dataf, b, c->ndataf);
	b += c->ndataf;

	if(!c->isspecial && (bsz - 2) >= 0){
		c->sw1 = *b++;
		c->sw2 = *b++;
	}
	return b - s;
}

static char *
sw2errstr(CmdiccR *c, Errcode *p)
{
	Errcode *e;
	ushort sw;

	if(p == iccerrs)
		sw = (c->sw1<<8) + c->sw2;
	else
		sw = c->sw1;

	for(e = p; e->sw != 0; e++){
		if(e->sw == sw)
			return e->errstr;
	}
	return nil;
}

void
dumpicc(uchar *b, int sz, int send)
{
	int i;
	if(send)
		diprint(2, "<- ");
	else
		diprint(2, "-> ");
		
	for(i = 0; i < sz; i++){
		diprint(2, "%2.2ux ", b[i]);
	}
	diprint(2, "\n");
}

CmdiccR *
iccrpc(int fd, Cmdicc *cm, int sz)
{
	uchar *b;
	CmdiccR *cmr;
	int n, psz;
	char *err;

	b = mallocz(sz, 1);
	if(b == nil)
		return nil;
	psz = packcmdicc(b, sz, cm);
	if(psz < 0){
		werrstr("icc: packing request\n");
		return nil;
	}

	dumpicc(b, psz, 1);
	n = write(fd, b, psz);
	free(b);
	if(n != psz){
		werrstr("icc: rpc error writing fd: %d %r\n", fd);
		return nil;
	}
	b = mallocz(Maxpayload, 1);	/* should be 2 + cm->ne but standards are non std */
	if(b == nil)
		return nil;

	n = read(fd, b, Maxpayload);
	if(n < 0){
		free(b);
		return nil;
	}
	dumpicc(b, n, 0);
	cmr = mallocz(sizeof(CmdiccR), 1);
	if(cmr == nil){
		free(b);
		return nil;
	}

	if(cm->isspecial)
		cmr->isspecial = 1;
	psz = unpackcmdiccR(cmr, b, n);
	free(b);
	if(psz < 0 ){
		werrstr("icc: unpacking response\n");
		free(cmr);
		return nil;
	}

	if((err = sw2errstr(cmr, iccerrs)) != nil){
		werrstr("icc: error, %s\n", err);
	}
	else if((err = sw2errstr(cmr, codes)) != nil){
		if(cmr->sw1 >= Sw1iserr)
			werrstr("icc: error, %s\n", err);
		else if(cmr->sw1 != Sw1Ok && cmr->sw1 != Sw1More)
			fprint(2, "icc:warning %s\n", err);
	}

	if(cmr->isspecial || cmr->sw1 == Sw1Ok || cmr->sw1 < Sw1iserr)
		return cmr;

	free(cmr);

	return nil;

}

int
procedraw(int fd, uchar *b, int sz)
{
	int res;
	Cmdicc c;
	CmdiccR *cr;

	memset(&c, 0, sizeof(Cmdicc));
	res = unpackcmdicc(b, sz, &c);	
	if(res < 0){
		werrstr("ttag:  raw unpack\n");
		return -1;
	}
	cr = iccrpc(fd, &c, res);
	if(cr == nil){
		werrstr("raw:  %r\n");
		return -1;
	}
	free(cr);
	return 0;
}

CmdiccR *
getresponse(int fd, int sz, uchar cla)
{
	Cmdicc c;
	CmdiccR *cr;

	memset(&c, 0, sizeof(Cmdicc));
	c.cla = cla;
	c.ins = 0xc0;
	c.p1 = 0x00;
	c.p2 = 0x00;
	c.ne = sz;
	cr = iccrpc(fd, &c, 5);
	if(cr == nil){
		werrstr("ttag: raw rpc error %r\n");
		return nil;
	}
	else
		return cr;
}

uchar
chan2cla(uchar chan)
{
	uchar cla;

	cla = 0;
	if(chan >= 3)
		cla |= Mask_B;
	cla |= chan;
	return cla;
}

