#include <stdlib.h>

long
labs(long a)
{
	if(a < 0)
		return -a;
	return a;
}
