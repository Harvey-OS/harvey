enum	objtype
{
	OB_NONE = 1 ,	
	OB_ARRAY = 2,
	OB_BOOLEAN = 4,
	OB_DICTIONARY = 010 ,
	OB_FILE = 020,
	OB_FONTID = 040 ,
	OB_INT = 0100,
	OB_MARK = 0200,
	OB_NAME = 0400,
	OB_NULL = 01000,
	OB_OPERATOR = 02000,
	OB_REAL = 04000,
	OB_SAVE = 010000,
	OB_STRING = 020000,
	OB_NUMBER = 04100
} ;

enum	access
{
	AC_NONE = 0 ,
	AC_EXECUTEONLY = 1 ,
	AC_READONLY = 2 ,
	AC_UNLIMITED = 3 ,
} ;

enum	xattr
{
	XA_LITERAL ,
	XA_EXECUTABLE ,
	XA_IMMEDIATE ,
} ;

enum	bool
{
	FALSE = 0 ,
	TRUE = 1 ,
} ;

struct	pspoint
{
	double	x ;
	double	y ;
} ;

struct	array
{
	enum	access	access ;
	int		length ;
	struct	object	*object ;
} ;

struct	pstring
{
	enum	access	access ;
	int		length ;
	unsigned char	*chars ;
} ;

struct	file
{
	enum	access	access ;
	FILE		*fp ;
} ;

struct	save
{
	int		level ;
	struct namelist	*namelist ;
	char		*cur_mem ;
} ;

struct fontid {
	int fontno;
};

struct	object
{
	enum	objtype		type ;
	enum	xattr		xattr ;
	union
	{
		void		(*v_operator)(void) ;
		struct	array	v_array ;
		enum	bool	v_boolean ;
		struct	dict	*v_dict ;
		int		v_int ;
		char		*v_pointer ;
		double		v_real ;
		struct	pstring	v_string ;
		struct	file	v_file ;
		struct	fontid	v_fontid ;
		struct	save	*v_save ;
	}
				value ;
} ;

extern struct object none, zero, true, false, null,lbrak,rbrak, blank, nullstr,
	notdef, filename, lbrace, rbrace;
						/*object.c*/
void operstackinit(void), oper_invalid(char *), push(struct object),
	pushpoint(struct pspoint), popOP(void), exchOP(void), dupOP(void),
	copyOP(void), indexOP(void), rollOP(void), rollrec(int, int, int),
	clearOP(void), countOP(void),markOP(void),cleartomarkOP(void),
	counttomarkOP(void), pstakOP(void), stackOP(void);
struct object pop(void), copyobject(struct object);
int modulus(int, int);
extern int searchflg;
struct object standardencoding;
						/*control.c*/
void forallOP(void), forOP(void), repeatOP(void), loopOP(void),
	ifOP(void), ifelseOP(void);
						/*dict.c*/
struct object dictget(struct dict *, struct object);
struct object odictget(struct dict *, struct object);
struct object dictstackget(struct object, char *);
void dictstackinit(void), dictOP(void), beginOP(void), endOP(void),
	maxlengthOP(void), currentdictOP(void), defOP(void), loadOP(void),
	knownOP(void), whereOP(void), storeOP(void),countdictstackOP(void),
	dictstackOP(void);
void dictput(struct dict *, struct object, struct object);
void sysdictput(struct object, void *);
void loaddict(struct dict *, struct object, void *);
void odictput(struct dict *, struct object, struct object);
void begin(struct object);

						/*color.c*/
void setgrayOP(void), sethsbcolorOP(void), setrgbcolorOP(void), currentgrayOP(void),
	currenthsbcolorOP(void), currentrgbcolorOP(void), currenttransferOP(void),
	settransferOP(void);
double fraction(void);

						/*device.c*/
void erasepageOP(void), showpageOP(void), copypageOP(void),initclipOP(void),
	framedeviceOP(void), nulldeviceOP(void), deviceinit(void), waittimeout(void),
	product(void), dummyop(void), letter(void), legal(void), setjobtimeout(void),
	jobname(void), revision(void), pagecount(void), dobitmap(void);
void do_fill(void(*)(void));

						/*execute.c*/
