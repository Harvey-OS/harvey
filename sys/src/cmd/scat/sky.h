#define DIR	"/lib/sky"
/*
 *	Keys:
 *		32 bits long. High 26 bits are encoded as implied below.
 *		Low 6 bits are types:
 *
 *		Patch is ~ one square degree of sky.  It points to an otherwise
 *			anonymous List of Catalog keys.
 *		Catalogs (SAO,NGC,M,...) are:
 *			31.........8|76|543210
 *			  catalog # |BB|catalog name
 *			BB is two bits of brightness:
 *				00	-inf <  m <=  7
 *				01	   7 <  m <= 10
 *				10	  10 <  m <= 13
 *				11	  13 <  m <  inf
 *			May point directly to object, or a list of Catalog keys.
 *			Note that if all you have is the number, you must
 *			do up to 4 accesses to find the object, but almost
 *			all accesses are from records that already exist.
 *		Constel yields two lists: a list of constellation boundaries
 *			and a list of patches.
 *		Nonstar yields four lists, one per BB, from bright to dim.
 *			The rest of the key is one of the NGC types, but
 *			if an object is contained in something
 *			(e.g. 65 is nebular cluster in external galaxy, 28 is
 *			globular cluster in LMC), it will be filed twice, once
 *			under its simple type (NebularCl) and once under the
 *			contained type (Galaxy<<8|NebularCl).
 *	
 *	Records:
 *
 *	List is a list of up to NLIST-1 keys; the NLIST-1 key is a continuation.
 *		The continuation is keyed as List|(unique id)
 *		Fetch key = List|0 to get next unique id.
 *	Star is an SAOrec
 *	Galaxy, PlanetaryN, OpenCl, GlobularCl, DiffuseN, NebularCl,
 *		Nonexistent and Unknown are NGCrecs.
 */

typedef enum
{
	List,		/* Key==0 ==> empty list */
	Patch,
	SAO,
	NGC,
	M,
	Constel,
	Nonstar,
	Star,
	Galaxy,
	PlanetaryN,
	OpenCl,
	GlobularCl,
	DiffuseN,
	NebularCl,
	Nonexistent,
	Unknown,
	LMC,
	SMC,
	/* internal */
	NGCN,
	PatchC,
}Type;
#define	UNKNOWNMAG	32767

typedef float	Angle;	/* in radians */
typedef	long	DAngle;	/* on disk: in units of milliarcsec */
typedef short	Mag;	/* multiplied by 10 */
typedef	long	Key;	/* known to be 4 bytes, unfortunately */

/*
 * All integers are stored in little-endian order.
 */
typedef struct NGCrec NGCrec;
struct NGCrec{
	DAngle	ra;
	DAngle	dec;
	DAngle	glat;
	DAngle	glong;
	Mag	mag;
	short	ngc;
	char	tag;
	char	type;
	char	code;
	char	desc[81];	/* three 0-terminated strings:
				 *  - Dreyer description
				 *  - Palomar description
				 *  - other catalogs
				 */
};

/*
 * Star names: 0,0==unused.  Numbers are name[0]=1,..,99.
 * Greek letters are alpha=101, etc.
 * Constellations are alphabetical order by abbreviation, and=1, etc.
 */
typedef struct SAOrec SAOrec;
struct SAOrec{
	DAngle	ra;
	DAngle	dec;
	DAngle	dra;
	DAngle	ddec;
	Mag	mag;		/* visual */
	Mag	mpg;
	char	spec[3];
	char	code;
	char	compid[2];
	char	hdcode;
	char	pad1;
	long	hd;		/* HD catalog number */
	char	name[3];	/* name[0]=alpha name[1]=2 name[3]=ori */
	char	nname;		/* number of prose names */
	/* 36 bytes to here */
};

typedef struct NGCindexrec NGCindexrec;
struct NGCindexrec{	/* code knows the bit patterns in here; this is a long */
	char	m;		/* for m list, is M number; for NGC is type, b */
	char	tag;
	short	ngc;
};
typedef struct Bayerec Bayerec;
struct Bayerec{
	long	sao;
	char	name[3];
	char	pad;
};

/*
 * Internal form
 */

typedef struct Starrec Starrec;
struct Starrec{
	char	name[21];	/* longest is "ras elased australis" */
};

typedef struct Namerec Namerec;
struct Namerec{
	long	sao;
	char	name[24];	/* null terminated */
};

typedef struct Patchrec Patchrec;
struct Patchrec{
	int	nkey;
	long	key[60];
};

typedef struct Record Record;
struct Record{
	Type	type;
	long	index;
	union{
		SAOrec	sao;
		NGCrec	ngc;
		int	tag;	/* NGCNrec */
		Starrec	star;
		Patchrec	patch;
		/* PatchCrec is empty */
	};
};

typedef struct Name Name;
struct Name{
	char	*name;
	int	type;
};

#define	RAD(x)	((x)*PI_180)
#define	DEG(x)	((x)/PI_180)

extern	double	PI_180;
extern	double	TWOPI;
extern	char	*progname;
extern	char	*odesctab[][2];
extern	char	*ndesctab[][2];
extern	Name	names[];

extern void saoopen(void);
extern void ngcopen(void);
extern void patchopen(void);
extern void mopen(void);
extern void constelopen(void);
extern void lowercase(char*);
extern void lookup(char*, int);
extern int typetab(int);
extern char*ngcstring(int);
extern char*skip(int, char*);
extern void prrec(Record*);
extern int equal(char*, char*);
extern int parsename(char*);
extern void radec(int, int*, int*, int*);
extern int btag(short);
extern long patcha(Angle, Angle);
extern long patch(int, int, int);
extern char*hms(Angle);
extern char*dms(Angle);
extern char*hm(Angle);
extern char*dm(Angle);
extern long dangle(Angle);
extern float angle(int);
extern void prdesc(char*, char*(*)[2], short*);

#define	NINDEX	300
