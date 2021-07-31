#include <u.h>
#include <libc.h>
#include <thread.h>
#include "usb.h"
#include "hid.h"

/*
 * Rough hid descriptor parsing and interpretation for mice
 *
 * Chain and its operations build the infrastructure needed
 * to manipulate non-aligned fields, which do appear (sigh!).
 */

/* Get, at most, 8 bits*/
static uchar
get8bits(Chain *ch, int nbits)
{
	int b, nbyb, nbib, nlb;
	uchar low, high;

	b = ch->b + nbits - 1;
	nbib = ch->b % 8;
	nbyb = ch->b / 8;
	nlb = 8 - nbib;
	if(nlb > nbits)
		nlb = nbits;

	low = MSK(nlb) & (ch->buf[nbyb] >> nbib);
	if(IsCut(ch->b, b))
		high = (ch->buf[nbyb + 1] & MSK(nbib)) << nlb;
	else
		high = 0;
	ch->b += nbits;
	return high | low;
}

static void
getbits(void *p, Chain *ch, int nbits)
{
	int nby, nbi, i;
	uchar *vp;

	assert(ch->e >= ch->b);
	nby = nbits / 8;
	nbi = nbits % 8;

	vp = p;
	for(i = 0; i < nby; i++)
		*vp++ = get8bits(ch, 8);

	if(nbi != 0)
		*vp = get8bits(ch, nbi);
}

/* TODO check report id, when it does appear (not all devices) */
int
parsereportdesc(HidRepTempl *temp, uchar *repdesc, int repsz)
{
	int i, j, l, n, isptr, hasxy, hasbut, nk;
	int ks[MaxVals];
	HidInterface *ifs;

	ifs = temp->ifcs;
	isptr = 0;
	hasxy = hasbut = 0;
	n = 0;
	nk = 0;
	memset(ifs, 0, sizeof *ifs * MaxIfc);
	for(i = 0; i < repsz / 2; i += 2){
		if(n == MaxIfc || repdesc[i] == HidEnd)
			break;

		switch(repdesc[i]){
		case HidTypeUsg:
			switch(repdesc[i+1]){
			case HidX:
				hasxy++;
				ks[nk++] = KindX;
				break;
			case HidY:
				hasxy++;
				ks[nk++] = KindY;
				break;
			case HidWheel:
				ks[nk++] = KindWheel;
				break;
			case HidPtr:
				isptr++;
				break;
			}
			break;
		case HidTypeUsgPg:
			switch(repdesc[i+1]){
			case HidPgButts:
				hasbut++;
				ks[nk++] = KindButtons;
				break;
			}
			break;
		case HidTypeRepSz:
			ifs[n].nbits = repdesc[i+1];
			break;
		case HidTypeCnt:
			ifs[n].count = repdesc[i+1];
			break;
		case HidInput:
			for(j = 0; j <nk; j++)
				ifs[n].kind[j] = ks[j];
			if(nk < ifs[n].count)
				for(l = j; l <ifs[n].count; l++)
					ifs[n].kind[l] = ks[j-1];
			n++;
			nk = 0;
			break;
		}
	}
	temp->nifcs = n;
	if(isptr && hasxy && hasbut)
		return 0;
	fprint(2, "bad report: isptr %d, hasxy %d, hasbut %d\n",
		isptr, hasxy, hasbut);
	return -1;
}

int
parsereport(HidRepTempl *templ, Chain *rep)
{
	int i, j, k, ifssz;
	ulong u;
	uchar *p;
	HidInterface *ifs;

	ifssz = templ->nifcs;
	ifs = templ->ifcs;
	for(i = 0; i < ifssz; i++)
		for(j = 0; j < ifs[i].count; j++){
			if(ifs[i].nbits > 8 * sizeof ifs[i].v[0]){
				fprint(2, "ptr: bad bits in parsereport");
				return -1;
			}
			u =0;
			getbits(&u, rep, ifs[i].nbits);
			p = (uchar *)&u;
			/* le to host */
			ifs[i].v[j] = p[3]<<24 | p[2]<<16 | p[1]<<8 | p[0]<<0;
			k = ifs[i].kind[j];
			if(k == KindX || k == KindY || k == KindWheel){
				/* propagate sign */
				if(ifs[i].v[j] & (1 << (ifs[i].nbits - 1)))
					ifs[i].v[j] |= ~MSK(ifs[i].nbits);
			}
		}
	return 0;
}

/* TODO: fmt representation */
void
dumpreport(HidRepTempl *templ)
{
	int i, j, ifssz;
	HidInterface *ifs;

	ifssz = templ->nifcs;
	ifs = templ->ifcs;
	for(i = 0; i < ifssz; i++){
		fprint(2, "\tcount %#ux", ifs[i].count);
		fprint(2, " nbits %d ", ifs[i].nbits);
		fprint(2, "\n");
		for(j = 0; j < ifs[i].count; j++){
			fprint(2, "\t\tkind %#ux ", ifs[i].kind[j]);
			fprint(2, "v %#lux\n", ifs[i].v[j]);
		}
		fprint(2, "\n");
	}
	fprint(2, "\n");
}

/* could cache indices after parsing the descriptor */
int
hidifcval(HidRepTempl *templ, int kind, int n)
{
	int i, j, ifssz;
	HidInterface *ifs;

	ifssz = templ->nifcs;
	ifs = templ->ifcs;
	assert(n <= nelem(ifs[i].v));
	for(i = 0; i < ifssz; i++)
		for(j = 0; j < ifs[i].count; j++)
			if(ifs[i].kind[j] == kind && n-- == 0)
				return (int)ifs[i].v[j];
	return 0;		/* least damage (no buttons, no movement) */
}
