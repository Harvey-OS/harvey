#include <u.h>
#include <libc.h>
#include <bio.h>
#include <thread.h>

/* acme */
typedef struct Event Event;
typedef struct Window Window;

enum
{
	STACK		= 8192,
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

	int		warned;
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
extern	int		winisdirty(Window*);
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

typedef struct Treq Treq;
typedef struct Wiki Wiki;

struct Treq {
	char *title;
	Channel *c;	/* chan(int) */
};

struct Wiki {
	QLock;
	int isnew;
	int special;
	char *arg;
	char *addr;
	int n;
	int dead;
	Window *win;
	ulong time;
	int linked;
	Wiki *next;
	Wiki *prev;
};

extern int debug;
extern int mapfd;
extern char *email;
extern char *dir;

void wikinew(char*);
int wikiopen(char*, char*);
int wikiput(Wiki*);
void wikiget(Wiki*);
int wikidiff(Wiki*);

