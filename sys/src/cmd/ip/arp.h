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
typedef struct Arppkt Arppkt;
typedef struct Arpentry Arpentry;
typedef struct Arpstats Arpstats;

/* Format of ethernet arp request */
struct Arppkt {
	uint8_t d[6];
	uint8_t s[6];
	uint8_t type[2];
	uint8_t hrd[2];
	uint8_t pro[2];
	uint8_t hln;
	uint8_t pln;
	uint8_t op[2];
	uint8_t sha[6];
	uint8_t spa[4];
	uint8_t tha[6];
	uint8_t tpa[4];
};

#define ARPSIZE 42

/* Format of request from starp to user level arpd */
struct Arpentry {
	uint8_t etaddr[6];
	uint8_t ipaddr[4];
};

/* Arp cache statistics */
struct Arpstats {
	int hit;
	int miss;
	int failed;
};

#define ET_ARP 0x0806
#define ET_RARP 0x8035

#define ARP_REQUEST 1
#define ARP_REPLY 2
#define RARP_REQUEST 3
#define RARP_REPLY 4
