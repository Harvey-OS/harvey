/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>


typedef	struct	Obj1	Obj1;
typedef	struct	Obj2	Obj2;
typedef	struct	Obj3	Obj3;
typedef	struct	Occ	Occ;
typedef	struct	Event	Event;
typedef	struct	Tim	Tim;
typedef	struct	Moontab	Moontab;

#define	NPTS	12
#define	PER	1.0

enum
{
	DARK	= 1<<0,
	SIGNIF	= 1<<1,
	PTIME	= 1<<2,
	LIGHT	= 1<<3,
};

struct	Obj1
{
	double	ra;
	double	decl2;
	double	semi2;
	double	az;
	double	el;
	double	mag;
};
struct	Obj2
{
	char*	name;
	char*	name1;
	void	(*obj)(void);
	Obj1	point[NPTS+2];
};
struct	Obj3
{
	double	t1;
	double	e1;
	double	t2;
	double	e2;
	double	t3;
	double	e3;
	double	t4;
	double	e4;
	double	t5;
	double	e5;
};
struct Event
{
	char*	format;
	char*	arg1;
	char*	arg2;
	double	tim;
	int	flag;
};
struct	Moontab
{
	double	f;
	char	c[4];
};
struct	Occ
{
	Obj1	act;
	Obj1	del0;
	Obj1	del1;
	Obj1	del2;
};
struct	Tim
{
	double	ifa[5];
	char	tz[4];
};

double	converge;

char	flags[128];
int	nperiods;
double	wlong, awlong, nlat, elev;
double	obliq, phi, eps, tobliq;
double	dphi, deps;
double	day, deld, per;
double	eday, capt, capt2, capt3, gst;
double	pi, pipi, radian, radsec, deltat;
double	erad, glat;
double	xms, yms, zms;
double	xdot, ydot, zdot;

double	ecc, incl, node, argp, mrad, anom, motion;

double	lambda, beta, rad, mag, semi;
double	alpha, delta, rp, hp;
double	ra, decl, semi2;
double	lha, decl2, lmb2;
double	az, el;

double	meday, seday, mhp, salph, sdelt, srad;

double*	cafp;
char*	cacp;

double	rah, ram, ras, dday, dmin, dsec;
int32_t	sao;
double	da, dd, px, epoch;
char	line[100];
Obj2	osun;
Obj2	omoon;
Obj2	oshad;
Obj2	omerc;
Obj2	ovenus;
Obj2	omars;
Obj2	osat;
Obj2	ouran;
Obj2	onept;
Obj2	oplut;
Obj2	ojup;
Obj2	ostar;
Obj2	ocomet;
Obj3	occ;
Obj2*	eobj1;
Obj2*	eobj2;

char*	startab;

extern	int	dmo[];
extern	Obj2*	objlst[];

extern	double	venfp[];
extern	char	vencp[];
extern	double	sunfp[];
extern	char	suncp[];
extern	double	mercfp[];
extern	char	merccp[];
extern	double	nutfp[];
extern	char	nutcp[];
extern	Moontab moontab[];

extern	void	args(int, char**);
extern	void	bdtsetup(double, Tim*);
extern	double	betcross(double);
extern	double	convdate(Tim*);
extern	double	cosadd(int, double, ...);
extern	double	cosx(double, int, int, int, int, double);
extern	double	dist(Obj1*, Obj1*);
extern	double	dsrc(double, Tim*, int);
extern	void	dtsetup(double, Tim*);
extern	int	evcomp(const void*, const void*);
extern	void	event(char*, char*, char*, double, int);
extern	void	evflush(void);
extern	double	fmod(double, double);
extern	void	fstar(void);
extern	void	fsun(void);
extern	void	geo(void);
extern	void	helio(void);
extern	void	icosadd(double*, char*);
extern	void	init(void);
extern	void	jup(void);
extern	int	lastsun(Tim*, int);
extern	void	main(int, char**);
extern	void	mars(void);
extern	double	melong(Obj2*);
extern	void	merc(void);
extern	void	moon(void);
extern	void	numb(int);
extern	void	nutate(void);
extern	void	occult(Obj2*, Obj2*, double);
extern	void	output(char*, Obj1*);
extern	void	pdate(double);
extern	double	pinorm(double);
extern	void	ptime(double);
extern	void	pstime(double);
extern	double	pyth(double);
extern	double	readate(void);
extern	double	readdt(void);
extern	void	readlat(int);
extern	double	rise(Obj2*, double);
extern	int	rline(int);
extern	void	sat(void);
extern	void	uran(void);
extern	void	nept(void);
extern	void	plut(void);
extern	void	satel(double);
extern	void	satels(void);
extern	void	search(void);
extern	double	set(Obj2*, double);
extern	void	set3pt(Obj2*, int, Occ*);
extern	void	setime(double);
extern	void	setobj(Obj1*);
extern	void	setpt(Occ*, double);
extern	void	shad(void);
extern	double	sinadd(int, double, ...);
extern	double	sinx(double, int, int, int, int, double);
extern	char*	skip(int);
extern	double	solstice(int);
extern	void	star(void);
extern	void	stars(void);
extern	void	sun(void);
extern	double	sunel(double);
extern	void	venus(void);
extern	int	vis(double, double, double, double);
extern	void	comet(void);
extern	int	Rconv(Fmt*);
extern	int	Dconv(Fmt*);
extern	double	etdate(int32_t, int, double);
