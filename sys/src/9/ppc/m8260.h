
typedef struct BD BD;
struct BD {
	ushort	status;
	ushort	length;
	ulong	addr;
};

enum{
	BDEmpty=	SBIT(0),
	BDReady=	SBIT(0),
	BDWrap=		SBIT(2),
	BDInt=		SBIT(3),
	BDLast=		SBIT(4),
	BDFirst=	SBIT(5),
};

typedef struct Ring Ring;

struct Ring {
	BD*	rdr;		/* receive descriptor ring */
	void*	rrb;		/* receive ring buffers */
	int	rdrx;		/* index into rdr */
	int	nrdre;		/* length of rdr */

	BD*	tdr;		/* transmit descriptor ring */
	void**	txb;		/* corresponding transmit ring buffers */
	int	tdrh;		/* host index into tdr */
	int	tdri;		/* interface index into tdr */
	int	ntdre;		/* length of tdr */
	int	ntq;		/* pending transmit requests */
};

int	ioringinit(Ring*, int, int, int);

/*
 * MCC parameters
 */
typedef struct MCCparam MCCparam;
struct MCCparam {
/*0x00*/	ulong	mccbase;
/*0x04*/	ushort	mccstate;
/*0x06*/	ushort	mrblr;	
/*0x08*/	ushort	grfthr;	
/*0x0a*/	ushort	grfcnt;	
/*0x0c*/	ulong	rinttmp;
/*0x10*/	ulong	data0;
/*0x14*/	ulong	data1;
/*0x18*/	ulong	tintbase;
/*0x1c*/	ulong	tintptr;
/*0x20*/	ulong	tinttmp;
/*0x24*/	ushort	sctpbase;
/*0x26*/	ushort	Rsvd26;
/*0x28*/	ulong	cmask32;
/*0x2c*/	ushort	xtrabase;
/*0x2e*/	ushort	cmask16;
/*0x30*/	ulong	rinttmp[4];
/*0x40*/	struct {
			ulong	base;
			ulong	ptr;
		}		rint[4];
/*0x60*/	ulong	tstmp;
/*0x64*/
};
/*
 * IO controller parameters
 */
typedef struct IOCparam IOCparam;
struct IOCparam {
/*0x00*/	ushort	rbase;
/*0x02*/	ushort	tbase;
/*0x04*/	uchar	rfcr;
/*0x05*/	uchar	tfcr;
/*0x06*/	ushort	mrblr;
/*0x08*/	ulong	rstate;
/*0x0c*/	ulong	rxidp;
/*0x10*/	ushort	rbptr;
/*0x12*/	ushort	rxibc;
/*0x14*/	ulong	rxtemp;
/*0x18*/	ulong	tstate;
/*0x1c*/	ulong	txidp;
/*0x20*/	ushort	tbptr;
/*0x22*/	ushort	txibc;
/*0x24*/	ulong	txtemp;
/*0x28*/
};

typedef struct SCCparam SCCparam;
struct SCCparam {
	IOCparam;
	ulong	rcrc;
	ulong	tcrc;
};

typedef struct FCCparam FCCparam;
struct FCCparam {
/*0x00*/	ushort	riptr;
/*0x02*/	ushort	tiptr;
/*0x04*/	ushort	Rsvd04;
/*0x06*/	ushort	mrblr;
/*0x08*/	ulong	rstate;
/*0x0c*/	ulong	rbase;
/*0x10*/	ushort	rbdstat;
/*0x12*/	ushort	rbdlen;
/*0x14*/	char*	rdptr;
/*0x18*/	ulong	tstate;
/*0x1c*/	ulong	tbase;
/*0x20*/	ushort	tbdstat;
/*0x22*/	ushort	tbdlen;
/*0x24*/	ulong	tdptr;
/*0x28*/	ulong	rbptr;
/*0x2c*/	ulong	tbptr;
/*0x30*/	ulong	rcrc;
/*0x34*/	ulong	Rsvd34;
/*0x38*/	ulong	tcrc;
/*0x3c*/
};

typedef struct SCC SCC;
struct SCC {
	ulong	gsmrl;
	ulong	gsmrh;
	ushort	psmr;
	uchar	rsvscc0[2];
	ushort	todr;
	ushort	dsr;
	ushort	scce;
	uchar	rsvscc1[2];
	ushort	sccm;
	uchar	rsvscc2;
	uchar	sccs;
	ushort	irmode;
	ushort	irsip;
	uchar	rsvscc3[4];	/* BUG */
};

