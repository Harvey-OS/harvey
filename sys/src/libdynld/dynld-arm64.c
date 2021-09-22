/* this is a copy of dynld-power64.c and may be wrong for arm64 */
#include <u.h>
#include <libc.h>
#include <a.out.h>
#include <dynld.h>

#define	CHK(i,ntab)	if((unsigned)(i)>=(ntab))return "bad relocation index"

long
dynmagic(void)
{
	return DYN_MAGIC | V_MAGIC;
}

char*
dynreloc(uchar *b, ulong p, int m, Dynsym **tab, int ntab)
{
	USED(b);
	USED(p);
	USED(m);
	USED(tab);
	USED(ntab);
	return "arm64 unimplemented";
}
