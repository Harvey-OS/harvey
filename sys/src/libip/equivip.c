#include <u.h>
#include <libc.h>
#include <ip.h>

int
equivip4(uchar *a, uchar *b)
{
	return memcmp(a, b, IPv4addrlen) == 0;
}

int
equivip6(uchar *a, uchar *b)
{
	return memcmp(a, b, IPaddrlen) == 0;
}
