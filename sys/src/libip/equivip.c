#include <u.h>
#include <libc.h>
#include <ip.h>

int
equivip4(uchar *a, uchar *b)
{
	int i;

	for(i = 0; i < 4; i++)
		if(a[i] != b[i])
			return 0;
	return 1;
}

int
equivip6(uchar *a, uchar *b)
{
	int i;

	for(i = 0; i < IPaddrlen; i++)
		if(a[i] != b[i])
			return 0;
	return 1;
}
