enum {
	MaxEther	= 4,
	Ntypes		= 8,
};

typedef struct Ether Ether;
struct Ether {
	ISAConf;			/* hardware info */
	int	ctlrno;
	int	tbdf;			/* type+busno+devno+funcno */
	int	mbps;			/* Mbps */
	uchar	ea[Eaddrlen];

	void	(*attach)(Ether*);	/* filled in by reset routine */
	void	(*closed)(Ether*);
	void	(*detach)(Ether*);
	void	(*transmit)(Ether*);
	void	(*interrupt)(Ureg*, void*);
	long	(*ifstat)(Ether*, void*, long, ulong);
	void	*ctlr;

	Queue*	oq;

	Netif;
};

extern Block* etheriq(Ether*, Block*, int);
extern void addethercard(char*, int(*)(Ether*));
