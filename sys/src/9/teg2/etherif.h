enum
{
	MaxEther= 4,	/* max. models of ether cards *and* interfaces */
	Ntypes	= 8,
};

typedef struct Ether Ether;
struct Ether {
	void*	ctlr;
	RWlock;
	ISAConf;			/* hardware info */

	int	ctlrno;
	int	tbdf;			/* type+busno+devno+funcno */
	Queue*	oq;

	void	(*attach)(Ether*);	/* filled in by reset routine */
	void	(*detach)(Ether*);
	void	(*transmit)(Ether*);
	void	(*interrupt)(Ureg*, void*);
	long	(*ifstat)(Ether*, void*, long, ulong);
	long 	(*ctl)(Ether*, void*, long); /* custom ctl messages */
	void	(*power)(Ether*, int);	/* power on/off */
	void	(*shutdown)(Ether*);	/* shutdown hardware before reboot */

	Netif;
	uchar	ea[Eaddrlen];
};

extern Block* etheriq(Ether*, Block*, int);
extern void addethercard(char*, int(*)(Ether*));
extern ulong ethercrc(uchar*, int);
extern int parseether(uchar*, char*);

#define NEXT(x, l)	(((x)+1)%(l))
#define PREV(x, l)	(((x) == 0) ? (l)-1: (x)-1)
