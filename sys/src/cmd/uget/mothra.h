#include <u.h>
#include <libc.h>
#include <ctype.h>

Tm* p9gmtime(long);

typedef	struct	Url	Url;
typedef	struct	Scheme	Scheme;
typedef	struct	Ibuf	Ibuf;

#define	NWWW	200	/* # of pages we can get before failure */
#define	NNAME	512
#define NAUTH	128
#define	NTITLE	81	/* length of title (including nul at end) */
#define	NREDIR	10	/* # of redirections we'll tolerate before declaring a loop */

struct	Scheme
{
	char*	name;
	int	type;
	int	flags;
	int	port;
};

struct	Url
{
	char	fullname[NNAME];
	Scheme*	scheme;
	char	ipaddr[NNAME];
	char	reltext[NNAME];
	char	tag[NNAME];
	char	redirname[NNAME];
	char	autharg[NAUTH];
	char	authtype[NTITLE];
	int	port;
	int	access;
	int	type;
	int	cachefd;	/* file descriptor on which to write cached data */
};
struct	Ibuf
{
	int	fd;
	char	buf[4096];
	char*	rp;
	char*	wp;
	char*	lp;
	char*	ep;
};

/*
 * url reference types -- COMPRESS and GUNZIP are flags that can modify any other type
 */
#define	GIF		1
#define	HTML		2
#define	JPEG		3
#define	PIC		4
#define	TIFF		5
#define	AUDIO		6
#define	PLAIN		7
#define	XBM		8
#define	POSTSCRIPT	9
#define	FORWARD		10
#define	PDF		11
#define	COMPRESS	16
#define	GUNZIP		32
#define	COMPRESSION	(COMPRESS|GUNZIP)

/*
 * url access types
 */
#define	HTTP		1
#define	FTP		2
#define	AFILE		3
#define	TELNET		4
#define	MAILTO		5
#define	EXEC		6
#define	GOPHER		7

/*
 *  authentication types
 */
#define ANONE 0
#define ABASIC 1

extern	int	debug;		/* command line flag */
extern	ulong	modtime;
extern	char	version[];
extern	int	server;

/*
 * HTTP methods
 */
#define	GET	1
#define	POST	2

void	htmlerror(char*, int, char*, ...);	/* user-supplied routine*/
void	crackurl(Url*, char*, Url*);
int	urlopen(Url*, int, char*);
void*	emalloc(int);
void	message(char*, ...);
int	ftp(Url*);
int	http(Url*, int, char*);
int	gopher(Url*);
int	cistrcmp(char*, char*);
int	cistrncmp(char*, char*, int);
int	suffix2type(char*);
int	content2type(char*);
char*	type2content(int);
int	encoding2type(char*);
void	geturl(char*, int, char*);
int	dir2html(char*, int);
int	auth(Url*, char*, int);

Ibuf*	ibufalloc(int);
Ibuf*	ibufopen(char*);
char*	rdline(Ibuf*);
int	linelen(Ibuf*);
void	ibufclose(Ibuf*);

int	tcpconnect(char*, char*, int, char*);
int	tcpannounce(char*, uchar*, int*);
int	tcpaccept(int, uchar*, int*);
void	tcpdenounce(int);
char*	readfile(char*, char*, int);
char*	sysname(void);
char*	domainname(void);
