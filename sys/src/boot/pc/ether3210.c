/*
 * EAGLE Technology Model NE3210
 * 32-Bit EISA BUS Ethernet LAN Adapter.
 * Programmer's Reference Guide kindly supplied
 * by Artisoft Inc/Eagle Technology.
 *
 * BUGS:
 *	no setting of values from config file;
 *	should we worry about doubleword memmove restrictions?
 *	no way to use mem addresses > 0xD8000 at present.
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "ether.h"

enum {					/* EISA slot space */
	NVLreset	= 0xC84,	/* 0 == reset, 1 == enable */
	NVLconfig	= 0xC90,

	DP83902off	= 0x000,	/* offset of DP83902 registers */
	Eaddroff	= 0x016,	/* offset of Ethernet address */
};

static struct {
	ulong	port;
	ulong	config;
} slotinfo[MaxEISA];

static ulong mem[8] = {
	0x00FF0000, 0x00FE0000, 0x000D8000, 0x0FFF0000,
	0x0FFE0000, 0x0FFC0000, 0x000D0000, 0x00000000,
};

static ulong irq[8] = {
	15, 12, 11, 10, 9, 7, 5, 3,
};

static struct {
	char	*type;
	uchar	val;
} media[] = {
	{ "10BaseT",	0x00, },
	{ "RJ-45",	0x00, },
	{ "10Base5",	0x80, },
	{ "AUI",	0x80, },
	{ "10Base2",	0xC0, },
	{ "BNC",	0xC0, },
	{ 0, },
};

static void*
read(Ctlr *ctlr, void *to, ulong from, ulong len)
{
	/*
	 * In this case, 'from' is an index into the shared memory.
	 */
	memmove(to, (void*)(ctlr->card.mem+from), len);
	return to;
}

static void*
write(Ctlr *ctlr, ulong to, void *from, ulong len)
{
	/*
	 * In this case, 'to' is an index into the shared memory.
	 */
	memmove((void*)(ctlr->card.mem+to), from, len);
	return (void*)to;
}

int
ne3210reset(Ctlr *ctlr)
{
	static int already;
	int i;
	ulong p;

	/*
	 * First time through, check if this is an EISA machine.
	 * If not, nothing to do. If it is, run through the slots
	 * looking for appropriate cards and saving the
	 * configuration info.
	 */
	if(already == 0){
		already = 1;
		if(strncmp((char*)(KZERO|0xFFFD9), "EISA", 4))
			return 0;

		for(i = 1; i < MaxEISA; i++){
			p = i*0x1000;
			if(inl(p+EISAconfig) != 0x0118CC3A)
				continue;

			slotinfo[i].port = p;
			slotinfo[i].config = inb(p+NVLconfig);
		}
	}

	/*
	 * Look through the found adapters for one that matches
	 * the given port address (if any). The possibilties are:
	 * 1) 0;
	 * 2) a slot address.
	 */
	i = 0;
	if(ctlr->card.port == 0){
		for(i = 1; i < MaxEISA; i++){
			if(slotinfo[i].port)
				break;
		}
	}
	else if(ctlr->card.port >= 0x1000){
		if((i = (ctlr->card.port>>16)) < MaxEISA){
			if((ctlr->card.port & 0xFFF) || slotinfo[i].port == 0)
				i = 0;
		}
	}
	if(i >= MaxEISA || slotinfo[i].port == 0)
		return 0;

	/*
	 * Set the software configuration using the values obtained.
	 * For now, ignore any values from the config file.
	 */
	ctlr->card.port = slotinfo[i].port;
	ctlr->card.mem = KZERO|mem[slotinfo[i].config & 0x07];
	ctlr->card.irq = irq[(slotinfo[i].config>>3) & 0x07];
	ctlr->card.size = 32*1024;

	/*
	 * Set up the stupid DP83902 configuration.
	 */
	ctlr->card.reset = ne3210reset;
	ctlr->card.attach = dp8390attach;
	ctlr->card.read = read;
	ctlr->card.write = write;
	ctlr->card.receive = dp8390receive;
	ctlr->card.transmit = dp8390transmit;
	ctlr->card.intr = dp8390intr;
	ctlr->card.bit16 = 1;
	ctlr->card.ram = 1;

	ctlr->card.dp8390 = ctlr->card.port+DP83902off;
	ctlr->card.tstart = 0;
	ctlr->card.pstart = HOWMANY(sizeof(Etherpkt), Dp8390BufSz);
	ctlr->card.pstop = HOWMANY(ctlr->card.size, Dp8390BufSz);

	/*
	 * Reset the board, then
	 * initialise the DP83902,
	 * set the ether address.
	 */
	outb(ctlr->card.port+NVLreset, 0x00);
	delay(2);
	outb(ctlr->card.port+NVLreset, 0x01);

	dp8390reset(ctlr);
	if((ctlr->card.ea[0]|ctlr->card.ea[1]|ctlr->card.ea[2]|ctlr->card.ea[3]|ctlr->card.ea[4]|ctlr->card.ea[5]) == 0){
		for(i = 0; i < sizeof(ctlr->card.ea); i++)
			ctlr->card.ea[i] = inb(ctlr->card.port+Eaddroff+i);
	}
	dp8390setea(ctlr);

	return 0;
}
