typedef struct ArmCtxt ArmCtxt;
typedef struct EiceChain1 EiceChain1;
typedef struct EiceChain2 EiceChain2;
typedef struct EiceRegs EiceRegs;
typedef struct EiceWp EiceWp;
typedef struct MMURegs MMURegs;
typedef struct PackedSz PackedSz;

enum{
	/*
	  *	This is an arm 926, except no MOE, only 1 Wpoint
	  *	I think this means "mcr access"
	  * 	for the MMU. It would be good knowing for real
	  *	what the good doc for it is or if it is just
	  *	random mix and match!!.
	  */
	FeroceonId	= 0x20a023d3,
	
	/* Looks like a pxa/feroceon frankenstein */
	ArmadaId	= 0x304113d3,

	ArmInstSz = 4,
	ThumbInstSz = 2,
};


/* instructions for the IR, 4 bits */
enum {
	InExtest	= 0x0,
	InScanN	= 0x2,		/* shift the scan chain id, 5 bits */
	InSampPre	= 0x3,
	InRestart	= 0x4,
	InHighZ	= 0x7,
	InClampZ	= 0x9,
	InTest		= 0xc,
	InIdCode	= 0xe,
	InBypass	= 0xf,

	InGuruTapctl = 0x98,
	InGuruLen = 9,
	DrGuruTapctl = 0x00a,
	DrGuruLen = 16,

	InLen		= 4,		/* bits */
	EiceCh1Len	= 67,		/* bits */
	EiceCh2Len	= 38,		/* bits */
};

/* sizes are in bits */
#define WpN(nreg, field) ((nreg<<(nreg))|(field))

enum {

	/* chain 1 */
	InstrSz			= 32,
	SysSpeedSz		= 1,
	WpTanDBKptSz		= 1,
	Ch1ResvdSz,
	RWDataSz			= 32,
	

	/* chain 2, here registers are directed through addr */
	DataSz			= 32,
	AddrSz			= 5,
	RWSz			= 1,

	DebugCtlReg		= 0x00,
	DebStsReg		= 0x01,
	VecCatReg		= 0x02,
	DebComCtlReg		= 0x04,
	DebComDataReg	= 0x05,

	Wp0				= 1<<4,	/* Wp0|AddrValReg and so on */
	Wp1				= 1<<5,	/* Wp1|AddrValReg and so on */

	AddrValReg		= 0x00,
	AddrMskReg		= 0x01,
	DataValReg		= 0x02,
	DataMskReg		= 0x03,
	CtlValReg			= 0x04,
	CtlMskReg		= 0x05,
	
	/* sizes in bits */
	DebugCtlRegSz		= 6,
	DebStsRegSz		= 10,		/* in feroceon 5, no Moe */
	VecCatRegSz		= 8,
	DebComCtlRegSz	= 6,
	DebComDataRegSz	= 32,

	AddrValRegSz		= 32,
	AddrMskRegSz		= 32,
	DataValRegSz		= 32,
	DataMskRegSz		= 32,
	CtlValRegSz		= 9,
	CtlMskRegSz		= 8,
	
	/*	DebugCtlReg (write) */
	DBGACK			= 1,		/* I am dealing with debug mode */
	DBGRQ			= 1<<1,	/* Enter debug mode */
	INTDIS			= 1<<2,	/* Disable interrupts */
	SBZ0				= 1<<3,	/* Should be Zero */
	MONENAB		= 1<<4,	/* Monitor mode/halt mode */
	EICEDISAB		= 1<<5,	/* Power down */

	/*	DebugSTSReg (read)
	 * 	two first bits same as ctl
	 */
	IFEN			= 1<<2,		/* Are interrupts enabled */
	SYSCOMP		= 1<<3,		/* Memory access complete */
	ITBIT		= 1<<4,		/* Thumb/Arm mode */
	SBZ1			= 1<<5,
	MOEMSK		= 0xf<<6,		/* Method of entry */

	/*	VecCatReg (write), one bit per exception to catch */
	ResetCat		= 1,
	UndefCat		= 1<<1,
	SWICat		= 1<<2,
	PAbortCat		= 1<<3,
	DAbortCat	= 1<<4,
	RsvdCat		= 1<<5,
	IrqCat		= 1<<6,
	FiqCat		= 1<<7,

	/*
	 *	Watchpoint control registers:
	 *	CtlValReg CtlMskReg
	 *	Bit 1 in Msk makes condition ignored.
	 */
	
	/* Data version of CtlValReg bits 6, 7, 8 are shared */
	DnRWWPCtl		= 1,		/*  0 access is read 1 write */
	DmasWPCtlMsk	= 3<<1,	/*  this two bits represent size of access */
	DataWPCtl		= 1<<3,	/* compare against data/0 is inst */
	DnTransWPCtl		= 1<<4,	/* 0 user 1 privileged */
	DbgExtWPCtl		= 1<<5,	/* External condition for watchpoint */
	ChainWPCtl		= 1<<6,	/* Chain together watchpoints */
	RangeWPCtl		= 1<<7,	/* Range connecting watchpoints together */
	EnableWPCtl		= 1<<8,	/* Cannot be masked */
	/* Inst version of CtlValReg */
	IgnWPCtl			= 1,	/*  ignored */
	ITbitWPCtlMsk		= 3<<1,	/*  1 Thumb  */
	IJbitWPCtl			= 1<<2,	/*  1 Jazelle  */
	InTransWPCtl		= 1<<4,	/* 0 user 1 privileged */


