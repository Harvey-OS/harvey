enum
{
	Eaddrlen	= 6,
	ETHERMINTU	= 60,		/* minimum transmit size */
	ETHERMAXTU	= 1514,		/* maximum transmit size */
	ETHERHDRSIZE	= 14,		/* size of an ethernet header */

	MaxEther= 48,	/* max. models of ether cards *and* interfaces */
	Ntypes	= 8,
};

typedef struct Ether Ether;
struct Ether {
	void	*ctlr;
	Intrcommon;
	ISAConf;			/* hardware info */

	int	ctlrno;
	int	tbdf;			/* type+busno+devno+funcno */
	int	minmtu;
	int 	maxmtu;
	int	attached;

	void	(*attach)(Ether*);	/* filled in by reset routine */
	void	(*detach)(Ether*);	/* nothing actually calls this */
	void	(*transmit)(Ether*);
	Intrsvcret (*interrupt)(Ureg*, void*);
	long	(*ifstat)(Ether*, void*, long, ulong);
	long 	(*ctl)(Ether*, void*, long); /* custom ctl messages */
	void	(*power)(Ether*, int);	/* power on/off */
	void	(*shutdown)(Ether*);	/* shutdown hardware before reboot */

	Netif;

	int	nscan;			/* number of base station scanners */
	int	scan[Ntypes];		/* base station scanning interval */

	uchar	ea[Eaddrlen];
};

typedef struct Ethident Ethident;
struct Ethident {		/* must be first member of Ctlr, if present */
	uint	*regs;		/* memory-mapped device registers */
	Ether	*edev;
	int	type;
	char	*prtype;
	uint	*physreg;	/* regs's phys addr, for discovery & printing */
};

/* this is a packet layout, so can't tolerate bogus padding */
typedef struct Etherpkt Etherpkt;
struct Etherpkt
{
	uchar	d[Eaddrlen];
	uchar	s[Eaddrlen];
	uchar	type[2];
	uchar	data[1500];
};

typedef struct Ctlr Ctlr;

/*
 * we steal %æ for ethernet Ctlrs with Ethident as their first members.
 */
#pragma varargck type "æ" Ctlr*

extern Block* etheriq(Ether*, Block*, int);
extern void addethercard(char*, int(*)(Ether*));
extern ulong ethercrc(uchar*, int);
extern int parseether(uchar*, char*);

#define NEXT(x, l)	(((x)+1)%(l))
#define PREV(x, l)	(((x) == 0) ? (l)-1: (x)-1)
