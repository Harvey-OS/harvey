/*
 *
 * Macros, definitions, structures, and function declarations for eexec and
 * Type 1 fonts. Much that's here, particularly constants dealing with erosion,
 * needs to be tuned for each output device.
 *
 */

#define INTERNALPASSWORD	1183615869
#define FONTPASSWORD		5839

#define EEXECSTART		0x1D971	/* seed for eexec */
#define SHOWSTART		0x6310EA	/* and font type 1 show */

#define MAGIC1			0x3FC5CE6D	/* constants for CharString */
#define MAGIC2			0xD8658BF	/* and eexec decryption */

#define BLUEFUZZ		1.0
#define BLUESHIFT		7.0
#define BLUESCALE		0.039625

#define FLATNESS		0.5
#define	DEFAULTMITER		2.5625
#define TYPE2MITER		2.0

#define FONTINCH		1000.0
#define SMALLCHAR		12.5
#define SMALLCTM		1e-5
#define NOMOVE			0.001
#define SMALLMOVE		0.01
#define LCKTBLSZ	32

#define BaselineNormal(P)	((Locktype == -1) ? P.x : P.y)
#define fmin(X, Y)		((X < Y) ? X : Y)
#define fmax(X, Y)		((X > Y) ? X : Y)
#define Round(x)	(x >= 0 ? (int)(x+.5) : (int)(x-.5))
#define Norm(p)	sqrt(p.x*p.x + p.y*p.y)
#define Moveto(p)	add_element(PA_MOVETO, _transform(p))
#define Lineto(p)	add_element(PA_LINETO, _transform(p))

typedef struct vector {
	double		tail;
	double		tip;
} Vector;

typedef struct lcktable {
	double		tail;
	double		tip;
	double		scale;
	int		count;
} LckTable;

struct source {
	unsigned	code;
	unsigned char	*string;
};

/*
 *
 * Function declarations - so they don't have to be repeated everywhere!
 *
 */
int Newchar(void);
void GetPrivate(struct dict *);
int GetMetricInfo(struct dict *, int);
int SetLocktype(void);
double VertStrokeWidth(void);
void InsertLck(double, double, LckTable *);
Vector GetXlckVector(double, double);
Vector GetYlckVector(double, double);
Vector SimpleYlck(double, double);
double LckMiddle(double, double);
struct pspoint LockPoint(struct pspoint);
struct pspoint Lck(struct pspoint);
double XYLck(double, LckTable *);
void LckTableInit(void);
void DumpLckTables(void);
struct element *AppendElement(int, struct pspoint, struct element *);
double PopDouble(void);
int IntLookup(struct dict *, struct object, int);
double NumLookup(struct dict *, struct object, double);
double DictDoubleGet(struct dict *, struct object);
double ArrayDoubleGet(struct object, int);
double Dnorm(double, double);
void spaint(int, int, int);
void FillCharString(int);
void strtlckOP(void);
void xlckOP(void);
void ylckOP(void);
void lckOP(void);
void superexecOP(void);
void CCRunOP(void);

#define ENDCHAR 0xe
#define HSBW	0xd
#define CLOSEPATH	0x9
#define HLINETO	0x6
#define HMOVETO	0x16
#define HVCURVETO	0x1f
#define RLINETO	0x5
#define RMOVETO	0x15
#define RRCURVETO	0x8
#define VHCURVETO	0x1e
#define VLINETO	0x7
#define VMOVETO	0x4
#define ESCAPE	0xc
#define HSTEM	0x1
#define VSTEM	0x3
#define CALLSUBR	0xa
#define RETURN	0xb

#define SEAC	0x6
#define SBW	0x7
#define DOTSECTION	0x0
#define HSTEM3	0x2
#define VSTEM3	0x1
#define DIV	0xc
#define CALLOTHERSUBR	0x10
#define POP	0x11
#define SETCP	0x21