void execstackinit(void), execute(void), execOP(void),
	arrayforall(void), stringforall(void), dictforall(void), xfor(void),
	xrepeat(void), xloop(void), execstackOP(void), exitOP(void),
	quitOP(void), countexecstackOP(void), xpathforall(void), currentfileOP(void),
	stoppedOP(void), stopOP(void), startOP(void);
void loopnoop(void), xstopped(void);
void exec_invalid(char *);
void execpush(struct object);

						/*file.c*/
void equalsOP(void), equalsequalsOP(void), fileOP(void), flushfileOP(void),
	closefileOP(void), readOP(void), writeOP(void), readlineOP(void),
	readstringOP(void), readhexstringOP(void), writestringOP(void),
	writehexstringOP(void), bytesavailableOP(void), statusOP(void), tokenOP(void),
	runOP(void);
void eqeq(struct object, int);
int fgetx(FILE *);

						/*font.c*/
void definefontOP(void), findfontOP(void), scalefontOP(void), setfontOP(void),
	currentfontOP(void), makefontOP(void), showOP(void), ashowOP(void),
	widthshowOP(void), awidthshowOP(void), kshowOP(void), show_save(void),
	show_restore(void), setcharwidthOP(void), setcachedeviceOP(void),
	stringwidthOP(void);
struct object fontcheck(char *);
void show(struct pspoint, int, struct pspoint, struct object, struct object);
void charpath(struct object, struct object);

						/*math.c*/
struct object number(char *), real(char *), integer(char *);
void absOP(void), addOP(void), divOP(void), idivOP(void), modOP(void),
	mulOP(void), negOP(void), subOP(void), ceilingOP(void), floorOP(void),
	roundOP(void), truncateOP(void), atanOP(void), cosOP(void), sinOP(void),
	expOP(void), lnOP(void), logOP(void), sqrtOP(void), randOP(void),
	srandOP(void), rrandOP(void);

						/*make.c*/
struct object makestring(int);
struct object makereal(double);
struct object makeint(int), makedict(int);
struct object makepointer(char *);
struct object makesave(int, struct namelist *, char *);
struct object makebool(enum bool);
struct object makenull(void), makenone(void), makefid(void);
struct object makearray(int, enum xattr);
struct object makefile(FILE *, enum access, enum xattr);
struct object makeoperator(void(*)(void));

						/*array.c*/
void lengthOP(void), arrayOP(void), endarrayOP(void), putOP(void), getOP(void),
	getintervalOP(void), putintervalOP(void), aloadOP(void), astoreOP(void);

						/*path.c*/
struct pspoint currentpoint(void);
void currentpointOP(void), movetoOP(void), rmovetoOP(void), linetoOP(void),
	rlinetoOP(void), curvetoOP(void), rcurvetoOP(void), closepathOP(void),
	reversepathOP(void), newpathOP(void), pathbboxOP(void), pathforallOP(void),
	flattenpathOP(void), arcOP(void), arcnOP(void), arctoOP(void), clippathOP(void),
	fillOP(void), eofillOP(void), close_components(void), strokeOP(void),
	strokepathOP(void), charpathOP(void), pathinit(void), countfreeOP(void);

						/*error.c*/
void error(char *);
void warning(char *);
void pserror(char *, char *);
void errorhandleOP(void), trapOP(void), debug(void);
void done(int);

void skiptopage(void);				/*get_token.c*/
extern int microsoft;

void imageOP(void);				/*image.c*/

struct object imageproc(void);			/*imagemake.c*/
void imagemaskOP(void), printCTM(char *);

						/*matrix.c*/
void matrixOP(void), identmatrixOP(void), currentmatrixOP(void), setmatrixOP(void),
	translateOP(void), scaleOP(void), rotateOP(void), concatOP(void),
	concatmatrixOP(void), transformOP(void), dtransformOP(void),
	initmatrixOP(void), defaultmatrixOP(void), invertmatrixOP(void),
	itransformOP(void), idtransformOP(void);
struct pspoint _transform(struct pspoint), dtransform(struct pspoint),
	itransform(struct pspoint), idtransform(struct pspoint);
struct	object realmatrix(struct object matrix);
struct	object makematrix(struct object, double, double, double, double, double,
		double);

							/*misc.c*/
void versionOP(void), usertimeOP(void), bindOP(void), printOP(void), flushOP(void),
	lettertray(void);
