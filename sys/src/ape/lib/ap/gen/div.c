#include <stdlib.h>

div_t div(int numer, int denom)
{
	div_t ans;
	ans.quot=numer/denom;  /* assumes division truncates */
	ans.rem=numer-ans.quot*denom;
	return ans;
}
