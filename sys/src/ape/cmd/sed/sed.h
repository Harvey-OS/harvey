#
/*
 * sed -- stream  editor
 *
 *
 */

#define CBRA	1
#define	CCHR	2
#define	CDOT	4
#define	CCL	6
#define	CNL	8
#define	CDOL	10
#define	CEOF	11
#define CKET	12
#define CNULL	13
#define CLNUM	14
#define CEND	16
#define CDONT	17
#define	CBACK	18

#define	STAR	01

#define NLINES	256
#define	DEPTH	20
#define PTRSIZE	1024
#define RESIZE	20000
#define	ABUFSIZE	20
#define	LBSIZE	4000
#define	LABSIZE	50
#define NBRA	9

typedef unsigned char uchar;

FILE	*fin;
union reptr	*abuf[ABUFSIZE];
union reptr **aptr;
uchar	*lastre;
uchar	ibuf[512];
uchar	*cbp;
uchar	*ebp;
uchar	genbuf[LBSIZE];
uchar	*loc1;
uchar	*loc2;
uchar	*locs;
uchar	seof;
uchar	*reend;
uchar	*lbend;
uchar	*hend;
uchar	*lcomend;
union reptr	*ptrend;
int	eflag;
int	dolflag;
int	sflag;
int	jflag;
int	numbra;
int	delflag;
long	lnum;
uchar	linebuf[LBSIZE+1];
uchar	holdsp[LBSIZE+1];
uchar	*spend;
uchar	*hspend;
int	nflag;
int	gflag;
uchar	*braelist[NBRA];
uchar	*braslist[NBRA];
long	tlno[NLINES];
int	nlno;
#define MAXFILES	120
char	fname[MAXFILES][40];
FILE	*fcode[MAXFILES];
int	nfiles;

#define ACOM	01
#define BCOM	020
#define CCOM	02
#define	CDCOM	025
#define	CNCOM	022
#define COCOM	017
#define	CPCOM	023
#define DCOM	03
#define ECOM	015
#define EQCOM	013
#define FCOM	016
#define GCOM	027
#define CGCOM	030
#define HCOM	031
#define CHCOM	032
#define ICOM	04
#define LCOM	05
#define NCOM	012
#define PCOM	010
#define QCOM	011
#define RCOM	06
#define SCOM	07
#define TCOM	021
#define WCOM	014
#define	CWCOM	024
#define	YCOM	026
#define XCOM	033

uchar	*cp;
uchar	*reend;
uchar	*lbend;

union	reptr {
	struct reptr1 {
		uchar	*ad1;
		uchar	*ad2;
		uchar	*re1;
		uchar	*rhs;
		FILE	*fcode;
		uchar	command;
		uchar	gfl;
		uchar	pfl;
		uchar	inar;
		uchar	negfl;
	} r1;
	struct reptr2 {
		uchar	*ad1;
		uchar	*ad2;
		union reptr	*lb1;
		uchar	*rhs;
		FILE	*fcode;
		uchar	command;
		uchar	gfl;
		uchar	pfl;
		uchar	inar;
		uchar	negfl;
	} r2;
} ptrspace[PTRSIZE], *rep;


uchar	respace[RESIZE];

struct label {
	uchar	asc[9];
	union reptr	*chain;
	union reptr	*address;
} ltab[LABSIZE];

struct label	*lab;
struct label	*labend;

int	f;
int	depth;

int	eargc;
uchar	**eargv;

uchar	*address(uchar *);
int		advance(uchar *, uchar *);
void	arout(void);
extern	uchar	bittab[];
uchar	bad;
uchar	*badp;
int		cmp(uchar *, uchar *);
union reptr	**cmpend[DEPTH];
void	command(union reptr *);
uchar	compfl;
uchar	*compile(uchar *);
uchar	*compsub(uchar *);
void	dechain(void);
int	depth;
void	dosub(uchar *);
int		ecmp(uchar *, uchar *, int);
void	execute(uchar *);
void		fcomp(void);
uchar	*gline(uchar *);
uchar	*lformat(int, uchar *);
int		match(uchar *, int);
union reptr	*pending;
uchar	*place(uchar *, uchar *, uchar *);
int		rline(uchar *);
struct label	*search(struct label *);
int		substitute(union reptr *);
uchar	*text(uchar *);
uchar	*ycomp(uchar *);
