#include	<u.h>
#include	<libc.h>
#include	<draw.h>
#include	<event.h>

enum
{
	But1		= 1<<0,
	But2		= 1<<1,
	But3		= 1<<2,
	Ewait		= 1<<4,

	Dselect		= 1<<0,
	Dvolume		= 1<<1,
	Dcursong	= 1<<2,
	Dreshape	= 1<<3,
	Dscroll		= 1<<4,

	Tunk		= 0,
	Tlab,
	Tlco,
	Tmco,
	Trco,
	Tabv,
	Tblo,
	Tscr,


	Npage		= 4000,		// max number of pages
	Pagesize	= 40,		// big lists broken into chunks
	Nabove		= 100,		// lines above the bar
	Nbelow		= 300,		// lines below the bar
	Nsearch		= 400,		// number of different searches in history
	Strsize		= 100,

	Oroot		= 46,
	Onone		= 0xff000000UL,
	Oqueue,
	Osearch		= Oqueue + Npage,
	Omissing	= Osearch + Nsearch*Npage,
	Oplaying,
};

typedef	struct	Ibuf	Ibuf;
typedef	struct	Rectangle	Box;
typedef	struct	Fio	Fio;

#pragma incomplete Ibuf

struct	Fio
{
	int	f;
	char*	p;
	char*	ep;
	char	buf[1024];
};

long	above[Nabove];
long	below[Nbelow];
long	queue[Npage];
long	search[Pagesize];
long	marks[Pagesize];
long	bschar[256*2];
long	oplaying;
long	strings;
int	abovdisp;
int	nabove;
int	nbelow;
int	nqueue;
int	nsearch;
int	nmark;
int	now;
int	aflag;
long	nstrings;
char*	rootbin;
char*	rootaudio;
char*	devaudio;
char*	devvolume;
int	mflag;

char	nowplaying[300];
char	markfile[300];
Ibuf*	bin;
int	error;
Event	ev;
int	random;
int	resetline;
long	totalmal;
ulong	update;
int	searchno;
char*	searches[Nsearch];

void	fixuproot(void);
void	guiinit(void);
void	guikbd(int);
void	guimouse(Mouse);
void	guiupdate(ulong);
void	initwait(void);
void*	mal(long);
void	mkgui(void);
void	playsong(char*, char*, char*);
int	songstopped(uchar*);
void	stopsong(void);
char*	strdict(char*);

void	doexit(void);

char*	tstring(long);
void	expand(long, int);
void	play(long);
void	mark(long);
int	get2(Ibuf*, long);
long	get4(Ibuf*, long);
char*	gets(Ibuf*, long);
void	firstsong(void);
void	skipsong(int);
void	donowplay(void);
void	execute(char*);
void	lookup(char*, long);
long	bsearch(char*);
void	readmark(char*, long);
int	openmark(char*);
int	dosearch(long);
int	doqueue(long);
char*	searchname(long);
char*	queuename(long);

int	isoplaying(long);
int	isonone(long);
int	isoqueue(long);
int	isosearch(long);
int	isomissing(long);
int	isospecial(long);

Ibuf*	Iopen(char*, int);
long	Iseek(Ibuf*, long, int);
int	Igetc(Ibuf*);
int	Igetc1(Ibuf*, long);
