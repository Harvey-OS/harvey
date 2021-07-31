enum	linecap
{
	CAP_BUTT = 0 ,
	CAP_ROUND = 1 ,
	CAP_PROJSQUARE = 2 ,
} ;

enum	linejoin
{
	JOIN_MITER = 0 ,
	JOIN_ROUND = 1 ,
	JOIN_BEVEL = 2 ,
} ;

struct	color
{
	int	red ;
	int	green ;
	int	blue ;
} ;

struct	dash
{
	int		count ;
	double		offset ;
	double		dash[DASH_LIMIT] ;
} ;

struct	device
{
	struct	object	proc ;
	int		height ;
	int		width ;
	double		matrix[CTM_SIZE] ;
} ;

struct	screen
{
	double		frequency ;
	double		angle ;
	struct	object	proc ;
	int	levels;
	struct	Bitmap	*bitmap[GRAYLEVELS] ;
} ;


struct	hta
{
	int	x ;
	int	y ;
	double	order ;
} ;

struct	graphics
{
	double		CTM[CTM_SIZE] ;
	struct	color	color ;
	struct	path	path ;
	struct	path	clippath ;
	int clipchanged;
	int iminx, iminy, imaxx, imaxy;
	struct	dict	*font ;
	double		linewidth ;
	enum	linecap	linecap ;
	enum   linejoin	linejoin ;
	struct	screen	screen ;
	struct	object	transfer ;
	double		graytab[GRAYLEVELS] ;
	double		flat ;
	double		miterlimit ;
	struct	dash	dash ;
	struct	device	device ;
	struct	pspoint	width ;
	int		inshow ;
	int		incharpath;
	struct pspoint	origin;
	struct	save	*save ;
} ;

struct  x {
	double	left, right ;
	int	direction ;
} ;

struct x intercepts(struct element *, struct element *, double);
int sxcomp(struct x *, struct x *);

struct color hsb2rgb(double, double, double);
double currentgray(void);
void paint(int, int, int, int);
int htcomp(struct hta *, struct hta *);
int gcd(int, int);
int rem(int, int);

# define E(m,p)	m.value.v_array.object[p].value.v_real
# define I(m,p)	m.value.v_array.object[p].value.v_int
# define G	Graphics.CTM

extern	struct graphics	Graphics ;
void mybinit(void);
#ifdef HIRES
void fwr_bm_g4(int, Bitmap *);
#endif
#ifdef FAX
long fwr_bm_g31(FILE *, Bitmap *);
#ifdef AHMDAL
void aswab(unsigned char[]);
void vswab(unsigned long *);
#else
void vswab(unsigned long *);
void aswab(unsigned char[]);
#endif
#endif
