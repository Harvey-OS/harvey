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
	u8	ip[IPaddrlen];

	char	*boundto;	/* id last bound to */
	char	*offeredto;	/* id we've offered this to */

	i32	lease;		/* absolute time at which binding expires */
	i32	expoffer;	/* absolute time at which offer times out */
	i32	offer;		/* lease offered */
	i32	lasttouched;	/* time this entry last assigned/unassigned */
	i32	lastcomplained;	/* last time we complained about a used but not leased */
	i32	tried;		/* last time we tried this entry */

	Qid	q;		/* qid at the last syncbinding */
};

typedef struct Info	Info;
struct Info
{
	int	indb;			/* true if found in database */
	char	domain[Maxstr];	/* system domain name */
	char	bootf[Maxstr];		/* boot file */
	char	bootf2[Maxstr];	/* alternative boot file */
	u8	tftp[NDB_IPlen];	/* ip addr of tftp server */
	u8	tftp2[NDB_IPlen];	/* ip addr of alternate server */
	u8	ipaddr[NDB_IPlen];	/* ip address of system */
	u8	ipmask[NDB_IPlen];	/* ip network mask */
	u8	ipnet[NDB_IPlen];	/* ip network address (ipaddr & ipmask) */
	u8	etheraddr[6];		/* ethernet address */
	u8	gwip[NDB_IPlen];	/* gateway ip address */
	u8	fsip[NDB_IPlen];	/* file system ip address */
	u8	auip[NDB_IPlen];	/* authentication server ip address */
	char	rootpath[Maxstr];	/* rootfs for diskless nfs clients */
	char	dhcpgroup[Maxstr];
	char	vendor[Maxstr];	/* vendor info */
};


/* from dhcp.c */
extern int	validip(u8*);
extern void	warning(int, char*, ...);
extern int	minlease;

/* from db.c */
extern char*	tohex(char*, u8*, int);
extern char*	toid(u8*, int);
extern void	initbinding(u8*, int);
extern Binding*	iptobinding(u8*, int);
extern Binding*	idtobinding(char*, Info*, int);
extern Binding*	idtooffer(char*, Info*);
extern int	commitbinding(Binding*);
extern int	releasebinding(Binding*, char*);
extern int	samenet(u8 *ip, Info *iip);
extern void	mkoffer(Binding*, char*, i32);
extern int	syncbinding(Binding*, int);

/* from ndb.c */
extern int	lookup(Bootp*, Info*, Info*);
extern int	lookupip(u8*, Info*, int);
extern void	lookupname(char*, Ndbtuple*);
extern Iplifc*	findlifc(u8*);
extern int	forme(u8*);
extern int	lookupserver(char*, u8**, Ndbtuple *t);
extern Ndbtuple* lookupinfo(u8 *ipaddr, char **attr, int n);

/* from icmp.c */
extern int	icmpecho(u8*);

extern char	*binddir;
extern int	debug;
extern char	*blog;
extern Ipifc	*ipifcs;
extern i32	now;
extern char	*ndbfile;
