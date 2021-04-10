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
#include <fcall.h>
#include <thread.h>
#include <mp.h>
#include <libsec.h>
#include <9p.h>

extern char *Debug;

typedef struct Pingcache Pingcache;
struct Pingcache {
	Pingcache*next;
	i32	rtt;
	char	*host;
	i32	expire;
};

typedef struct {
	u8	vihl;		/* Version and header length */
	u8	tos;		/* Type of service */
	u8	length[2];	/* packet length */
	u8	id[2];		/* Identification */
	u8	frag[2];	/* Fragment information */
	u8	ttl;		/* Time to live */
	u8	proto;		/* Protocol */
	u8	ipcksum[2];	/* Header checksum */
	u8	src[4];		/* Ip source */
	u8	dst[4];		/* Ip destination */
	u8	type;
	u8	code;
	u8	cksum[2];
	u8	icmpid[2];
	u8	seq[2];
	u8	data[1];
} Icmp;

enum {			/* Packet Types */
	EchoReply	= 0,
	Unreachable	= 3,
	SrcQuench	= 4,
	EchoRequest	= 8,
	TimeExceed	= 11,
	Timestamp	= 13,
	TimestampReply	= 14,
	InfoRequest	= 15,
	InfoReply	= 16,

	ICMP_IPSIZE	= 20,
	ICMP_HDRSIZE	= 8,

	Npings		= 8,
	Payload		= 32,

	Cachetime	= 60,
};

static Pingcache *Cache;

/*
 * We ignore the first result as that is probably bigger
 * than expected due to IP sorting out the routing to the host
 */
int
ping(char *host, int timeout)
{
	int rtt, fd, i, seq;
	i32 now;
	i64 then;
	u8 buf[128];
	Icmp *ip;
	Pingcache *c;

	now = time(nil);
	for(c = Cache; c; c = c->next)
		if(strcmp(c->host, host) == 0 && now < c->expire){
			if(Debug && strstr(Debug, "dfs") != nil)
				print("\t\tping host=%s timeout=%d - cache hit\n",
					host, timeout);
			return c->rtt;
		}

	rtt = -1;
	ip = (Icmp*)buf;

	if((fd = dial(netmkaddr(host, "icmp", "1"), 0, 0, 0)) == -1)
		goto fail;

	for(seq = 0; seq < Npings; seq++){
		then = nsec();
		for(i = Payload; i < sizeof buf; i++)
			buf[i] = i + seq;
		ip->type = EchoRequest;
		ip->code = 0;
		ip->seq[0] = seq;
		ip->seq[1] = seq;
		alarm(timeout);
		if(write(fd, ip, sizeof buf) != sizeof buf ||
		    read(fd, ip, sizeof buf) != sizeof buf)
			goto fail;
		alarm(0);
		if(ip->type != EchoReply || ip->code != 0 ||
		    ip->seq[0] != seq || ip->seq[1] != seq)
			goto fail;
		for(i = Payload; i < sizeof buf; i++)
			if((u8)buf[i] != (u8)(i + seq))
				goto fail;
		rtt = (rtt + nsec() - then) / 2;
	}
fail:
	if(fd != -1)
		close(fd);

	if(Debug && strstr(Debug, "dfs") != nil)
		print("\t\tping host=%s timeout=%d rtt=%d - failed\n",
			host, timeout, rtt);

	/*
	 * failures get cached too
	 */
	for(c = Cache; c; c = c->next)
		if(strcmp(c->host, host) == 0)
			break;
	if(c == nil){
		c = emalloc9p(sizeof(Pingcache));
		c->host = estrdup9p(host);
		c->next = Cache;
		Cache = c;
	}
	c->rtt = rtt;
	c->expire = now+Cachetime;
	return rtt;
}
