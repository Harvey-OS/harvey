#include <u.h>
#include <libc.h>
#include <libg.h>
#include "menu.h"
#include "proto.h"
#include "term.h"
int errfd;
struct Screen scrn;
int godot;
struct Board b;
Font * gfont(char *);
Font *tiny, *defont;
Bitmap screen;
int debug = 0;
extern char rfilename[];
extern Cursor coffeecup;
char message[200];
char * machine = "helix";
extern char * mydir;
Mouse mouse;
void minit(void);
/*
int
death(void *a, char *s)
{
	if(strncmp(s,"sys:", 4) == 0){
		print("place: note: %s\n");
		exits(s);
	}
	return 1;
}
*/

void
main(int argc, char * argv[])
{
	Event e;
	char *fontname, *tfontname;
	char buf[200];
	char *s = "NO BOARD LOADED";
	mydir = "";
	tfontname = "/lib/font/bit/misc/latin1.6x10.font";
	fontname = "/lib/font/bit/pelm/latin1.9.font";
	while (argc>1 && argv[1][0]=='-') {
		switch(argv[1][1]) {
		case 'D':
			debug = 1;
			break;
		case 'M':
			argc--;
			argv++;
			if(argc>1) machine = argv[1];
			break;
		case 'F':
			if(argv[1][2]) fontname = &argv[1][2];
			argc--;
			argv++;
			if(argc>1) fontname = argv[1];
			break;
		case 'f':
			if(argv[1][2]) tfontname = &argv[1][2];
			argc--;
			argv++;
			if(argc>1) tfontname = argv[1];
			break;
			
			
		case 'd':
			if(argv[1][2]) mydir = &argv[1][2];
			else {
				argc--;
				argv++;
				if(argc>1) mydir = argv[1];
			}
			break;
			

		default:
			sprint(buf,"unknown option %c\n",argv[1][1]);
			perror(buf);
			exits(buf);
			exits("BAD");
		}
		argc--;
		argv++;
	}
	while(argc > 1) {
		argc--;
		argv++;
		if(rfilename[0]) strcat(rfilename, " ");
		strcat(rfilename, argv[0]);
	}
	errfd = create("place.err",1,0666);
	fprint(errfd, "starting\n");
	binit(0, 0, "place");
	einit(Emouse | Ekeyboard);
/*	atnotify(death, 1); */
	iconinit();
	minit();
	scrn.mag = UNITMAG;
	scrn.showo = 1;
	scrn.shown = 1;
	scrn.showp = 0;
	scrn.selws = 0;
	if ((tiny = gfont(tfontname)) == 0)
		tiny = font;
	if ((defont = gfont(fontname)) == 0)
		defont = font;

	scrn.font = defont;
	scrn.grid = 100;		/* .01i */
	scrn.bmax = scrn.bmaxx = raddp(Rect(0, 0, 2, 2), Pt(0, 0));
	scrn.br = scrn.bmax;
	scrn.bd = sub(scrn.bmax.corner, scrn.bmax.origin);
	b.name = (char *) calloc(strlen(s)+1, sizeof(char));
	strcpy(b.name, s);
	caption = (char *) calloc(strlen(s)+1, sizeof(char));
	strcpy(caption, s);
	b.pinholes = 0;
	b.chips = 0;
	b.chipstr = 0;
	b.sigs = 0;
	b.ncursig = 0;
	b.resig = 0;
	b.nplanes = 0;
	b.planes = 0;
	b.nkeepouts = 0;
	b.keepouts = 0;
	ereshaped(screen.r);
	getmydir();
	call_host(machine);
	if(rfilename[0]) {
		expand(rfilename);
		sprint(buf, "reading: %s", rfilename);
		bepatient(buf);
		put1(READFILE);
		puts(rfilename);
		while(rcv());
		cursorswitch((Cursor *) 0);
	fprint(errfd, " -->opened\n ");
	}
	while (1) {
		switch (event(&e)) {
		case Emouse:
			mouse = e.mouse;
			if(!ptinrect(mouse.xy, Drect))
				continue;
			if (mouse.buttons&1) {
				but1();
			}
			else if (mouse.buttons&2) {
				menu2(&mouse);
			}
			else if (mouse.buttons&4){
				menu3(&mouse);
			}
			break;
		case  Ekeyboard:
			break;
		default:
			print("huh?");
			abort();
		}
		track();
	}
}


void
bepatient(char *s)
{
	cursorswitch(&coffeecup);
	if(message[0]) {
		string(&screen, scrn.bname.min, defont, message, S^D);
		message[0] = 0;
	}	
	else if(caption)
		string(&screen, scrn.bname.min, defont, caption, S^D);
	if(*s) {
		string(&screen, scrn.bname.min, defont, s, S^D);
		strcpy(message,s);
	}
	bflush();
}

void
bedamned(void)
{
	cursorswitch((Cursor *) 0);
	if(message[0]) {
		string(&screen, scrn.bname.min, defont, message, S^D);
		message[0] = 0;
		if(caption)
			string(&screen, scrn.bname.min, defont, caption, S^D);
	}
}

void
hosterr(char *s, int bored)
{
	Point p, pp;
	Bitmap *b;

/*	bitblt(&screen, add(b->r.min,p), &screen, raddp(b->r,p), S^D); */
	pp = p = add(Drect.origin, Pt(20, 20));
	if(b = balloc(Rect(0, 0, 400, 6*defont->height), screen.ldepth))
		bitblt(b, b->r.origin, &screen, raddp(b->r, p), F_STORE);
	rectf(&screen, raddp(b->r, p), F_CLR);
	p.y += defont->height;
	if(bored){
		string(&screen, p, defont, "we're in big trouble now!", F_STORE);
		p.y += defont->height;
	}
	string(&screen, p, defont, s, F_STORE);
	p.y += defont->height;
	if(bored){
		string(&screen, p, defont, "see 'fizz.errs' for details", F_STORE);
		p.y += defont->height;
	}
	string(&screen, p, defont, "click mouse to continue", F_STORE);
	for(mouse = emouse(); (mouse.buttons & 7) == 0; mouse = emouse())
		track();
	for(; mouse.buttons & 7; mouse = emouse())
		track();
	if(b){
		bitblt(&screen, pp, b, b->r, F_STORE);
		bfree(b);
	}
	/*bitblt(&screen, add(b->r.min,p), &screen, raddp(b->r,p), S^D);
	rectf(&screen, Drect, F_XOR);*/
}

void
bye(void)
{
	char buf[100];

	buf[0] = 0;
	getline("really exit? ", buf);
	if(buf[0] == 'y')  {
		put1(EXIT);
		exits((char *) 0);
	}
}

void
put1(int n)
{
	char buf[1];

	buf[0] = n;
	sendnchars(1, buf);
}

void
putn(int n)
{
	char buf[2];

	buf[0] = n>>8;
	buf[1] = n;
	sendnchars(2, buf);
}

void
puts(char * s)
{
	sendnchars(strlen(s)+1, s);
}

void
putp(Point p)
{
	putn(p.x);
	putn(p.y);
}

void
panic(char *s)
{
	fprint(2, "panic: ");
	perror(s);
	abort();
}