typedef struct FCC FCC;
struct FCC {
/*0x00*/	ulong	gfmr;		/*  general mode register 28.2/28-3 */
/*0x04*/	ulong	fpsmr;		/*  protocol-specific mode reg. 29.13.2(ATM) 30.18.1(Ether) */
/*0x08*/	ushort	ftodr;		/*  transmit on demand register 28.5/28-7 */
/*0x0A*/	ushort	Rsvd0A;
/*0x0C*/	ushort	fdsr;		/*  data synchronization register 28.4/28-7 */
/*0x0E*/	ushort	Rsvd0E;
/*0x10*/	ushort	fcce;		/* event register 29.13.3 (ATM), 30.18.2 (Ethernet) */
/*0x12*/	ushort	Rsvd12;
/*0x14*/	ushort	fccm;		/* mask register */
/*0x16*/	ushort	Rsvd16;
/*0x18*/	uchar	fccs;		/* status register 8 bits 31.10 (HDLC) */
/*0x19*/	uchar	Rsvd19[3];
/*0x1C*/	uchar	ftirrphy[4];	/* transmit internal rate registers for PHY0DH3 29.13.4/29-88 (ATM) */
/*0x20*/
};

typedef struct SMC SMC;
struct SMC {
/*0x0*/	ushort	pad1;
/*0x2*/	ushort	smcmr;
/*0x4*/	ushort	pad2;
/*0x6*/	uchar	smce;
/*0x7*/	uchar	pad3[3];
/*0xa*/	uchar	smcm;
/*0xb*/	uchar	pad4[5];
/*0x10*/
};

typedef struct SPI SPI;
struct SPI {
	ushort	spmode;
	uchar	res1[4];
	uchar	spie;
	uchar	res2[3];
	uchar	spim;
	uchar	res3[2];
	uchar	spcom;
	uchar	res4[2];
};

typedef struct Bankmap Bankmap;
struct Bankmap {
/*0*/	ulong	br;		/*  Base register bank 32 bits 10.3.1/10-14 */
/*4*/	ulong	or;		/*  Option register bank 32 bits 10.3.2/10-16 */
/*8*/
};

typedef struct Port Port;
struct Port {
/*0x00*/	ulong	pdir;		/*  Port A data direction register 32 bits 35.2.3/35-3 */
/*0x04*/	ulong	ppar;	/*  Port Apin assignment register 32 bits 35.2.4/35-4 */
/*0x08*/	ulong	psor;		/*  Port A special options register 32 bits 35.2.5/35-4 */
/*0x0C*/	ulong	podr;	/*  Port Aopen drain register 32 bits 35.2.1/35-2 */
/*0x10*/	ulong	pdat;		/*  Port A data register 32 bits 35.2.2/35-2 */
/*0x14*/	uchar	Rsvd14[12];
/*0x20*/
};

typedef struct IDMA IDMA;
struct IDMA {
/*0x0*/	uchar	idsr;		/*  IDMA event register 8 bits 18.8.4/18-22 */
/*0x1*/	uchar	Rsvd1[3];
/*0x4*/	uchar	idmr;	/*  IDMA mask register 8 bits 18.8.4/18-22 */
/*0x5*/	uchar	Rsvd5[3];
/*0x8*/
};

typedef struct PrmSCC PrmSCC;
struct PrmSCC {
	uchar	sccbytes[0x100];
};

typedef struct PrmFCC PrmFCC;
struct PrmFCC {
	uchar	fccbytes[0x100];
};

typedef struct Bases Bases;
struct Bases {
/*0x00*/	uchar	mcc[0x80];
/*0x80*/	uchar	Rsvd80[0x60];
/*0xe0*/	uchar	risctimers[0x10];
/*0xf0*/	ushort	revnum;
/*0xf2*/	uchar	Rsvdf2[6];
/*0xf8*/	ulong	rand;
/*0xfc*/	ushort	smcbase;
#define	i2cbase	smcbase
/*0xfe*/	ushort	idmabase;
/*0x100*/
};

