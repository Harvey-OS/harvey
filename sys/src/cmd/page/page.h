typedef struct Document Document;

struct Document {
	char *docname;
	int npage;
	int fwdonly;
	char* (*pagename)(Document*, int);
	Image* (*drawpage)(Document*, int);
	int	(*addpage)(Document*, char*);
	int	(*rmpage)(Document*, int);
	Biobuf *b;
	void *extra;
};

void *emalloc(int);
void *erealloc(void*, int);
char *estrdup(char*);
int spawncmd(char*, char **, int, int, int);

int spooltodisk(uchar*, int, char**);
int stdinpipe(uchar*, int);
Document *initps(Biobuf*, int, char**, uchar*, int);
Document *initpdf(Biobuf*, int, char**, uchar*, int);
Document *initgfx(Biobuf*, int, char**, uchar*, int);
Document *inittroff(Biobuf*, int, char**, uchar*, int);
Document *initdvi(Biobuf*, int, char**, uchar*, int);

void viewer(Document*);
extern Cursor reading;
extern int chatty;
extern int goodps;
extern int textbits, gfxbits;
extern int reverse;
extern int clean;
extern int ppi;
extern int teegs;
extern int truetoboundingbox;
extern int wctlfd;
extern int resizing;
extern int newwindow;

void rot180(Image*);

/* ghostscript interface shared by ps, pdf */
typedef struct GSInfo	GSInfo;
struct GSInfo {
	int gsfd;
	Biobuf gsrd;
	int gspid;
	int gsdfd;
	int ppi;
};
void	waitgs(GSInfo*);
int	gscmd(GSInfo*, char*, ...);
int	spawngs(GSInfo*);
void	setdim(GSInfo*, Rectangle, int);
int	spawnwriter(GSInfo*, Biobuf*);
Rectangle	screenrect(void);
void	newwin(void);
Rectangle winrect(void);
void	resize(int, int);
int	max(int, int);
int	min(int, int);
void	wexits(char*);
Image*	xallocimage(Display*, Rectangle, ulong, int, ulong);
int	bell(void*, char*);
int	opentemp(char *template);

extern int stdinfd;
extern int truecolor;
