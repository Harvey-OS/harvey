/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#define	RAD(x)	((x)*PI_180)
#define	DEG(x)	((x)/PI_180)
#define ARCSECONDS_PER_RADIAN	(DEG(1)*3600)
#define input_nybble(infile)    input_nbits(infile,4)

typedef float	Angle;	/* in radians */

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
typedef	long	Type;

typedef struct	Image	Image;
struct	Image
{
	int	nx;
	int	ny;	/* ny is the fast-varying dimension */
	Type	a[1];
};

int	nplate;
Plate	plate[2000];		/* needs to go to 2000 when the north comes */
double	PI_180;
double	TWOPI;
int	debug;
struct
{
	float	min;
	float	max;
	float	del;
	double	gamma;
	int	neg;
} gam;

char*	hms(Angle);
char*	dms(Angle);
double	xsqrt(double);
Angle	dist(Angle, Angle, Angle, Angle);
Header*	getheader(char*);
char*	getword(char*, char*);
void	amdinv(Header*, Angle, Angle, float, float);
void	ppoinv(Header*, Angle, Angle);
void	xypos(Header*, Angle, Angle, float, float);
void	traneqstd(Header*, Angle, Angle);
Angle	getra(char*);
Angle	getdec(char*);
void	getplates(void);

Image*	dssread(char*);
void	hinv(Type*, int, int);
int	input_bit(Biobuf*);
int	input_nbits(Biobuf*, int);
void	qtree_decode(Biobuf*, Type*, int, int, int, int);
void	start_inputing_bits(void);
Bitmap*	image(Angle, Angle, Angle, Angle);
int	dogamma(int);