typedef struct Uartsmc Uartsmc;
struct Uartsmc {
/*0x00*/	IOCparam;
/*0x28*/	ushort	maxidl;
/*0x2a*/	ushort	idlc;
/*0x2c*/	ushort	brkln;
/*0x2e*/	ushort	brkec;
/*0x30*/	ushort	brkcr;
/*0x32*/	ushort	r_mask;
/*0x34*/	ulong	sdminternal;
/*0x38*/	uchar	Rsvd38[8];
/*0x40*/
};

typedef struct SI SI;
struct SI {
/*0x11B20*/	ushort	siamr;		/*  SI TDMA1 mode register 16 bits 14.5.2/14-17 */
/*0x11B22*/	ushort	sibmr;		/*  SI TDMB1 mode register 16 bits */
/*0x11B24*/	ushort	sicmr;		/*  SI TDMC1 mode register 16 bits */
/*0x11B26*/	ushort	sidmr;		/*  SI TDMD1 mode register 16 bits */
/*0x11B28*/	uchar	sigmr;		/*  SI global mode register 8 bits 14.5.1/14-17 */
/*0x11B29*/	uchar	Rsvd11B29;
/*0x11B2A*/	ushort	sicmdr;		/*  SI command register 8 bits 14.5.4/14-24 */
/*0x11B2C*/	ushort	sistr;			/*  SI status register 8 bits 14.5.5/14-25 */
/*0x11B2E*/	ushort	sirsr;			/*  SI RAM shadow address register 16 bits 14.5.3/14-23 */
};

