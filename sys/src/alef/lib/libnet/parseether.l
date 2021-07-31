#include <alef.h>
#include <bio.h>
#include <net.h>

int
parseether(byte *to, byte *from)
{
	int i;
	byte nip[4], *p;

	p = from;
	for(i = 0; i < 6; i++){
		if(*p == 0)
			return -1;
		nip[0] = *p++;
		if(*p == 0)
			return -1;
		nip[1] = *p++;
		nip[2] = 0;
		to[i] = strtoui(nip, nil, 16);
		if(*p == ':')
			p++;
	}
	return 0;
}
