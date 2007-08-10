/*  console state (for consctl) */
typedef struct Consstate	Consstate;
struct Consstate{
	int raw;
	int hold;
};

extern Consstate*	consctl(void);
extern Consstate*	cs;

#define	XMARGIN	5	/* inset from border of layer */
#define	YMARGIN	5
#define	INSET	3
#define	BUFS	32
#define	HISTSIZ	4096	/* number of history characters */
#define BSIZE	1000

#define	SCROLL	2
#define NEWLINE	1
#define OTHER	0

#define COOKED	0
#define RAW	1

/* text attributes */
enum {
	THighIntensity = (1<<0),
	TUnderline = (1<<1),
	TBlink = (1<<2),
	TReverse = (1<<3),
	TInvisible = (1<<4),
};
	

#define	button2()	((mouse.buttons & 07)==2)
#define	button3()	((mouse.buttons & 07)==4)

struct ttystate {
	int	crnl;
	int	nlcr;
};
extern struct ttystate ttystate[];

#define NKEYS 32	/* max key definitions */
struct funckey {
	char	*name;
	char	*sequence;
};
extern struct funckey *fk;
extern struct funckey vt100fk[], vt220fk[], ansifk[], xtermfk[];

extern int	x, y, xmax, ymax, olines;
extern int	peekc, attribute;
extern char*	term;

extern void	emulate(void);
extern int	host_avail(void);
extern void	clear(Rectangle);
extern void	newline(void);
extern int	get_next_char(void);
extern void	ringbell(void);
extern int	number(char *, int *);
extern void	scroll(int,int,int,int);
extern void	backup(int);
extern void	sendnchars(int, char *);
extern void	sendnchars2(int, char *);
extern Point	pt(int, int);
extern void	funckey(int);
extern void	drawstring(Point, char*, int);

extern int	debug;
extern int	yscrmin, yscrmax;
extern int	attr;
extern int	defattr;

extern Image *fgcolor;
extern Image *bgcolor;
extern Image *colors[];
extern Image *hicolors[];
extern Image *bgdefault;
extern Image *fgdefault;

extern int cursoron;
extern int nocolor;

extern void curson(int);
extern void cursoff(void);
extern void setdim(int, int);