typedef struct IMM IMM;
struct IMM {
/* General SIU */
/*0x10000*/	ulong	siumcr;		/*  SIU module configuration register 32 bits 4.3.2.6/4-31 */
/*0x10004*/	ulong	sypcr;		/*  System protection control register 32 bits 4.3.2.8/4-35 */
/*0x10008*/	uchar	Rsvd10008[0xe-0x8];
/*0x1000E*/	ushort	swsr;		/*  Softwareservice register 16 bits 4.3.2.9/4-36 */
/*0x10010*/	uchar	Rsvd10010[0x14];
/*0x10024*/	ulong	bcr;			/*  Bus configuration register 32 bits 4.3.2.1/4-25 */
/*0x10028*/	ulong	PPC_ACR;		/*  60x bus arbiter configuration register 8 bits 4.3.2.2/4-28 */
/*0x1002C*/	ulong	PPCALRH;		/*  60x bus arbitration-level register high (first 8 clients) 32 bits 4.3.2.3/4-28 */
/*0x10030*/	ulong	PPC_ALRL;	/*  60x bus arbitration-level register low (next 8 clients) 32 bits 4.3.2.3/4-28 */
/*0x10034*/	ulong	LCL_ACR;		/*  Local arbiter configuration register 8 bits 4.3.2.4/4-29 */
/*0x10038*/	ulong	LCL_ALRH;	/*  Local arbitration-level register (first 8 clients) 32 bits 4.3.2.5/4-30 */

/*0x1003C*/	ulong	LCL_ALRL;	/*  Local arbitration-level register (next 8 clients) 32 bits 4.3.2.3/4-28 */
/*0x10040*/	ulong	TESCR1;		/*  60x bus transfer error status control register1 32 bits 4.3.2.10/4-36 */
/*0x10044*/	ulong	TESCR2;		/*  60x bus transfer error status control register2 32 bits 4.3.2.11/4-37 */
/*0x10048*/	ulong	L_TESCR1;	/*  Local bus transfer error status control register1 32 bits 4.3.2.12/4-38 */
/*0x1004C*/	ulong	L_TESCR2;	/*  Local bus transfer error status control register2 32 bits 4.3.2.13/4-39 */
/*0x10050*/	ulong	pdtea;		/*  60x bus DMAtransfer error address 32 bits 18.2.3/18-4 */
/*0x10054*/	uchar	pdtem;		/*  60x bus DMAtransfer error MSNUM 8 bits 18.2.4/18-4 */
/*0x10055*/	uchar	Rsvd10055[3];
/*0x10058*/	void*	ldtea;		/*  Local bus DMA transfer error address 32 bits 18.2.3/18-4 */
/*0x1005C*/	uchar	ldtem;		/*  Local bus DMA transfer error MSNUM 8 bits 18.2.4/18-4 */
/*0x1005D*/	uchar	Rsvd1005D[163];

/* Memory Controller */
/*0x10100*/	Bankmap	bank[12];

/*0x10160*/	uchar	Rsvd10160[8];
/*0x10168*/	void*	MAR;		/*  Memory address register 32 bits 10.3.7/10-29 */
/*0x1016C*/	ulong	Rsvd1016C;
/*0x10170*/	ulong	MAMR;		/*  Machine A mode register 32 bits 10.3.5/10-26 */
/*0x10174*/	ulong	MBMR;		/*  Machine B mode register 32 bits */
/*0x10178*/	ulong	MCMR;		/*  Machine C mode register 32 bits */
/*0x1017C*/	uchar	Rsvd1017C[6];
/*0x10184*/	ulong	mptpr;		/*  Memory periodic timer prescaler 16 bits 10.3.12/10-32 */
/*0x10188*/	ulong	mdr;			/*  Memorydata register 32 bits 10.3.6/10-28 */
/*0x1018C*/	ulong	Rsvd1018C;
/*0x10190*/	ulong	psdmr;		/*  60x bus SDRAM mode register 32 bits 10.3.3/10-21 */
/*0x10194*/	ulong	lsdmr;		/*  Local bus SDRAM mode register 32 bits 10.3.4/10-24 */
/*0x10198*/	ulong	PURT;		/*  60x bus-assigned UPM refresh timer 8 bits 10.3.8/10-30 */
/*0x1019C*/	ulong	PSRT;		/*  60x bus-assigned SDRAM refresh timer 8 bits 10.3.10/10-31 */

/*0x101A0*/	ulong	LURT;		/*  Local bus-assigned UPM refresh timer8 bits 10.3.9/10-30 */
/*0x101A4*/	ulong	LSRT;		/*  Local bus-assigned SDRAM refresh timer 8 bits 10.3.11/10-32 */

/*0x101A8*/	ulong	immr;		/*  Internal memory map register 32 bits 4.3.2.7/4-34 */
/*0x101AC*/	uchar	Rsvd101AC[84];
/* System Integration Timers */
/*0x10200*/	uchar	Rsvd10200[32];
/*0x10220*/	ulong	TMCNTSC;	/*  Time counter statusand control register 16 bits 4.3.2.14/4-40 */

/*0x10224*/	ulong	TMCNT;		/*  Time counter register 32 bits 4.3.2.15/4-41 */
/*0x10228*/	ulong	Rsvd10228;
/*0x1022C*/	ulong	TMCNTAL;	/*  Time counter alarm register 32 bits 4.3.2.16/4-41 */
/*0x10230*/	uchar	Rsvd10230[0x10];
/*0x10240*/	ulong	PISCR;		/*  Periodic interrupt statusand control register 16 bits 4.3.3.1/4-42 */

/*0x10244*/	ulong	PITC;		/*  Periodic interrupt count register 32 bits 4.3.3.2/4-43 */
/*0x10248*/	ulong	PITR;			/*  Periodic interrupt timer register 32 bits 4.3.3.3/4-44 */
/*0x1024C*/	uchar	Rsvd1024C[94];
/*0x102AA*/	uchar	Rsvd102AA[2390];

/* Interrupt Controller */
/*0x10C00*/	ushort	sicr;			/*  SIU interrupt configuration register 16 bits 4.3.1.1/4-17 */
/*0x10C02*/	ushort	Rsvd10C02;
/*0x10C04*/	ulong	sivec;		/*  SIU interrupt vector register 32 bits 4.3.1.6/4-23 */
/*0x10C08*/	ulong	sipnr_h;		/*  SIU interrupt pending register(high) 32 bits 4.3.1.4/4-21 */
/*0x10C0C*/	ulong	sipnr_l;		/*  SIU interrupt pending register(low) 32 bits 4.3.1.4/4-21 */
/*0x10C10*/	ulong	siprr;		/*  SIU interrupt priority register 32 bits 4.3.1.2/4-18 */
/*0x10C14*/	ulong	scprr_h;		/*  CPM interrupt priority register(high) 32 bits 4.3.1.3/4-19 */
/*0x10C18*/	ulong	scprr_l;		/*  CPM interrupt priority register(low) 32 bits 4.3.1.3/4-19 */
/*0x10C1C*/	ulong	simr_h;		/*  SIU interrupt mask register(high) 32 bits 4.3.1.5/4-22 */
/*0x10C20*/	ulong	simr_l;		/*  SIU interrupt mask register(low) 32 bits 4.3.1.5/4-22 */
/*0x10C24*/	ulong	siexr;		/*  SIUexternal interrupt control register 32 bits 4.3.1.7/4-24 */
/*0x10C28*/	uchar	Rsvd10C28[88];

/* Clocksand Reset */
/*0x10C80*/	ulong	sccr;			/*  Systemclock control register 32 bits 9.8/9-8 */
/*0x10C84*/	uchar	Rsvd10C84[4];
/*0x10C88*/	ulong	scmr;		/*  Systemclock mode register 32 bits 9.9/9-9 */
/*0x10C8C*/	uchar	Rsvd10C8C[4];
/*0x10C90*/	ulong	rsr;			/*  Reset status register 32 bits 5.2/5-4 */
/*0x10C94*/	ulong	rmr;			/*  Reset mode register 32 bits 5.3/5-5 */
/*0x10C98*/	uchar	Rsvd10C98[104];

/* Part I.Overview Input/Output Port */
/*0x10D00*/	Port		port[4];

/* CPMTimers */
/*0x10D80*/	uchar	tgcr1;		/*  Timer1 and timer2 global configuration register 8 bits 17.2.2/17-4 */

/*0x10D81*/	uchar	Rsvd10D81[3];
/*0x10D84*/	uchar	tgcr2;		/*  Timer3 and timer4 global configuration register 8 bits 17.2.2/17-4 */
/*0x10D85*/	uchar	Rsvd10D85[3];

/*0x10D88*/	uchar	Rsvd10D88[8];
/*0x10D90*/	ushort	tmr1;		/*  Timer1 mode register 16 bits 17.2.3/17-6 */
/*0x10D92*/	ushort	tmr2;		/*  Timer2 mode register 16 bits 17.2.3/17-6 */
		union{
			struct {
/*0x10D94*/	ushort	trr1;			/*  Timer1 reference register 16 bits 17.2.4/17-7 */
/*0x10D96*/	ushort	trr2;			/*  Timer2 reference register 16 bits 17.2.4/17-7 */
			};
/*0x10D94*/	ulong	trrl1;			/*  Combined Timer 1/2 trr register */
		};
		union{
			struct {
/*0x10D98*/	ushort	tcr1;			/*  Timer1 capture register 16 bits 17.2.5/17-8 */
/*0x10D9A*/	ushort	tcr2;			/*  Timer2 capture register 16 bits 17.2.5/17-8 */
			};
/*0x10D98*/	ulong	tcrl1;		/*  Combined timer1/2 capture register */
		};
		union{
			struct {
/*0x10D9C*/	ushort	tcn1;			/*  Timer1 counter 16 bits 17.2.6/17-8 */
/*0x10D9E*/	ushort	tcn2;			/*  Timer2 counter 16 bits 17.2.6/17-8 */
			};
/*0x10D9C*/	ulong	tcnl1;		/*  Combined timer1/2 counter */
		};
/*0x10DA0*/	ushort	tmr3;		/*  Timer3 mode register 16 bits 17.2.3/17-6 */
/*0x10DA2*/	ushort	tmr4;		/*  Timer4 mode register 16 bits 17.2.3/17-6 */
		union{
			struct {
/*0x10DA4*/	ushort	trr3;			/*  Timer3 reference register 16 bits 17.2.4/17-7 */
/*0x10DA6*/	ushort	trr4;			/*  Timer4 reference register 16 bits 17.2.4/17-7 */
			};
/*0x10DA4*/	ulong	trrl3;
		};			
		union{
			struct {
/*0x10DA8*/	ushort	tcr3;			/*  Timer3 capture register 16 bits 17.2.5/17-8 */
/*0x10DAA*/	ushort	tcr4;			/*  Timer4 capture register 16 bits 17.2.5/17-8 */
			};
/*0x10DA8*/	ulong	tcrl3;
		};
		union{
			struct {
/*0x10DAC*/	ushort	tcn3;			/*  Timer3 counter 16 bits 17.2.6/17-8 */
/*0x10DAE*/	ushort	tcn4;			/*  Timer4 counter 16 bits 17.2.6/17-8 */
			};
/*0x10DAC*/	ulong	tcnl3;
		};
/*0x10DB0*/	ushort	ter1;			/*  Timer1 event register 16 bits 17.2.7/17-8 */
/*0x10DB2*/	ushort	ter2;			/*  Timer2 event register 16 bits 17.2.7/17-8 */
/*0x10DB4*/	ushort	ter3;			/*  Timer3 event register 16 bits 17.2.7/17-8 */
/*0x10DB6*/	ushort	ter4;			/*  Timer4 event register 16 bits 17.2.7/17-8 */
/*0x10DB8*/	uchar	Rsvd10DB8[608];

/* SDMADHGeneral */
/*0x11018*/	uchar	sdsr;			/*  SDMA status register 8 bits 18.2.1/18-3 */
/*0x11019*/	uchar	Rsvd11019[3];
/*0x1101C*/	uchar	sdmr;		/*  SDMA mask register 8 bits 18.2.2/18-4 */
/*0x1101D*/	uchar	Rsvd1101D[3];

/* IDMA */
/*0x11020*/	IDMA	idma[4];

/*0x11040*/	uchar	Rsvd11040[704];

/*0x11300*/	FCC		fcc[3];

/*0x11360*/	uchar	Rsvd11360[0x290];

/* BRGs5DH8 */
/*0x115F0*/	ulong	BRGC5;		/*  BRG5 configuration register 32 bits 16.1/16-2 */
/*0x115F4*/	ulong	BRGC6;		/*  BRG6configuration register 32 bits */
/*0x115F8*/	ulong	BRGC7;		/*  BRG7configuration register 32 bits */
/*0x115FC*/	ulong	BRGC8;		/*  BRG8configuration register 32 bits */
/*0x11600*/	uchar	Rsvd11600[0x260];
/*0x11860*/	uchar	I2MOD;		/*  I2C mode register 8 bits 34.4.1/34-6 */
/*0x11861*/	uchar	Rsvd11861[3];
/*0x11864*/	uchar	I2ADD;		/*  I2C address register 8 bits 34.4.2/34-7 */
/*0x11865*/	uchar	Rsvd11865[3];
/*0x11868*/	uchar	I2BRG;		/*  I2C BRG register 8 bits 34.4.3/34-7 */
/*0x11869*/	uchar	Rsvd11869[3];
/*0x1186C*/	uchar	I2COM;		/*  I2C command register 8 bits 34.4.5/34-8 */
/*0x1186D*/	uchar	Rsvd1186D[3];
/*0x11870*/	uchar	I2CER;		/*  I2C event register 8 bits 34.4.4/34-8 */
/*0x11871*/	uchar	Rsvd11871[3];
/*0x11874*/	uchar	I2CMR;		/*  I2C mask register 8 bits 34.4.4/34-8 */
/*0x11875*/	uchar	Rsvd11875[331];

/* Communications Processor */
/*0x119C0*/	ulong	cpcr;		/*  Communications processor command register 32 bits 13.4.1/13-11 */

/*0x119C4*/	ulong	rccr;		/*  CP configuration register 32 bits 13.3.6/13-7 */
/*0x119C8*/	uchar	Rsvd119C8[14];
/*0x119D6*/	ushort	rter;		/*  CP timers event register 16 bits 13.6.4/13-21 */
/*0x119D8*/	ushort	Rsvd119D8;
/*0x119DA*/	ushort	rtmr;		/*  CP timers mask register 16 bits */
/*0x119DC*/	ushort	rtscr;		/*  CPtime-stamp timer control register 16 bits 13.3.7/13-9 */
/*0x119DE*/	ushort	Rsvd119DE;
/*0x119E0*/	ulong	rtsr;		/*  CPtime-stamp register 32 bits 13.3.8/13-10 */
/*0x119E4*/	uchar	Rsvd119E4[12];

/*0x119F0*/	ulong	brgc[4];		/*  BRG configuration registers 32 bits 16.1/16-2 */

/*0x11A00*/	SCC		scc[4];

/*0x11A80*/	SMC		smc[2];

