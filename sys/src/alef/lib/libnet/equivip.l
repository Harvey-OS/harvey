#include <alef.h>
#include <bio.h>
#include <net.h>

int
equivip(byte *a, byte *b)
{
	int i;

	for(i = 0; i < 4; i++)
		if(a[i] != b[i])
			return 0;
	return 1;
}
