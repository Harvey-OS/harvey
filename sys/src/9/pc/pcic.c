#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "devtab.h"

/*
 *  Intel 82365SL PCIC controller for the PCMCIA
 */
enum
{
	/* ports */
	Pindex=		0x3E0,
	Pdata=		0x3E1,

	/*
	 *  registers indices
	 */
	Rid=		0x0,		/* identification and revision */
	Ris=		0x1,		/* interface status */
	Rpc=	 	0x2,		/* power control */
	 Fautopower=	 (1<<5),	/*  automatic power switching */
	Rigc= 		0x3,		/* interrupt and general control */
	 Fiocard=	 (1<<5),	/*  I/O card (vs memory) */
	 Fnotreset=	 (1<<6),	/*  reset if not set */	
	 FSMIena=	 (1<<4),	/*  enable change interrupt on SMI */ 
	Rcsc= 		0x4,		/* card status change */
	Rcscic= 	0x5,		/* card status change interrupt config */
	 Fchangeena=	 (1<<3),	/*  card changed */
	 Fbwarnena=	 (1<<1),	/*  card battery warning */
	 Fbdeadena=	 (1<<0),	/*  card battery dead */
	Rena= 		0x6,		/* address window enable */
	Rio= 		0x7,		/* I/O control */
	Riobtm0lo=	0x8,		/* I/O address 0 start low byte */
	Riobtm0hi,			/* I/O address 0 start high byte */
	Riotop0lo,			/* I/O address 0 stop low byte */
	Riotop0hi,			/* I/O address 0 stop high byte */
	Riobtm1lo,			/* I/O address 1 start low byte */
	Riobtm1hi,			/* I/O address 1 start high byte */
	Riotop1lo,			/* I/O address 1 stop low byte */
	Riotop1hi,			/* I/O address 1 stop high byte */
	Rmap=		0x10,		/* map 0 */

	/*
	 *  offsets into the system memory address maps
	 */
	Mbtmlo=		0x0,		/* System mem addr mapping start low byte */
	Mbtmhi,				/* System mem addr mapping start high byte */
	Mtoplo,				/* System mem addr mapping stop low byte */
	Mtophi,				/* System mem addr mapping stop high byte */
	Mofflo,				/* Card memory offset address low byte */
	Moffhi,				/* Card memory offset address high byte */
};

#define MAP(x,o)	(Rmap + (x)*0x8 + o)

typedef struct PCMCIA	PCMCIA;
struct PCMCIA
{
	uchar	occupied;
	uchar	battery;
	uchar	wrprot;
	uchar	powered;
};
PCMCIA	cia;

/*
 *  reading and writing card registers
 */
uchar
pcicrd(int index)
{
	outb(Pindex, index);
	return inb(Pdata);
}
void
pcicwr(int index, uchar val)
{
	outb(Pindex, index);
	outb(Pdata, val);
}

/*
 *  get info about card
 */
void
pcicinfo(void)
{
	uchar isr;

	isr = pcicrd(Ris);
	cia.occupied = (isr & (3<<2)) == (3<<2);
	cia.powered = isr & (1<<6);
	cia.battery = (isr & 3) == 3;
	cia.wrprot = isr & (1<<4);
}

/*
 *  card detect status change interrupt
 */
void
pcicintr(Ureg *ur)
{
	uchar csc;

	USED(ur);
	csc = pcicrd(Rcsc);
	pcicinfo();
	if(csc & 1)
		print("cia card battery dead\n");
	if(csc & (1<<1))
		print("cia card battery warning\n");
	if(csc & (1<<3)){
		if(cia.occupied)
			print("cia card inserted\n");
		else
			print("cia card removed\n");
	}
}

/*
 *  set up for pcmcia cards
 */
void
pcicreset(void)
{
	/* turn on 5V power if the card is there */
	pcicwr(Rpc, 5|Fautopower);

	/* display status */
	pcicinfo();
	if(cia.occupied){
		print("cia occupied");
		if(cia.powered)
			print(", powered");
		if(cia.battery)
			print(", battery good");
		if(cia.wrprot)
			print(", write protected");
		print("\n");
	} else
		print("cia unoccupied\n");

	/* interrupt on card status change */
	pcicwr(Rigc, (PCMCIAvec-Int0vec) | Fnotreset);
	pcicwr(Rcscic, ((PCMCIAvec-Int0vec)<<4) | Fchangeena | Fbwarnena | Fbdeadena);
	setvec(PCMCIAvec, pcicintr);
}

/*
 *  configure card as n bytes of memory, return address.
 */
void
pcicmem(int n)
{
}