			SPI		spi;

/*0x11AB0*/	uchar	Rsvd11AB0[80];

/* CPMMux */
/*0x11B00*/	uchar	cmxsi1cr;	/*  CPM mux SI1clock route register 8 bits 15.4.2/15-10 */
/*0x11B01*/	uchar	Rsvd11B01;
/*0x11B02*/	uchar	cmxsi2cr;	/*  CPM mux SI2clock route register 8 bits 15.4.3/15-11 */
/*0x11B03*/	uchar	Rsvd11B03;
/*0x11B04*/	ulong	cmxfcr;	/*  CPM mux FCC clock route register 32 bits 15.4.4/15-12 */
/*0x11B08*/	ulong	cmxscr;	/*  CPM mux SCC clock route register 32 bits 15.4.5/15-14 */
/*0x11B0C*/	uchar	cmxsmr;	/*  CPM mux SMC clock route register 8 bits 15.4.6/15-17 */
/*0x11B0D*/	uchar	Rsvd11B0D;
/*0x11B0E*/	ushort	cmxuar;	/*  CPM mux UTOPIA address register 16 bits 15.4.1/15-7 */
/*0x11B10*/	uchar	Rsvd11B10[16];

			SI		si1;			/* SI 1 Registers */

/* MCC1Registers */
/*0x11B30*/	ushort	MCCE1;		/*  MCC1 event register 16 bits 27.10.1/27-18 */
/*0x11B32*/	ushort	Rsvd11B32;
/*0x11B34*/	ushort	MCCM1;		/*  MCC1 mask register 16 bits */
/*0x11B36*/	ushort	Rsvd11B36;
/*0x11B38*/	uchar	MCCF1;		/*  MCC1 configuration register 8 bits 27.8/27-15 */
/*0x11B39*/	uchar	Rsvd11B39[7];

