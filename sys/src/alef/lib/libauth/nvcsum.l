#include <alef.h>
#include <auth.h>

byte
nvcsum(void *vmem, int n)
{
	byte *mem, sum;
	int i;

	sum = 9;
	mem = vmem;
	for(i = 0; i < n; i++)
		sum += mem[i];
	return sum;
}
