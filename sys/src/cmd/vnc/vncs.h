typedef struct	Vncs	Vncs;

#include "region.h"

struct Vncs {
	Vnc;

	int		preferredencoding;
	int		usecopyrect;
	int		canmousewarp;
	int		mousewarpneeded;
	int		updaterequested;
	Region		updateregion;
	int		ndeadprocs;
	int		nprocs;
	int		cursorver;
	Point		cursorpos;
	Rectangle	cursorr;
	int		snarfvers;

        Memimage	*clientimage;
};

/* rre.c */
void	sendraw(Vncs *, Rectangle);
int	sendrre(Vncs *, Rectangle, int, int);
int	rrerects(Rectangle, int);
void	sendhextile(Vncs *, Rectangle);