			SI		si2;			/* SI 2 Registers */

/* MCC2Registers */
/*0x11B50*/	ushort	MCCE2;		/*  MCC2 event register 16 bits 27.10.1/27-18 */
/*0x11B52*/	ushort	Rsvd11B52;
/*0x11B54*/	ushort	MCCM2;		/*  MCC2 mask register 16 bits */
/*0x11B56*/	ushort	Rsvd11B56;
/*0x11B58*/	uchar	MCCF2;		/*  MCC2 configuration register 8 bits 27.8/27-15 */
/*0x11B59*/	uchar	Rsvd11B59[1191];

/* SI1RAM */
/*0x12000*/	uchar	SI1TxRAM[0x200];/*  SI1 transmit routing RAM	512 14.4.3/14-10 */
/*0x12200*/	uchar	Rsvd12200[0x200];
/*0x12400*/	uchar	SI1RxRAM[0x200];/*  SI1 receive routing RAM	512 14.4.3/14-10 */
/*0x12600*/	uchar	Rsvd12600[0x200];

/* SI2RAM */
/*0x12800*/	uchar	SI2TxRAM[0x200];/*  SI2 transmit routing RAM	512 14.4.3/14-10 */
/*0x12A00*/	uchar	Rsvd12A00[0x200];
/*0x12C00*/	uchar	SI2RxRAM[0x200];/*  SI2 receive routing RAM	512 14.4.3/14-10 */
/*0x12E00*/	uchar	Rsvd12E00[0x200];
/*0x13000*/	uchar	Rsvd13000[0x800];
/*0x13800*/	uchar	Rsvd13800[0x800];
};

