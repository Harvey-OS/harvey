/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "../dhcp.h"

enum
{
	Maxstr=	256,
};

typedef struct Binding Binding;
struct Binding
{
	Binding *next;
	uint8_t	ip[IPaddrlen];

	char	*boundto;	/* id last bound to */
	char	*offeredto;	/* id we've offered this to */

	int32_t	lease;		/* absolute time at which binding expires */
	int32_t	expoffer;	/* absolute time at which offer times out */
	int32_t	offer;		/* lease offered */
	int32_t	lasttouched;	/* time this entry last assigned/unassigned */
	int32_t	lastcomplained;	/* last time we complained about a used but not leased */
	int32_t	tried;		/* last time we tried this entry */

	Qid	q;		/* qid at the last syncbinding */
};

typedef struct Info	Info;
struct Info
{
	int	indb;			/* true if found in database */
	char	domain[Maxstr];	/* system domain name */
	char	bootf[Maxstr];		/* boot file */
	char	bootf2[Maxstr];	/* alternative boot file */
	uint8_t	tftp[NDB_IPlen];	/* ip addr of tftp server */
	uint8_t	tftp2[NDB_IPlen];	/* ip addr of alternate server */
	uint8_t	ipaddr[NDB_IPlen];	/* ip address of system */
	uint8_t	ipmask[NDB_IPlen];	/* ip network mask */
	uint8_t	ipnet[NDB_IPlen];	/* ip network address (ipaddr & ipmask) */
	uint8_t	etheraddr[6];		/* ethernet address */
	uint8_t	gwip[NDB_IPlen];	/* gateway ip address */
	uint8_t	fsip[NDB_IPlen];	/* file system ip address */
	uint8_t	auip[NDB_IPlen];	/* authentication server ip address */
	char	rootpath[Maxstr];	/* rootfs for diskless nfs clients */
	char	dhcpgroup[Maxstr];
	char	vendor[Maxstr];	/* vendor info */
};


/* from dhcp.c */
extern int	validip(uint8_t*);
extern void	warning(int, char*, ...);
extern int	minlease;

/* from db.c */
extern char*	tohex(char*, uint8_t*, int);
extern char*	toid(uint8_t*, int);
extern void	initbinding(uint8_t*, int);
extern Binding*	iptobinding(uint8_t*, int);
extern Binding*	idtobinding(char*, Info*, int);
extern Binding*	idtooffer(char*, Info*);
extern int	commitbinding(Binding*);
extern int	releasebinding(Binding*, char*);
extern int	samenet(uint8_t *ip, Info *iip);
extern void	mkoffer(Binding*, char*, int32_t);
extern int	syncbinding(Binding*, int);

/* from ndb.c */
extern int	lookup(Bootp*, Info*, Info*);
extern int	lookupip(uint8_t*, Info*, int);
extern void	lookupname(char*, Ndbtuple*);
extern Iplifc*	findlifc(uint8_t*);
extern int	forme(uint8_t*);
extern int	lookupserver(char*, uint8_t**, Ndbtuple *t);
extern Ndbtuple* lookupinfo(uint8_t *ipaddr, char **attr, int n);

/* from icmp.c */
extern int	icmpecho(uint8_t*);

extern char	*binddir;
extern int	debug;
extern char	*blog;
extern Ipifc	*ipifcs;
extern int32_t	now;
extern char	*ndbfile;

