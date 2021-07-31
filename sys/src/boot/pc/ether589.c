/*
 * 3C589 and 3C562.
 * To do:
 *	check xcvr10Base2 still works (is GlobalReset necessary?).
 */
#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "error.h"

#include "etherif.h"

enum {						/* all windows */
	CommandR		= 0x000E,
	IntStatusR		= 0x000E,
};

enum {						/* Commands */
	GlobalReset		= 0x0000,
	SelectRegisterWindow	= 0x0001,
	RxReset			= 0x0005,
	TxReset			= 0x000B,
	AcknowledgeInterrupt	= 0x000D,
};

enum {						/* IntStatus bits */
	commandInProgress	= 0x1000,
};

#define COMMAND(port, cmd, a)	outs((port)+CommandR, ((cmd)<<11)|(a))
#define STATUS(port)		ins((port)+IntStatusR)

enum {						/* Window 0 - setup */
	Wsetup			= 0x0000,
						/* registers */
	ManufacturerID		= 0x0000,	/* 3C5[08]*, 3C59[27] */
	ProductID		= 0x0002,	/* 3C5[08]*, 3C59[27] */
	ConfigControl		= 0x0004,	/* 3C5[08]*, 3C59[27] */
	AddressConfig		= 0x0006,	/* 3C5[08]*, 3C59[27] */
	ResourceConfig		= 0x0008,	/* 3C5[08]*, 3C59[27] */
	EepromCommand		= 0x000A,
	EepromData		= 0x000C,
						/* AddressConfig Bits */
	autoSelect9		= 0x0080,
	xcvrMask9		= 0xC000,
						/* ConfigControl bits */
	Ena			= 0x0001,
	base10TAvailable9	= 0x0200,
	coaxAvailable9		= 0x1000,
	auiAvailable9		= 0x2000,
						/* EepromCommand bits */
	EepromReadRegister	= 0x0080,
	EepromBusy		= 0x8000,
};

enum {						/* Window 1 - operating set */
	Wop			= 0x0001,
};

enum {						/* Window 3 - FIFO management */
	Wfifo			= 0x0003,
						/* registers */
	InternalConfig		= 0x0000,	/* 3C509B, 3C589, 3C59[0257] */
						/* InternalConfig bits */
	xcvr10BaseT		= 0x00000000,
	xcvr10Base2		= 0x00300000,
};

enum {						/* Window 4 - diagnostic */
	Wdiagnostic		= 0x0004,
						/* registers */
	MediaStatus		= 0x000A,
						/* MediaStatus bits */
	linkBeatDetect		= 0x0800,
};

extern int elnk3reset(Ether*);

static char *tcmpcmcia[] = {
	"3C589",			/* 3COM 589[ABCD] */
	"3C562",			/* 3COM 562 */
	"589E",				/* 3COM Megahertz 589E */
	nil,
};

static int
configASIC(Ether* ether, int port, int xcvr)
{
	int x;

	/* set Window 0 configuration registers */
	COMMAND(port, SelectRegisterWindow, Wsetup);
	outs(port+ConfigControl, Ena);

	/* IRQ must be 3 on 3C589/3C562 */
	outs(port + ResourceConfig, 0x3F00);

	x = ins(port+AddressConfig) & ~xcvrMask9;
	x |= (xcvr>>20)<<14;
	outs(port+AddressConfig, x);

	COMMAND(port, TxReset, 0);
	while(STATUS(port) & commandInProgress)
		;
	COMMAND(port, RxReset, 0);
	while(STATUS(port) & commandInProgress)
		;

	return elnk3reset(ether);
}

int
ether589reset(Ether* ether)
{
	int i, t, slot;
	char *type;
	int port;
	enum { WantAny, Want10BT, Want10B2 };
	int want;
	uchar ea[6];
	char *p;

	if(ether->irq == 0)
		ether->irq = 10;
	if(ether->port == 0)
		ether->port = 0x240;
	port = ether->port;

//	if(ioalloc(port, 0x10, 0, "3C589") < 0)
//		return -1;

	type = nil;
	slot = -1;
	for(i = 0; tcmpcmcia[i] != nil; i++){
		type = tcmpcmcia[i];
if(debug) print("try %s...", type);
		if((slot = pcmspecial(type, ether)) >= 0)
			break;
	}
	if(slot < 0){
if(debug) print("none found\n");
//		iofree(port);
		return -1;
	}

	/*
	 * Read Ethernet address from card memory
	 * on 3C562, but only if the user has not 
	 * overridden it.
	 */
	memset(ea, 0, sizeof ea);
	if(memcmp(ea, ether->ea, 6) == 0 && strcmp(type, "3C562") == 0) {
		if(debug)
			print("read 562...");
		if(pcmcistuple(slot, 0x88, ea, 6) == 6) {
			for(i = 0; i < 6; i += 2){
				t = ea[i];
				ea[i] = ea[i+1];
				ea[i+1] = t;
			}
			memmove(ether->ea, ea, 6);
			if(debug)
				print("ea %E", ea);
		}
	}
	/*
	 * Allow user to specify desired media in plan9.ini
	 */
	want = WantAny;
	for(i = 0; i < ether->nopt; i++){
		if(cistrncmp(ether->opt[i], "media=", 6) != 0)
			continue;
		p = ether->opt[i]+6;
		if(cistrcmp(p, "10base2") == 0)
			want = Want10B2;
		else if(cistrcmp(p, "10baseT") == 0)
			want = Want10BT;
	}
	
	/* try configuring as a 10BaseT */
	if(want==WantAny || want==Want10BT){
		if(configASIC(ether, port, xcvr10BaseT) < 0){
			pcmspecialclose(slot);
//			iofree(port);
			return -1;
		}
		delay(100);
		COMMAND(port, SelectRegisterWindow, Wdiagnostic);
		if((ins(port+MediaStatus)&linkBeatDetect) || want==Want10BT){
			COMMAND(port, SelectRegisterWindow, Wop);
			print("#l%d: xcvr10BaseT %s\n", ether->ctlrno, type);
			return 0;
		}
	}

	/* try configuring as a 10base2 */
	if(want==WantAny || want==Want10B2){
		COMMAND(port, GlobalReset, 0);
		if(configASIC(ether, port, xcvr10Base2) < 0){
			pcmspecialclose(slot);
//			iofree(port);
			return -1;
		}
		print("#l%d: xcvr10Base2 %s\n", ether->ctlrno, type);
		return 0;
	}
	return -1;		/* not reached */
}
