#ifndef	MAXMV
#define	MAXMV	500
#endif
#ifndef	MAXMG
#define	MAXMG	100
#endif

enum
{
	WPAWN	= 1,
	WKNIGHT,
	WBISHOP,
	WROOK,
	WQUEEN,
	WKING,

	BPAWN	= 9,
	BKNIGHT,
	BBISHOP,
	BROOK,
	BQUEEN,
	BKING,

	BLACK	= 8,
	INIT	= 16,
	AMARK	= 040000,
	EPMARK	= 020000,

	TMARK	= 0,
	TNORM,
	TEPENAB,
	TOOO,
	TOO,
	TNPRO,
	TBPRO,
	TRPRO,
	TQPRO,
	TENPAS,
	TNULL	= 15,

	U1	= 0x01,
	U2	= 0x02,
	L1	= 0x04,
	L2	= 0x08,
	D1	= 0x10,
	D2	= 0x20,
	R1	= 0x40,
	R2	= ~0x80+1,

	ULEFT	= U1|L1,
	URIGHT	= U1|R1,
	DLEFT	= D1|L1,
	DRIGHT	= D1|R1,
	LEFT	= L1,
	RIGHT	= R1,
	UP	= U1,
	DOWN	= D1,
	U2R1	= U1|U2|R1,
	U1R2	= U1|R1|R2,
	D1R2	= D1|R1|R2,
	D2R1	= D1|D2|R1,
	D2L1	= D1|D2|L1,
	D1L2	= D1|L1|L2,
	U1L2	= U1|L1|L2,
	U2L1	= U1|U2|L1,
	RANK2	= D2,
	RANK7	= U2,
};

short*	lmp;
short*	amp;
short	ambuf[MAXMV*2 + 10];
int	moveno;
int	eppos;
int	bkpos;
int	wkpos;
char	board[64];
short	mvlist[MAXMV];
int	mvno;
extern	schar	dir[];
extern	schar	attab[];
extern	schar	color[];

typedef
struct
{
	char	bd[64];
	short	epp;
	short	mvno;
	short	mv[1];
} Posn;
Posn	initp;

extern	void	ginit(Posn*);
extern	void	gstore(Posn*);
extern	void	getamv(short*);
extern	void	getmv(short*);
extern	void	move(int);
extern	void	qmove(int);
extern	int	rmove(void);
extern	int	retract(int);
extern	int	check(void);
extern	void	wgen(void);
extern	int	wcheck(int, int, int);
extern	void	wmove(int);
extern	void	wremove(void);
extern	int	battack(int);
extern	void	bgen(void);
extern	int	bcheck(int, int, int);
extern	void	bmove(int);
extern	void	bremove(void);
extern	int	wattack(int);

extern	void	chessinit(int, int, int);
extern	int	chessfmt(void*, Fconv*);
extern	int	chessin(char*);
extern	int	xalgin(short*, char*);
extern	int	xdscin(short*, char*);
extern	char*	getpie(int*, int*, char*);
extern	char*	getpla(int*, int*, char*);
extern	void	algout(int, char*, int);
extern	void	xalgout(short*, int, char*, int);
extern	void	dscout(int, char*, int);
extern	void	xdscout(short*, int, char*, int);
extern	void	setxy(int, int, int*, int*);
extern	char*	putpie(int, int, int, char*);
extern	char*	putpla(int, int, char*);
extern	int	mate(void);
extern	int	match(int, int, int, int, int, int, int, int);
extern	int	mat(int, int);
