#define RBUFSIZE	(6*1024)	/* raw buffer size */

#include "../port/portdat.h"

struct	Vmedevice
{
	uchar	bus;
	uchar	ctlr;
	uchar	vector;
	uchar	irq;
	void*	address;
	char*	name;
	int	(*init)(Vmedevice*);
	void	(*intr)(Vmedevice*);
	void*	private;
};
