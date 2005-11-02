/* Federico Benavento <benavento@gmail.com> */
#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

enum { 
	Facesize = 48 
};


void memoinit(void);
void redraw(void);
void eresized(int);
void resize(int i);
void afaces(void);
void allocblocks(void);
Image *openface(char *path);

Image *face[18];
char buf[100];
ushort winflag, level;
Image *back;
Image *fore;

enum
{
	Eninit,
	Eshow,
	Ehide,
	Edisc,
};

struct
{
	Image *face;
	Rectangle r;
	int	flag;
}block[36];

char *buttons[] =
{
	"restart",
	"easy",
	"hard",
	"exit",
	0
};

Menu menu =
{
	buttons
};

void
main(int argc, char *argv[])
{
	Mouse m;
	int i, j;
	ushort ran, score, attempt, prev, br[2];
	Image *c[2];
	char *fmt;

	level = 16;
	fmt = "win in %d attempts!";

	ARGBEGIN{
	default:
		goto Usage;
	case 'h':
		level=36;
		break;
	}ARGEND

	if(argc){
    Usage:
		fprint(2, "usage: %s [-h]\n", argv0);
		exits("usage");
	}
	if(initdraw(0,0,"memo") < 0)
		sysfatal("initdraw failed: %r");
	srand(time(0));
	memoinit();
	einit(Emouse);

    Start:
	afaces();
	winflag=0;
	prev=level+1;
	score=attempt=0;
	for(i=0;i!=level;i++)
		block[i].flag = Eninit;

	for(i=0;i!=level/2;i++){
		for(j=0;j!=2;){
			ran = rand()%level;
			if(block[ran].flag == Eninit){
				block[ran].face = face[i];
				block[ran].flag = Eshow;
				j++;
			}
		}
	}
	eresized(0);
	for(;;m=emouse())
		if(m.buttons)
			break;

	for(i=0;i!=level;i++)
		block[i].flag = Ehide;

	redraw();
	j = 0;
	for(;; m=emouse()){
		switch(m.buttons){
		case 1:
			while(m.buttons){
				for(i=0;i!=level;i++){
					if(i!=prev && ptinrect(m.xy,block[i].r)){
						if(block[i].flag==Ehide  && j<2){
							block[i].flag = Eshow;
							draw(screen, block[i].r, block[i].face, nil, ZP);
							c[j] = block[i].face;
							br[j] = prev = i;
							j++;
						}
						break;
					}
				}
				m=emouse();
			}
			break;
		case 4:
			switch(emenuhit(3, &m, &menu)) {
			case 0:	/* restart */
				goto Start;
				break;
			case 1:
				level=16;
				goto Start;
				break;
			case 2:
				level=36;
				goto Start;
				break;
			case 3:
				exits(0);
				break;
			}
		}
		if(j==2){
			attempt++;
			prev = level+1;
			j = 0;
			if(c[0] == c[1]){
				score++;
				block[br[0]].flag = Edisc;
				block[br[1]].flag = Edisc;
			} else{
				block[br[0]].flag = Ehide;
				block[br[1]].flag = Ehide;
			}
			redraw();
			continue;
		}
		if(score == level/2){
			winflag = 1;
			sprint(buf, fmt, attempt);
			redraw();
			for(;;m=emouse())
				if(m.buttons&1 || m.buttons&4)
					break;
			goto Start;
		}
	}
}

void
memoinit(void)
{
	back = allocimagemix(display, DPalebluegreen,DWhite);
	fore = allocimagemix(display, 0x00DDDDFF, 0x00DDDDFF);
}

void
eresized(int new)
{
	double sq;
	Point p;

	if(new && getwindow(display, Refnone) < 0){
		fprint(2, "can't reattach to window");
		exits("resized");
	}

	sq = sqrt(level);
	p = Pt(Dx(screen->r)+8, Dy(screen->r)+8);
	if(!new || !eqpt(p, Pt(Facesize*sq+sq*4+17, Facesize*sq+sq*4+17)))
		resize(Facesize*sq+sq*4+17);

	allocblocks();
	draw(screen, screen->r, back, nil, ZP);
	redraw();
}

void
redraw(void)
{
	int i;
	Rectangle r;
	Point p;

	if(winflag == 1){
		p = Pt(Dx(screen->r)/8, Dy(screen->r)/4);
		r = screen->r;
		r.min = addpt(r.min, p);
		r.max = subpt(r.max, p);
		draw(screen, r, fore, nil, ZP);
		p=addpt(r.min, Pt(5,5));
		string(screen,p,display->black,ZP,font,buf);
		return;
	}

	for(i=0;i!=level;i++){
		r = block[i].r;
		switch(block[i].flag){
		case Eshow:
			draw(screen, r,block[i].face,nil,ZP);
			break;
		case Edisc:
			draw(screen, r, back, nil, ZP);
			break;
		case Ehide:
			draw(screen, r, fore, nil, ZP);
			break;
		default:
			fprint(2, "something went wrong!");
			exits("wrong");
			break;
		}
	}
}

