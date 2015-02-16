/* 
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

typedef struct Netaddr	Netaddr;
typedef struct Netfile	Netfile;
typedef struct Netif	Netif;

enum
{
	Nmaxaddr=	64,
	Nmhash=		31,

	Ncloneqid=	1,
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
#define NETTYPE(x)	(((uint32_t)x)&0x1f)
#define NETID(x)	((((uint32_t)x))>>5)
#define NETQID(i,t)	((((uint32_t)i)<<5)|(t))

/*
 *  one per multiplexed connection
 */
struct Netfile
{
	QLock;

	int	inuse;
	ulong	mode;
	char	owner[KNAMELEN];

	int	type;			/* multiplexor type */
	int	prom;			/* promiscuous mode */
	int	scan;			/* base station scanning interval */
	int	bridge;			/* bridge mode */
	int	headersonly;		/* headers only - no data */
	uchar	maddr[8];		/* bitmask of multicast addresses requested */
	int	nmaddr;			/* number of multicast addresses */

	Queue*	iq;			/* input */
};

/*
 *  a network address
 */
struct Netaddr
{
	Netaddr	*next;			/* allocation chain */
	Netaddr	*hnext;
	uchar	addr[Nmaxaddr];
	int	ref;
};

/*
 *  a network interface
 */
struct Netif
{
	QLock;

	/* multiplexing */
	char	name[KNAMELEN];		/* for top level directory */
	int	nfile;			/* max number of Netfiles */
	Netfile	**f;

	/* about net */
	int	limit;			/* flow control */
	int	alen;			/* address length */
	int	mbps;			/* megabits per sec */
	int	link;			/* link status */
	int	minmtu;
	int 	maxmtu;
	int	mtu;
	uchar	addr[Nmaxaddr];
	uchar	bcast[Nmaxaddr];
	Netaddr	*maddr;			/* known multicast addresses */
	int	nmaddr;			/* number of known multicast addresses */
	Netaddr *mhash[Nmhash];		/* hash table of multicast addresses */
	int	prom;			/* number of promiscuous opens */
	int	scan;			/* number of base station scanners */
	int	all;			/* number of -1 multiplexors */

	Queue*	oq;			/* output */

	/* statistics */
	uvlong	misses;
	uvlong	inpackets;
	uvlong	outpackets;
	uvlong	crcs;			/* input crc errors */
	uvlong	oerrs;			/* output errors */
	uvlong	frames;			/* framing errors */
	uvlong	overflows;		/* packet overflows */
	uvlong	buffs;			/* buffering errors */
	uvlong	soverflows;		/* software overflow */

	/* routines for touching the hardware */
	void	*arg;
	void	(*promiscuous)(void*, int);
	void	(*multicast)(void*, uchar*, int);
	int	(*hwmtu)(void*, int);	/* get/set mtu */
	void	(*scanbs)(void*, uint);	/* scan for base stations */
};

void	netifinit(Netif*, char*, int, uint32_t);
Walkqid*	netifwalk(Netif*, Chan*, Chan*, char **, int);
Chan*	netifopen(Netif*, Chan*, int);
void	netifclose(Netif*, Chan*);
long	netifread(Netif*, Chan*, void*, long, vlong);
Block*	netifbread(Netif*, Chan*, long, vlong);
long	netifwrite(Netif*, Chan*, void*, long);
long	netifwstat(Netif*, Chan*, uchar*, long);
long	netifstat(Netif*, Chan*, uchar*, long);
int	activemulti(Netif*, uchar*, int);
