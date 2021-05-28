enum
{
	MaxVector=	8,

	/* some flags to change polarity and sensitivity */
	IRQmask=	0xFF,	/* actual vector address */
	IRQactivelow=	1<<8,
	IRQedge=	1<<9,
	IRQcritical=	1<<10,
};

#define BUSUNKNOWN	(-1)

#define NEXT(x, l)	(((x)+1)%(l))
#define PREV(x, l)	(((x) == 0) ? (l)-1: (x)-1)
