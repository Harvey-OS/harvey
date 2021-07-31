typedef struct Ether Ether;
struct Ether {
	ISAConf;			/* hardware info */

	int	ctlrno;
	char	iname[NAMELEN];
	char	oname[NAMELEN];
	int	tbdf;			/* type+busno+devno+funcno */
	int	mbps;			/* Mbps */
	uchar	ea[Easize];

	void	(*attach)(Ether*);	/* filled in by reset routine */
	void	(*transmit)(Ether*);
	void	(*interrupt)(Ureg*, void*);
	void	*ctlr;

	Ifc	ifc;

	Lock	rqlock;
	Msgbuf*	rqhead;
	Msgbuf*	rqtail;
	Rendez	rqr;

	Lock	tqlock;
	Msgbuf*	tqhead;
	Msgbuf*	tqtail;
	Rendez	tqr;
};

#define NEXT(x, l)	(((x)+1)%(l))
#define PREV(x, l)	(((x) == 0) ? (l)-1: (x)-1)
#define	HOWMANY(x, y)	(((x)+((y)-1))/(y))
#define ROUNDUP(x, y)	(HOWMANY((x), (y))*(y))

extern void etheriq(Ether*, Msgbuf*);
extern Msgbuf* etheroq(Ether*);
