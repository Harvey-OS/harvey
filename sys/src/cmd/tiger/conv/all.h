#include	<u.h>
#include	<libc.h>
#include	<bio.h>

/*
 *	output format:
 *
 *  0 , county entry
 *	y county, state
 *
 *	1 , line entry
 *	    c cfcc code
 *	    n feature name *
 *	    r feature type *
 *	    p feature prefix *
 *	    s feature suffix *
 *
 *	    2 , left area entry
 *		a from address *
 *		b to address *
 *		z zip *
 *		m city
 *		c feature type
 *		n feature name
 *		s feature suffix
 *
 *	    3 , right area entry
 *		a from address
 *		b to address
 *		z zip
 *		m city
 *		c feature type
 *		n feature name
 *		s feature suffix
 *
 *	    l 32-bit latitude
 *		d 15-bit signed relative latitude
 *	    g 32-bit longitude
 *		d 15-bit signed relative longitude
 *
 *	4 , point entry
 *	    c cfcc code
 *	    n feature name
 *	    s feature suffix
 *	    t 32-bit latitude
 *	    g 32-bit longitude
 *************************
 *
 *	0 county entry
 *	1 line entry, 0
 *	2 left area entry, 0
 *	3 right area entry, 0
 *	4 point entry, 0
 *	a address from, 23
 *	b address to, 23
 *	c cfcc code, 1234
 *	d delta lat/lng, lg
 *	g longitude, 14
 *	l place, 01234
 *	m mcd/ccd, 01234
 *	n feature name, 1234
 *	p feature prefix, n
 *	r feature type, n
 *	s feature suffix, n
 *	t latitude, 14
 *	y county state, 0
 *	z zip, 23
 */

#define	nelem(x)	(sizeof(x)/sizeof(x)[0])

typedef	struct	Sym	Sym;
typedef	struct	Addl	Addl;

typedef	struct	Rec1	Rec1;
typedef	struct	Rec2	Rec2;
typedef	struct	Rec3	Rec3;
typedef	struct	Rec4	Rec4;
typedef	struct	Rec5	Rec5;
typedef	struct	Rec6	Rec6;
typedef	struct	Rec7	Rec7;
typedef	struct	Rec8	Rec8;
typedef	struct	Reca	Reca;
typedef	struct	Reci	Reci;
typedef	struct	Recp	Recp;
typedef	struct	Recr	Recr;
typedef	struct	Recx	Recx;
typedef	struct	Side	Side;

struct	Sym
{
	Sym*	link;
	char*	name;
	long	count;
};

struct	Addl
{
	Addl*	link;
	void*	rec;
};

struct	Rec1
{
	long	tlid;
	long	frlng;
	long	frlat;
	long	tolng;
	long	tolat;
	Sym*	version;
	Sym*	side1;
	Sym*	source;
	Sym*	fedirp;
	Sym*	fename;
	Sym*	fetype;
	Sym*	fedirs;
	Sym*	cfcc;
	Sym*	fraddl;
	Sym*	toaddl;
	Sym*	fraddr;
	Sym*	toaddr;
	Sym*	friaddfl;
	Sym*	toiaddfl;
	Sym*	friaddfr;
	Sym*	toiaddfr;
	long	zipl;
	long	zipr;
	Sym*	airl;
	Sym*	airr;
	Sym*	anrcl;
	Sym*	anrcr;
	long	statel;
	long	stater;
	long	countyl;
	long	countyr;
	long	fmcdl;
	long	fmcdr;
	long	fsmcdl;
	long	fsmcdr;
	long	fpll;
	long	fplr;
	Sym*	ctbnal;
	Sym*	ctbnar;
	Sym*	blkl;
	Sym*	blkr;

