#define DIR	"/lib/sky"
/*
 *	This code reflects many years of changes.  There remain residues
 *		of prior implementations.
 *
 *	Keys:
 *		32 bits long. High 26 bits are encoded as described below.
 *		Low 6 bits are types:
 *
 *		Patch is ~ one square degree of sky.  It points to an otherwise
 *			anonymous list of Catalog keys.  The 0th key is special:
 *			it contains up to 4 constellation identifiers.
 *		Catalogs (SAO,NGC,M,...) are:
 *			31.........8|76|543210
 *			  catalog # |BB|catalog name
 *			BB is two bits of brightness:
 *				00	-inf <  m <=  7
 *				01	   7 <  m <= 10
 *				10	  10 <  m <= 13
 *				11	  13 <  m <  inf
 *			The BB field is a dreg, and correct only for SAO and NGC.
 *			IC(n) is just NGC(n+7840)
 *		Others should be self-explanatory.
 *	
 *	Records:
 *
 *	Star is an SAOrec
 *	Galaxy, PlanetaryN, OpenCl, GlobularCl, DiffuseN, etc., are NGCrecs.
 *	Abell is an Abellrec
 *	The Namedxxx records hold a name and a catalog entry; they result from
 *		name lookups.
 */

typedef enum
{
	List_deprecated,
	Patch,
	SAO,
	NGC,
	M,
	Constel_deprecated,
	Nonstar_deprecated,
	NamedSAO,
	NamedNGC,
	NamedAbell,
	Abell,
	/* NGC types */
	Galaxy,
	PlanetaryN,
	OpenCl,
	GlobularCl,
	DiffuseN,
	NebularCl,
	Asterism,
	Knot,
	Triple,
	Double,
	Single,
	Uncertain,
	Nonexistent,
	Unknown,
	PlateDefect,
	/* internal */
	NGCN,
	PatchC,
}Type;

enum
{
	/*
	 * parameters for plate
	 */
	Pppo1	= 0,
	Pppo2,
	Pppo3,
	Pppo4,
	Pppo5,
	Pppo6,
	Pamdx1,
	Pamdx2,
	Pamdx3,
	Pamdx4,
	Pamdx5,
	Pamdx6,
	Pamdx7,
	Pamdx8,
	Pamdx9,
	Pamdx10,
	Pamdx11,
	Pamdx12,
	Pamdx13,
	Pamdx14,
	Pamdx15,
	Pamdx16,
	Pamdx17,
	Pamdx18,
	Pamdx19,
	Pamdx20,
	Pamdy1,
	Pamdy2,
	Pamdy3,
	Pamdy4,
	Pamdy5,
	Pamdy6,
	Pamdy7,
	Pamdy8,
	Pamdy9,
	Pamdy10,
	Pamdy11,
	Pamdy12,
	Pamdy13,
	Pamdy14,
	Pamdy15,
	Pamdy16,
	Pamdy17,
	Pamdy18,
	Pamdy19,
	Pamdy20,
	Ppltscale,
	Pxpixelsz,
	Pypixelsz,
	Ppltra,
	Ppltrah,
	Ppltram,
	Ppltras,
	Ppltdec,
	Ppltdecd,
	Ppltdecm,
	Ppltdecs,
	Pnparam,
};

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
	DAngle	dummy1;	/* compatibility with old RNGC version */
	DAngle	diam;
	Mag	mag;
	short	ngc;	/* if >NNGC, IC number is ngc-NNGC */
	char	diamlim;
	char	type;
	char	magtype;
	char	dummy2;
	char	desc[52];	/* 0-terminated Dreyer description */
};

typedef struct Abellrec Abellrec;
struct Abellrec{
	DAngle	ra;
	DAngle	dec;
	DAngle	glat;
	DAngle	glong;
	Mag	mag10;	/* mag of 10th brightest cluster member; in same place as ngc.mag*/
	short	abell;
	DAngle	rad;
	short	pop;
	short	dist;
	char	distgrp;
	char	richgrp;
	char	flag;
	char	pad;
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

typedef struct Mindexrec Mindexrec;
struct Mindexrec{	/* code knows the bit patterns in here; this is a long */
	char	m;		/* M number */
	char	dummy;
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

typedef struct Namedrec Namedrec;
struct Namedrec{
	char	name[36];
};

typedef struct Namerec Namerec;
struct Namerec{
	long	sao;
	long	ngc;
	long	abell;
	char	name[36];	/* null terminated */
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
		Abellrec	abell;
		Namedrec	named;
		Patchrec	patch;
		/* PatchCrec is empty */
	};
};

typedef struct Name Name;
struct Name{
	char	*name;
	int	type;
};

typedef	struct	Plate	Plate;
struct	Plate
{
	char	rgn[7];
	char	disk;
	Angle	ra;
	Angle	dec;
};

typedef	struct	Header	Header;
struct	Header
{
	float	param[Pnparam];
	int	amdflag;

	float	x;
	float	y;
	float	xi;
	float	eta;
};
typedef	long	Pix;

typedef struct	Image	Image;
struct	Image
{
	int	nx;
	int	ny;	/* ny is the fast-varying dimension */
	Pix	a[1];
};

#define	RAD(x)	((x)*PI_180)
#define	DEG(x)	((x)/PI_180)
#define	ARCSECONDS_PER_RADIAN	(DEG(1)*3600)
#define	NULL	0

int	nplate;
Plate	plate[2000];		/* needs to go to 2000 when the north comes */
double	PI_180;
double	TWOPI;
double	LN2;
int	debug;
struct
{
	float	min;
	float	max;
	float	gamma;
	float	absgamma;
	float	mult1;
	float	mult2;
	int	neg;
} gam;

typedef struct Picture Picture;
struct Picture
{
	int	minx;
	int	miny;
	int	maxx;
	int	maxy;
	char	name[16];
	uchar	*data;
};


extern	double	PI_180;
extern	double	TWOPI;
extern	char	*progname;
extern	char	*desctab[][2];
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
extern char*ms(Angle);
extern char*hm(Angle);
extern char*dm(Angle);
extern long dangle(Angle);
extern float angle(int);
extern void prdesc(char*, char*(*)[2], short*);
extern double	xsqrt(double);
extern Angle	dist(Angle, Angle, Angle, Angle);
extern Header*	getheader(char*);
extern char*	getword(char*, char*);
extern void	amdinv(Header*, Angle, Angle, float, float);
extern void	ppoinv(Header*, Angle, Angle);
extern void	xypos(Header*, Angle, Angle, float, float);
extern void	traneqstd(Header*, Angle, Angle);
extern Angle	getra(char*);
extern Angle	getdec(char*);
extern void	getplates(void);
extern Image*	dssread(char*);
extern void	hinv(Pix*, int, int);
extern int	input_bit(Biobuf*);
extern int	input_nbits(Biobuf*, int);
extern int	input_huffman(Biobuf*);
extern	int	input_nybble(Biobuf*);
extern void	qtree_decode(Biobuf*, Pix*, int, int, int, int);
extern void	start_inputing_bits(void);
extern Picture*	image(Angle, Angle, Angle, Angle);
extern int	dogamma(Pix);
extern void display(Picture*);

#define	NINDEX	400
