#define	NWWW	200	/* # of pages we can get before failure */
#define	NNAME	128
#define	NTITLE	81	/* length of title (including nul at end) */
#define	NREDIR	10	/* # of redirections we'll tolerate before declaring a loop */
typedef struct Action Action;
typedef struct Url Url;
typedef struct Www Www;
typedef struct Scheme Scheme;
typedef struct Field Field;
struct Scheme{
	char *name;
	int type;
	int flags;
	int port;
};
struct Action{
	char *image;
	Rectangle r;
	char *imagebits;
	Field *field;
	char *link;
	char *name;
	int ismap;
};
struct Url{
	char fullname[NNAME];
	Scheme *scheme;
	char ipaddr[NNAME];
	char reltext[NNAME];
	char tag[NNAME];
	char redirname[NNAME];
	int port;
	int access;
	int type;
	int cachefd;		/* file descriptor on which to write cached data */
};
struct Www{
	Url *url;
	char title[NTITLE];
	Rtext *text;
	Rtext *top;
	int changed;		/* reader sets this every time it updates page */
	int finished;		/* reader sets this when done */
	int alldone;		/* page will not change further -- used to adjust cursor */
	int index;
};
/*
 * url reference types -- COMPRESS and GUNZIP are flags that can modify any other type
 */
#define	GIF		1
#define	HTML		2
#define	JPEG		3
#define	PIC		4
#define	AUDIO		5
#define	PLAIN		6
#define	XBM		7
#define	POSTSCRIPT	8
#define	FORWARD		9
#define	COMPRESS	16
#define	GUNZIP		32
#define	COMPRESSION	(COMPRESS|GUNZIP)
/*
 * url access types
 */
#define	HTTP		1
#define	FTP		2
#define	FILE		3
#define	TELNET		4
#define	MAILTO		5
#define	EXEC		6
#define	GOPHER		7
Bitmap *hrule, *bullet;
char home[512];		/* where to put files */
int chrwidth;		/* nominal width of characters in font */
Panel *text;		/* Panel displaying the current www page */
/*
 * HTTP methods
 */
#define	GET	1
#define	POST	2
void plrdhtml(char *, int, Www *);
void plrdplain(char *, int, Www *);
void htmlerror(char *, int, char *, ...);	/* user-supplied routine */
void crackurl(Url *, char *, Url *);
void getpix(Rtext *, Www *);
int urlopen(Url *, int, char *);
void getfonts(void);
void *emalloc(int);
void setbitmap(Rtext *);
void message(char *, ...);
int http(Url *, int, char *);
int gopher(Url *);
int cistrcmp(char *, char *);
int cistrncmp(char *, char *, int);
int suffix2type(char *);
int content2type(char *);
int encoding2type(char *);
void mkfieldpanel(Rtext *);
void geturl(char *, int, char *, int);
