#define UNCACHED	0xA0000000
#define	IO(t, x)	((t *)(UNCACHED|(x)))

#define	LINESIZE	(32*4)

typedef struct Sbc	Sbc;

struct Sbc {
	ulong	intvecset;		/* interrupt vector set */
	ulong	intvecclr;		/* interrupt vector clear */
	ulong	intmask;		/* interrupt mask */
	ulong	boardaddr;		/* board address */
	ulong	ctlmisc;		/* control miscellaneous */
	ulong	scandata;		/* scan data */
	ulong	memecc;			/* memory ECC */
	ulong	memctl;			/* memory control */
	ulong	errreg;			/* error */
	ulong	resetrefresh;		/* reset refresh */
	ulong	compare;		/* timer compare */
	ulong	count;			/* timer count */
	ulong	q0ctake;		/* read queue 0 */
	ulong	q1ctake;		/* read queue 1 */
	ulong	resetbrd;		/* reset some board FSMs */
};

#define TIMERIE		0x00000001	/* int(vecset|vecclr|mask) */
#define DUARTIE		0x00000008
#define VMEIE		0x00003330
#define VMELOPIE	0x0000CCC0
#define SW0IE		0x40000000
#define SW1IE		0x80000000

/*
 *  non-volatile ram (every 4th location)
 */
typedef struct Nvram	Nvram;
struct Nvram
{
	uchar	pad[3];
	uchar	val;
};
#define	NVRAM	IO(Nvram, 0x1E02C000)
#define NVLEN	2040

#define NVRAUTHADDR	(1024+900)

/*
 * real-time clock (every 4th location)
 */
typedef struct RTCdev	RTCdev;
struct RTCdev
{
	uchar	pad1[3];
	uchar	control;		/* read or write the device */
	uchar	pad2[3];
	uchar	sec;
	uchar	pad3[3];
	uchar	min;
	uchar	pad4[3];
	uchar	hour;
	uchar	pad5[3];
	uchar	wday;
	uchar	pad6[3];
	uchar	mday;
	uchar	pad7[3];
	uchar	mon;
	uchar	pad8[3];
	uchar	year;
};
#define RTC		IO(RTCdev, 0x1E02DFE0)
#define RTCREAD		(0x40)
#define RTCWRITE	(0x80)

/*
 * fix up all these awful defines
 */

#define CPUIVECTSET	0x1E01F000
#define DUARTIOAVEC	IO(ulong, 0x1E027800)
#define	LEDREG		IO(uchar, 0x1E02E003)
#define	DUARTREG	IO(Duart, 0x1E02E203)
#define IOAERRREG	IO(ulong, 0x1E02EA00)

#define SBCREG		IO(Sbc,   0x1E00F000)

#define SLAVE	0x0


extern Vmedevice vmedevtab[];

extern	int	jaginit(Vmedevice *);
extern	void	jagintr(Vmedevice *);
extern	void	jagstats(void);
extern	Drive*	jagdrive(Device);

extern	int	hsvmeinit(Vmedevice *);
extern	void	hsvmeintr(Vmedevice *);

extern	int	hrodinit(Vmedevice *);
extern	void	hrodintr(Vmedevice *);

extern	int	eagleinit(Vmedevice *);
extern	void	eagleintr(Vmedevice *);
