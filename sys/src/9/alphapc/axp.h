typedef struct Hwrpb	Hwrpb;
typedef struct Hwcpu	Hwcpu;
typedef struct Hwdsr	Hwdsr;

struct Hwrpb
{
	uvlong	phys;
	uvlong	sign;
	uvlong	rev;
	uvlong	size;
	uvlong	cpu0;
	uvlong	by2pg;
	uvlong	pabits;
	uvlong	maxasn;
	char	ssn[16];
	uvlong	systype;
	uvlong	sysvar;
	uvlong	sysrev;
	uvlong	ifreq;
	uvlong	cfreq;
	uvlong	vptb;
	uvlong	resv;
	uvlong	tbhint;
	uvlong	ncpu;
	uvlong	cpulen;
	uvlong	cpuoff;
	uvlong	nctb;
	uvlong	ctblen;
	uvlong	ctboff;
	uvlong	ccrboff;
	uvlong	memoff;
	uvlong	confoff;
	uvlong	fruoff;
	uvlong	termsaveva;
	uvlong	termsavex;
	uvlong	termrestva;
	uvlong	termrestx;
	uvlong	termresetva;
	uvlong	termresetx;
	uvlong	sysresv;
	uvlong	hardresv;
	uvlong	csum;
	uvlong	rxrdymsk;
	uvlong	txrdymsk;
	uvlong	dsroff;		/* rev 6 or higher */
};

extern Hwrpb* hwrpb;

struct Hwcpu
{
	uvlong	hwpcb[16];
	uvlong	state;
	uvlong	palmainlen;
	uvlong	palscratchlen;
	uvlong	palmainpa;
	uvlong	palscratchpa;
	uvlong	palrev;
	uvlong	cputype;
	uvlong	cpuvar;
	uvlong	cpurev;
	uvlong	serial[2];
	/* more crap ... */
};

struct Hwdsr
{
	vlong	smm;
	uvlong	lurtoff;
	uvlong	sysnameoff;
};
