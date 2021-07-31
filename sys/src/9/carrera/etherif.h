enum {
	MaxEther	= 1,
	Ntypes		= 8,
};

typedef struct Ether Ether;
typedef struct Endev Endev;

struct Ether {
	ISAConf;			/* hardware info */
	int	ctlrno;
	int	tbdf;			/* type+busno+devno+funcno */
	int	mbps;			/* Mbps */

	Netif;

	Endev*	dev;
	void	*ctlr;
	Queue*	oq;
};

struct Endev {
	char*	name;

	int	(*reset)(Ether*);
	void	(*attach)(Ether*);
	void	(*transmit)(Ether*);
	void	(*interrupt)(Ureg*, void*);
	long	(*ifstat)(Ether*, void*, long, ulong);
	void	(*promiscuous)(void*, int);
	void	(*multicast)(void*, uchar*, int);

	int	vid;
	int	did;
};

/*
 * devether.c
 */
extern Block* etheriq(Ether*, Block*, int);
extern ulong ethercrc(uchar*, int);

extern Endev* endev[];

#define NEXT(x, l)	(((x)+1)%(l))
#define PREV(x, l)	(((x) == 0) ? (l)-1: (x)-1)
#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))