char *facepaths[] = {
	/* logos */
	"/lib/face/48x48x4/g/glenda.1",
	"/lib/face/48x48x2/p/pjw+9ball.2",

	/* /sys/doc/9.ms authors */
	"/lib/face/48x48x2/k/ken.1",
	"/lib/face/48x48x4/b/bobf.1",
	"/lib/face/48x48x4/p/philw.1",
	"/lib/face/48x48x4/p/presotto.1",
	"/lib/face/48x48x4/r/rob.1",
	"/lib/face/48x48x4/s/sean.1",

	/* additional authors and luminaries for harder levels */
	"/lib/face/48x48x4/b/bwk.1",
	"/lib/face/48x48x4/c/cyoung.1",
	"/lib/face/48x48x4/d/dmr.1",
	"/lib/face/48x48x4/d/doug.1",
	"/lib/face/48x48x4/h/howard.1",
	"/lib/face/48x48x4/j/jmk.1",
	"/lib/face/48x48x4/s/sape.1",
	"/lib/face/48x48x4/s/seanq.1",
	"/lib/face/48x48x4/t/td.1",
	"/lib/face/48x48x8/l/lucent.1",
};

void
afaces(void)
{
	int i;

	for(i=0; i<18; i++)
		face[i] = openface(facepaths[i]);
}

void
resize(int i)
{
	int fd;

	fd = open("/dev/wctl", OWRITE);
	if(fd >= 0){
		fprint(fd, "resize -dx %d -dy %d", i, i);
		close(fd);
	}
}

Image *
openimage(char *path)
{
	Image *i;
	int fd;

	fd = open(path, OREAD);
	if(fd < 0)
		sysfatal("open %s: %r", path);
	i = readimage(display, fd, 0);
	if(i == nil)
		sysfatal("readimage %s: %r", path);
	close(fd);
	return i;
}

void
allocblocks(void)
{
	Rectangle r, b;
	ushort i, x, y, sq;

	sq = sqrt(level);
	r = insetrect(screen->r, 5);
	r.max.x = r.min.x+Facesize*sq+sq*4-1;
	r.max.y = r.min.y+Facesize*sq+sq*4-1;
	b.max.y = r.min.y;
	for(i=level-1, y=0; y!=sq; y++){
		b.min.y = b.max.y;
		b.max.y = r.min.y+Dy(r)*(y+1)/sq;
		b.max.x = r.min.x;
		for(x=0; x!=sq; x++, i--){
			b.min.x = b.max.x;
			b.max.x = r.min.x+Dx(r)*(x+1)/sq;
			block[i].r = insetrect(b, 2 );
		}
	}
}

Image*
readbit(int fd, ulong chan, char *path)
{
	char buf[4096], hx[4], *p;
	uchar data[Facesize*Facesize];	/* more than enough */
	int nhx, i, n, ndata, nbit;
	Image *img;

	n = readn(fd, buf, sizeof buf);
	if(n <= 0)
		return nil;
	if(n >= sizeof buf)
		n = sizeof(buf)-1;
	buf[n] = '\0';

	n = 0;
	nhx = 0;
	nbit = chantodepth(chan);
	ndata = (Facesize*Facesize*nbit)/8;
	p = buf;
	while(n < ndata) {
		p = strpbrk(p+1, "0123456789abcdefABCDEF");
		if(p == nil)
			break;
		if(p[0] == '0' && p[1] == 'x')
			continue;

		hx[nhx] = *p;
		if(++nhx == 2) {
			hx[nhx] = 0;
			i = strtoul(hx, 0, 16);
			data[n++] = ~i;
			nhx = 0;
		}
	}
	if(n < ndata)
		sysfatal("short face %s", path);

	img = allocimage(display, Rect(0,0,Facesize,Facesize), chan, 0, 0);
	if(img == nil)
		return nil;

	loadimage(img, img->r, data, ndata);
	return img;
}

Image*
openface(char *path)
{
	char *p;
	int fd, n;

	p = strstr(path, "48x48x");
	if(p == nil)
		return openimage(path);
	n = atoi(p+6);
	if(n < 4){
		if((fd = open(path, OREAD)) < 0)
			sysfatal("open %s: %r", path);
		return readbit(fd, n==1 ? GREY1 : GREY2, path);
	}
	return openimage(path);
}
