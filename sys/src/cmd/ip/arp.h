/*
 *  this file used by (at least) the kernel, arpd, snoopy, tboot
 */
typedef struct Arppkt	Arppkt;
typedef struct Arpentry	Arpentry;
typedef struct Arpstats	Arpstats;

/* Format of ethernet arp request */
struct Arppkt {
	uchar	d[6];
	uchar	s[6];
	uchar	type[2];
	uchar	hrd[2];
	uchar	pro[2];
	uchar	hln;
	uchar	pln;
	uchar	op[2];
	uchar	sha[6];
	uchar	spa[4];
	uchar	tha[6];
	uchar	tpa[4];
	};

#define ARPSIZE		42

/* Format of request from starp to user level arpd */
struct Arpentry {
	uchar	etaddr[6];
	uchar	ipaddr[4];
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
