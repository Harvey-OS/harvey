#include <u.h>
#include <libc.h>
#include <bio.h>
#include <disk.h>
#include <auth.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>

/* acme */
typedef struct Event Event;
typedef struct Window Window;

enum
{
	STACK		= 16384,
	EVENTSIZE	= 256,
	NEVENT		= 5,
};

struct Event
{
	int	c1;
	int	c2;
	int	q0;
	int	q1;
	int	flag;
	int	nb;
	int	nr;
	char	b[EVENTSIZE*UTFmax+1];
	Rune	r[EVENTSIZE+1];
};

struct Window
{
	/* file descriptors */
	int		ctl;
	int		event;
	int		addr;
	int		data;
	Biobuf	*body;

	/* event input */
	char		buf[512];
	char		*bufp;
	int		nbuf;
	Event	e[NEVENT];

	int		id;
	int		open;
	Channel	*cevent;	/* chan(Event*) */
};

extern	Window*	newwindow(void);
extern	int		winopenfile(Window*, char*);
extern	void		winopenbody(Window*, int);
extern	void		winclosebody(Window*);
extern	void		wintagwrite(Window*, char*, int);
extern	void		winname(Window*, char*);
extern	void		winwriteevent(Window*, Event*);
extern	void		winread(Window*, uint, uint, char*);
extern	int		windel(Window*, int);
extern	void		wingetevent(Window*, Event*);
extern	void		wineventproc(void*);
extern	void		winwritebody(Window*, char*, int);
extern	void		winclean(Window*);
extern	int		winselect(Window*, char*, int);
extern	int		winsetaddr(Window*, char*, int);
extern	char*	winreadbody(Window*, int*);
extern	void		windormant(Window*);
extern	void		winsetdump(Window*, char*, char*);

extern	char*	readfile(char*, char*, int*);
extern	void		ctlprint(int, char*, ...);
extern	void*	emalloc(uint);
extern	char*	estrdup(char*);
extern	char*	estrstrdup(char*, char*);
extern	char*	egrow(char*, char*, char*);
extern	char*	eappend(char*, char*, char*);
extern	void		error(char*, ...);
extern	int		tokenizec(char*, char**, int, char*);

/* cd stuff */
typedef struct Msf Msf;	/* minute, second, frame */
struct Msf {
	int m;
	int s;
	int f;
};

typedef struct Track Track;
struct Track {
	Msf start;
	Msf end;
	ulong bstart;
	ulong bend;
	char *title;
};

enum {
	MTRACK = 64,
};
typedef struct Toc Toc;
struct Toc {
	int ntrack;
	int nchange;
	int changetime;
	int track0;
	Track track[MTRACK];
	char *title;
};

extern int msfconv(Fmt*);

#pragma	varargck	argpos	error	1
#pragma	varargck	argpos	ctlprint	2
#pragma	varargck	type		"M"	Msf

enum {	/* state */
	Sunknown,
	Splaying,
	Spaused,
	Scompleted,
	Serror,
};

typedef struct Cdstatus Cdstatus;
struct Cdstatus {
	int state;
	int track;
	int index;
	Msf abs;
	Msf rel;
};

typedef struct Drive Drive;
struct Drive {
	Window *w;
	Channel *cstatus;	/* chan(Cdstatus) */
	Channel *ctocdisp;	/* chan(Toc) */
	Channel *cdbreq;	/* chan(Toc) */
	Channel *cdbreply; /* chan(Toc) */
	Scsi *scsi;
	Toc toc;
	Cdstatus status;
};

int gettoc(Scsi*, Toc*);
void drawtoc(Window*, Drive*, Toc*);
void redrawtoc(Window*, Toc*);
void tocproc(void*);	/* Drive* */
void cddbproc(void*);	/* Drive* */
void cdstatusproc(void*);	/* Drive* */

extern int debug;

#define DPRINT if(debug)fprint
void acmeevent(Drive*, Window*, Event*);

int playtrack(Drive*, int, int);
int pause(Drive*);
int resume(Drive*);
int stop(Drive*);
int eject(Drive*);
int ingest(Drive*);

int markplay(Window*, ulong);
int setplaytime(Window*, char*);
void advancetrack(Drive*, Window*);


