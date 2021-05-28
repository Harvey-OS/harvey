#include <u.h>
#include <libc.h>
#include <a.out.h>
#include <dynld.h>

#define	CHK(i,ntab)	if((unsigned)(i)>=(ntab))return "bad relocation index"

long
dynmagic(void)
{
	return DYN_MAGIC | T_MAGIC;
}

char*
dynreloc(uchar *b, ulong p, int m, Dynsym **tab, int ntab)
{
	USED(b);
	USED(p);
	USED(m);
	USED(tab);
	USED(ntab);
	return "power64 unimplemented";
}
