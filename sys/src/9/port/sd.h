/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * Storage Device.
 */
typedef struct SDev SDev;
typedef struct SDifc SDifc;
typedef struct SDio SDio;
typedef struct SDpart SDpart;
typedef struct SDperm SDperm;
typedef struct SDreq SDreq;
typedef struct SDunit SDunit;

struct SDperm {
	char*	name;
	char*	user;
	uint32_t	perm;
};

struct SDpart {
	uint64_t	start;
	uint64_t	end;
	SDperm;
	int	valid;
	uint32_t	vers;
};

struct SDunit {
	SDev*	dev;
	int	subno;
	unsigned char	inquiry[255];		/* format follows SCSI spec */
	unsigned char	sense[18];		/* format follows SCSI spec */
	SDperm;

	QLock	ctl;
	uint64_t	sectors;
	uint32_t	secsize;
	SDpart*	part;			/* nil or array of size npart */
	int	npart;
	uint32_t	vers;
	SDperm	ctlperm;

	QLock	raw;			/* raw read or write in progress */
	uint32_t	rawinuse;		/* really just a test-and-set */
	int	state;
	SDreq*	req;
	SDperm	rawperm;
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
	SDev*	(*legacy)(int, int);
	int	(*enable)(SDev*);
	int	(*disable)(SDev*);

	int	(*verify)(SDunit*);
	int	(*online)(SDunit*);
	int	(*rio)(SDreq*);
	int	(*rctl)(SDunit*, char*, int);
	int	(*wctl)(SDunit*, Cmdbuf*);

	int32_t	(*bio)(SDunit*, int, int, void*, int32_t, uint64_t);
	SDev*	(*probe)(DevConf*);
	void	(*clear)(SDev*);
	char*	(*rtopctl)(SDev*, char*, char*);
	int	(*wtopctl)(SDev*, Cmdbuf*);
};

struct SDreq {
	SDunit*	unit;
	int	lun;
	int	write;
	unsigned char	cmd[16];
	int	clen;
	void*	data;
	int	dlen;

	int	flags;

	int	status;
	int32_t	rlen;
	unsigned char	sense[256];
};

enum {
	SDnosense	= 0x00000001,
	SDvalidsense	= 0x00010000,

	SDinq0periphqual= 0xe0,
	SDinq0periphtype= 0x1f,
	SDinq1removable	= 0x80,

	/* periphtype values */
	SDperdisk	= 0,	/* Direct access (disk) */
	SDpertape	= 1,	/* Sequential eg, tape */
	SDperpr		= 2,	/* Printer */
	SDperworm	= 4,	/* Worm */
	SDpercd		= 5,	/* CD-ROM */
	SDpermo		= 7,	/* rewriteable MO */
	SDperjuke	= 8,	/* medium-changer */
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

/*
 * Allow the default #defines for sdmalloc & sdfree to be overridden by
 * system-specific versions.  This can be used to avoid extra copying
 * by making sure sd buffers are cache-aligned (some ARM systems) or
 * page-aligned (xen) for DMA.
 */
#ifndef sdmalloc
#define sdmalloc(n)	malloc(n)
#define sdfree(p)	free(p)
#endif

/*
 * mmc/sd/sdio host controller interface
 */

struct SDio {
	char	*name;
	int	(*init)(void);
	void	(*enable)(void);
	int	(*inquiry)(char*, int);
	int	(*cmd)(uint32_t, uint32_t, uint32_t*);
	void	(*iosetup)(int, void*, int, int);
	void	(*io)(int, unsigned char*, int);
};

extern SDio sdio;

/* devsd.c */
extern void sdadddevs(SDev*);
extern void sdaddconf(SDunit*);
extern void sdaddallconfs(void (*f)(SDunit*));
extern void sdaddpart(SDunit*, char*, uint64_t, uint64_t);
extern int sdsetsense(SDreq*, int, int, int, int);
extern int sdmodesense(SDreq*, unsigned char*, void*, int);
extern int sdfakescsi(SDreq*, void*, int);

/* sdscsi.c */
extern int scsiverify(SDunit*);
extern int scsionline(SDunit*);
extern int32_t scsibio(SDunit*, int, int, void*, int32_t, uint64_t);
extern SDev* scsiid(SDev*, SDifc*);

/*
 *  hardware info about a device
 */
typedef struct {
	uint32_t	port;
	int	size;
} Devport;

struct DevConf
{
	uint32_t	intnum;			/* interrupt number */
	char	*type;			/* card type, malloced */
	int	nports;			/* Number of ports */
	Devport	*ports;			/* The ports themselves */
};
