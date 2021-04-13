/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Etherpkt Etherpkt;
typedef struct Netaddr Netaddr;
typedef struct Netfile Netfile;
typedef struct Netif Netif;

enum {
	Nmaxaddr = 64,
	Nmhash = 31,

	Ncloneqid = 1,
	Naddrqid,
	N2ndqid,
	N3rdqid,
	Ndataqid,
	Nctlqid,
	Nstatqid,
	Ntypeqid,
	Nifstatqid,
	Nmtuqid,
};

/*
 *  Macros to manage Qid's used for multiplexed devices
 */
#define NETTYPE(x) (((u32)x) & 0x1f)
#define NETID(x) ((((u32)x)) >> 5)
#define NETQID(i, t) ((((u32)i) << 5) | (t))

/*
 *  one per multiplexed connection
 */
struct Netfile {
	QLock q;

	int inuse;
	u32 mode;
	char owner[KNAMELEN];

	int type;	  /* multiplexor type */
	int prom;	  /* promiscuous mode */
	int scan;	  /* base station scanning interval */
	int bridge;	  /* bridge mode */
	int headersonly;  /* headers only - no data */
	u8 maddr[8]; /* bitmask of multicast addresses requested */
	int nmaddr;	  /* number of multicast addresses */

	Queue *iq; /* input */
};

/*
 *  a network address
 */
struct Netaddr {
	Netaddr *next; /* allocation chain */
	Netaddr *hnext;
	u8 addr[Nmaxaddr];
	int ref;
};

/*
 *  a network interface
 */
struct Netif {
	QLock q;

	/* multiplexing */
	char name[KNAMELEN]; /* for top level directory */
	int nfile;	     /* max number of Netfiles */
	Netfile **f;

	/* about net */
	int limit; /* flow control */
	int alen;  /* address length */
	int mbps;  /* megabits per sec */
	int link;  /* link status */
	int minmtu;
	int maxmtu;
	int mtu;
	u8 addr[Nmaxaddr];
	u8 bcast[Nmaxaddr];
	Netaddr *maddr;		/* known multicast addresses */
	int nmaddr;		/* number of known multicast addresses */
	Netaddr *mhash[Nmhash]; /* hash table of multicast addresses */
	int prom;		/* number of promiscuous opens */
	int _scan;		/* number of base station scanners */
	int all;		/* number of -1 multiplexors */

	/* statistics */
	u64 misses;
	u64 inpackets;
	u64 outpackets;
	u64 crcs;	     /* input crc errors */
	u64 oerrs;	     /* output errors */
	u64 frames;     /* framing errors */
	u64 overflows;  /* packet overflows */
	u64 buffs;	     /* buffering errors */
	u64 soverflows; /* software overflow */

	/* routines for touching the hardware */
	void *arg;
	void (*promiscuous)(void *, int);
	void (*multicast)(void *, u8 *, int);
	int (*hwmtu)(void *, int);    /* get/set mtu */
	void (*scanbs)(void *, u32); /* scan for base stations */
};

void netifinit(Netif *, char *, int, u32);
Walkqid *netifwalk(Netif *, Chan *, Chan *, char **, int);
Chan *netifopen(Netif *, Chan *, int);
void netifclose(Netif *, Chan *);
i32 netifread(Netif *, Chan *, void *, i32, i64);
Block *netifbread(Netif *, Chan *, i32, i64);
i32 netifwrite(Netif *, Chan *, void *, i32);
i32 netifwstat(Netif *, Chan *, unsigned char *, i32);
i32 netifstat(Netif *, Chan *, unsigned char *, i32);
int activemulti(Netif *, unsigned char *, int);

/*
 *  Ethernet specific
 */
enum {
	Eaddrlen = 6,
	ETHERMINTU = 60,   /* minimum transmit size */
	ETHERMAXTU = 1514, /* maximum transmit size */
	ETHERHDRSIZE = 14, /* size of an ethernet header */

	/* ethernet packet types */
	ETARP = 0x0806,
	ETIP4 = 0x0800,
	ETIP6 = 0x86DD,
};

struct Etherpkt {
	u8 d[Eaddrlen];
	u8 s[Eaddrlen];
	u8 type[2];
	u8 data[1500];
};
