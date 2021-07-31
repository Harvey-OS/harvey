#include <u.h>
#include <libc.h>
#include <stdio.h>
#ifdef PLOT
#include PLOT
#else
#include "iplot.h"
#endif

#ifndef PI
#define PI	3.1415926535897932384626433832795028841971693993751
#endif

#define TWOPI (2*PI)
#define RAD (PI/180)
double	hypot(double, double);	/* sqrt(a*a+b*b) */
double	tan(double);		/* not in K&R library */

#define ECC .08227185422	/* eccentricity of earth */
#define EC2 .006768657997

#define FUZZ .0001
#define UNUSED 0.0		/* a dummy float parameter */

struct coord {
	float l;	/* lat or lon in radians*/
	float s;	/* sin */
	float c;	/* cos */
};
struct place {
	struct coord nlat;
	struct coord wlon;
};

typedef int (*proj)(struct place *, float *, float *);

struct index {		/* index of known projections */
	char *name;	/* name of projection */
	proj (*prog)(float, float);
			/* pointer to projection function */
	int npar;	/* number of params */
	int (*cut)(struct place *, struct place *, float *);
			/* function that handles cuts--eg longitude 180 */
	int poles;	/*1 S pole is a line, 2 N pole is, 3 both*/
	int spheroid;	/* poles must be at 90 deg if nonzero */
};


proj	aitoff(void);
proj	albers(float, float);
int	Xazequalarea(struct place *, float *, float *);
proj	azequalarea(void);
int	Xazequidistant(struct place *, float *, float *);
proj	azequidistant(void);
proj	bicentric(float);
proj	bonne(float);
proj	conic(float);
proj	cylequalarea(float);
int	Xcylindrical(struct place *, float *, float *);
proj	cylindrical(void);
proj	elliptic(float);
proj	fisheye(float);
proj	gall(float);
proj	globular(void);
proj	gnomonic(void);
int	guycut(struct place *, struct place *, float *);
int	Xguyou(struct place *, float *, float *);
proj	guyou(void);
proj	harrison(float, float);
int	hexcut(struct place *, struct place *, float *);
proj	hex(void);
proj	homing(float);
proj	lagrange(void);
proj	lambert(float, float);
proj	laue(void);
proj	loxodromic(float);	/* not in library */
proj	mecca(float);
proj	mercator(void);
proj	mollweide(void);
proj	newyorker(float);
proj	ortelius(float, float);	/* not in library */
int	Xorthographic(struct place *place, float *x, float *y);
proj	orthographic(void);
proj	perspective(float);
int	Xpolyconic(struct place *, float *, float *);
proj	polyconic(void);
proj	rectangular(float);
proj	simpleconic(float, float);
int	Xsinusoidal(struct place *, float *, float *);
proj	sinusoidal(void);
proj	sp_albers(float, float);
proj	sp_mercator(void);
proj	square(void);
int	Xstereographic(struct place *, float *, float *);
proj	stereographic(void);
int	Xtetra(struct place *, float *, float *);
int	tetracut(struct place *, struct place *, float *);
proj	tetra(void);
proj	trapezoidal(float, float);
proj	vandergrinten(void);
proj	wreath(float, float);	/* not in library */

void	findxy(double, double *, double *);
void	albscale(float, float, float, float);
void	invalb(float, float, float *, float *);

void	cdiv(double, double, double, double, double *, double *);
void	cmul(double, double, double, double, double *, double *);
void	csq(double, double, double *, double *);
void	csqrt(double, double, double *, double *);
void	ccubrt(double, double, double *, double *);
double	cubrt(double);
int	elco2(double, double, double, double, double, float *, float *);
void	cdiv2(double, double, double, double, double *, double *);
void	csqr(double, double, double *, double *);

void	orient(float, float, float);
void	latlon(float, float, struct place *);
void	deg2rad(float, struct coord *);
void	sincos(struct coord *);
void	normalize(struct place *);
void	invert(struct place *);
void	norm(struct place *, struct place *, struct coord *);
void	printp(struct place *);
void	copyplace(struct place *, struct place *);

int	picut(struct place *, struct place *, float *);
int	ckcut(struct place *, struct place *, float);
float	reduce(float);

void	getsyms(char *);
int	putsym(struct place *, char *, double, int);
void	filerror(char *s, char *f);
void	error(char *s);
int	doproj(struct place *, int *, int *);
int	cpoint(int, int, int);
int	plotpt(struct place *, int);
int	nocut(struct place *, struct place *, float *);

extern int (*projection)(struct place *, float *, float *);
