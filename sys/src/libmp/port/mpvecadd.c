#include "os.h"
#include <mp.h>
#include "dat.h"

// prereq: alen >= blen, sum has at least blen+1 digits
void
mpvecadd(mpdigit *a, int alen, mpdigit *b, int blen, mpdigit *sum)
{
	int i, carry;
	mpdigit x, y;

	carry = 0;
	for(i = 0; i < blen; i++){
		x = *a++;
		y = *b++;
		x += carry;
		carry = x < carry;
		x += y;
		carry += x < y;
		*sum++ = x;
	}
	for(; i < alen; i++){
		x = *a++ + carry;
		carry = x < carry;
		*sum++ = x;
	}
	*sum = carry;
}
