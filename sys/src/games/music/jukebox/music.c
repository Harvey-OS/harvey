#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <keyboard.h>
#include <mouse.h>
#include <control.h>
#include "colors.h"
#include "client.h"
#include "playlist.h"
#include "../debug.h"

int	debug = 0; //DBGSERVER|DBGPUMP|DBGSTATE|DBGPICKLE|DBGPLAY;

char	usage[] = "Usage: %s [-d mask] [-t] [-w]\n";

typedef struct But {
	char	*name;
	Control	*ctl;
} But;

typedef struct Simpleitem {
	char	*address;
	char	*data;
} Simpleitem;

typedef struct Multiitem {
	char	*address;
	int	ndata;
	char	**data;
} Multiitem;

enum {
	WinBrowse,
	WinPlay,
	WinPlaylist,
	WinError,
	Topselect = 0x7fffffff,

	Browsedepth = 63,
};

typedef enum {
	PlayIdle,
	PlayStart,
	Playing,
	PlayPause,
} Playstate;

typedef enum {
	User,
	Troot,
	Rroot,
	Tchildren,
	Rchildren,
	Tparent,
	Rparent,
	Tinfo,
	Rinfo,
	Tparentage,
	Rparentage,
	Tplay,
	Rplay,
} Srvstate;

enum {
	Exitbutton,
	Pausebutton,
	Playbutton,
	Stopbutton,
	Prevbutton,
	Nextbutton,
	Rootbutton,
	Deletebutton,
	Helpbutton,
	Volume,
	Browsetopwin,
	Browsebotwin,
	Browsebotscr,
	Playevent,
	Playlistwin,
	Nalt,
};

But buts[] = {
	[Exitbutton] =		{"skull", nil},
	[Pausebutton] =		{"pause", nil},
	[Playbutton] =		{"play", nil},
	[Stopbutton] =		{"stop", nil},
	[Prevbutton] =		{"prev", nil},
	[Nextbutton] =		{"next", nil},
	[Rootbutton] =		{"root", nil},
	[Deletebutton] =	{"trash", nil},
	[Helpbutton] =		{"question", nil},
};

struct tab {
	char *tabname;
	char *winname;
	Control *tab;
	Control *win;
} tabs[4] = {
	[WinBrowse] =	{"Browse",	"browsewin",	nil, nil},
	[WinPlay] =	{"Playing",	"playwin",	nil, nil},
	[WinPlaylist] =	{"Playlist",	"listwin",	nil, nil},
	[WinError] =	{"Errors",	"errorwin",	nil, nil},
};

char *helptext[] = {
	"Buttons, left to right:",
	"    Exit: exit jukebox",
	"    Pause: pause/resume playback",
	"    Play: play selection in Playlist",
	"    Stop: stop playback",
	"    Prev: play previous item in Playlist",
	"    Next: play next item in Playlist",
	"    Root: browse to root of database tree",
	"    Delete: empty Playlist, reread database",
	"    Help: show this window",
	"",
	"Browse window: (click tab to bring forward)",
	"  Top window displays current item",
	"  Bottom window displays selectable subitems",
	"  Mouse commands:",
	"    Left: selected subitem becomes current",
	"    Right: parent of current item becomes current",
	"    Middle: add item or subitem to Playlist",
	"",
	"Playing window",
	"  Displays item currently playing",
	"",
	"Playlist window",
	"  Displays contents of Playlist",
	"  Mouse commands:",
	"    Left: select item",
	"    (then click the play button)",
	"",
	"Error window",
	"  Displays error messages received from player",
	"  (e.g., can't open file)",
	nil,
};

struct Browsestack {
	char	*onum;
	int	scrollpos;
} browsestack[Browsedepth];
int browsesp;	/* browse stack pointer */
int browseline;	/* current browse position */

Control		*vol;
Control		*browsetopwin;
Control		*browsebotwin;
Control		*playlistwin;
Control		*errortext;
Control		*browsetopscr;
Control		*browsebotscr;

Playstate	playstate;

ulong		playingbuts = 1<<Pausebutton | 1<<Stopbutton | 1<<Prevbutton | 1<<Nextbutton;
ulong		activebuts;

int		tabht;
Image		*vol1img;
Image		*vol2img;

int		resizeready;
int		borderwidth = 1;
int		butht, butwid;
int		errorlines;

int		tflag;
int		pflag;

Controlset	*cs;

char		*root;
Multiitem	parent;
Simpleitem	children[2048];
int		nchildren;

int		selected;

Channel		*playevent;

