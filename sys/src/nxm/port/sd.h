/*
 * Storage Device.
 */
typedef struct SDev SDev;
typedef struct SDfile SDfile;
typedef struct SDifc SDifc;
typedef struct SDpart SDpart;
typedef struct SDperm SDperm;
typedef struct SDreq SDreq;
typedef struct SDunit SDunit;
typedef struct Devport Devport;
typedef struct Devconf Devconf;

struct SDperm {
	char*	name;
	char*	user;
	ulong	perm;
};

struct SDpart {
	uvlong	start;
	uvlong	end;
	SDperm;
	int	valid;
	ulong	vers;
};

typedef long SDrw(SDunit*, Chan*, void*, long, vlong);
struct SDfile {
	SDperm;
	SDrw	*r;
	SDrw	*w;
};

struct SDunit {
	SDev*	dev;
	int	subno;
	uchar	inquiry[255];		/* format follows SCSI spec */
	uchar	sense[18];		/* format follows SCSI spec */
	uchar	rsense[18];		/* support seperate rq sense and inline return */
	uchar	haversense;
	SDperm;

	QLock	ctl;
	uvlong	sectors;
	ulong	secsize;
	SDpart*	part;			/* nil or array of size npart */
	int	npart;
	ulong	vers;
	SDperm	ctlperm;

	QLock	raw;			/* raw read or write in progress */
	ulong	rawinuse;		/* really just a test-and-set */
	int	state;
	SDreq*	req;
	SDperm	rawperm;
	SDfile	efile[5];
	int	nefile;
};

/*
 * Each controller is represented by a SDev.
 */
struct SDev {
	Ref	r;			/* Number of callers using device */
	SDifc*	ifc;			/* pnp/legacy */
	void*	ctlr;
	int	idno;
	char	name[8];
	SDev*	next;

	QLock;				/* enable/disable */
	int	enabled;
	int	nunit;			/* Number of units */
	QLock	unitlock;		/* `Loading' of units */
	int*	unitflg;		/* Unit flags */
	SDunit**unit;
};

struct SDifc {
	char*	name;

	SDev*	(*pnp)(void);
	SDev*	(*xxlegacy)(int, int);		/* unused.  remove me */
	int	(*enable)(SDev*);
	int	(*disable)(SDev*);

	int	(*verify)(SDunit*);
	int	(*online)(SDunit*);
	int	(*rio)(SDreq*);
	int	(*rctl)(SDunit*, char*, int);
	int	(*wctl)(SDunit*, Cmdbuf*);

	long	(*bio)(SDunit*, int, int, void*, long, uvlong);
	SDev*	(*probe)(DevConf*);
	void	(*clear)(SDev*);
	char*	(*rtopctl)(SDev*, char*, char*);
	int	(*wtopctl)(SDev*, Cmdbuf*);
	int	(*ataio)(SDreq*);
};

struct SDreq {
	SDunit*	unit;
	int	lun;
	char	write;
	char	proto;
	char	ataproto;
	uchar	cmd[0x20];
	int	clen;
	void*	data;
	int	dlen;

	int	flags;
	ulong	timeout;		/* in ticks */

	int	status;
	long	rlen;
	uchar	sense[32];
};

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

	SDread	= 0,
	SDwrite,

	SData		= 1,
	SDcdb		= 2,
};

#define sdmalloc(n)	malloc(n)
#define sdfree(p)	free(p)

/* devsd.c */
extern void sdadddevs(SDev*);
extern int sdsetsense(SDreq*, int, int, int, int);
extern int sdfakescsi(SDreq*);
extern int sdfakescsirw(SDreq*, uvlong*, int*, int*);
extern int sdaddfile(SDunit*, char*, int, char*, SDrw*, SDrw*);

/* sdscsi.c */
extern int scsiverify(SDunit*);
extern int scsionline(SDunit*);
extern long scsibio(SDunit*, int, int, void*, long, uvlong);


/*
 *  hardware info about a device
 */
struct Devport {
	ulong	port;	
	int	size;
};

struct DevConf {
	ulong	intnum;			/* interrupt number */
	char	*type;			/* card type, malloced */
	int	nports;			/* Number of ports */
	Devport	*ports;			/* The ports themselves */
};
