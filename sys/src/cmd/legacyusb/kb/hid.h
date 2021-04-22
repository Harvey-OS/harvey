/*
 * USB keyboard/mouse constants
 */

typedef struct Chain Chain;
typedef struct HidInterface HidInterface;
typedef struct HidRepTempl HidRepTempl;

enum {
	Stack = 32 * 1024,

	/* HID class subclass protocol ids */
	PtrCSP		= 0x020103,	/* mouse.boot.hid */
	KbdCSP		= 0x010103,	/* keyboard.boot.hid */

	/* Requests */
	Getproto	= 0x03,
	Setidle		= 0x0a,
	Setproto	= 0x0b,

	/* protocols for SET_PROTO request */
	Bootproto	= 0,
	Reportproto	= 1,
};

enum {
	/* keyboard modifier bits */
	Mlctrl		= 0,
	Mlshift		= 1,
	Mlalt		= 2,
	Mlgui		= 3,
	Mrctrl		= 4,
	Mrshift		= 5,
	Mralt		= 6,
	Mrgui		= 7,

	/* masks for byte[0] */
	Mctrl		= 1<<Mlctrl | 1<<Mrctrl,
	Mshift		= 1<<Mlshift | 1<<Mrshift,
	Malt		= 1<<Mlalt | 1<<Mralt,
	Mcompose	= 1<<Mlalt,
	Maltgr		= 1<<Mralt,
	Mgui		= 1<<Mlgui | 1<<Mrgui,

	MaxAcc = 3,			/* max. ptr acceleration */
	PtrMask= 0xf,			/* 4 buttons: should allow for more. */
};

/*
 * Plan 9 keyboard driver constants.
 */
enum {
	/* Scan codes (see kbd.c) */
	SCesc1		= 0xe0,		/* first of a 2-character sequence */
	SCesc2		= 0xe1,
	SClshift	= 0x2a,
	SCrshift	= 0x36,
	SCctrl		= 0x1d,
	SCcompose	= 0x38,
	Keyup		= 0x80,		/* flag bit */
	Keymask		= 0x7f,		/* regular scan code bits */
};

int kbmain(Dev *d, int argc, char*argv[]);

enum{
	MaxChLen	= 64,	/* bytes */
};

struct Chain {
	int	b;			/* offset start in bits, (first full) */
	int	e;			/* offset end in bits (first empty) */
	uchar	buf[MaxChLen];
};

#define MSK(nbits)		((1UL << (nbits)) - 1)
#define IsCut(bbits, ebits)	(((ebits)/8 - (bbits)/8) > 0)

enum {
	KindPad = 0,
	KindButtons,
	KindX,
	KindY,
	KindWheel,

	MaxVals	= 16,
	MaxIfc	= 8,
};

struct HidInterface {
	ulong	v[MaxVals];	/* one ulong per val should be enough */
	uchar	kind[MaxVals];
	int	nbits;
	int	count;
};

struct HidRepTempl{
	int	id;				/* id which may not be present */
	uint	sz;				/* in bytes */
	int	nifcs;
	HidInterface ifcs[MaxIfc];
};

enum {
	/* report types */

	HidReportApp	= 0x01,
	HidTypeUsgPg	= 0x05,
	HidPgButts	= 0x09,

	HidTypeRepSz	= 0x75,
	HidTypeCnt	= 0x95,
	HidCollection	= 0xa1,

	HidTypeUsg	= 0x09,
	HidPtr		= 0x01,
	HidX		= 0x30,
	HidY		= 0x31,
	HidZ		= 0x32,
	HidWheel	= 0x38,

	HidInput	= 0x81,
	HidReportId	= 0x85,
	HidReportIdPtr	= 0x01,

	HidEnd		= 0xc0,
};

void	dumpreport(HidRepTempl *templ);
int	hidifcval(HidRepTempl *templ, int kind, int n);
int	parsereport(HidRepTempl *templ, Chain *rep);
int	parsereportdesc(HidRepTempl *temp, uchar *repdesc, int repsz);