void
readbuts(void)
{
	static char str[32], file[64];
	But *b;
	int fd;
	Image *img, *mask;

	for(b = buts; b < &buts[nelem(buts)]; b++){
		sprint(file, "%s/%s.bit", ICONPATH, b->name);
		if((fd = open(file, OREAD)) < 0)
			sysfatal("open: %s: %r", file);
		mask = readimage(display, fd, 0);
		close(fd);
		butwid = Dx(mask->r);
		butht = Dy(mask->r);
		b->ctl = createbutton(cs, b->name);
		chanprint(cs->ctl, "%q align center", b->name);
		chanprint(cs->ctl, "%q border 0", b->name);

		img = allocimage(display, mask->r, screen->chan, 0, 0xe0e0ffff);
		draw(img, img->r, darkgreen, mask, mask->r.min);
		sprint(str, "%s.active", b->name);
		namectlimage(img, str);

		img = allocimage(display, mask->r, screen->chan, 0, 0xe0e0ffff);
		draw(img, img->r, lightblue, mask, mask->r.min);
		sprint(str, "%s.passive", b->name);
		namectlimage(img, str);

		chanprint(cs->ctl, "%q image %q", b->name, str);
		sprint(str, "%s.mask", b->name);
		namectlimage(mask, str);
		chanprint(cs->ctl, "%q mask %q", b->name, str);
		chanprint(cs->ctl, "%q light red", b->name);
		chanprint(cs->ctl, "%q size %d %d %d %d", b->name, butwid, butht, butwid, butht);
	}
}

void
activatebuttons(ulong mask)
{	// mask bit i corresponds to buts[i];
	ulong bit;
	But *b;
	static char str[40];
	int i;

	for(i = 0; i < nelem(buts); i++){
		b = &buts[i];
		bit = 1 << i;
		if((mask & bit) && (activebuts & bit) == 0){
			// button was `deactive'
			activate(b->ctl);
			activebuts |= bit;
			sprint(str, "%s.active", b->name);
			chanprint(cs->ctl, "%q image %q", b->name, str);
			chanprint(cs->ctl, "%q show", b->name);
		}
	}
}

void
deactivatebuttons(ulong mask)
{	// mask bit i corresponds with buts[i];
	ulong bit;
	But *b;
	static char str[40];
	int i;

	for(i = 0; i < nelem(buts); i++){
		b = &buts[i];
		bit = 1 << i;
		if((mask & bit) && (activebuts & bit)){
			// button was active
			deactivate(b->ctl);
			activebuts &= ~bit;
			sprint(str, "%s.passive", b->name);
			chanprint(cs->ctl, "%q image %q", b->name, str);
			chanprint(cs->ctl, "%q show", b->name);
		}
	}
}

void
resizecontrolset(Controlset *){
	static Point pol[3];
	char *p;

	if(getwindow(display, Refbackup) < 0)
		sysfatal("getwindow");
	draw(screen, screen->r, bordercolor, nil, screen->r.min);
	if(!resizeready)
		return;
#ifndef REPLACESEMANTICS
	if(vol1img)
		chanprint(cs->ctl, "volume image darkgreen");
	if(vol2img)
		chanprint(cs->ctl, "volume indicatorcolor red");
	chanprint(cs->ctl, "wholewin size");
	chanprint(cs->ctl, "wholewin rect %R", screen->r);

	chanprint(cs->ctl, "sync");
	p = recvp(cs->data);
	if(strcmp(p, "sync"))
		sysfatal("huh?");
	free(p);

	if(vol1img){
		freectlimage("volume.img");
		freeimage(vol1img);
	}
	if(vol2img){
		freectlimage("indicator.img");
		freeimage(vol2img);
	}
	vol1img = allocimage(display, vol->rect, screen->chan, 0, 0xe0e0ffff);
	vol2img = allocimage(display, vol->rect, screen->chan, 0, 0xe0e0ffff);
	pol[0] = Pt(vol->rect.min.x, vol->rect.max.y);
	pol[1] = Pt(vol->rect.max.x, vol->rect.min.y);
	pol[2] = vol->rect.max;
	fillpoly(vol1img, pol, 3, 0, darkgreen, ZP);
	fillpoly(vol2img, pol, 3, 0, red, ZP);
	namectlimage(vol1img, "volume.img");
	namectlimage(vol2img, "indicator.img");
	chanprint(cs->ctl, "volume image volume.img");
	chanprint(cs->ctl, "volume indicatorcolor indicator.img");
#else
	chanprint(cs->ctl, "wholewin size");
	chanprint(cs->ctl, "wholewin rect %R", screen->r);

	chanprint(cs->ctl, "sync");
	p = recvp(cs->data);
	if(strcmp(p, "sync"))
		sysfatal("huh?");
	free(p);

	new1img = allocimage(display, vol->rect, screen->chan, 0, 0xe0e0ffff);
	new2img = allocimage(display, vol->rect, screen->chan, 0, 0xe0e0ffff);
	pol[0] = Pt(vol->rect.min.x, vol->rect.max.y);
	pol[1] = Pt(vol->rect.max.x, vol->rect.min.y);
	pol[2] = vol->rect.max;
	fillpoly(new1img, pol, 3, 0, darkgreen, ZP);
	fillpoly(new2img, pol, 3, 0, red, ZP);
	namectlimage(new1img, "volume.img");
	namectlimage(new2img, "indicator.img");
	if(vol1img)
		freeimage(vol1img);
	else
		chanprint(cs->ctl, "volume image volume.img");
	if(vol2img)
		freeimage(vol2img);
	else
		chanprint(cs->ctl, "volume indicatorcolor indicator.img");
	vol1img = new1img;
	vol2img = new2img;
#endif
	chanprint(cs->ctl, "browsetopscr vis '%d'",
		Dy(controlcalled("browsetopscr")->rect)/romanfont->height);
	chanprint(cs->ctl, "browsebotscr vis '%d'",
		Dy(controlcalled("browsebotscr")->rect)/romanfont->height);
	chanprint(cs->ctl, "playscr vis '%d'",
		Dy(controlcalled("playscr")->rect)/romanfont->height);
	chanprint(cs->ctl, "playlistscr vis '%d'",
		Dy(controlcalled("playlistscr")->rect)/romanfont->height);
	chanprint(cs->ctl, "wholewin show");
}

