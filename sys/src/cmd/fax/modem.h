typedef struct {
	char	*t;
	int	fd;
	int	cfd;
	char	*id;
	char	response[128];
	char	error[128];

	int	fax;
	char	phase;
	char	ftsi[128];		/* remote ID */
	long	fdcs[8];		/* frame information */
	long	fpts[8];		/* page reception response */
	long	fet;			/* post page message */
	long	fhng;			/* call termination status */
	int	pageno;			/* current page number */
	char	pageid[128];		/* current page file */
	int	pagefd;			/* current page fd */
	int	valid;			/* valid page responses */
	long	time;			/* timestamp */
	int	pid;

	char	ibuf[1024];		/* modem input buffering */
	char	*iptr;
	long	icount;

	Biobuf	*bp;			/* file input buffering */

	/* FDCS parameters */
	long	wd;			/* width */
	long	vr;			/* resolution */
	long	ln;			/* page size (length) */
	long	df;			/* huffman encoding */
} Modem;

enum {					/* ResultCodes */
	Rok		= 0,
	Rconnect,
	Rring,
	Rfailure,
	Rrerror,
	Rcontinue,
	Rhangup,
	Rnoise,
};

enum {					/* ErrorCodes */
	Eok	= 0,	/* no error */
	Eattn,		/* can't get modem's attention */
	Enoresponse,	/* no response from modem */
	Enoanswer,	/* no answer from other side */
	Enofax,		/* other side isn't a fax machine */
	Eincompatible,	/* transmission incompatible with receiver */
	Esys,		/* system call error */
	Eproto,		/* fax protocol botch */
};

enum {					/* things that are valid */
	Vfdcs		= 0x0001,	/* page responses */
	Vftsi		= 0x0002,
	Vfpts		= 0x0004,
	Vfet		= 0x0008,
	Vfhng		= 0x0010,

	Vwd		= 0x4000,
	Vtype		= 0x8000,
};

/* fax2modem.c */
extern int initfaxmodem(Modem*);
extern int fcon(Modem*);
extern int ftsi(Modem*);
extern int fdcs(Modem*);
extern int fcfr(Modem*);
extern int fpts(Modem*);
extern int fet(Modem*);
extern int fhng(Modem*);

/* fax2receive.c */
extern int faxreceive(Modem*, char*);

/* fax2send.c */
extern int faxsend(Modem*, int, char*[]);

/* modem.c */
extern int setflow(Modem*, int);
extern int setspeed(Modem*, int);
extern int rawmchar(Modem*, char*);
extern int getmchar(Modem*, char*, long);
extern int putmchar(Modem*, char*);
extern int command(Modem*, char*);
extern int response(Modem*, int);
extern void initmodem(Modem*, int, int, char*, char*);
extern void xonoff(Modem*, int);

/* spool.c */
extern void setpageid(char*, char*, long, int, int);
extern int createfaxfile(Modem*, char*);
extern int openfaxfile(Modem*, char*);

/* subr.c */
extern void verbose(char*, ...);
extern void error(char*, ...);
extern int seterror(Modem*, int);
extern void faxrlog(Modem*, int);
extern void faxxlog(Modem*, int);
extern int vflag;
