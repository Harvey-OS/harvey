#define RBUFSIZE	(6*1024)	/* raw buffer size */

#include "../port/portdat.h"


struct Vmedevice
{
	uchar	bus;
	uchar	ctlr;
	uchar	vector;
	uchar	irq;
	void*	address;
	ulong	address1;
	char*	name;
	int	(*init)(Vmedevice*);
	void	(*intr)(Vmedevice*);
	void*	private;
};

extern	Vmedevice vmedevtab[];
extern	int	ioid;
extern	int	probeflag;