	Addl*	rec2;		/* addl lat/lng */
	Addl*	rec5;		/* addl names */
	Addl*	rec6;		/* addl addr/zip */
	Recp*	recpl;		/* polygon left */
	Recp*	recpr;		/* polygon right */
};
struct	Rec2
{
	long	tlid;
	long	rtsq;
	long	lng[10];
	long	lat[10];
	Sym*	rt;
	Sym*	version;
};
#ifdef REC3
struct	Rec3
{
	long	tlid;
	Sym*	version;
	long	statel;
	long	stater;
	long	counl;
	long	counr;
	long	fmcdl;
	long	fmcdr;
	long	fpll;
	long	fplr;
	Sym*	ctbnal;
	Sym*	ctbnar;
	Sym*	blkl;
	Sym*	blkr;
	Sym*	mcdl80;
	Sym*	mcdr80;
	Sym*	pll80;
	Sym*	plr80;
	Sym*	airl;
	Sym*	airr;
	Sym*	mcdl90;
	Sym*	mcdr90;
	Sym*	smcdl;
	Sym*	smcdr;
	Sym*	pll90;
	Sym*	plr90;
	Sym*	vtdl;
	Sym*	vtdr;
};
#endif
struct	Rec4
{
	long	tlid;
	long	rtsq;
	long	feat[5];
	Sym*	rt;
	Sym*	version;
};
struct	Rec5
{
	long	feat;
	long	state;
	long	county;
	Sym*	fedirp;
	Sym*	fename;
	Sym*	fetype;
	Sym*	fedirs;
};
struct	Rec6
{
	long	tlid;
	long	rtsq;
	Sym*	version;
	Sym*	fraddl;
	Sym*	toaddl;
	Sym*	fraddr;
	Sym*	toaddr;
	Sym*	friaddfl;
	Sym*	toiaddfl;
	Sym*	friaddfr;
	Sym*	toiaddfr;
	long	zipl;
	long	zipr;
};
struct	Rec7
{
	long	land;
	long	lng;
	long	lat;
	Sym*	version;
	long	state;
	long	county;
	Sym*	source;
	Sym*	cfcc;
	Sym*	laname;

	int	sequence;
};
struct	Rec8
{
	long	polyid;
	long	cenid;
	long	land;
	Sym*	version;
	long	state;
	long	county;
};
struct	Reca
{
	long	polyid;
	long	cenid;
	Sym*	version;
	long	state;
	long	county;
	Sym*	fair;
	long	fmcd;
	long	fpl;
	Sym*	ctbna;
	Sym*	blk;
	Sym*	cd101;
	Sym*	cd103;
	Sym*	sdelm;
	Sym*	sdmid;
	Sym*	sdsec;
	Sym*	sduni;
	Sym*	taz;
	Sym*	ua;
	Sym*	urbflag;
	Sym*	rs;
};
struct	Reci
{
	long	tlid;
	long	cenidl;
	long	polyl;
	long	cenidr;
	long	polyr;
	Sym*	version;
	long	state;
	long	county;
	Sym*	rtpoint;
};
struct	Recp
{
	long	polyid;
	long	cenid;
	long	lng;
	long	lat;
	Sym*	version;
	long	state;
	long	county;

	Rec7*	rec7;
	Reca*	reca;
};
struct	Recr
{
	Sym*	version;
	long	state;
	long	county;
	Sym*	cenid;
	Sym*	maxid;
	Sym*	minid;
	Sym*	highid;
};
struct	Recx
{
	long	type;
	long	state;
	long	fips;
	Sym*	name;

	Recx*	link;
};
struct	Side
{
	int	del;
	int	side;
	int	type;
	union
	{
		void*	param;
		long	paraml;
	};
};

enum
{
	A, N,
	LARGE	= 0x7fffffff,
	SMALL	= 0x80000000,
};

Sym*	hash[100003];
Sym*	null;
char	recs[256];
Biobuf	obuf;
long	laststate;
Recx*	missedx;
Side	side[100];

Rec1*	rec1;	long	nrec1;	long	mrec1;	long	xrec1;
Rec2*	rec2;	long	nrec2;	long	mrec2;	long	xrec2;
Rec3*	rec3;	long	nrec3;	long	mrec3;	long	xrec3;
Rec4*	rec4;	long	nrec4;	long	mrec4;	long	xrec4;
Rec5*	rec5;	long	nrec5;	long	mrec5;	long	xrec5;
Rec6*	rec6;	long	nrec6;	long	mrec6;	long	xrec6;
Rec7*	rec7;	long	nrec7;	long	mrec7;	long	xrec7;
Rec8*	rec8;	long	nrec8;	long	mrec8;	long	xrec8;
Reca*	reca;	long	nreca;	long	mreca;	long	xreca;
Reci*	reci;	long	nreci;	long	mreci;	long	xreci;
Recp*	recp;	long	nrecp;	long	mrecp;	long	xrecp;
Recr*	recr;	long	nrecr;	long	mrecr;	long	xrecr;
Recx*	recx;	long	nrecx;	long	mrecx;	long	xrecx;

void	decode1(char*);
void	decode2(char*);
void	decode3(char*);
void	decode4(char*);
void	decode5(char*);
void	decode7(char*);
void	decode6(char*);
void	decode8(char*);
void	decodea(char*);
void	decodei(char*);
void	decodep(char*);
void	decoder(char*);
void	decodex(char*);
void	dofile(char*);

void	debrief(void);
void	output(void);

Sym*	lookup(char*);
Sym*	rec(char*, int, int, int);
long	lat(char*, int, int, int);
long	num(char*, int, int, int);

void*	alloc(long);
