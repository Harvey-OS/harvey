typedef struct	Rlist Rlist;
typedef struct	Vncs	Vncs;

struct Rlist
{
	Rectangle	bbox;
	int	maxrect;
	int	nrect;
	Rectangle *rect;
};

struct Vncs
{
	Vnc;

	Vncs	*next;
	char		remote[NETPATHLEN];
	char		netpath[NETPATHLEN];

	char		*encname;
	int		(*countrect)(Vncs*, Rectangle);
	int		(*sendrect)(Vncs*, Rectangle);
	int		copyrect;
	int		canwarp;
	int		needwarp;
	Point		warppt;

	int		updaterequest;
	Rlist		rlist;
	int		ndead;
	int		nproc;
	int		cursorver;
	Point		cursorpos;
	Rectangle	cursorr;
	int		snarfvers;

	Memimage	*image;
	ulong	imagechan;
};

/* rre.c */
int	countcorre(Vncs*, Rectangle);
int	counthextile(Vncs*, Rectangle);
int	countraw(Vncs*, Rectangle);
int	countrre(Vncs*, Rectangle);
int	sendcorre(Vncs*, Rectangle);
int	sendhextile(Vncs*, Rectangle);
int	sendraw(Vncs*, Rectangle);
int	sendrre(Vncs*, Rectangle);

/* rlist.c */
void addtorlist(Rlist*, Rectangle);
void freerlist(Rlist*);
