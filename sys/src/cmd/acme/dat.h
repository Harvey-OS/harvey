enum
{
	Qdir,
	Qacme,
	Qcons,
	Qconsctl,
	Qindex,
	Qlabel,
	Qnew,

	QWaddr,
	QWbody,
	QWctl,
	QWdata,
	QWevent,
	QWtag,
	QMAX,
};

enum
{
	Blockincr =	256,
	Maxblock = 	8*1024,
	NRange =		10,
	Infinity = 		0x7FFFFFFF,	/* huge value for regexp address */
};

typedef	adt	Column;
typedef	adt	Row;
typedef	adt	Text;
typedef	adt	Window;
typedef	adt	Xfid;
typedef	adt	Reffont;
typedef	aggr	Dirlist;
typedef	aggr	Range;
typedef	aggr	Mntdir;

aggr Range
{
	int	q0;
	int	q1;
};

aggr Block
{
	uint		addr;	/* disk address in bytes */
	union
	{
		uint	n;		/* number of used runes in block */
		Block	*next;	/* pointer to next in free list */
	};
};

adt Disk
{
		int		fd;
		uint	addr;	/* length of temp file */
		Block	*free[Maxblock/Blockincr+1];

		Disk*	init();
		Block*	new(*Disk, uint);
		void	release(*Disk, Block*);
		void	read(*Disk, Block*, Rune*, uint);
		void	write(*Disk, Block**, Rune*, uint);
};

adt Buffer
{
extern	uint	nc;
		Rune	*c;		/* cache */
		uint	cnc;	/* bytes in cache */
		uint	cmax;	/* size of allocated cache */
		uint	cq;		/* position of cache */
		int		cdirty;	/* cache needs to be written */
		uint	cbi;	/* index of cache Block */
		Block	**bl;	/* array of blocks */
		uint	nbl;	/* number of blocks */

		void	insert(*Buffer, uint, Rune*, uint);
		void	delete(*Buffer, uint, uint);
		uint	load(*Buffer, uint, int, int*);
		void	read(*Buffer, uint, Rune*, uint);
		void	close(*Buffer);
		void	reset(*Buffer);
intern	void	sizecache(*Buffer, uint);
intern	void	flush(*Buffer);
intern	void	setcache(*Buffer, uint);
intern	void	addblock(*Buffer, uint, uint);
intern	void	delblock(*Buffer, uint);
};

adt File
{
extern	Buffer;			/* the data */
extern	Buffer	delta;	/* transcript of changes */
extern	Buffer	epsilon;/* inversion of delta for redo */
extern	Rune		*name;	/* name of associated file */
extern	int		nname;	/* size of name */
	
extern	int		seq;	/* if seq==0, File acts like Buffer */
extern	int		mod;
extern	Text		*curtext;	/* most recently used associated text */
extern	Text		**text;	/* list of associated texts */
extern	int		ntext;
extern	int		dumpid;	/* used in dumping zeroxed windows */

		File*	addtext(*File, Text*);
		void	deltext(*File, Text*);
		void	insert(*File, uint, Rune*, uint);
		void	delete(*File, uint, uint);
		uint	load(*File, uint, int, int*);
		void	setname(*File, Rune*, int);
		(uint, uint)	undo(*File, int, uint, uint);
		void	mark(*File);
		void	reset(*File);
intern	void	close(*File);
intern	void	undelete(*File, Buffer*, uint, uint);
intern	void	uninsert(*File, Buffer*, uint, uint);
intern	void	unsetname(*File, Buffer*);
};

enum	/* Text.what */
{
	Columntag,
	Rowtag,
	Tag,
	Body,
};

