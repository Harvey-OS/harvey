#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

enum {
	Edge = 5,
	Maxmag = 16
};

enum {
	Mzoom,
	Munzoom,
	Mgrid,
	Mredraw,
	Mexit
};

char *menustr[] = {
	"zoom",
	"unzoom",
	"grid",
	"redraw",
	"exit",
	nil
};

Menu menu = {
	menustr,
	nil,
	-1
};

Point lastp;
Image *red;
Image *tmp;
Image *grid;
Image *chequer;
int	screenfd;
int	mag = 4;
int	showgrid = 0;
Rectangle	screenr;
uchar	*screenbuf;

void	magnify(void);
void makegrid(void);

void
drawit(void)
{
	Rectangle r;
	border(screen, screen->r, Edge, red, ZP);
	magnify();
	r = insetrect(screen->r, Edge);
	draw(screen, r, tmp, nil, tmp->r.min);
	flushimage(display, 1);
}

int bypp;

void
main(int argc, char *argv[])
{
	Event e;
	char buf[5*12];
	ulong chan;
	int d;

	USED(argc, argv);
	if(initdraw(nil, nil, "lens") < 0){
		fprint(2, "lens: initdraw failed: %r\n");
		exits("initdraw");
	}
	einit(Emouse|Ekeyboard);
	red = allocimage(display, Rect(0, 0, 1, 1), CMAP8, 1, DRed);
	chequer = allocimage(display, Rect(0, 0, 2, 2), GREY1, 1, DBlack);
	draw(chequer, Rect(0, 0, 1, 1), display->white, nil, ZP);
	draw(chequer, Rect(1, 1, 2, 2), display->white, nil, ZP);
	lastp = divpt(addpt(screen->r.min, screen->r.max), 2);
	screenfd = open("/dev/screen", OREAD);
	if(screenfd < 0){
		fprint(2, "lens: can't open /dev/screen: %r\n");
		exits("screen");
	}
	if(read(screenfd, buf, sizeof buf) != sizeof buf){
		fprint(2, "lens: can't read /dev/screen: %r\n");
		exits("screen");
	}
	chan = strtochan(buf);
	d = chantodepth(chan);
	if(d < 8){
		fprint(2, "lens: can't handle screen format %11.11s\n", buf);
		exits("screen");
	}
	bypp = d/8;
	screenr.min.x = atoi(buf+1*12);
	screenr.min.y = atoi(buf+2*12);
	screenr.max.x = atoi(buf+3*12);
	screenr.max.y = atoi(buf+4*12);
	screenbuf = malloc(bypp*Dx(screenr)*Dy(screenr));
	if(screenbuf == nil){
		fprint(2, "lens: buffer malloc failed: %r\n");
		exits("malloc");
	}
	eresized(0);

	for(;;)
		switch(event(&e)){
		case Ekeyboard:
			switch(e.kbdc){
			case 'q':
			case 0x7f:
			case '\04':
			caseexit:
				exits(nil);
			case '=':
			case '+':
			casezoom:
				if(mag < Maxmag){
					mag++;
					makegrid();
					drawit();
				}
				break;
			case 'g':
			casegrid:
				showgrid = !showgrid;
				makegrid();
				drawit();
				break;
			case '-':
			case '_':
			caseunzoom:
				if(mag > 1){
					mag--;
					makegrid();
					drawit();
				}
				break;
			case '.':
			case ' ':
			caseredraw:
				drawit();
				break;
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case'0':
				mag = e.kbdc-'0';
				if(mag == 0)
					mag = 10;
				makegrid();
				drawit();
				break;
			}
			break;
		case Emouse:
			if(e.mouse.buttons & 1){
				lastp = e.mouse.xy;
				drawit();
			}
			if(e.mouse.buttons & 4)
				switch(emenuhit(3, &e.mouse, &menu)){
				case Mzoom:
					goto casezoom;
				case Munzoom:
					goto caseunzoom;
				case Mgrid:
					goto casegrid;
				case Mredraw:
					goto caseredraw;
				case Mexit:
					goto caseexit;
				}
			break;
		}
}

void
makegrid(void)
{
	int m;
	if (grid != nil) {
		freeimage(grid);
		grid = nil;
	}
	if (showgrid) {
		m = mag;
		if (m < 5)
			m *= 10;
		grid = allocimage(display, Rect(0, 0, m, m),
			CHAN2(CGrey, 8, CAlpha, 8), 1, DTransparent);
		if (grid != nil){
			draw(grid, Rect(0, 0, m, 1), chequer, nil, ZP);
			draw(grid, Rect(0, 1, 1, m), chequer, nil, ZP);
		}
	}
}

void
eresized(int new)
{
	if(new && getwindow(display, Refnone) < 0){
		fprint(2, "lens: can't reattach to window: %r\n");
		exits("attach");
	}
	freeimage(tmp);
	tmp = allocimage(display, Rect(0, 0, Dx(screen->r)-Edge, Dy(screen->r)-Edge+Maxmag), screen->chan, 0, DNofill);
	if(tmp == nil){
		fprint(2, "lens: allocimage failed: %r\n");
		exits("allocimage");
	}
	drawit();
}

void
magnify(void)
{
	int x, y, xx, yy, dd, i;
	int dx, dy;
	int xoff, yoff;
	uchar out[8192];
	uchar sp[4];

	dx = (Dx(tmp->r)+mag-1)/mag;
	dy = (Dy(tmp->r)+mag-1)/mag;
	xoff = lastp.x-Dx(tmp->r)/(mag*2);
	yoff  = lastp.y-Dy(tmp->r)/(mag*2);

	yy = yoff;
	dd = dy;
	if(yy < 0){
		dd += dy;
		yy = 0;
	}
	if(yy+dd > Dy(screenr))
		dd = Dy(screenr)-yy;
	seek(screenfd, 5*12+bypp*yy*Dx(screenr), 0);
	if(readn(screenfd, screenbuf+bypp*yy*Dx(screenr), bypp*Dx(screenr)*dd) != bypp*Dx(screenr)*dd){
		fprint(2, "lens: can't read screen: %r\n");
		return;
	}

	for(y=0; y<dy; y++){
		yy = yoff+y;
		if(yy>=0 && yy<Dy(screenr))
			for(x=0; x<dx; x++){
				xx = xoff+x;
				if(xx>=0 && xx<Dx(screenr))	/* snarf pixel at xx, yy */
					for(i=0; i<bypp; i++)
						sp[i] = screenbuf[bypp*(yy*Dx(screenr)+xx)+i];
				else
					sp[0] = sp[1] = sp[2] = sp[3] = 0;

				for(xx=0; xx<mag; xx++)
					if(x*mag+xx < tmp->r.max.x)
						for(i=0; i<bypp; i++)
							out[(x*mag+xx)*bypp+i] = sp[i];
			}
		else
			memset(out, 0, bypp*Dx(tmp->r));
		for(yy=0; yy<mag && y*mag+yy<Dy(tmp->r); yy++){
			werrstr("no error");
			if(loadimage(tmp, Rect(0, y*mag+yy, Dx(tmp->r), y*mag+yy+1), out, bypp*Dx(tmp->r)) != bypp*Dx(tmp->r)){
				exits("load");
			}
		}
	}
	if (showgrid && mag && grid)
		draw(tmp, tmp->r, grid, nil, mulpt(Pt(xoff, yoff), mag));
}
