typedef struct Cursor Cursor;
typedef struct Cursorinfo Cursorinfo;
struct Cursorinfo {
	Cursor;
	Lock;
};

extern Cursorinfo	cursor;
extern Cursor		arrow;
extern Memimage		*gscreen;
extern int		cursorver;
extern Point		cursorpos;

Point 		mousexy(void);
int		cursoron(int);
void		cursoroff(int);
void		setcursor(Cursor*);
void		flushmemscreen(Rectangle r);
Rectangle	cursorrect(void);
void		cursordraw(Memimage *dst, Rectangle r);

void		drawactive(int);
void		drawlock(void);
void		drawunlock(void);
int		candrawlock(void);
void		getcolor(ulong, ulong*, ulong*, ulong*);
int		setcolor(ulong, ulong, ulong, ulong);
#define		TK2SEC(x)	0
extern void	blankscreen(int);
void		screeninit(int x, int y, char *chanstr);
void		mousetrack(int x, int y, int b, int msec);
uchar		*attachscreen(Rectangle*, ulong*, int*, int*, int*);

void		fsinit(char *mntpt, int x, int y, char *chanstr);