void
maketab(void)
{
	int i;

	tabht = boldfont->height + 1 + borderwidth;

	createtab(cs, "tabs");

	for(i = 0; i < nelem(tabs); i++){
		tabs[i].tab = createtextbutton(cs, tabs[i].tabname);
		chanprint(cs->ctl, "%q size %d %d %d %d", tabs[i].tab->name,
			stringwidth(boldfont, tabs[i].tabname), tabht, 1024, tabht);
		chanprint(cs->ctl, "%q align uppercenter", tabs[i].tab->name);
		chanprint(cs->ctl, "%q font boldfont", tabs[i].tab->name);
		chanprint(cs->ctl, "%q image background", tabs[i].tab->name);
		chanprint(cs->ctl, "%q light background", tabs[i].tab->name);
		chanprint(cs->ctl, "%q pressedtextcolor red", tabs[i].tab->name);
		chanprint(cs->ctl, "%q textcolor darkgreen", tabs[i].tab->name);
		chanprint(cs->ctl, "%q mask transparent", tabs[i].tab->name);
		chanprint(cs->ctl, "%q text %q", tabs[i].tab->name, tabs[i].tabname);

		chanprint(cs->ctl, "tabs add %s %s", tabs[i].tabname, tabs[i].winname);
	}

	chanprint(cs->ctl, "tabs separation %d", 2);
	chanprint(cs->ctl, "tabs image background");
	chanprint(cs->ctl, "tabs value 0");
}

void
makeplaycontrols(void)
{
	int w;
	Control *playscr;

	w = stringwidth(romanfont, "Roll over Beethoven");
	playscr = createslider(cs, "playscr");
	chanprint(cs->ctl, "playscr size 12, 24, 12, 1024");
	createtext(cs, "playtext");
	chanprint(cs->ctl, "playtext size %d %d %d %d",
		w, 5*romanfont->height, 2048, 1024);

	chanprint(cs->ctl, "playscr format '%%s: playtext topline %%d'");
	controlwire(playscr, "event", cs->ctl);

	tabs[WinPlay].win = createrow(cs, tabs[WinPlay].winname);
	chanprint(cs->ctl, "%q add playscr playtext", tabs[WinPlay].win->name);
}

void
makebrowsecontrols(void)
{
	int w;

	w = stringwidth(romanfont, "Roll over Beethoven");
	browsetopscr = createslider(cs, "browsetopscr");
	chanprint(cs->ctl, "browsetopscr size 12, 24, 12, %d", 12*romanfont->height);
	browsetopwin = createtext(cs, "browsetopwin");
	chanprint(cs->ctl, "browsetopwin size %d %d %d %d",
		w, 3*romanfont->height, 2048, 12*romanfont->height);
	createrow(cs, "browsetop");
	chanprint(cs->ctl, "browsetop add browsetopscr browsetopwin");

	browsebotscr = createslider(cs, "browsebotscr");
	chanprint(cs->ctl, "browsebotscr size 12, 24, 12, 1024");
	browsebotwin = createtext(cs, "browsebotwin");
	chanprint(cs->ctl, "browsebotwin size %d %d %d %d",
		w, 3*romanfont->height, 2048, 1024);
	createrow(cs, "browsebot");
	chanprint(cs->ctl, "browsebot add browsebotscr browsebotwin");

	chanprint(cs->ctl, "browsetopscr format '%%s: browsetopwin topline %%d'");
	controlwire(browsetopscr, "event", cs->ctl);
//	chanprint(cs->ctl, "browsebotscr format '%%s: browsebotwin topline %%d'");
//	controlwire(browsebotscr, "event", cs->ctl);

	tabs[WinBrowse].win = createcolumn(cs, tabs[WinBrowse].winname);
	chanprint(cs->ctl, "%q add browsetop browsebot", tabs[WinBrowse].win->name);
}

void
makeplaylistcontrols(void)
{
	int w;
	Control *playlistscr;

	w = stringwidth(romanfont, "Roll over Beethoven");
	playlistscr = createslider(cs, "playlistscr");
	chanprint(cs->ctl, "playlistscr size 12, 24, 12, 1024");
	playlistwin = createtext(cs, "playlistwin");
	chanprint(cs->ctl, "playlistwin size %d %d %d %d",
		w, 5*romanfont->height, 2048, 1024);
//	chanprint(cs->ctl, "playlistwin selectmode multi");

	chanprint(cs->ctl, "playlistscr format '%%s: playlistwin topline %%d'");
	controlwire(playlistscr, "event", cs->ctl);

	tabs[WinPlaylist].win = createrow(cs, tabs[WinPlaylist].winname);
	chanprint(cs->ctl, "%q add playlistscr playlistwin", tabs[WinPlaylist].win->name);
}

