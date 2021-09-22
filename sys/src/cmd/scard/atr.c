#include <u.h>
#include <libc.h>
#include <thread.h>
#include <bio.h>
#include "tlv.h"
#include "atr.h"
#include "icc.h"

/* many things in common with the driver one
 * I wan to keep the driver one simple, though...
 */

enum{
	TA = 1,
	TB = 2,
	TC = 4,
	TD = 8,
};

static int
nbits(uchar m)
{
	int i, nb;

	nb = 0;
	for(i = 0; i < sizeof m; i++){
		if(m&1)
			nb++;
		m >>= 1;
	}
	return nb;
}

void
fillparam(Param *p, ParsedAtr *pa)
{
	p->clkrate = pa->fi;
	p->bitrate = pa->di;
	p->tcck = 0;
	if(pa->direct){
		p->tcck |= 2;		/* not used in ccidonly a placeholder*/
	}
	p->guardtime = pa->n;
	p->waitingint = pa->wi;		/* if t=0 WWT */
	return;
}

static void
dumpatr(ParsedAtr *pa)
{
	diprint(2, "ccid: atr->direct = %2.2ux\n", pa->direct );
	diprint(2, "ccid: atr->fi = %2.2ux\n", pa->fi );
	diprint(2, "ccid: atr->di = %2.2ux\n", pa->di );
	diprint(2, "ccid: atr->ii = %2.2ux\n", pa->ii );
	diprint(2, "ccid: atr->pi = %2.2ux\n", pa->pi );
	diprint(2, "ccid: atr->n = %2.2ux\n", pa->n );
	diprint(2, "ccid: atr->t = %2.2ux\n", pa->t);
	diprint(2, "ccid: atr->wi = %2.2ux\n", pa->wi);
}

/* bsz corresponds here exactly with the size */
int
parsehist(ParsedHist *pa, uchar *b, int bsz)
{
	int res;
	uchar *s;
	Tlv t, tin, *pt;
	int l;

	s = b;
	memset(pa, 0, sizeof (ParsedAtr));

	if(bsz < 1){
		werrstr("bad size");
		return -1;
	}
	pa->cat = *b++;
	if(pa->cat == CatStat3Last){
		if(bsz < 3 + b - s){
			werrstr("bad size");
			return -1;
		}
		diprint(2, "catstat\n");	
		pa->statuslen = 3;
		memmove(pa->status, b + bsz - 3, 3);
		bsz -= 3;	/* not tlv, take them out of the equation */
	}
	t.len = bsz - ( b - s );
	t.data = b;
		
	for(l = 0; l< t.len; l += res){
		res = unpacktlv(&tin, t.data+l, t.len-l, TComp);
		if(res < 0)
			sysfatal("unpacking tag: %r");
		diprint(2, "tag: %lux, len: %lud\n", tin.tag, tin.len);	
		if((tin.tag & 0xf0) == CatStatComp){
			pa->statuslen = tin.len;
			memmove(pa->status, tin.data, tin.len);
			break;
		}
		if((tin.tag & 0xf) > Maxpars)
			sysfatal("unpacking tag: %r");
		if(tin.data != nil){
			pt = &pa->tlv[tin.tag & 0xf];
			pt->tag = tin.tag;
			pt->data = pt->datapack;
			memmove(pt->data, tin.data, tin.len);
		}
	}

	b += l;
	pt = &pa->tlv[TagCCsrv & 0xf];
	if(pt->data){
		pa->csrv = pt->data[0];
		pa->getdata = (pt->data[0] & CsrvEFPerm) >> 1;
	}

	pt = &pa->tlv[TagCapab & 0xf];
	if(pt->data && pt->len >= 2)
		pa->writefun = (pt->data[1] & CapabWrite) >> 5;
	if(pt->data && pt->len >= 3)
		pa->caplchanmax = pt->data[2] & CapabMaxLchan;
	diprint(2, "status len: %d\n", pa->statuslen);	
	return b - s;
}

int
parseatr(ParsedAtr *pa, uchar *atr, int size)
{
	int nhist;
	uchar msk;
	uchar *s;

	s = atr;
	memset(pa, 0, sizeof *pa);
	if(size < 1){
		fprint(2, "ccid: bad atr len hdr\n");
		return -1;
	}
	if(*atr == 0x3b)
		pa->direct = 1;
	else if(*atr == 0x3f)
		pa->direct = 0;
	else
		return -1;
	atr++;
	
	if(size < 2)
		return -1;
	nhist = *atr & 0x0f;
	msk = *atr >> 4;
	atr++;

	if(size < (atr - s) + nhist + nbits(msk)){
		fprint(2, "ccid: bad atr len (1)\n");
		return -1;
	}

	if(msk & TA){
		pa->fi = *atr  >> 4;
		pa->di = *atr & 0x0f;
		atr++;
	}
	else{
		pa->fi = 372;
		pa->di = 1;
	}
	
	if(msk & TB){
		pa->ii = *atr  >> 4;
		pa->pi= *atr & 0x0f;
		atr++;
	}
	else{
		pa->ii = 50;
		pa->pi = 5;
	}
	if(msk & TC){
		pa->n = *atr;
		atr++;
	}
	else
		pa->n = 0;
	
	if(msk & TD){
		pa->t = *atr & 0x0f;
		msk = *atr >> 4;
		atr++;
	}
	else{
		pa->t = 0;
		msk = 0;
	}

	if(size < (atr - s) + nhist + nbits(msk)){
		fprint(2, "ccid: bad atr len (2), %lud %d %d\n", 
				atr - s, nhist, nbits(msk));
		return -1;
	}

	if(msk & TA)
		atr++;
	
	
	if(msk & TB)
		atr++;
	
	if(msk & TC){
		pa->wi = *atr;
		atr++;
	}
	else
		pa->wi = 10;
	if(msk & TD){
		atr++;
	}
	memmove(pa->hist, atr, nhist);
	pa->nhist = nhist;
	atr += nhist;
	dumpatr(pa);
	return atr - s;
}


char *
lookupatr(uchar *data, int sz)
{
	Biobuf *sl;
	char *l, *atr, *s, *e;
	int i, n;

	atr = malloc(sz*3 + 1);
	if(!atr)
		return nil;
	s = atr;
	for(i = 0; i < sz; i++){
		s = seprint(s, s+sz, "%2.02ux ", data[i]);
	}
	s[-1] = 0;
	/* BUG: find a better place for this file... */
	sl = Bopen("/sys/src/cmd/usb/tagrd/scard.txt", OREAD);
	if(sl == nil){
		free(atr);
		return nil;
	}

	while(1){
		l = Brdline(sl, '\n');
		if(l == nil || (n = Blinelen(sl)) == 0)
			break;
		l[n - 1] = '\0';
		if(l[0] == '#' || l[0] == '\t')
			continue;
		if(cistrcmp(atr, l) == 0)
			break;
	}
	s = mallocz(80*4, 1);
	if(s == nil){
		free(atr);
		return nil;
	}
	e = s;
	while(1){
		l = Brdline(sl, '\n');
		if(l == nil || (n = Blinelen(sl)) == 0)
			break;
		if(l[0] == '#')
			continue;
		if(l[0] != '\t')
			break;
		l[n - 1] = '\0';
		e = seprint(e, s+80*4, "%s\n", l+1);
	}
	e[0] = '\0';
	Bterm(sl);
	
	diprint(2, "Atr: %s\n%s\n", atr, s);
	free(atr);
	return s;
	
}
