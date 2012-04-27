#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

typedef struct KbMap KbMap;
struct KbMap {
	char *name;
	char *file;
	Rectangle r;
	int current;
};

KbMap *map;
int nmap;
Image *lightblue;
Image *justblue;

enum {
	PAD = 3,
	MARGIN = 5
};

char *dir = "/sys/lib/kbmap";

void*
erealloc(void *v, ulong n)
{
	v = realloc(v, n);
	if(v == nil)
		sysfatal("out of memory reallocating %lud", n);
	return v;
}

void*
emalloc(ulong n)
{
	void *v;

	v = malloc(n);
	if(v == nil)
		sysfatal("out of memory allocating %lud", n);
	memset(v, 0, n);
	return v;
}

char*
estrdup(char *s)
{
	int l;
	char *t;

	if (s == nil)
		return nil;
	l = strlen(s)+1;
	t = emalloc(l);
	memcpy(t, s, l);

	return t;
}

void
init(void)
{
	int i, fd, nr;
	Dir *pd;
	char buf[128];

	if((fd = open(dir, OREAD)) < 0)
		return;

	nmap = nr = dirreadall(fd, &pd);
	map = emalloc(nr * sizeof(KbMap));
	for(i=0; i<nr; i++){
		sprint(buf, "%s/%s", dir, pd[i].name);
		map[i].file = estrdup(buf);
		map[i].name = estrdup(pd[i].name);
		map[i].current = 0;
	}
	free(pd);

	close(fd);
}

void
drawmap(int i)
{
	if(map[i].current)
		draw(screen, map[i].r, justblue, nil, ZP);
	else
		draw(screen, map[i].r, lightblue, nil, ZP);

	_string(screen, addpt(map[i].r.min, Pt(2,0)), display->black, ZP,
		font, map[i].name, nil, strlen(map[i].name), 
		map[i].r, nil, ZP, SoverD);
	border(screen, map[i].r, 1, display->black, ZP);	
}

void
geometry(void)
{
	int i, rows;
	Rectangle r;

	rows = (Dy(screen->r)-2*MARGIN+PAD)/(font->height+PAD);

	r = Rect(0,0,(Dx(screen->r)-2*MARGIN), font->height);
	for(i=0; i<nmap; i++)
		map[i].r = rectaddpt(rectaddpt(r, Pt(MARGIN+(PAD+Dx(r))*(i/rows),
					MARGIN+(PAD+Dy(r))*(i%rows))), screen->r.min);

}

void
redraw(Image *screen)
{
	int i;

	draw(screen, screen->r, lightblue, nil, ZP);
	for(i=0; i<nmap; i++)
		drawmap(i);
	flushimage(display, 1);
}

void
eresized(int new)
{
	if(new && getwindow(display, Refmesg) < 0)
		fprint(2,"can't reattach to window");
	geometry();
	redraw(screen);
}

int
writemap(char *file)
{
	int i, fd, ofd;
	char buf[8192];

	if((fd = open(file, OREAD)) < 0){
		fprint(2, "cannot open %s: %r", file);
		return -1;
	}
	if((ofd = open("/dev/kbmap", OWRITE)) < 0) {
		fprint(2, "cannot open /dev/kbmap: %r");
		close(fd);
		return -1;
	}
	while((i = read(fd, buf, sizeof buf)) > 0)
		if(write(ofd, buf, i) != i){
			fprint(2, "writing /dev/kbmap: %r");
			break;
		}

	close(fd);
	close(ofd);
	return 0;
}

void
click(Mouse m)
{
	int i, j;
	char buf[128];

	if(m.buttons == 0 || (m.buttons & ~4))
		return;

	for(i=0; i<nmap; i++)
		if(ptinrect(m.xy, map[i].r))
			break;
	if(i == nmap)
		return;

	do
		m = emouse();
	while(m.buttons == 4);

	if(m.buttons != 0){
		do
			m = emouse();
		while(m.buttons);
		return;
	}

	for(j=0; j<nmap; j++)
		if(ptinrect(m.xy, map[j].r))
			break;
	if(j != i)
		return;

	/* since maps are often just a delta of the distributed map... */
	snprint(buf, sizeof buf, "%s/ascii", dir);
	writemap(buf);
	writemap(map[i].file);

	/* clean the previous current map */
	for(j=0; j<nmap; j++)
		map[j].current = 0;

	map[i].current = 1;

	redraw(screen);
}

void
usage(void)
{
	fprint(2, "usage: kbmap [file...]\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	Event e;
	char *c;

	if(argc > 1) {
		argv++; argc--;
		map = emalloc((argc)*sizeof(KbMap));
		while(argc--) {
			map[argc].file = estrdup(argv[argc]);
			c = strrchr(map[argc].file, '/');
			map[argc].name = (c == nil ? map[argc].file : c+1);
			map[argc].current = 0;
			nmap++;
		}
	} else 
		init();

	initdraw(0, 0, "kbmap");
	lightblue = allocimagemix(display, DPalebluegreen, DWhite);
	if(lightblue == nil)
		sysfatal("allocimagemix: %r");
	justblue = allocimagemix(display, DBlue, DWhite);
	if(justblue == nil)
		sysfatal("allocimagemix: %r");

	eresized(0);
	einit(Emouse|Ekeyboard);

	for(;;){
		switch(eread(Emouse|Ekeyboard, &e)){
		case Ekeyboard:
			if(e.kbdc==0x7F || e.kbdc=='q')
				exits(0);
			break;
		case Emouse:
			if(e.mouse.buttons)
				click(e.mouse);
			break;
		}
	}
}

