enum {
	MaxEther= 48,	/* max. models of ether cards *and* interfaces */
	Ntypes	= 8,
};

typedef struct Ether Ether;
struct Ether {
	void	*ctlr;
	ISAConf;			/* hardware info */
	Pcidev	*pcidev;

	int	ctlrno;
	int	tbdf;			/* type+busno+devno+funcno */
	Queue*	oq;

	void	(*attach)(Ether*);	/* filled in by reset routine */
	void	(*detach)(Ether*);
	void	(*transmit)(Ether*);
	int	(*interrupt)(Ureg*, void*);
	long	(*ifstat)(Ether*, void*, long, ulong);
	long 	(*ctl)(Ether*, void*, long); /* custom ctl messages */
	void	(*power)(Ether*, int);	/* power on/off */
	void	(*shutdown)(Ether*);	/* shutdown hardware before reboot */

	int	attached;
	Netif;
	uchar	ea[Eaddrlen];
};

typedef struct Ethident Ethident;
struct Ethident {		/* must be first member of Ctlr, if present */
	uint	*regs;		/* memory-mapped device registers */
	Ether	*edev;
	Pcidev	*pcidev;
	int	type;
	char	*prtype;
	uint	*physreg;	/* regs's phys addr, for discovery & printing */
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