adt Text
{
extern	File		*file;
extern	Frame;
extern	Reffont	*reffont;
extern	uint	org;
extern	uint	q0;
extern	uint	q1;
extern	int	what;
extern	Window	*w;
extern	Rectangle scrollr;
extern	Rectangle lastsr;
extern	Rectangle all;
extern	Row		*row;
extern	Column	*col;

extern	uint	eq0;	/* start of typing for ESC */
		uint	cq0;	/* cache position */
extern	int		ncache;	/* storage for insert */
		int		ncachealloc;
		Rune	*cache;
		int	nofill;

		void	init(*Text, File*, Rectangle, Reffont*);
		void	redraw(*Text, Rectangle, Font*, Bitmap*, int);
		void	insert(*Text, uint, Rune*, uint, int);
		(uint, int)	bsinsert(*Text, uint, Rune*, uint, int);
		void	delete(*Text, uint, uint, int);
		uint	load(*Text, uint, byte*);
		void	type(*Text, Rune);
		void	select(*Text);
		(int, Text*)	select2(*Text, uint*, uint*);
		int	select3(*Text, uint*, uint*);
		void	setselect(*Text, uint, uint);
		void	show(*Text, uint, uint);
		void	fill(*Text);
		void	commit(*Text, int);
		void	scroll(*Text, int);
		void	setorigin(*Text, uint, int);
		void	scrdraw(*Text);
		Rune	readc(*Text, uint);
		void	reset(*Text);
		int	reshape(*Text, Rectangle);
		void	close(*Text);
		void	framescroll(*Text, int);
intern	int	select23(*Text, uint*, uint*, Bitmap*, int);
intern	uint	backnl(*Text, uint, uint);
intern	int	bswidth(*Text, Rune);
intern	void	doubleclick(*Text, uint*, uint*);
intern	int	clickmatch(*Text, int, int, int, uint*);
intern	void	columnate(*Text, Dirlist**, int);
};

adt Window
{
		QLock;
		Ref;
extern	Text		tag;
extern	Text		body;
extern	Rectangle	r;
extern	byte		isdir;
extern	byte		isscratch;
extern	byte		filemenu;
extern	byte		dirty;
extern	int		id;
extern	Range	addr;
extern	Range	limit;
extern	byte		nopen[QMAX];
extern	byte		nomark;
extern	byte		noscroll;
extern	Column	*col;
extern	Xfid		*eventx;
extern	byte		*events;
extern	int		nevents;
extern	int		owner;
extern	int		maxlines;
extern	Dirlist	**dlp;
extern	int		ndl;
extern	int		putseq;
extern	int		nincl;
extern	Rune		**incl;
extern	Reffont	*reffont;
extern	QLock	ctllock;
extern	uint		ctlfid;
extern	byte		*dumpstr;
extern	byte		*dumpdir;
extern	int		dumpid;

		void	init(*Window, Window*, Rectangle);
		void	lock(*Window, int);
		void	lock1(*Window, int);
		void	unlock(*Window);
		void	type(*Window, Text*, Rune);
		void	undo(*Window, int);
		void	setname(*Window, Rune*, int);
		void	settag(*Window);
		void	settag1(*Window);
		void	commit(*Window, Text*);
		int	reshape(*Window, Rectangle, int);
		void	close(*Window);
		void	delete(*Window);
		int	clean(*Window, int);
		void	dirfree(*Window);
		void	event(*Window, byte*, ...);
		void	mousebut(*Window);
		void	addincl(*Window, Rune*, int);
		void	cleartag(*Window);
		void	ctlprint(*Window, byte*);
};

adt Column
{
extern	Rectangle r;
extern	Text	tag;
extern	Row		*row;
extern	Window	**w;
extern	int		nw;
		int		safe;

		void	init(*Column, Rectangle);
		Window*	add(*Column, Window*, Window*, int);
		void	close(*Column, Window*, int);
		void	closeall(*Column);
		void	reshape(*Column, Rectangle);
		Text*	which(*Column, Point);
		void	dragwin(*Column, Window*, int);
		void	grow(*Column, Window*, int);
		int	clean(*Column);
		void	sort(*Column);
		void	mousebut(*Column);
};

adt Row
{
		QLock;
extern	Rectangle r;
extern	Text	tag;
extern	Column	**col;
extern	int		ncol;

		void	init(*Row, Rectangle);
		Column*	add(*Row, Column *c, int);
		void	close(*Row, Column*, int);
		Text*	which(*Row, Point);
		Column*	whichcol(*Row, Point);
		void	reshape(*Row, Rectangle);
		Text*	type(*Row, Rune, Point);
		void	dragcol(*Row, Column*, int but);
		int		clean(*Row);
		void		dump(*Row, byte*);
		void		load(*Row, byte*, int);
};