typedef struct FCCextra FCCextra;
struct FCCextra {
/*0x00*/	uchar	ri[0x20];
/*0x20*/	uchar	ti[0x20];
/*0x40*/	uchar	pad[0x20];
};

typedef struct Imap Imap;
struct Imap {
/* CPMDual-Port RAM */
/*0x00000*/	uchar	dpram1[0x3800];	/*  Dual-port RAM	16Kbytes 13.5/13-15 */
/*0x03800*/	FCCextra	fccextra[4];
/*0x03980*/	Uartsmc	uartsmc[2];
/*0x03a00*/	uchar	dsp1p[0x40];
/*0x03a40*/	uchar	dsp2p[0x40];
/*0x03a80*/	BD		bd[(0x04000-0x03a80)/sizeof(BD)];	/* Buffer descriptors */
/*0x04000*/	uchar	Rsvd4000[0x04000];

/* Dual port RAM bank 2 -- Parameter Ram, Section 13.5 */
/*0x08000*/	PrmSCC	prmscc[4];
/*0x08400*/	PrmFCC	prmfcc[3];
/*0x08700*/	Bases	param[4];
/*0x08b00*/	uchar	dpram2[0x500];

/*0x09000*/	uchar	Rsvd9000[0x2000];

/* Dual port RAM bank 3 -- Section 13.5 */
/*0x0B000*/	uchar	dpram3[0x1000];	/*  Dual-port RAM	4Kbytes 13.5/13-15 */
/*0x0C000*/	uchar	Rsvdc000[0x4000];

/*0x10000*/	IMM;
};