void psibind(struct array, int);

							/*relational.c*/
enum bool equal(struct object, struct object);
void eqOP(void), neOP(void), geOP(void), leOP(void), gtOP(void), ltOP(void),
	andOP(void), orOP(void), xorOP(void), notOP(void), bitshiftOP(void);
int comp_str(struct pstring, struct pstring);
int comp_int(int, int), comp_real(double, double);
int compare(struct object, struct object);

struct object scanner(FILE *), scan_proc(FILE *);		/*scanner.c*/

							/*io.c*/
FILE *sopen(struct pstring);
int stell(FILE *), f_getc(FILE *), f_close(FILE *);
int un_getc(int, FILE *);

							/*tac.c*/
void typeOP(void), cvlitOP(void), cvxOP(void), cviOP(void), cvrOP(void), cvrsOP(void),
	cvnOP(void), cvsOP(void), xcheckOP(void), rcheckOP(void), wcheckOP(void),
	noaccessOP(void), readonlyOP(void), executeonlyOP(void);
unsigned char *radconv(unsigned int, int, unsigned char *, unsigned char *);
unsigned char *cvs(struct object, int *);
void setaccess(enum access);

void stringOP(void), anchorsearchOP(void), searchOP(void);	/*string.c*/

								/*save.c*/
void savestackinit(void), saveOP(void), restoreOP(void), vmstatusOP(void);
void saveitem(void *, int);
struct object makename(unsigned char *, int, enum xattr);
struct object pckname(struct pstring, enum xattr);
struct object pname(struct pstring, enum xattr);
char *vm_alloc(int);
void invalid(struct object, char *);

void setscreenOP(void), currentscreenOP(void);		/*screen.c*/

void type1init(void), eexecOP(void);			/*type1.c*/
void sdo_fill(void(*)(void));
int df_getc(FILE *);
extern struct object n_FontName;

void initfont(void);					/*initfont.c*/
struct object makebits(int *, int, int);

void putfcacahe(double, double, double, double);	/*cache.c*/
void BuildChar(void), cachestatusOP(void);
void printmatrix(char *, struct object);
extern double savex, savey;

							/*graphics.c*/
void graphicsstackinit(void), gsaveOP(void), grestoreOP(void), grestoreallOP(void),
	initgraphicsOP(void), setlinewidthOP(void), setmiterlimitOP(void),
	setflatOP(void), setlinecapOP(void), setlinejoinOP(void), setdashOP(void),
	currentmiterlimitOP(void), currentlinewidthOP(void), currentflatOP(void),
	currentlinecapOP(void), currentlinejoinOP(void), currentdashOP(void);
void gsave(struct save *), grestoreall(struct save *);

void init(char *), mybinit(void);

struct object procedure(void);			/*util.c*/
int hexcvt(int);

int do_clip(void);
void fixpgct(void);
void clipOP(void);
void eoclipOP(void);
void outlineOP(void);
void pstackOP(void), internaldictOP(void);

#define cname(p)	makename((unsigned char *)p,strlen((char *)p),XA_EXECUTABLE)


extern	char	*Prog_name ;
extern int resolution, trytype1;

extern char pagestr[10];
extern int pageno, prolog, page, pageflag, prologline, skipflag, Fullbits,restart,sawshow, reversed;
extern long prologend, Char_ct;
extern int nofontinit;
extern struct Point anchor, orig, corn, p_anchor;
extern FILE *xerr, *currentfile, *fpcache;
extern int dontcache, stdflag, merganthal, cacheout, korean, bbflg;
extern int Line_no, charpathflg;
extern int mdebug, dontask;
extern struct object FontMatrix, ImageMatrix, FID, FontBBox, Encoding,
	Chardata, BHeight, Ascent, FontInfo;
extern struct object Buildchar, FontDirectory, FontType, unknown;
extern struct object Charstrings, SCharstrings, Fontinfo, CharStrings;
extern struct object xalpha[52], lalpha[52], saveobj;
extern int current_type, saveflag;
extern double yadj, nxadj;
extern int chct, cacheit, shown, texcode;
extern int msize, fontchanged;
extern int blimit, bmax, mmax, cmax, csize, bsize;
extern int decryptf, d_unget;

extern int instring, errno;