aggr Timer
{
	int		dt;
	chan(int)	c;
	Timer	*next;
};

aggr Command
{
	int		pid;
	Rune		*name;
	int		nname;
	byte		*text;
	byte		**av;
	Mntdir	*md;
	Command	*next;
};

aggr Dirtab
{
	byte	*name;
	uint	qid;
	uint	perm;
};

aggr Mntdir
{
	int		id;
	int		ref;
	Rune		*dir;
	int		ndir;
	Mntdir	*next;
	int		nincl;
	Rune		**incl;
};

aggr Fid
{
	uint		fid;
	int		busy;
	int		open;
	Qid		qid;
	Window	*w;
	Dirtab	*dir;
	Fid		*next;
	Mntdir	*mntdir;
	int		nrpart;
	byte		rpart[UTFmax];
};

adt Xfid
{
		uint	tid;
extern	Fcall;
extern	Xfid	*next;
extern	chan(void(*)(Xfid*))	c;
extern	Fid	*f;
extern	byte	*buf;
		int	flushed;

		void		ctl(*Xfid);
		void		flush(*Xfid);
		void		walk(*Xfid);
		void		open(*Xfid);
		void		close(*Xfid);
		void		read(*Xfid);
		void		write(*Xfid);

		void		ctlwrite(*Xfid, Window*);
		void		eventread(*Xfid, Window*);
		void		eventwrite(*Xfid, Window*);
		void		indexread(*Xfid);
		void		utfread(*Xfid, Text*, uint, uint);
		int		runeread(*Xfid, Text*, uint, uint);
};

adt
Reffont
{
		Ref;
extern	Font		*f;

		Reffont*	get(int, int, int, byte*);
		void		close(*Reffont);
};

aggr Rangeset
{
	Range	r[NRange];
};

aggr Dirlist
{
	Rune	*r;
	int		nr;
	int		wid;
};

aggr Expand
{
	uint	q0;
	uint	q1;
	Rune	*name;
	int	nname;
	byte	*bname;
	int	jump;
	Text	*at;
	int	a0;
	int	a1;
};

enum
{
	/* fbufalloc() guarantees room off end of BUFSIZE */
	BUFSIZE = MAXRPC,	/* size from fbufalloc() */
	RBUFSIZE = BUFSIZE/sizeof(Rune),
	EVENTSIZE = 256,
	Scrollwid = 12,	/* width of scroll bar */
	Scrollgap = 4,	/* gap right of scroll bar */
	Margin = 4,	/* margin around text */
	Border = 2,	/* line between rows, cols, windows */
	Maxtab = 4,	/* size of a tab, in units of the '0' character */
};

#define	QID(w,q)	((w<<8)|(q))
#define	WIN(q)	((((q).path&~CHDIR)>>8) & 0xFFFFFF)
#define	FILE(q)	((q).path & 0xFF)

enum
{
	FALSE,
	TRUE,
	XXX,
};

enum
{
	Null		= '-',
	Delete	= 'd',
	Insert	= 'i',
	Filename	= 'f',
};

uint		seq;

Mouse		mouse;
Bitmap		*darkgrey;
Bitmap		*lightgrey;
Reffont		reffont;
Bitmap		*modbutton;
Bitmap		*colbutton;
Bitmap		*button;
Cursor		boxcursor;
Row			row;
int			timerpid;
Disk			*disk;
Text			*seltext;
Text			*argtext;
Text			*mousetext;	/* global because Text.close needs to clear it */
Text			*typetext;		/* global because Text.close needs to clear it */
Text			*barttext;		/* shared between mousetask and keyboardtask */
int			bartflag;
Column		*activecol;
Rectangle		nullrect;
int			fsyspid;
byte			*cputype;
byte			*objtype;
byte			*home;
byte			*fontnames[2];

chan(Rune)[10]	ckeyboard;
chan(Mouse)		cmouse;
chan(Waitmsg)	cwait;
chan(Command*)	ccommand;
chan(Rune*)	ckill;
chan(Xfid*)	cxfidalloc;
chan(Xfid*)	cxfidfree;
chan(int)		*mouseexit0;
chan(int)		*mouseexit1;
chan(byte*)	cerr;