enum {
/* CPM Command register. */
	cpm_rst		= 0x80000000,
	cpm_page	= 0x7c000000,
	cpm_sblock	= 0x03e00000,
	cpm_flg		= 0x00010000,
	cpm_mcn		= 0x00003fc0,
	cpm_opcode	= 0x0000000f,

/* Device sub-block and page codes. */
	cpm_fcc1_sblock	= 0x10,
	cpm_fcc2_sblock	= 0x11,
	cpm_fcc3_sblock	= 0x12,
	cpm_scc1_sblock	= 0x04,
	cpm_scc2_sblock	= 0x05,
	cpm_scc3_sblock	= 0x06,
	cpm_scc4_sblock	= 0x07,
	cpm_smc1_sblock	= 0x08,
	cpm_smc2_sblock	= 0x09,
	cpm_rand_sblock	= 0x0e,
	cpm_spi_sblock	= 0x0a,
	cpm_i2c_sblock	= 0x0b,
	cpm_timer_sblock	= 0x0f,
	cpm_mcc1_sblock	= 0x1c,
	cpm_mcc2_sblock	= 0x1d,
	cpm_idma1_sblock	= 0x14,
	cpm_idma2_sblock	= 0x15,
	cpm_idma3_sblock	= 0x16,
	cpm_idma4_sblock	= 0x17,

	cpm_scc1_page	= 0x00,
	cpm_scc2_page	= 0x01,
	cpm_scc3_page	= 0x02,
	cpm_scc4_page	= 0x03,
	cpm_smc1_page	= 0x07,
	cpm_smc2_page	= 0x08,
	cpm_spi_page		= 0x09,
	cpm_i2c_page		= 0x0a,
	cpm_timer_page	= 0x0a,
	cpm_rand_page	= 0x0a,
	cpm_fcc1_page	= 0x04,
	cpm_fcc2_page	= 0x05,
	cpm_fcc3_page	= 0x06,
	cpm_idma1_page	= 0x07,
	cpm_idma2_page	= 0x08,
	cpm_idma3_page	= 0x09,
	cpm_idma4_page	= 0x0a,
	cpm_mcc1_page	= 0x07,
	cpm_mcc2_page	= 0x08,

};

/*
 * CPM
 */
enum {
	/* commands */
	InitRxTx =	0,
	InitRx =		1,
	InitTx =		2,
	EnterHunt=	3,
	StopTx=		4,
	GracefulStopTx = 5,
	InitIDMA =	5,
	RestartTx =	6,
	CloseRxBD =	7,
	SetGroupAddr = 8,
	SetTimer =	8,
	GCITimeout =	9,
	GCIAbort =	10,
	StopIDMA =	11,
	StartDSP = 	12,
	ArmIDMA =	13,
	InitDSP =		13,
	USBCmd =	15,

	/* channel IDs */
	SCC1ID=	cpm_scc1_page << 5 | cpm_scc1_sblock,
	SCC2ID=	cpm_scc2_page << 5 | cpm_scc2_sblock,
	SCC3ID=	cpm_scc3_page << 5 | cpm_scc3_sblock,
	SMC1ID=	cpm_smc1_page << 5 | cpm_smc1_sblock,
	SMC2ID=	cpm_smc2_page << 5 | cpm_smc2_sblock,
	FCC1ID=	cpm_fcc1_page << 5 | cpm_fcc1_sblock,
	FCC2ID=	cpm_fcc2_page << 5 | cpm_fcc2_sblock,
	FCC3ID=	cpm_fcc3_page << 5 | cpm_fcc3_sblock,
//	USBID=	0,		These are wrong
//	I2CID=	1,
//	IDMA1ID= 1,
//	SPIID=	5,
//	IDMA2ID= 5,
//	TIMERID=	5,
//	DSP1ID=9,
//	SCC4ID=	10,
//	DSP2ID=	13,

	/* sicr */
	BRG1 = 0,
	BRG2 = 1,
	BRG3 = 2,
	BRG4 = 4,
	CLK1 = 4,
	CLK2 = 5,
	CLK3 = 6,
	CLK4 = 7,

};

extern IMM* iomem;

BD*	bdalloc(int);
void	cpmop(int, int, int);
void	ioplock(void);
void	iopunlock(void);
void	kreboot(ulong);
