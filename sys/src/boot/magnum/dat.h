/*
 * dat.h for the mini kernel
 */
typedef struct IOQ	IOQ;
typedef struct Scsi	Scsi;
typedef struct Scsidata	Scsidata;
typedef struct Ureg	Ureg;

#define NQ	4096
struct IOQ
{
	uchar	buf[NQ];
	uchar	*in;
	uchar	*out;
	void	*ptr;
};

/*
 *  for SCSI bus
 */
struct Scsidata
{
	uchar	*base;
	uchar	*lim;
	uchar	*ptr;
};

struct Scsi
{
	ushort	target;
	ushort	lun;
	ushort	rflag;
	ushort	status;
	Scsidata cmd;
	Scsidata data;
	uchar	*save;
	uchar	cmdblk[16];
};

#define PRINTSIZE	256

#define COUNT	((6250000L*MS2HZ)/(1000L))		/* counts per HZ */
#define	GETCHZ	(1000*1000)				/* getc's per second */

extern int	scsidebug;
extern ulong	ticks;
extern IOQ	kbdq;
