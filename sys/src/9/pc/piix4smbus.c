#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

//
//	SMBus support for the PIIX4
//
enum
{
	IntelVendID=	0x8086,
	Piix4PMID=	0x7113,		/* PIIX4 power management function */

	// SMBus configuration registers (function 3)
	SMBbase=	0x90,		// 4 byte base address (bit 0 == 1, bit 3:1 == 0)
	SMBconfig=	0xd2,
	SMBintrselect=	(7<<1),
	SMIenable=	(0<<1),		//  interrupts sent to SMI#
	IRQ9enable=	(4<<1),		//  intettupts sent to IRQ9
	SMBenable=	(1<<0),		//  1 enables

	// SMBus IO space registers
	Hoststatus=	0x0,		//  (writing 1 bits reset the interrupt bits)
	Failed=		(1<<4),	 	//  transaction terminated by KILL
	Bus_error=	(1<<3),		//  transactio collision
	Dev_error=	(1<<2),		//  device error interrupt
	Host_complete=	(1<<1),		//  host command completion interrupt 
	Host_busy=	(1<<0),		//
	Slavestatus=	0x1,		//  (writing 1 bits reset)
	Alert_sts=	(1<<5),		//  someone asserted SMBALERT#
	Shdw2_sts=	(1<<4),		//  slave accessed shadow 2 port
	Shdw1_sts=	(1<<3),		//  slave accessed shadow 1 port
	Slv_sts=	(1<<2),		//  slave accessed shadow 1 port
	Slv_bsy=	(1<<0),
	Hostcontrol=	0x2,
	Start=		(1<<6),		//  start execution
	Cmd_prot=	(7<<2),		//  command protocol mask
	Quick=		(0<<2),		//   address only
	Byte=		(1<<2),		//   address + cmd
	ByteData=	(2<<2),		//   address + cmd + data
	WordData=	(3<<2),		//   address + cmd + data + data
	Kill=		(1<<1),		//  abort in progress command
	Ienable=	(1<<0),		//  enable completion interrupts
	Hostcommand=	0x3,
	Hostaddress=	0x4,
	AddressMask=	(0x7f<<1),	//  target address
	Read=		(1<<0),		//  1 == read, 0 == write
	Hostdata0=	0x5,
	Hostdata1=	0x6,
	Blockdata=	0x7,
	Slavecontrol=	0x8,
	Alert_en=	(1<<3),		//  enable inter on SMBALERT#
	Shdw2_en=	(1<<2),		//  enable inter on external shadow 2 access
	Shdw1_en=	(1<<1),		//  enable inter on external shadow 1 access
	Slv_en=		(1<<0),		//  enable inter on access of host ctlr slave port
	Shadowcommand=	0x9,
	Slaveevent=	0xa,
	Slavedata=	0xc,
};

static struct
{
	int	rw;
	int	cmd;
	int	len;
	int	proto;
} proto[] =
{
	[SMBquick]	{ 0,	0,	0,	Quick },
	[SMBsend]	{ 0,	1,	0,	Byte },
	[SMBbytewrite]	{ 0,	1,	1,	ByteData },
	[SMBwordwrite]	{ 0,	1,	2,	WordData },
	[SMBrecv]	{ Read,	0,	1, 	Byte },
	[SMBbyteread]	{ Read,	1,	1,	ByteData },
	[SMBwordread]	{ Read,	1,	2,	WordData },
};

static void
transact(SMBus *s, int type, int addr, int cmd, uchar *data)
{
	int tries, status;
	char err[256];

	if(type < 0 || type > nelem(proto))
		panic("piix4smbus: illegal transaction type %d", type);

	if(waserror()){
		qunlock(s);
		nexterror();
	}
	qlock(s);

	// wait a while for the host interface to be available
	for(tries = 0; tries < 1000000; tries++){
		if((inb(s->base+Hoststatus) & Host_busy) == 0)
			break;
		sched();
	}
	if(tries >= 1000000){
		// try aborting current transaction
		outb(s->base+Hostcontrol, Kill);
		for(tries = 0; tries < 1000000; tries++){
			if((inb(s->base+Hoststatus) & Host_busy) == 0)
				break;
			sched();
		}
		if(tries >= 1000000){
			snprint(err, sizeof(err), "SMBus jammed: %2.2ux", inb(s->base+Hoststatus));
			error(err);
		}
	}

	// set up for transaction
	outb(s->base+Hostaddress, (addr<<1)|proto[type].rw);
	if(proto[type].cmd)
		outb(s->base+Hostcommand, cmd);
	if(proto[type].rw != Read){
		switch(proto[type].len){
		case 2:
			outb(s->base+Hostdata1, data[1]);
			// fall through
		case 1:
			outb(s->base+Hostdata0, data[0]);
			break;
		}
	}
	 

	// reset the completion/error bits and start transaction
	outb(s->base+Hoststatus, Failed|Bus_error|Dev_error|Host_complete);
	outb(s->base+Hostcontrol, Start|proto[type].proto);

	// wait for completion
	status = 0;
	for(tries = 0; tries < 1000000; tries++){
		status = inb(s->base+Hoststatus);
		if(status & (Failed|Bus_error|Dev_error|Host_complete))
			break;
		sched();
	}
	if((status & Host_complete) == 0){
		snprint(err, sizeof(err), "SMBus request failed: %2.2ux", status);
		error(err);
	}

	// get results
	if(proto[type].rw == Read){
		switch(proto[type].len){
		case 2:
			data[1] = inb(s->base+Hostdata1);
			// fall through
		case 1:
			data[0] = inb(s->base+Hostdata0);
			break;
		}
	}
	qunlock(s);
	poperror();
}

static SMBus smbusproto =
{
	.transact = transact,
};

//
//  return 0 if this is a piix4 with an smbus interface
//
SMBus*
piix4smbus(void)
{
	Pcidev *p;
	static SMBus *s;

	if(s != nil)
		return s;

	p = pcimatch(nil, IntelVendID, Piix4PMID);
	if(p == nil)
		return nil;

	s = smalloc(sizeof(*s));	
	memmove(s, &smbusproto, sizeof(*s));
	s->arg = p;

	// disable the smbus
	pcicfgw8(p, SMBconfig, IRQ9enable|0);

	// see if bios gave us a viable port space
	s->base = pcicfgr32(p, SMBbase) & ~1;
print("SMB base from bios is 0x%lux\n", s->base);
	if(ioalloc(s->base, 0xd, 0, "piix4smbus") < 0){
		s->base = ioalloc(-1, 0xd, 2, "piix4smbus");
		if(s->base < 0){
			free(s);
			print("piix4smbus: can't allocate io port\n");
			return nil;
		}
print("SMB base ialloc is 0x%lux\n", s->base);
		pcicfgw32(p, SMBbase, s->base|1);
	}

	// disable SMBus interrupts, abort any transaction in progress
	outb(s->base+Hostcontrol, Kill);
	outb(s->base+Slavecontrol, 0);

	// enable the smbus
	pcicfgw8(p, SMBconfig, IRQ9enable|SMBenable);

	return s;
}
