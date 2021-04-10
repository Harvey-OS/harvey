/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *  this file used by (at least) the kernel, arpd, snoopy, tboot
 */
typedef struct Arppkt	Arppkt;
typedef struct Arpentry	Arpentry;
typedef struct Arpstats	Arpstats;

/* Format of ethernet arp request */
struct Arppkt {
	u8	d[6];
	u8	s[6];
	u8	type[2];
	u8	hrd[2];
	u8	pro[2];
	u8	hln;
	u8	pln;
	u8	op[2];
	u8	sha[6];
	u8	spa[4];
	u8	tha[6];
	u8	tpa[4];
	};

#define ARPSIZE		42

/* Format of request from starp to user level arpd */
struct Arpentry {
	u8	etaddr[6];
	u8	ipaddr[4];
	};

/* Arp cache statistics */
struct Arpstats {
	int	hit;
	int	miss;
	int	failed;
	};

#define ET_ARP		0x0806
#define ET_RARP		0x8035

#define ARP_REQUEST	1
#define ARP_REPLY	2
#define RARP_REQUEST	3
#define RARP_REPLY	4