void
makeerrorcontrols(void)
{
	int w;
	Control *errorscr;

	w = stringwidth(romanfont, "Roll over Beethoven");
	errorscr = createslider(cs, "errorscr");
	chanprint(cs->ctl, "errorscr size 12, 24, 12, 1024");
	errortext = createtext(cs, "errortext");
	chanprint(cs->ctl, "errortext size %d %d %d %d",
		w, 5*romanfont->height, 2048, 1024);
	chanprint(cs->ctl, "errortext selectmode multi");

	chanprint(cs->ctl, "errorscr format '%%s: errortext topline %%d'");
	controlwire(errorscr, "event", cs->ctl);

	tabs[WinError].win = createrow(cs, tabs[WinError].winname);
	chanprint(cs->ctl, "%q add errorscr errortext", tabs[WinError].win->name);
}

void
makecontrols(void)
{
	int i;

	cs = newcontrolset(screen, nil, nil, nil);

	// make shared buttons
	readbuts();

	vol = createslider(cs, "volume");
	chanprint(cs->ctl, "volume size %d %d %d %d", 2*butwid, butht, 2048, butht);
	chanprint(cs->ctl, "volume absolute 1");
	chanprint(cs->ctl, "volume indicatorcolor red");
	chanprint(cs->ctl, "volume max 100");
	chanprint(cs->ctl, "volume orient hor");
	chanprint(cs->ctl, "volume clamp low 1");
	chanprint(cs->ctl, "volume clamp high 0");
	chanprint(cs->ctl, "volume format '%%s volume %%d'");

	createrow(cs, "buttonrow");
	for(i = 0; i < nelem(buts); i++)
		chanprint(cs->ctl, "buttonrow add %s", buts[i].name);
	chanprint(cs->ctl, "buttonrow add volume");
	chanprint(cs->ctl, "buttonrow separation %d", borderwidth);
	chanprint(cs->ctl, "buttonrow image darkgreen");

	makebrowsecontrols();
	makeplaycontrols();
	makeplaylistcontrols();
	makeerrorcontrols();

	maketab();

	chanprint(cs->ctl, "%q image background", "text slider");
	chanprint(cs->ctl, "text font romanfont");
	chanprint(cs->ctl, "slider indicatorcolor darkgreen");
	chanprint(cs->ctl, "row separation %d", borderwidth);
	chanprint(cs->ctl, "row image darkgreen");
	chanprint(cs->ctl, "column separation %d", 2);
	chanprint(cs->ctl, "column image darkgreen");

	createcolumn(cs, "wholewin");
	chanprint(cs->ctl, "wholewin separation %d", borderwidth);
	chanprint(cs->ctl, "wholewin add buttonrow tabs");
	chanprint(cs->ctl, "wholewin image darkgreen");
	chanprint(cs->ctl, "%q image darkgreen", "column row");
}

void
makewindow(int dx, int dy, int wflag){
	int mountfd, fd, n;
	static char aname[128];
	static char rio[128] = "/mnt/term";
	char *args[6];

	if(wflag){
		/* find out screen size */
		fd = open("/mnt/wsys/screen", OREAD);
		if(fd >= 0 && read(fd, aname, 60) == 60){
			aname[60] = '\0';
			n = tokenize(aname, args, nelem(args));
			if(n != 5)
				fprint(2, "Not an image: /mnt/wsys/screen\n");
			else{
				n = atoi(args[3]) - atoi(args[1]);
				if(n <= 0 || n > 2048)
					fprint(2, "/mnt/wsys/screen very wide: %d\n", n);
				else
					if(n < dx) dx = n-1;
				n = atoi(args[4]) - atoi(args[2]);
				if(n <= 0 || n > 2048)
					fprint(2, "/mnt/wsys/screen very high: %d\n", n);
				else
					if(n < dy) dy = n-1;
			}
			close(fd);
		}
		n = 0;
		if((fd = open("/env/wsys", OREAD)) < 0){
			n = strlen(rio);
			fd = open("/mnt/term/env/wsys", OREAD);
			if(fd < 0)
				sysfatal("/env/wsys");
		}
		if(read(fd, rio+n, sizeof(rio)-n-1) <= 0)
			sysfatal("/env/wsys");
		mountfd = open(rio, ORDWR);
		if(mountfd < 0)
			sysfatal("open %s: %r", rio);
		snprint(aname, sizeof aname, "new -dx %d -dy %d", dx, dy);
		rfork(RFNAMEG);
		if(mount(mountfd, -1, "/mnt/wsys", MREPL, aname) < 0)
			sysfatal("mount: %r");
		if(bind("/mnt/wsys", "/dev", MBEFORE) < 0)
			sysfatal("mount: %r");
	}

	if(initdraw(nil, nil, "music") < 0)
		sysfatal("initdraw: %r");

	initcontrols();
	if(dx <= 320)
		colorinit("/lib/font/bit/lucidasans/unicode.6.font",
			"/lib/font/bit/lucidasans/boldunicode.8.font");
	else
		colorinit("/lib/font/bit/lucidasans/unicode.8.font",
			"/lib/font/bit/lucidasans/boldunicode.10.font");
	makecontrols();
	resizeready = 1;

	resizecontrolset(cs);
	if(debug & DBGCONTROL)
		fprint(2, "resize done\n");
}

