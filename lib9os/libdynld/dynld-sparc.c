#include "lib9.h"
#include <a.out.h>
#include <dynld.h>

long
dynmagic(void)
{
	return DYN_MAGIC | K_MAGIC;
}

char*
dynreloc(uchar *b, ulong p, int m, Dynsym **tab, int ntab)
{
	USED(b);
	USED(p);
	USED(m);
	USED(tab);
	USED(ntab);
	return "sparc unimplemented";
}
