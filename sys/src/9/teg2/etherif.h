enum
{
	MaxEther	= 4,
	Ntypes		= 8,
};

typedef struct Ether Ether;
struct Ether {
	RWlock;
	ISAConf;			/* hardware info */

	int	ctlrno;
	ulong	tbdf;
	int	minmtu;
	int 	maxmtu;

	Netif;

	void	(*attach)(Ether*);	/* filled in by reset routine */
	void	(*detach)(Ether*);
	void	(*transmit)(Ether*);
	void	(*interrupt)(Ureg*, void*);
	long	(*ifstat)(Ether*, void*, long, ulong);
	long 	(*ctl)(Ether*, void*, long); /* custom ctl messages */
	void	(*power)(Ether*, int);	/* power on/off */
	void	(*shutdown)(Ether*);	/* shutdown hardware before reboot */

	void*	ctlr;
	uchar	ea[Eaddrlen];
	void*	address;
	int	irq;

	Queue*	oq;
};

extern Block* etheriq(Ether*, Block*, int);
extern void addethercard(char*, int(*)(Ether*));
extern ulong ethercrc(uchar*, int);
extern int parseether(uchar*, char*);

#define NEXT(x, l)	(((x)+1)%(l))
#define PREV(x, l)	(((x) == 0) ? (l)-1: (x)-1)
