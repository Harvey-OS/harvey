#include <u.h>
#include <libc.h>
#include "dat.h"
#include "protos.h"
Proto *protos[] =
{
	&ether,
	&ip,
	&ip6,
	&dump,
	&arp,
	&rarp,
	&udp,
	&bootp,
	&dhcp,
	&tcp,
	&il,
	&icmp,
	&icmp6,
	&ninep,
	&ospf,
	&ppp,
	&ppp_ccp,
	&ppp_lcp,
	&ppp_chap,
	&ppp_ipcp,
	0,
};
