/*
 * Storage Device.
 */
typedef struct SDev SDev;
typedef struct SDifc SDifc;
typedef struct SDpart SDpart;
typedef struct SDreq SDreq;
typedef struct SDunit SDunit;

typedef struct SDpart {
	Devsize	start;
	Devsize	end;
	char	name[NAMELEN];
	char	user[NAMELEN];
	ulong	perm;
	int	valid;
	void	*crud;
} SDpart;

typedef struct SDunit {
	SDev*	dev;
	int	subno;
	uchar	inquiry[256];		/* format follows SCSI spec */
	uchar	sense[18];		/* format follows SCSI spec */
	char	name[NAMELEN];
	Rendez	rendez;

	QLock	ctl;
	Devsize	sectors;	/* not Off: even 32-bit fs can have big disks */
	ulong	secsize;
	SDpart*	part;
	int	npart;			/* of valid partitions */
	int	changed;

	QLock	raw;
	int	state;
	ulong	pid;
	SDreq*	req;
} SDunit;

typedef struct SDev {
	SDifc*	ifc;			/* pnp/legacy */
	void	*ctlr;
	int	idno;
	int	index;			/* into unit space */
	int	nunit;
	SDev*	next;

	QLock;				/* enable/disable */
	int	enabled;
} SDev;

typedef struct SDifc {
	char*	name;

	SDev*	(*pnp)(void);
	SDev*	(*legacy)(int, int);
	SDev*	(*id)(SDev*);
	int	(*enable)(SDev*);
	int	(*disable)(SDev*);

	int	(*verify)(SDunit*);
	int	(*online)(SDunit*);
	int	(*rio)(SDreq*);
	int	(*rctl)(SDunit*, char*, int);
	int	(*wctl)(SDunit*, void*);

	long	(*bio)(SDunit*, int, int, void*, long, Off);
} SDifc;

typedef struct SDreq {
	SDunit*	unit;
	int	lun;
	int	write;
	uchar	cmd[16];
	int	clen;
	void*	data;
	int	dlen;

	int	flags;

	int	status;
	long	rlen;
	uchar	sense[256];
} SDreq;

enum {
	SDnosense	= 0x00000001,
	SDvalidsense	= 0x00010000,
};

enum {
	SDretry		= -5,		/* internal to controllers */
	SDmalloc	= -4,
	SDeio		= -3,
	SDtimeout	= -2,
	SDnostatus	= -1,

	SDok		= 0,

	SDcheck		= 0x02,		/* check condition */
	SDbusy		= 0x08,		/* busy */

	SDmaxio		= 2048*1024,
	SDnpart		= 16,
};

/* sdscsi.c */
int	scsiverify(SDunit*);
int	scsionline(SDunit*);
long	scsibio(SDunit*, int, int, void*, long, Off);
SDev*	scsiid(SDev*, SDifc*);

/* devsd.c */
int	sdfakescsi(SDreq *r, void *info, int ilen);
int	sdmodesense(SDreq *r, uchar *cmd, void *info, int ilen);
long	sdbio(SDunit *unit, SDpart *pp, void *a, long len, Off off);
void	partition(SDunit*);
void	addpartconf(SDunit*);
SDpart* sdfindpart(SDunit*, char*);
void	sdaddpart(SDunit*, char*, Devsize, Devsize);
void*	sdmalloc(void*, ulong);

enum {
	IrqATA0 = 14,
	IrqATA1,
};
