/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <ip.h>
#include "../icmp.h"

static void
catch(void *a, char *msg)
{
	USED(a);
	if(strstr(msg, "alarm"))
		noted(NCONT);
	else
		noted(NDFLT);
}

#define MSG "dhcp probe"

/*
 *  make sure noone is using the address
 *  TODO: ipv6 ping
 */
int
icmpecho(uchar *a)
{
	int fd, i, n, len, rv;
	ushort sseq, x;
	char buf[512];
	Icmphdr *ip;

	rv = 0;
	if (!isv4(a))
		return 0;
	sprint(buf, "%I", a);
	fd = dial(netmkaddr(buf, "icmp", "1"), 0, 0, 0);
	if(fd < 0){
		return 0;
	}

	sseq = getpid()*time(0);

	ip = (Icmphdr *)(buf + IPV4HDR_LEN);
	notify(catch);
	for(i = 0; i < 3; i++){
		ip->type = EchoRequest;
		ip->code = 0;
		strcpy((char*)ip->data, MSG);
		ip->seq[0] = sseq;
		ip->seq[1] = sseq>>8;
		len = IPV4HDR_LEN + ICMP_HDRSIZE + sizeof(MSG);

		/* send a request */
		if(write(fd, buf, len) < len)
			break;

		/* wait 1/10th second for a reply and try again */
		alarm(100);
		n = read(fd, buf, sizeof(buf));
		alarm(0);
		if(n <= 0)
			continue;

		/* an answer to our echo request? */
		x = (ip->seq[1]<<8) | ip->seq[0];
		if(n >= len && ip->type == EchoReply && x == sseq &&
		    strcmp((char*)ip->data, MSG) == 0){
			rv = 1;
			break;
		}
	}
	close(fd);
	return rv;
}
