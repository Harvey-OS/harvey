#include <u.h>
#include <libc.h>
#include <ip.h>

/*
 *  well known IP addresses
 */
uchar IPv4bcast[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff
};
uchar IPv4allsys[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
	0xe0, 0, 0, 0x01
};
uchar IPv4allrouter[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
	0xe0, 0, 0, 0x02
};
uchar IPallbits[IPaddrlen] = {
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff
};
uchar IPnoaddr[IPaddrlen];

/*
 *  prefix of all v4 addresses
 */
uchar v4prefix[IPaddrlen] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0xff, 0xff,
	0, 0, 0, 0
};

int
isv4(uchar *ip)
{
	return memcmp(ip, v4prefix, IPv4off) == 0;
}

void
v4tov6(uchar *v6, uchar *v4)
{
	if(memcmp(v4, IPnoaddr+IPv4off, IPv4addrlen) == 0){
		memmove(v6, IPnoaddr, IPaddrlen);
		return;
	}
	memmove(v6, v4prefix, IPv4off);
	memmove(v6 + IPv4off, v4, IPv4addrlen);
}

int
v6tov4(uchar *v4, uchar *v6)
{
	if(ipcmp(v6, IPnoaddr) == 0){
		memset(v4, 0, IPv4addrlen);
		return 0;
	}
	if(memcmp(v6, v4prefix, IPv4off) == 0){
		memmove(v4, v6 + IPv4off, IPv4addrlen);
		return 0;
	}
	return -1;
}