void
setparent(char *addr)
{
	int i;

	if(parent.address)
		free(parent.address);
	parent.address = strdup(addr);
	for(i = 0; i < parent.ndata; i++)
		if(parent.data[i])
			free(parent.data[i]);
	parent.ndata = 0;
	if(parent.data){
		free(parent.data);
		parent.data = nil;
	}
	chanprint(cs->ctl, "browsetopwin clear");
	chanprint(cs->ctl, "browsetopscr max 0");
	chanprint(cs->ctl, "browsetopscr value 0");
}

void
addparent(char *str)
{
	parent.data = realloc(parent.data, (parent.ndata+1)*sizeof(char*));
	parent.data[parent.ndata] = strdup(str);
	parent.ndata++;
	chanprint(cs->ctl, "browsetopwin accumulate %q", str);
	chanprint(cs->ctl, "browsetopscr max %d", parent.ndata);
}

void
clearchildren(void)
{
	int i;

	for(i = 0; i < nchildren; i++){
		if(children[i].address)
			free(children[i].address);
		if(children[i].data)
			free(children[i].data);
	}
	nchildren= 0;
	chanprint(cs->ctl, "browsebotwin clear");
	chanprint(cs->ctl, "browsebotwin topline 0");
	chanprint(cs->ctl, "browsebotscr max 0");
	chanprint(cs->ctl, "browsebotscr value 0");
	selected = -1;
}

void
addchild(char *addr, char *data)
{
	children[nchildren].address = addr;
	children[nchildren].data = data;
	nchildren++;
	chanprint(cs->ctl, "browsebotwin accumulate %q", data);
	chanprint(cs->ctl, "browsebotscr max %d", nchildren);
}

static void
playlistselect(int n)
{
	if(playlist.selected >= 0 && playlist.selected < playlist.nentries){
		chanprint(cs->ctl, "playlistwin select %d 0", playlist.selected);
		deactivatebuttons(1<<Playbutton);
	}
	playlist.selected = n;
	if(playlist.selected < 0 || playlist.selected >= playlist.nentries)
		return;
	activatebuttons(1<<Playbutton);
	chanprint(cs->ctl, "playlistwin select %d 1", n);
	if(n >= 0 && n <= playlist.nentries - Dy(playlistwin->rect)/romanfont->height)
		n--;
	else
		n = playlist.nentries - Dy(playlistwin->rect)/romanfont->height + 1;
	if(n < 0) n = 0;
	if(n < playlist.nentries){
		chanprint(cs->ctl, "playlistwin topline %d",  n);
		chanprint(cs->ctl, "playlistscr value %d",  n);
	}
	chanprint(cs->ctl, "playlist show");
}

void
updateplaylist(int trunc)
{
	char *s;
	int fd;

	while(cs->ctl->s - cs->ctl->n < cs->ctl->s/4)
		sleep(100);
	if(trunc){
		playlistselect(-1);
		chanprint(cs->ctl, "playlistwin clear");
		chanprint(cs->ctl, "playlistwin topline 0");
		chanprint(cs->ctl, "playlistscr max 0");
		chanprint(cs->ctl, "playlistscr value 0");
		deactivatebuttons(1<<Playbutton | 1<<Deletebutton);
		chanprint(cs->ctl, "playlistwin show");
		chanprint(cs->ctl, "playlistscr show");
		s = smprint("%s/ctl", srvmount);
		if((fd = open(s, OWRITE)) >= 0){
			fprint(fd, "reread");
			close(fd);
		}
		free(s);
		return;
	}
	if(playlist.entry[playlist.nentries].onum){
		s = getoneliner(playlist.entry[playlist.nentries].onum);
		chanprint(cs->ctl, "playlistwin accumulate %q", s);
		free(s);
	}else
		chanprint(cs->ctl, "playlistwin accumulate %q", playlist.entry[playlist.nentries].file);
	playlist.nentries++;
	chanprint(cs->ctl, "playlistscr max %d", playlist.nentries);
	if(playlist.selected == playlist.nentries - 1)
		playlistselect(playlist.selected);
	activatebuttons(1<<Playbutton|1<<Deletebutton);
	chanprint(cs->ctl, "playlistscr show");
	chanprint(cs->ctl, "playlistwin show");
}

void
browseto(char *onum, int line)
{
	onum = strdup(onum);
	setparent(onum);
	clearchildren();
	fillbrowsetop(onum);
	chanprint(cs->ctl, "browsetop show");
	fillbrowsebot(onum);
	if(line){
		chanprint(cs->ctl, "browsebotscr value %d", line);
		chanprint(cs->ctl, "browsebotwin topline %d", line);
	}
	chanprint(cs->ctl, "browsebot show");
	free(onum);
}

