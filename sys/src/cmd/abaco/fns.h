#define	runemalloc(a)		emalloc((a)*sizeof(Rune))
#define	runerealloc(a, b)	erealloc(a, (b)*sizeof(Rune))
#define	runemove(a, b, c)	memmove(a, b, (c)*sizeof(Rune))

#define	hasbrk(x)	((x)&IFbrk || (x)&IFbrksp)
#define	istrue(x)	((x) ? "true" : "false")

void plumblook(Plumbmsg*m);
int plumbrunestr(Runestr *, char *);
void putsnarf(Runestr *);
void getsnarf(Runestr *);

void tablesize(Table *, int);
void drawtable(Box *, Page *, Image *);
void laytable(Itable *, Rectangle);
void settables(Page *);
void laysnarf(Page *, Lay *, Runestr *);
Timer* timerstart(int);
void timerstop(Timer*);
void timercancel(Timer*);
void timerinit(void);

void cut(Text *, Text *, int, int, Rune *, int);
void get(Text *, Text *, int, int, Rune *, int);
void paste(Text *, Text *, int, int, Rune *, int);
void execute(Text *, uint, uint, Text *);
void look3(Text *, uint, uint);
int search(Text *, Rune *, uint);

void scrsleep(uint);
void scrlresize(void);
void tmpresize(void);

void	initfontpaths(void);
void cvttorunes(char*, int, Rune*, int*, int*, int*);
void error(char *);
void closerunestr(Runestr *);
void copyrunestr(Runestr *, Runestr *);
int runestreq(Runestr, Runestr);
int validurl(Rune *);
int runeeq(Rune *, uint, Rune *, uint);
int min(int, int);
int max(int, int);
int isalnum(Rune);
Rune* skipbl(Rune *, int, int *);
Rune* findbl(Rune *r, int, int *);
char* estrdup(char *);
Rune* erunestrdup(Rune *);
Rune* ucvt(Rune *s);
int dimwidth(Dimen , int);
void frdims(Dimen *, int, int, int **);
Image* getbg(Page *);
Rune* getbase(Page *);
Image* eallocimage(Display *, Rectangle, ulong, int, int);
Image* getcolor(int);
void freecolors(void);
Font* getfont(int);
void freefonts(void);
void colarray(Image **, Image *, Image *, Image *, int);
void rect3d(Image *, Rectangle, int, Image **, Point);
void ellipse3d(Image *, Point, int, int, Image **, Point);
void reverseimages(Iimage **);
void setstatus(Window *, char *, ...);
int istextfield(Item *);
int forceitem(Item *);
int xtofchar(Rune *, Font *, long);
int istextsel(Page *, Rectangle, int *, int *, Rune *, Font *);
char* convert(Runestr, char *, long *);
void execproc(void *);
void getimage(Cimage *, Rune *);
Point getpt(Page *p, Point);
Rune *urlcombine(Rune *, Rune *);
void fixtext(Page *);
void addrefresh(Page *, char *, ...);
void flushrefresh(void);
void savemouse(Window *);
void restoremouse(Window *);
void clearmouse(void);
void bytetorunestr(char *, Runestr *);
Window* makenewwindow(Page *);

Line* linewhich(Lay *, Point);
Box* pttobox(Line *, Point);
Box* boxwhich(Lay *, Point);

