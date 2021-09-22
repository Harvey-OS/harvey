#include	<u.h>
#include	<libc.h>
#include	<bio.h>

#ifndef	EXTERN
#define	EXTERN	extern
#endif

enum
{
	Tstring		= 1,
	Tpoint,
	Tline1,
	Tline2,
	Tline3,
	Tline4,
	Tgroup1,
	Tgroup2,
	Tgroup3,
	Tvect1,
	Tvect2,
	Tvect3,
	Tcirc1,
	Tcirc2,
	Tcirc3,
	Tarc,

	Pcol		= 0,	// 80	1
	Psiz,			// 40	2
	Pdsh,			// 20	1
	Pjux,			// 10	1
	Pfil,			// 08	2
	Pfnt,			// 04	1
	Prot,			// 02	2
	P7,			// 01	1
	Px,			// mysterious first parameter in xmode

	Foutline	= 0,
	Finterior,
	Fexterior,
	Fpattern,

	Maxs		= 50,
	Maxc		= 20,
	Maxv		= 10,
	Offx		= 100,

	Black		= 0,
	White		= 1,
	None		= -1,
	Grey		= -2,
};

typedef	struct	Rectangle	Box;

typedef	struct	Line	Line;
typedef	struct	String	String;
typedef	struct	Group	Group;
typedef	struct	Vect	Vect;
typedef	struct	Vec	Vec;
typedef	struct	Circ	Circ;
typedef	struct	Node	Node;
typedef	struct	Points	Points;
typedef	struct	Header	Header;
typedef	struct	Param	Param;
typedef	struct	Encap	Encap;

struct	Param
{
	ushort	paramf;
	ushort	param[9];
	uchar	fil[20];
};
struct	Vec
{
	int	x;
	int	y;
};
struct	Node
{
	uchar	type;
	uchar	op;
	uchar	here;
	int	ord;
	Param	p;
	Param	dg;
	Param	dh;
	ulong	beg;
	ulong	end;
	Node*	parent;
	Node*	link;
	union
	{
		struct
		{
			int	count;
			int	oldcount;
			char*	name;
			Param	p;
		} g;
		struct
		{
			Vec;
			char*	str;
		} s;
		struct
		{
			Vec	vec[4];
		} l;
		struct
		{
			int	count;
			Vec*	vec;
		} v;
		struct
		{
			Vec;
			int	r;
			int	a1;
			int	a2;
		} c;
	};
};

typedef	struct	Encap
{
	int	gok;
	int	ndata;
	uchar*	data;
};

struct	Header
{
	int	minbx;
	int	minby;
	int	maxbx;
	int	maxby;
	int	minx;
	int	miny;
	int	maxx;
	int	maxy;
	int	count;
	int	dx;		// calculated
	int	dy;
	int	ncolor;
	uchar	color[50];
	char	name[10];
	char	nam1[16];
	char	nam2[16];
	Param	p1;
	Param	p2;
	int	nencap;		// encapulated macros
	Encap*	encap;		// pointer to array of macros
};

EXTERN	uchar*	base;
EXTERN	Node*	root;
EXTERN	int	aflag;
EXTERN	int	dflag;
EXTERN	int	fflag;
EXTERN	int	pflag;
EXTERN	int	qflag;
EXTERN	int	sflag;
EXTERN	int	vflag;
EXTERN	int	xflag;
EXTERN	int	pxflag;
EXTERN	char*	file;
EXTERN	char*	type;
EXTERN	int	herel;
EXTERN	int	hereh;
EXTERN	char	conbuf[1000];
EXTERN	char*	morse[];
EXTERN	char	apbuf[500];
EXTERN	Biobuf*	pmout;
EXTERN	Biobuf*	pmin;
EXTERN	int	pmary[20];
EXTERN	ulong	cksum;

EXTERN	uchar*	getparam(uchar*, int, Param*);
EXTERN	void	setparam(Param*, int, uint);
EXTERN	void	jep(void);
EXTERN	ulong	checksum(uchar*, int);
EXTERN	Node*	parse(uchar*, int);
EXTERN	void*	mal(int);
EXTERN	void	doarc(Node*);
EXTERN	void	docirc(Node*);
EXTERN	void	prep(char*, char*);
EXTERN	void	fixgroups(void);
EXTERN	uchar*	doheader(uchar*, int*);
EXTERN	int	signp(uchar*);
EXTERN	char*	aprint(Node*);

EXTERN	void	allnode(uchar*);
EXTERN	void	prnode(uchar*);
EXTERN	void	drnode(uchar*);
EXTERN	void	psnode(uchar*);
EXTERN	void	bounds(uchar*);

EXTERN	char*	snorm(char*);
EXTERN	char*	smorse(char*);
EXTERN	char*	sfoot(char*);
EXTERN	char*	sbbox(char*);

EXTERN	char*	pname[20];
EXTERN	char*	tname[20];
EXTERN	char*	psdat[];
EXTERN	int	xcount;
EXTERN	uchar	dashc[];
EXTERN	Header	h;

EXTERN	char*	S0s1;
EXTERN	char*	S0s2;
EXTERN	char*	V1s1;
EXTERN	char*	V1s2;
EXTERN	char*	V1s3;