void
browsedown(char *onum)
{
	if(browsesp == 0){
		/* Make room for an entry by deleting the last */
		free(browsestack[Browsedepth-1].onum);
		memmove(browsestack + 1, browsestack, (Browsedepth-1) * sizeof(browsestack[0]));
		browsesp++;
	}
	/* Store current position in current stack frame */
	assert(browsesp > 0 && browsesp < Browsedepth);
	browsestack[browsesp].onum = strdup(parent.address);
	browsestack[browsesp].scrollpos = browseline;
	browsesp--;
	browseline = 0;
	if(browsestack[browsesp].onum && strcmp(browsestack[browsesp].onum, onum) == 0)
		browseline = browsestack[browsesp].scrollpos;
	browseto(onum, browseline);
}

void
browseup(char *onum)
{
	if(browsesp == Browsedepth){
		/* Make room for an entry by deleting the first */
		free(browsestack[0].onum);
		memmove(browsestack, browsestack + 1, browsesp * sizeof(browsestack[0]));
		browsesp--;
	}
	/* Store current position in current stack frame */
	assert(browsesp >= 0 && browsesp < Browsedepth);
	browsestack[browsesp].onum = strdup(parent.address);
	browsestack[browsesp].scrollpos = browseline;
	browsesp++;
	browseline = 0;
	if(browsestack[browsesp].onum && strcmp(browsestack[browsesp].onum, onum) == 0)
		browseline = browsestack[browsesp].scrollpos;
	browseto(onum, browseline);
}

void
addplaytext(char *s)
{
	chanprint(cs->ctl, "playtext accumulate %q", s);
}

