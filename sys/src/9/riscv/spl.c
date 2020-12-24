#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "encoding.h"

/* these are declared in this file as we do not want them externally visible.
 * inline assembly is not allowed in harvey.
 */

i64 _splhi(void);
i64 _spllo(void);

// splhi and spllo return 1 if we were at splhi. This is used in splx, below.
int
splhi(void)
{
	u64 cur;
	cur = read_csr(sstatus);
	_splhi();
	return !(cur & 2);
}

int
spllo(void)
{
	u64 cur;
	cur = read_csr(sstatus);
	_spllo();
	return !(cur & 2);
}

void
splx(int s)
{
	if(s)
		_splhi();
	else
		_spllo();
}
