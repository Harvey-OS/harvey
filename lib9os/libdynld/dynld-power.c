#include "lib9.h"
#include <a.out.h>
#include <dynld.h>

#define	CHK(i,ntab)	if((unsigned)(i)>=(ntab))return "bad relocation index"

long
dynmagic(void)
{
	return DYN_MAGIC | Q_MAGIC;
}

char*
dynreloc(uchar *b, ulong p, int m, Dynsym **tab, int ntab)
{
	int i;
	ulong v, *pp0, *pp1;

	p <<= 2;
	p += (ulong)b;
	pp0 = (ulong*)p;
	v = *pp0;
	switch(m){
	case 0:
		v += (ulong)b;
		break;
	case 1:
		i = v>>22;
		v &= 0x3fffff;
		CHK(i, ntab);
		v += tab[i]->addr;
		break;
	case 2:
		i = (v&0xffc)>>2;
		v &= ~0xffc;
		CHK(i, ntab);
		v |= (tab[i]->addr-p)&0x3fffffc;
		break;
	case 3:
	case 4:
	case 5:
	case 6:
		pp1 = (ulong*)(p+4);
		v = (v<<16)|(*pp1&0xffff);
		if(m&1)
			v += (ulong)b;
		else{
			i = v>>22;
			v &= 0x3fffff;
			CHK(i, ntab);
			v += tab[i]->addr;
		}
		if(m >= 5 && (v&0x8000))
			v += 0x10000;
		*pp0 &= ~0xffff;
		*pp0 |= v>>16;
		*pp1 &= ~0xffff;
		*pp1 |= v&0xffff;
		return nil;
	default:
		return "invalid relocation mode";
	}
	*pp0 = v;
	return nil;
}
