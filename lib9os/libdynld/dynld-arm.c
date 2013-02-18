#include "lib9.h"
#include <a.out.h>
#include <dynld.h>

#define	CHK(i,ntab)	if((unsigned)(i)>=(ntab))return "bad relocation index"

long
dynmagic(void)
{
	return DYN_MAGIC | E_MAGIC;
}

char*
dynreloc(uchar *b, ulong p, int m, Dynsym **tab, int ntab)
{
	int i;
	ulong v, *pp;

	p <<= 2;
	p += (ulong)b;
	pp = (ulong*)p;
	v = *pp;
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
		i = v&0x3ff;
		v &= ~0x3ff;
		CHK(i, ntab);
		v |= ((tab[i]->addr-p-8)>>2)&0xffffff;
		break;
	default:
		return "invalid relocation mode";
	}
	*pp = v;
	return nil;
}