void
work(void)
{
	static char *eventstr, *args[64], *s;
	static char buf[4096];
	int a, n, i;
	Alt alts[] = {
	[Exitbutton] =		{buts[Exitbutton].ctl->event, &eventstr, CHANRCV},
	[Pausebutton] =		{buts[Pausebutton].ctl->event, &eventstr, CHANRCV},
	[Playbutton] =		{buts[Playbutton].ctl->event, &eventstr, CHANRCV},
	[Stopbutton] =		{buts[Stopbutton].ctl->event, &eventstr, CHANRCV},
	[Prevbutton] =		{buts[Prevbutton].ctl->event, &eventstr, CHANRCV},
	[Nextbutton] =		{buts[Nextbutton].ctl->event, &eventstr, CHANRCV},
	[Rootbutton] =		{buts[Rootbutton].ctl->event, &eventstr, CHANRCV},
	[Deletebutton] =	{buts[Deletebutton].ctl->event, &eventstr, CHANRCV},
	[Helpbutton] =		{buts[Helpbutton].ctl->event, &eventstr, CHANRCV},
	[Volume] =		{vol->event, &eventstr, CHANRCV},
	[Browsetopwin] =	{browsetopwin->event, &eventstr, CHANRCV},
	[Browsebotwin] =	{browsebotwin->event, &eventstr, CHANRCV},
	[Browsebotscr] =	{browsebotscr->event, &eventstr, CHANRCV},
	[Playevent] =		{playevent, &eventstr, CHANRCV},
	[Playlistwin] =		{playlistwin->event, &eventstr, CHANRCV},
	[Nalt] =		{nil, nil, CHANEND}
	};

	activate(vol);
	activate(controlcalled("tabs"));
	activatebuttons(1 << Exitbutton | 1 << Rootbutton | 1 << Helpbutton);
	
	root = getroot();
	setparent(root);
	clearchildren();
	addparent("Root");
	chanprint(cs->ctl, "browsetop show");
	fillbrowsebot(root);
	chanprint(cs->ctl, "browsebot show");

	eventstr = nil;
	selected = -1;

	for(;;){
		a = alt(alts);
		if(debug & DBGCONTROL)
			fprint(2, "Event: %s\n", eventstr);
		n = tokenize(eventstr, args, nelem(args));
		switch(a){
		default:
			sysfatal("Illegal event %d in work", a);
		case Volume:
			if(n != 3 || strcmp(args[0], "volume") || strcmp(args[1], "volume"))
				sysfatal("Bad Volume event[%d]: %s %s", n, args[0], args[1]);
			setvolume(args[2]);
			break;
		case Exitbutton:
			return;
		case Pausebutton:
			if(n != 3 || strcmp(args[0], "pause:") || strcmp(args[1], "value"))
				sysfatal("Bad Pausebutton event[%d]: %s %s", n, args[0], args[1]);
			if(strcmp(args[2], "0") == 0)
				fprint(playctlfd, "resume");
			else
				fprint(playctlfd, "pause");
			break;
		case Playbutton:
			if(n != 3 || strcmp(args[0], "play:") || strcmp(args[1], "value"))
				sysfatal("Bad Playbutton event[%d]: %s %s", n, args[0], args[1]);
			if(playlist.selected >= 0){
				fprint(playctlfd, "play %d", playlist.selected);
			}else
				fprint(playctlfd, "play");
			break;
		case Stopbutton:
			if(n != 3 || strcmp(args[0], "stop:") || strcmp(args[1], "value"))
				sysfatal("Bad Stopbutton event[%d]: %s %s", n, args[0], args[1]);
			if(strcmp(args[2], "0") == 0)
				chanprint(cs->ctl, "%q value 1", buts[Stopbutton].ctl->name);
			fprint(playctlfd, "stop");
			break;
		case Prevbutton:
			if(n != 3 || strcmp(args[0], "prev:") || strcmp(args[1], "value"))
				sysfatal("Bad Prevbutton event[%d]: %s %s", n, args[0], args[1]);
			if(strcmp(args[2], "0") == 0)
				break;
			chanprint(cs->ctl, "%q value 0", buts[Prevbutton].ctl->name);
			fprint(playctlfd, "skip -1");
			break;
		case Nextbutton:
			if(n != 3 || strcmp(args[0], "next:") || strcmp(args[1], "value"))
				sysfatal("Bad Nextbutton event[%d]: %s %s", n, args[0], args[1]);
			if(strcmp(args[2], "0") == 0)
				break;
			chanprint(cs->ctl, "%q value 0", buts[Nextbutton].ctl->name);
			fprint(playctlfd, "skip 1");
			break;
		case Playlistwin:
			if(debug & (DBGCONTROL|DBGPLAY))
				fprint(2, "Playlistevent: %s %s\n", args[0], args[1]);
			if(n != 4 || strcmp(args[0], "playlistwin:") || strcmp(args[1], "select"))
				sysfatal("Bad Playlistwin event[%d]: %s %s", n, args[0], args[1]);
			n = atoi(args[2]);
			if(n < 0 || n >= playlist.nentries)
				sysfatal("Selecting line %d of %d", n, playlist.nentries);
			if(playlist.selected >= 0 && playlist.selected < playlist.nentries){
				chanprint(cs->ctl, "playlistwin select %d 0", playlist.selected);
				chanprint(cs->ctl, "playlistwin show");
			}
			playlist.selected = -1;
			deactivatebuttons(1<<Playbutton);
			if(strcmp(args[3], "1") == 0)
				playlistselect(n);
			break;
		case Rootbutton:
			chanprint(cs->ctl, "%q value 0", buts[Rootbutton].ctl->name);
			setparent(root);
			clearchildren();
			addparent("Root");
			chanprint(cs->ctl, "browsetop show");
			fillbrowsebot(root);
			chanprint(cs->ctl, "browsebot show");
			break;
		case Deletebutton:
			if(n != 3 || strcmp(args[0], "trash:") || strcmp(args[1], "value"))
				sysfatal("Bad Deletebutton event[%d]: %s %s", n, args[0], args[1]);
			if(strcmp(args[2], "0") == 0)
				break;
			chanprint(cs->ctl, "%q value 0", buts[Deletebutton].ctl->name);
			sendplaylist(nil, nil);
			break;
		case Helpbutton:
			chanprint(cs->ctl, "%q value 0", buts[Helpbutton].ctl->name);
			if(errorlines > 0){
				chanprint(cs->ctl, "errortext clear");
				chanprint(cs->ctl, "errortext topline 0");
				chanprint(cs->ctl, "errorscr max 0");
				chanprint(cs->ctl, "errorscr value 0");
			}
			if(errorlines >= 0){
				for(i = 0; helptext[i]; i++)
					chanprint(cs->ctl, "errortext accumulate %q", helptext[i]);
				chanprint(cs->ctl, "errorscr max %d", i);
			}
			chanprint(cs->ctl, "errortext topline 0");
			chanprint(cs->ctl, "errorscr value 0");
			errorlines = -1;
			chanprint(cs->ctl, "tabs value %d", WinError);
			break;
		case Browsetopwin:
			if(n != 4 || strcmp(args[0], "browsetopwin:") || strcmp(args[1], "select"))
				sysfatal("Bad Browsetopwin event[%d]: %s %s", n, args[0], args[1]);
			if(strcmp(args[3], "0") == 0)
				break;
			chanprint(cs->ctl, "browsetopwin select %s 0", args[2]);
			selected = -1;
			if(strcmp(args[3], "2") == 0)
				doplay(parent.address);
			else if(strcmp(args[3], "4") == 0){
				s = getparent(parent.address);
				browsedown(s);
			}
			break;
		case Browsebotwin:
			if(n != 4 || strcmp(args[0], "browsebotwin:") || strcmp(args[1], "select"))
				sysfatal("Bad Browsebotwin event[%d]: %s %s", n, args[0], args[1]);
			n = atoi(args[2]);
			if(n < 0 || n >= nchildren)
				sysfatal("Selection out of range: %d [%d]", n, nchildren);
			if(strcmp(args[3], "0") == 0){
				selected = -1;
				break;
			}
			if(n < 0)
				break;
			chanprint(cs->ctl, "browsebotwin select %d 0", n);
			selected = n;
			if(selected >= nchildren)
				sysfatal("Select out of range: %d [0⋯%d)", selected, nchildren);
			if(strcmp(args[3], "1") == 0){
				browseup(children[selected].address);
			}else if(strcmp(args[3], "2") == 0)
				doplay(children[selected].address);
			else if(strcmp(args[3], "4") == 0)
				browsedown(getparent(parent.address));
			break;
		case Browsebotscr:
			browseline = atoi(args[2]);
			chanprint(cs->ctl, "browsebotwin topline %d", browseline);
			break;
		case Playevent:
			if(n < 3 || strcmp(args[0], "playctlproc:"))
				sysfatal("Bad Playevent event[%d]: %s", n, args[0]);
			if(debug & (DBGCONTROL|DBGPLAY))
				fprint(2, "Playevent: %s %s\n", args[1], args[2]);
			if(strcmp(args[1], "error") ==0){
				if(n != 4){
					fprint(2, "Playevent: %s: arg count: %d\n", args[1], n);
					break;
				}
				if(errorlines < 0){
					chanprint(cs->ctl, "errortext clear");
					chanprint(cs->ctl, "errortext topline 0");
					chanprint(cs->ctl, "errorscr max 0");
					chanprint(cs->ctl, "errorscr value 0");
					errorlines = 0;
				}
				n = errorlines;
				chanprint(cs->ctl, "errortext accumulate %q", args[3]);
				chanprint(cs->ctl, "errorscr max %d", ++errorlines);
				if(n >= 0 && n <= errorlines - Dy(errortext->rect)/romanfont->height)
					n--;
				else
					n = errorlines - Dy(errortext->rect)/romanfont->height + 1;
				if(n < 0) n = 0;
				if(n < errorlines){
					chanprint(cs->ctl, "errortext topline %d",  n);
					chanprint(cs->ctl, "errorscr value %d",  n);
				}
				chanprint(cs->ctl, "tabs value %d", WinError);
			}else if(strcmp(args[1], "play") ==0){
				chanprint(cs->ctl, "%q value 1", buts[Playbutton].ctl->name);
				chanprint(cs->ctl, "%q value 0", buts[Stopbutton].ctl->name);
				chanprint(cs->ctl, "%q value 0", buts[Pausebutton].ctl->name);
				playlistselect(strtoul(args[2], nil, 0));
				chanprint(cs->ctl, "playtext clear");
				chanprint(cs->ctl, "playtext topline 0");
				chanprint(cs->ctl, "playscr max 0");
				chanprint(cs->ctl, "playscr value 0");
				playstate = Playing;
				activatebuttons(playingbuts);
				qlock(&playlist);
				if(playlist.selected < playlist.nentries){
					fillplaytext(playlist.entry[playlist.selected].onum);
					chanprint(cs->ctl, "playscr max %d", n);
				}
				qunlock(&playlist);
				chanprint(cs->ctl, "playwin show");
			}else if(strcmp(args[1], "stop") ==0){
				chanprint(cs->ctl, "%q value 0", buts[Playbutton].ctl->name);
				chanprint(cs->ctl, "%q value 1", buts[Stopbutton].ctl->name);
				chanprint(cs->ctl, "%q value 0", buts[Pausebutton].ctl->name);
				playlistselect(strtoul(args[2], nil, 0));
				chanprint(cs->ctl, "%q show", tabs[WinPlaylist].winname);
				playstate = PlayIdle;
				deactivatebuttons(playingbuts);
			}else if(strcmp(args[1], "pause") ==0){
				activatebuttons(playingbuts);
				chanprint(cs->ctl, "%q value 1", buts[Playbutton].ctl->name);
				chanprint(cs->ctl, "%q value 0", buts[Stopbutton].ctl->name);
				if(playstate == PlayPause){
					chanprint(cs->ctl, "%q value 0", buts[Pausebutton].ctl->name);
					playstate = Playing;
				}else{
					chanprint(cs->ctl, "%q value 1", buts[Pausebutton].ctl->name);
					playstate = PlayPause;
				}
			}else if(strcmp(args[1], "exits") ==0){
				threadexits("exitevent");
			}else{
				fprint(2, "Unknown play event:");
				for(i=0; i<n; i++)
					fprint(2, " %s", args[i]);
				fprint(2, "\n");
			}
			break;
		}
		if(eventstr){
			free(eventstr);
			eventstr = nil;
		}
	}
}

void
threadmain(int argc, char *argv[]){
	int wflag;

	wflag = 0;
	ARGBEGIN{
	case 'd':
		debug = strtol(ARGF(), nil, 0);
		break;
	case 't':
		tflag = 1;
		break;
	case 'w':
		wflag = 1;
		break;
	default:
		sysfatal(usage, argv0);
	}ARGEND

	quotefmtinstall();

	if(tflag)
		makewindow(320, 320, wflag);
	else
		makewindow(480, 480, wflag);

	playlist.selected = -1;

	playctlfd = open(playctlfile, OWRITE);
	if(playctlfd < 0)
		sysfatal("%s: %r", playctlfile);
	proccreate(playlistproc, nil, 8192);
	playevent = chancreate(sizeof(char *), 1);
	proccreate(playctlproc, playevent, 8192);
	proccreate(playvolproc, cs->ctl, 8192);

	work();

	closecontrolset(cs);
	threadexitsall(nil);
}
