#pragma	lib	"libfb.a"

/*
 * picfile
 *
 * Not working: TYPE=ccitt-g31, TYPE=ccitt-g32, TYPE=ccitt-g4, TYPE=ccir601.
 * picconf.c has unimplemented TYPEs commented out.
 */
typedef struct PICFILE PICFILE;
struct PICFILE{
	int fd;
	int nchan;
	int x, y;
	int width, height;
	char *type;
	char *chan;
	char *cmap;
	int argc;
	char **argv;
	int flags;
	int line;
	int depth;			/* TYPE=plan9 only */
	unsigned char *buf;
	unsigned char *ebuf;
	unsigned char *bufp;
	int (*rd)(PICFILE *, void *);
	int (*wr)(PICFILE *, void *);
	int (*cl)(PICFILE *);
};
#define	PIC_NCHAN(p)		((p)->nchan)
#define	PIC_WIDTH(p)		((p)->width)
#define	PIC_HEIGHT(p)		((p)->height)
#define	PIC_XOFFS(p)		((p)->x)
#define	PIC_YOFFS(p)		((p)->y)
#define	PIC_RECT(p)		Rect((p)->x, (p)->y, (p)->x+(p)->width, (p)->y+(p)->height)	/* needs <geometry.h> */
#define	PIC_SAMEARGS(p)		(p)->type, (p)->x, (p)->y, (p)->width, (p)->height, (p)->chan, argv, (p)->cmap
#define	picread(f, buf)		(*(f)->rd)(f, buf)
#define	picwrite(f, buf)	(*(f)->wr)(f, buf)
PICFILE *picopen_r(char *);
PICFILE *picopen_w(char *, char *, int, int, int, int, char *, char *[], char *);
PICFILE *picputprop(PICFILE *, char *, char *);
char *picgetprop(PICFILE *, char *);
void picclose(PICFILE *);
void picpack(PICFILE *, char *, char *, ...);
void picunpack(PICFILE *, char *, char *, ...);	/* wrong? */
void picerror(char *);
int getcmap(char *, unsigned char *);
/*
 * Private data
 */
char *_PICcommand;
char *_PICerror;
void _PWRheader(PICFILE *);
int _PICplan9header(PICFILE *, char *);
int _PICread(int, void *, int);
#define	PIC_NOCLOSE	1	/* don't close p->fd on picclose */
#define	PIC_INPUT	2	/* open for input */
struct _PICconf{
	char *type;
	int (*rd)(PICFILE *, void *);
	int (*wr)(PICFILE *, void *);
	int (*cl)(PICFILE *);
	int nchan;
}_PICconf[];
/*
 * getflags
 */
#define	NFLAG	128
#define	NCMDLINE	512
#define	NGETFLAGSARGV	256
extern char **flag[NFLAG];
extern char cmdline[NCMDLINE+1];
extern char *cmdname;
extern char *flagset[];
extern char *getflagsargv[NGETFLAGSARGV+2];
extern getflags(int, char **, char *);
extern void usage(char *);
/*
 * rdpicfile, wrpicfile
 */
Bitmap *rdpicfile(PICFILE *, int);
int wrpicfile(PICFILE *, Bitmap *);
