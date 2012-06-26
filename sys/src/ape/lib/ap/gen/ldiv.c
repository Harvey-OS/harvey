#include <stdlib.h>

ldiv_t ldiv(long int numer, long int denom)
{
	ldiv_t ans;
	ans.quot=numer/denom;
	ans.rem=numer-ans.quot*denom;
	return ans;
}
