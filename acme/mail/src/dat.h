enum
{
	False,
	True,
	EVENTSIZE=256,
};

typedef	adt	Box;
typedef	adt	Mesg;

typedef	aggr	Event;

adt Win
{
extern	int	winid;
		int	addr;
		Biobuf *body;
		int	ctl;
		int	data;
		int	event;
		byte	buf[512];
		byte	*bufp;
		int	nbuf;

		void	wnew(*Win);
		void	wwritebody(*Win, byte *s, int n);
		void	wread(*Win, uint, uint, byte*);
		void	wclean(*Win);
		void	wname(*Win, byte*);
		void	wdormant(*Win);
		void	wevent(*Win, Event*);
		void	wshow(*Win);
		void	wtagwrite(*Win, byte*, int);
		void	wwriteevent(*Win, Event*);
		void	wslave(*Win, chan(Event));
		void	wreplace(*Win, byte*, byte*, int);
		void	wselect(*Win, byte*);
		void	wsetdump(*Win, byte*, byte*);
		int	wdel(*Win, int);
		(int, byte*)	wreadall(*Win);

intern	void	ctlwrite(*Win, byte*);
intern	int	getec(*Win);
intern	int	geten(*Win);
intern	int	geter(*Win, byte*, int*);
intern	int	openfile(*Win, byte*);
intern	void	openbody(*Win, int);
};

adt Mesg
{
		Win;
extern	int		id;
extern	byte		*hdr;
extern	byte		*realhdr;
extern	byte		*replyto;
extern	byte		*text;
extern	byte		*subj;
extern	Mesg	*next;
extern	int		lhdr;
extern	int		ltext;
extern	int		lrealhdr;
extern	int		lline1;
		Box		*box;
		int		isopen;
		int		posted;

		Mesg*	read(Box*);
		void		open(*Mesg);
		void		slave(*Mesg);
		void		free(*Mesg);
		void		save(*Mesg, byte*);
		void		mkreply(*Mesg);
		void		mkmail(Box*, byte*);
		void		putpost(*Mesg, Event*);

intern	int		command(*Mesg, byte*);
intern	void		send(*Mesg);
};

adt Box
{
		Win;
		int		nm;
		int		readonly;
		Mesg	*m;
extern	byte		*file;
		Biobuf	*io;
		int		clean;
extern	uint		len;
extern	chan(Mesg*)	cdel;
extern	chan(Event)	cevent;
extern	chan(int)		cmore;
	
		byte		*line;
		int		linelen;
		int		freeline;
		byte		*peekline;
		int		peeklinelen;
		int		peekfreeline;

		Box*		read(byte*, int);
		void		readmore(*Box);
		(int,byte*)	readline(*Box);
		void		unreadline(*Box);
		void		slave(*Box);
		void		mopen(*Box, int);
		void		rewrite(*Box);
		void		mdel(*Box, Mesg*);
		void		event(*Box, Event);

intern	int		command(*Box, byte*);
};

aggr Event
{
	int	c1;
	int	c2;
	int	q0;
	int	q1;
	int	flag;
	int	nb;
	int	nr;
	byte	b[EVENTSIZE*UTFmax+1];
	Rune	r[EVENTSIZE+1];
};

Box	*mbox;
byte	*mboxfile;
byte	*usermboxfile;
byte	*usermboxdir;
byte	*user;
byte	*date;
