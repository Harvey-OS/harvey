enum
{
	MaxEther	= 2,
	Ntypes		= 8,
};

typedef struct Ether Ether;
struct Ether {
	RWlock;				/* TO DO */
	ISAConf;			/* hardware info */
	int	ctlrno;
	int	minmtu;
	int	maxmtu;
	uchar	ea[Eaddrlen];
	void	*address;
	int	tbusy;

	void	(*attach)(Ether*);	/* filled in by reset routine */
	void	(*closed)(Ether*);
	void	(*detach)(Ether*);
	void	(*transmit)(Ether*);
	void	(*interrupt)(Ureg*, void*);
	long	(*ifstat)(Ether*, void*, long, ulong);
	long	(*ctl)(Ether*, void*, long); /* custom ctl messages */
	void	(*power)(Ether*, int);	/* power on/off */
	void	(*shutdown)(Ether*);	/* shutdown hardware before reboot */
	void	*ctlr;
	int	pcmslot;		/* PCMCIA */
	int	fullduplex;		/* non-zero if full duplex */
	int	linkchg;		/* link status changed? */
	uvlong	starttime;		/* last activity time */

	Queue*	oq;

	/* statistics */
	ulong	interrupts;
	ulong	dmarxintr;
	ulong	dmatxintr;
	ulong	promisc;
	ulong	pktsdropped;
	ulong	pktsmisaligned;
	ulong	resets;			/* after initialisation */
	ulong	bcasts;			/* broadcast pkts rcv'd */
	ulong	mcasts;			/* multicast pkts rcv'd */

	Netif;
};

extern Block* etheriq(Ether*, Block*, int);
extern void addethercard(char*, int(*)(Ether*));
extern ulong ethercrc(uchar*, int);
extern int parseether(uchar*, char*);

#define NEXT(x, l)	(((x)+1)%(l))
#define PREV(x, l)	(((x) == 0) ? (l)-1: (x)-1)
