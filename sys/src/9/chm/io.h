#define	IO(t,x)		((t*)(KSEG1|((ulong)x)))

/* Interrupt Vectors */
#define IVGIO0		0
#define IVSCSI		1
#define IVSCSI1		2
#define IVENET		3
#define IVGDMA		4
#define IVPLP		5
#define IVGIO1		6
#define IVLCL2		7
#define IVISDN_ISAC	8
#define IVPOWER		9
#define IVISDN_HSCX	10
#define IVLCL3		11
#define IVHPCDMA	12
#define IVACFAIL	13
#define IVVIDEO		14
#define IVGIO2		15
#define IVEISA		19
#define IVKBDMS		20
#define IVDUART		21
#define IVDRAIN0	22
#define IVDRAIN1	23
#define IVGIOEXP0	22
#define IVGIOEXP1	23

enum
{
	Nuart		= 2,
	SCCFREQ		= 3672000,
	DUART0		= 0x1fbd9830,
};

typedef struct SCCdev	SCCdev;
struct SCCdev
{
	uchar	pad[3];
	uchar	ptrb;
	uchar	pad[3];
	uchar	datab;
	uchar	pad[3];
	uchar	ptra;
	uchar	pad[3];
	uchar	dataa;
};

/*
 * Local Interrupt registers (INT2)
 */
#define HPC3IA		0x1fbd9000
#define	LIO_0_ISR	(HPC3IA+0x03)
#define	LIO_0_MASK	(HPC3IA+0x07)
#define	LIO_1_ISR	(HPC3IA+0x0b)
#define	LIO_1_MASK	(HPC3IA+0x0f)
#define LIO_2_ISR	(HPC3IA+0x13)
#define LIO_2_MASK	(HPC3IA+0x17)
/*
 * Memory bank configuration
 */
#define	MEMCFG0		0x1fa000c4
#define	MEMCFG1		0x1fa000cc

#define HPC3_ETHER	0x1fb80000
#define HPC3_NVRTC	0x1fbb000b
#define NVSIZE		256
#define NVEADDR		(NVSIZE-6)
#define NVSUM		0

#define HPC3_RTC	0x1fbe0000

enum
{
	GIOARB		= 0x1FA00084,
	   GIOHPCSZ64	= 0x00000001,
	   GIOGRXSZ64	= 0x00000002,
	   GIOEXP0SZ64	= 0x00000004,
	   GIOEXP1SZ64	= 0x00000008,
	   GIOEISASZ	= 0x00000010,
	   GIOHPCEXPSZ64= 0x00000020,
	   GIOGRXRT	= 0x00000040,
	   GIOEXP0RT	= 0x00000080,
	   GIOEXP1RT	= 0x00000100,
	   GIOEISAMST	= 0x00000200,
	   GIO1GIO	= 0x00000400,
	   GIOGRXMST	= 0x00000800,
	   GIOEXP0MST	= 0x00001000,
	   GIOEXP1MST	= 0x00002000,
	   GIOEXP0PIPED	= 0x00004000,
	   GIOEXP1PIPED	= 0x00008000,
	GIOBASECFG	= GIOHPCSZ64|GIOHPCEXPSZ64|GIOEISAMST|GIO1GIO,
	GIOSLOT0CFG 	= GIOEXP0SZ64|GIOEXP0PIPED|GIOEXP0MST|GIOEXP0RT,
	GIOSLOT1CFG 	= GIOEXP1SZ64|GIOEXP1PIPED,
};

typedef struct Vectors Vectors;
struct Vectors
{
	void 	(*handler)(Ureg *ur, void *arg);
	void*	arg;
};

/*
 * Prom entry points
 */
#define	PROM_RESET	0	/* run diags, check bootmode, reinit */
#define	PROM_EXEC	1	/* load new program image */
#define	PROM_RESTART	2	/* re-enter monitor command loop */
#define	PROM_REINIT	3	/* re-init monitor, then cmd loop */
#define	PROM_REBOOT	4	/* check bootmode, no config */
#define	PROM_AUTOBOOT	5	/* autoboot the system */

typedef struct RTCdev	RTCdev;
struct RTCdev
{
	ulong	status;
	union{
		struct{
			ulong	pad[2];
			ulong	pflag;
			ulong	savctl;
		};
		struct{
			ulong	rtime;
			ulong	outmode;
			ulong	int0ctl;
			ulong	int1ctl;
		};
	};
	ulong	hsec;
	ulong	sec;
	ulong	min;
	ulong	hour;
	ulong	mday;
	ulong	mon;
	ulong	year;
	ulong	ram0[2];
	ulong	dow;
	ulong	pad[4];
	ulong	timcmp[6];
	ulong	timsav[5];
	ulong	ram1[2];
};
