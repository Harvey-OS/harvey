#pragma	lib	"libfis.a"
#pragma	src	"/sys/src/libfis"

/* ata errors */
enum {
	Emed	= 1<<0,		/* media error */
	Enm	= 1<<1,		/* no media */
	Eabrt	= 1<<2,		/* abort */
	Emcr	= 1<<3,		/* media change request */
	Eidnf	= 1<<4,		/* no user-accessible address */
	Emc	= 1<<5,		/* media change */
	Eunc	= 1<<6,		/* data error */
	Ewp	= 1<<6,		/* write protect */
	Eicrc	= 1<<7,		/* interface crc error */

	Efatal	= Eidnf|Eicrc,	/* must sw reset */
};

/* ata status */
enum {
	ASerr	= 1<<0,		/* error */
	ASdrq	= 1<<3,		/* request */
	ASdf	= 1<<5,		/* fault */
	ASdrdy	= 1<<6,		/* ready */
	ASbsy	= 1<<7,		/* busy */

	ASobs	= 1<<1|1<<2|1<<4,
};

enum {
	/* fis types */
	H2dev		= 0x27,
	D2host		= 0x34,

	/* fis flags bits */
	Fiscmd		= 0x80,

	/* ata bits */
	Ataobs		= 0xa0,
	Atalba		= 0x40,

	/* nominal fis size (fits any fis) */
	Fissize		= 0x20,
};

/* sata device-to-host (0x27) fis layout */
enum {
	Ftype,
	Fflags,
	Fcmd,
	Ffeat,
	Flba0,
	Flba8,
	Flba16,
	Fdev,
	Flba24,
	Flba32,
	Flba40,
	Ffeat8,
	Fsc,
	Fsc8,
	Ficc,		/* isochronous cmd completion */
	Fcontrol,
};

/* sata host-to-device fis (0x34) differences */
enum{
	Fioport	= 1,
	Fstatus,
	Frerror,
};

/* ata protcol type */
enum{
	Pnd	= 0<<0,	/* data direction */
	Pin	= 1<<0,
	Pout	= 2<<0,
	Pdatam	= 3<<0,

	Ppio	= 1<<2,	/* ata protocol */
	Pdma	= 2<<2,
	Pdmq	= 3<<2,
	Preset	= 4<<2,
	Pdiag	= 5<<2,
	Ppkt	= 6<<2,
	Pprotom	= 7<<2,

	P48	= 0<<5,	/* command “size” */
	P28	= 1<<5,
	Pcmdszm	= 1<<5,

	Pssn	= 0<<6,	/* sector size */
	P512	= 1<<6,
	Pssm	= 1<<6,
};

typedef struct Sfis Sfis;
struct Sfis {
	ushort	feat;
	uchar	udma;
	uchar	speeds;
	uint	sig;
	uint	lsectsz;
	uint	physshift;	/* log2(log/phys) */
	uint	c;		/* disgusting, no? */
	uint	h;
	uint	s;
};

enum {
	Dlba	= 1<<0,	/* required for sata */
	Dllba 	= 1<<1,
	Dsmart	= 1<<2,
	Dpower	= 1<<3,
	Dnop	= 1<<4,
	Datapi	= 1<<5,
	Datapi16= 1<<6,
	Data8	= 1<<7,
	Dsct	= 1<<8,
	Dnflag	= 9,
};

enum {
	Pspinup	= 1<<0,
	Pidready	= 1<<1,
};

void	setfissig(Sfis*, uint);
int	txmodefis(Sfis*, uchar*, uchar);
int	atapirwfis(Sfis*, uchar*, uchar*, int, int);
int	featfis(Sfis*, uchar*, uchar);
int	flushcachefis(Sfis*, uchar*);
int	identifyfis(Sfis*, uchar*);
int	nopfis(Sfis*, uchar*, int);
int	rwfis(Sfis*, uchar*, int, int, uvlong);
void	skelfis(uchar*);
void	sigtofis(Sfis*, uchar*);
uvlong	fisrw(Sfis*, uchar*, int*);

void	idmove(char*, ushort*, int);
vlong	idfeat(Sfis*, ushort*);
uvlong	idwwn(Sfis*, ushort*);
int	idss(Sfis*, ushort*);
int	idpuis(ushort*);
ushort	id16(ushort*, int);
uint	id32(ushort*, int);
uvlong	id64(ushort*, int);
char	*pflag(char*, char*, Sfis*);
uint	fistosig(uchar*);

/* scsi */
typedef struct Cfis Cfis;
struct Cfis {
	uchar	phyid;
	uchar	encid[8];
	uchar	tsasaddr[8];
	uchar	ssasaddr[8];
	uchar	ict[2];
};

void	smpskelframe(Cfis*, uchar*, int);
uint	sashash(uvlong);
uchar	*sasbhash(uchar*, uchar*);
