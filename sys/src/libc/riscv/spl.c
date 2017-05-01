#include <u.h>
#include <libc.h>
#include <ureg.h>


/* these are declared in this file as we do not want them externally visible.
 * inline assembly is not allowed in harvey.
 */

int64_t _splhi(void);
int64_t _spllo(void);

int splhi(void)
{
	return _splhi();
}

int spllo(void)
{
	return _spllo();
}

void splx(int s)
{
	if (s)
		_splhi();
	else
		_spllo();
}