	/*	Id code */
	ManIdSz		= 11,
	PartNoSz		= 16,
	VerSz		= 4,

};

enum{
	ChNoCommit	= 0,
	ChCommit	= 1,
};

struct PackedSz {
	uchar unpkSz;
	uchar pkSz;
};

/*
 * The bit ordering on this one is peculiar, see packech1()
 * in particular the instruction is upside down,
 */

struct EiceChain1 {
	u32int	instr;			/* 32 bits */
	uchar	sysspeed;		/* 1 bit */
	uchar	wptandbkpt;	/* 1 bit */
	/* 1 bit rsvd */
	u32int	rwdata;		/* 32 bits */
};

struct EiceChain2 {
	u32int data;		/* 32 bits */
	uchar addr;		/* 5 bits */
	uchar	rw;		/* 1 bit */
};

struct EiceWp {
	u32int	addrval;	/* 32 bits */
	u32int	addrmsk;	/* 32 bits */
	u32int	dataval;	/* 32 bits */
	u32int	datamsk;	/* 32 bits */
	u16int	ctlval;	/* 9 bits */
	uchar	ctlmsk;	/* 8 bits */
};

struct EiceRegs {
	uchar	debug;		/* 4 bits */
	uchar	debsts;		/* 5 bits */
	uchar	veccat;		/* 8 bits */
	uchar	debcomctl;	/* 6 bits */
	u32int	debcomdata;	/* 32 bits */
	EiceWp wp[2];
};

enum{
	ARMSTMIA = 0xe8800000,
	ARMLDMIA = 0xe8900000,
	BYTEWDTH = 0x00400000,
	ARMLDRindr1xr0 = 0xe5901000,	/* LDR r1, [r0]	|| [r0]->r1*/
	ARMSTRindxr0r1 = 0xe5801000,	/* STR [r0], r1 || r1->[r0] */
	ARMNOP = 0xe1a08008,
	ARMMRSr0CPSR = 0xE10F0000,
	ARMMSRr0CPSR = 0xE12FF000,
	SPSR = 1<<22,
	ARMMRSr0SPSR = ARMMRSr0CPSR|SPSR,
	ARMMSRr0SPSR = ARMMSRr0CPSR|SPSR,
	ARMBRIMM = 0xea000000,
};


#define MOE(dbgreg)	(((dbgreg)&MOEMSK)>>6);

#define MANID(id)	(((id)>>1)&MSK(ManIdSz))
#define PARTNO(id)	(((id)>>ManIdSz+1)&MSK(PartNoSz))
#define VERS(id)	(((id)>>ManIdSz+1+PartNoSz)&MSK(VerSz))

struct MMURegs {
	u32int	cpid;		/* main ID */
	u32int	control;	/* control */
	u32int	ttb;		/* translation table base */
	u32int	dac;		/* domain access control */
	u32int	fsr;		/* fault status */
	u32int	far;		/* fault address */
	u32int	ct;		/* cache type */
	u32int	pid;		/* address translation pid */
};

enum{
	NoReas,
	BreakReqReas,
	BreakReas,
	VeccatReqReas,
	VeccatReas,
	DebugReas,
};

struct ArmCtxt {
	int pcadjust;
	int debug;
	int debugreas;
	int exitreas;
	int cpuid;
	u32int r[16];
	u32int cpsr;
	u32int spsr;
	MMURegs;
};

extern int	armbpasstest(JMedium *jmed);
extern int	armfastexec(JMedium *jmed, u32int inst);
extern int	armgetexec(JMedium *jmed, int nregs, u32int *regs, u32int inst);
extern int	armgofetch(JMedium *jmed, u32int instr, int sysspeed);
extern u32int	armidentify(JMedium *jmed);
extern int	armrdmemwd(JMedium *jmed, u32int addr, u32int *data, int sz);
extern int	armsavectxt(JMedium *jmed, ArmCtxt *ctxt);
extern int	armsetexec(JMedium *jmed, int nregs, u32int *regs, u32int inst);
extern int	armsetregs(JMedium *jmed, int mask, u32int *regs);
extern int	armwrmemwd(JMedium *jmed, u32int addr, u32int data, int sz);
extern int	icedebugstate(JMedium *jmed);
extern int	iceenterdebug(JMedium *jmed, ArmCtxt *ctxt);
extern int	iceexitdebug(JMedium *jmed, ArmCtxt *ctxt);
extern u32int	icegetreg(JMedium *jmed, int regaddr);
extern int	icesetreg(JMedium *jmed, int regaddr, u32int val);
extern int	icewaitdebug(JMedium *jmed);
extern int	icewaitentry(JMedium *jmed, ArmCtxt *ctxt);
extern int	setchain(JMedium *jmed, int commit, uchar chain);
extern int	setinst(JMedium *jmed, uchar inst, int nocommit);
extern char *	armsprctxt(ArmCtxt *ctxt, char *s, int ssz);
