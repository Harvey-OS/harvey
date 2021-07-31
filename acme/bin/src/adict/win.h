enum
{
	False,
	True,
	EVENTSIZE=256,
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

		int     dead(*Win);
		void	wnew(*Win);
		void	wwritebody(*Win, byte *s, int n);
		void	wread(*Win, uint, uint, byte*);
		void	wclean(*Win);
		void	wname(*Win, byte*);
		void	wdormant(*Win);
		void	wevent(*Win, Event*);
		void	wtagwrite(*Win, byte*, int);
		void	wwriteevent(*Win, Event*);
		void	wslave(*Win, chan(Event));
		void	wreplace(*Win, byte*, byte*, int);
		void	wselect(*Win, byte*);
		int	wdel(*Win);
		(int, byte*)	wreadall(*Win);

intern	void	ctlwrite(*Win, byte*);
intern	int	getec(*Win);
intern	int	geten(*Win);
intern	int	geter(*Win, byte*, int*);
intern	int	openfile(*Win, byte*);
intern	void	openbody(*Win, int);
};

