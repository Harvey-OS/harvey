/*
 * Storage Device.
 */
typedef struct SDev SDev;
typedef struct SDifc SDifc;
typedef struct SDpart SDpart;
typedef struct SDreq SDreq;
typedef struct SDunit SDunit;

typedef struct SDpart {
	ulong	start;
	ulong	end;
	char	name[NAMELEN];
	char	user[NAMELEN];
	ulong	perm;
	int	valid;
	int	nopen;			/* of this partition */
} SDpart;

typedef struct SDunit {
	SDev*	dev;
	int	subno;
	uchar	inquiry[256];		/* format follows SCSI spec */
	char	name[NAMELEN];
	Rendez	rendez;

	QLock	ctl;
	ulong	sectors;
	ulong	secsize;
	SDpart*	part;
	int	npart;			/* of valid partitions */
	int	nopen;			/* of partitions on this unit */
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
	char	name[NAMELEN];
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
	int	(*wctl)(SDunit*, Cmdbuf*);

	long	(*bio)(SDunit*, int, int, void*, long, long);
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

#define sdmalloc(n)	malloc(n)
#define sdfree(p)	free(p)

/* sdscsi.c */
extern int scsiverify(SDunit*);
extern int scsionline(SDunit*);
extern long scsibio(SDunit*, int, int, void*, long, long);
extern SDev* scsiid(SDev*, SDifc*);
