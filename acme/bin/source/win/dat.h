typedef struct Fsevent Fsevent;
typedef struct Event Event;
typedef struct Message Message;
typedef struct Window Window;

enum
{
	STACK		= 8192,
	NPIPEDATA	= 8000,
	NPIPE		= NPIPEDATA+32,
	/* EVENTSIZE is really 256 in acme, but we use events internally and want bigger buffers */
	EVENTSIZE	= 8192,
	NEVENT		= 5,
};

struct Fsevent
{
	int	type;
	void	*r;
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
	int		body;

	/* event input */
	char		buf[512];
	char		*bufp;
	int		nbuf;
	Event	e[NEVENT];

	int		id;
	int		open;
	Channel	*cevent;
};

extern	Window*	newwindow(void);
extern	int		winopenfile(Window*, char*);
extern	void		wintagwrite(Window*, char*, int);
extern	void		winname(Window*, char*);
extern	void		winwriteevent(Window*, Event*);
extern	int		winread(Window*, uint, uint, char*);
extern	int		windel(Window*, int);
extern	void		wingetevent(Window*, Event*);
extern	void		wineventproc(void*);
extern	void		winclean(Window*);
extern	int		winselect(Window*, char*, int);
extern	int		winsetaddr(Window*, char*, int);
extern	void		windormant(Window*);
extern	void		winsetdump(Window*, char*, char*);

extern	void		ctlprint(int, char*, ...);
extern	void*	emalloc(uint);
extern	char*	estrdup(char*);
extern	char*	estrstrdup(char*, char*);
extern	char*	egrow(char*, char*, char*);
extern	char*	eappend(char*, char*, char*);
extern	void		error(char*, ...);

extern	void		startpipe(void);
extern	void		sendit(char*);
extern	void		execevent(Window *w, Event *e, int (*)(Window*, char*));

extern	void		mountcons(void);
extern	void		fsloop(void*);

extern	int		newpipewin(int, char*);
extern	void		startpipe(void);
extern	int		pipecommand(Window*, char*);
extern	void		pipectl(void*);

#pragma	varargck	argpos	error	1
#pragma	varargck	argpos	ctlprint	2

extern	Window	*win;
extern	Channel	*fschan, *writechan;

