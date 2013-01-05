#define	Ms2tk(t)		(((t)*HZ)/1000)

enum {
	Sas		= 1,
	Sata,

	Spd15		= 8,
	Spd30		= 9,
	Spd60		= 10,

	SDspinup	= SDretry - 1,
	SDrate		= SDretry - 2,
};

/*
 * the lock allows ataonline to exclude other commands
 * during the online process.  we could extend this to allow
 * for exclusive access for periods of time.
 */
typedef struct Sfisx Sfisx;
struct Sfisx{
	uchar	type;
	Sfis;
	Cfis;			/* sas and media info */
	uint	sasspd;		/* botch; move to fis.h */
	uchar	*oaf;

	RWlock;
	int	drivechange;
	char	serial[20+1];
	char	firmware[8+1];
	char	model[40+1];
	uvlong	wwn;
	uvlong	sectors;
	ulong	secsize;
	uint	tler;		/* time limit for error recovery */
	uint	atamaxxfr;
	uint	maxspd;
};

int	tur(SDunit *, int, uint*);

int	ataonline(SDunit*, Sfisx*);
//long	atabio(SDunit*, int, int, void*, long, uvlong);
long	atabio(SDunit*, Sfisx*, int, int, void*, long, uvlong);

int	scsionlinex(SDunit*, Sfisx*);
//long	scsibio(SDunit*, int, int, void*, long, uvlong);
long	scsibiox(SDunit*, Sfisx*, int, int, void*, long, uvlong);

//int	atariosata(SDunit*, SDreq*);
int	atariosata(SDunit*, Sfisx*, SDreq*);

char	*sfisxrdctl(Sfisx*, char*, char*);
void	pronline(SDunit*, Sfisx*);

ulong	totk(ulong);
int	setreqto(SDreq*, ulong);
ulong	gettotk(Sfisx*);

uvlong	getle(uchar*, int);
void	putle(uchar*, uvlong, int);
uvlong	getbe(uchar*, int);
void	putbe(uchar*, uvlong, int);

/* junk to make private later */
void	edelay(ulong, ulong);
