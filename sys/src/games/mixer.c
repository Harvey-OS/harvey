#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>

Font *font;

enum {
	Master,
	Treble,
	Bass,
	CD,
	Line,
	Synth,
	Mic,
	Speaker,
	Nvolume,

	Unknown = 0,
	Usb,
	SharpA,
};

typedef struct Volume	Volume;
struct Volume {
	char *name;
	char *usbname;
	char *desc;
	int llevel;
	int rlevel;
	int res;
	int min;
	int max;
	int active;
	Rectangle r;
	Image *label;
};

Volume voltab[Nvolume] = {
[Master]	{ "audio", "volume", "volume", },
[Treble]	{ "treble", "treb", "treble", },
[Bass]	{ "bass", "bass", "bass", },
[CD]		{ "cd", nil, "cd", 0, },
[Speaker]	{ "speaker", nil, "speaker", 0, },
[Synth]	{ "synth", nil, "synth", 0, },
[Mic]		{ "mic", nil, "mic", 0, },
[Line]	{ "line", nil, "line", 0 },
};

int type;
int vfd;
int boxwid;

void
getvol(void)
{
	char buf[4096];
	char *line[32], *f[32];
	int i, j, n, nf;
	Volume *v, *ev;

	seek(vfd, 0, 0);
	if((n = readn(vfd, buf, sizeof buf)) <= 0) {
		fprint(2, "cannot read volume file: %r\n");
		exits("read");
	}

	if(n >= sizeof buf)
		n = sizeof(buf)-1;
	buf[n] = 0;
	n = getfields(buf, line, nelem(line), 0, "\n");
	ev = voltab+nelem(voltab);
	for(i=0; i<n; i++){
		nf = tokenize(line[i], f, nelem(f));
		if(nf < 2)
			continue;
		for(v=voltab; v<ev; v++){
			if(v->name && strcmp(v->name, f[0])==0){
				if(!v->usbname || strcmp(v->usbname, v->name) != 0)
					type = SharpA;
				break;
			}
			if(v->usbname && strcmp(v->usbname, f[0])==0){
				type = Usb;
				break;
			}
		}

		if(v==ev)
{
			continue;
}

// #A:             source in left value right value out left value right value
// USB:           volume      out               -5376      -15616           0         256
		j = 1;
		if(strcmp(f[j], "in") == 0){
			while(j < nf && strcmp(f[j], "out") != 0)
				j++;
			j++;
		}else{
			if(strcmp(f[j], "out") == 0)
				j++;
		}
		if(j >= nf)
{
			continue;
}
		v->active = 1;
		if(strcmp(f[j], "left") == 0 && nf-j >= 4 && strcmp(f[j+2], "right") == 0){
			type = SharpA;
			v->llevel = atoi(f[j+1]);
			v->rlevel = atoi(f[j+3]);
			v->min = 0;
			v->max = 100;
			v->res = 1;
			continue;
		}
		if(j==2 && nf==6){
			type = Usb;
			v->llevel = v->rlevel = atoi(f[j]);
			v->min = atoi(f[j+1]);
			v->max = atoi(f[j+2]);
			v->res = atoi(f[j+3]);
			continue;
		}
		type = SharpA;
		v->llevel = v->rlevel = atoi(f[j]);
		v->min = 0;
		v->max = 100;
		v->res = 1;
	}
}

void
setvol(Volume *v)
{
	seek(vfd, 0, 0);
	switch(type){
	case SharpA:
		if(v->llevel==v->rlevel)
			fprint(vfd, "%s %d", v->name, v->llevel);
		else
			fprint(vfd, "%s left %d right %d", v->name, v->llevel, v->rlevel);
		break;
	case Usb:
		fprint(vfd, "%s '%d %d'", v->usbname, v->llevel, v->rlevel);
		break;
	default:
		fprint(2, "unknown mixer type");
		break;
	}
}

int nvrect;
int toprect;
Image *slidercolor;
Image *lightgrey;

void
drawvol(Volume *v, int new)
{
	int midly, midry, midx;
	Rectangle r;

	midly = v->r.max.y-(Dy(v->r)*(v->llevel-v->min))/(v->max - v->min);
	midry = v->r.max.y-(Dy(v->r)*(v->rlevel-v->min))/(v->max - v->min);
	midx = v->r.min.x+Dx(v->r)/2;

	if(new) {
		border(screen, v->r, -1, display->black, ZP);
		draw(screen, v->r, lightgrey, nil, ZP);
	}

	draw(screen, Rect(v->r.min.x, midly, midx, v->r.max.y), slidercolor, nil, ZP);
	draw(screen, Rect(midx+1, midry, v->r.max.x, v->r.max.y), slidercolor, nil, ZP);
//	line(screen, Pt(midx, v->r.max.y), Pt(midx, midly > midry ? midly : midry), 0, 0, 0, display->black, ZP);

	if(!new) {
		draw(screen, Rect(v->r.min.x, v->r.min.y, midx, midly), lightgrey, nil, ZP);
		draw(screen, Rect(midx+1, v->r.min.y, v->r.max.x, midry), lightgrey, nil, ZP);
	//	line(screen, Pt(midx, midly > midry ? midly : midry), Pt(midx, v->r.min.y), 0, 0, 0, lightgrey, ZP);
	}

	r = Rpt(subpt(v->label->r.min, v->label->r.max), ZP);
	draw(screen, rectaddpt(r, Pt(v->r.max.x-2, v->r.max.y-5)), display->black, v->label, ZP);
}
	
void
redraw(Image *screen)
{
	enum { PAD=3, MARGIN=5 };
	int i, ht;
	Point p;
	Volume *v;

	ht = Dy(screen->r)-2*MARGIN;
	draw(screen, screen->r, lightgrey, nil, ZP);
	p = addpt(screen->r.min, Pt(MARGIN, MARGIN));
	for(i=0; i<Nvolume; i++) {
		v = &voltab[i];
		if(!v->active){
			v->r = Rect(0,0,0,0);
			continue;
		}
		v->r = Rpt(p, addpt(p, Pt(boxwid, ht)));
		p.x += boxwid+PAD;
		drawvol(v, 1);
	}
}

void
click(Mouse m)
{
	Volume *v, *ev;
	int delta, lev, or, ol;
	int what;

	if(m.buttons == 0 || (m.buttons & ~2))
		return;

	for(v=voltab, ev=v+nelem(voltab); v<ev; v++) {
		if(ptinrect(m.xy, v->r))
			break;
	}
	if(v >= ev)
		return;

	delta = m.xy.x - v->r.min.x;
	if(delta < Dx(v->r)/3)
		what = 1;
	else if(delta < (2*Dx(v->r))/3)
		what = 3;
	else
		what = 2;

	or = v->rlevel;
	ol = v->llevel;
	do {
		lev = v->min+((v->r.max.y - m.xy.y)*(v->max-v->min))/Dy(v->r);
		if(lev < v->min)
			lev = v->min;
		if(lev > v->max)
			lev = v->max;
		if(what & 1)
			v->llevel = lev;
		if(what & 2)
			v->rlevel = lev;
		setvol(v);
		drawvol(v, 0);
		m = emouse();
	} while(m.buttons == 2);

	if(m.buttons != 0) {
		v->rlevel = or;
		v->llevel = ol;
		setvol(v);
		drawvol(v, 0);
		while(m.buttons)
			m = emouse();
	}
}

/*
 * you can do this with clever blts.
 * i don't care at the moment. 
 */
void
rotate90(Image **ip)
{
	int dx, bx, x, y, dy, by;
	uchar *s, *d;
	Image *i, *ni;
	Rectangle r;

	i = *ip;
	assert(i->depth==1);
	r = i->r;
	ni = allocimage(display, Rect(r.min.y, r.min.x, r.max.y, r.max.x), i->chan, i->repl, DNofill);
	if(ni == nil)
		sysfatal("rotate90 allocimage: %r");

	s = malloc(2*Dx(r)*Dy(r));
	if(s == nil)
		sysfatal("rotate90 malloc: %r");

	if(unloadimage(i, r, s, bytesperline(i->r, i->depth)*Dy(r)) != bytesperline(i->r, i->depth)*Dy(r))
		sysfatal("unloadimage: %r");

	d = s+Dx(r)*Dy(r);
	memset(d, 0, Dx(r)*Dy(r));
	dx = Dx(r);
	dy = Dy(r);
	bx = (dx+7)/8;
	by = (dy+7)/8;
	for(y=0; y<dy; y++)
	for(x=0; x<dx; x++)
		d[x*by+y/8] |= (s[y*bx+(dx-1-x)/8] & (1<<(7-(dx-1-x)%8))) ? (1<<(7-(y%8))) : 0;

	if(loadimage(ni, ni->r, d, bytesperline(ni->r, ni->depth)*Dy(ni->r)) != bytesperline(ni->r, ni->depth)*Dy(ni->r))
		sysfatal("loadimage: %r");

	freeimage(i);
	free(s);
	*ip = ni;
}

char *files[] = {
	"/dev/audioctl",
	"/dev/volume",
	"#A/volume",
};

void
main(int argc, char **argv)
{
	int i;
	Event e;
	Point p;

	ARGBEGIN{
	default:
		goto Usage;
	}ARGEND;

	switch(argc){
	case 0:
		vfd = -1;
		for(i=0; i<nelem(files); i++)
			if((vfd = open(files[i], ORDWR)) >= 0)
				break;
		if(vfd < 0)
			sysfatal("couldn't find volume file");
		break;
	case 1:
		if((vfd = open(argv[0], ORDWR)) < 0)
			sysfatal("open %s: %r", argv[0]);
		break;
	default:
	Usage:
		fprint(2, "usage: games/mixer [/dev/volume]\n");
		exits("usage");
	}

	getvol();
	initdraw(0, 0, "mixer");
	slidercolor = allocimage(display, Rect(0,0,1,1), CMAP8, 1, DYellow);
	lightgrey = allocimage(display, Rect(0,0,1,1), CMAP8, 1, 0xCCCCCCFF);
	font = display->defaultfont;
	for(i=0; i<Nvolume; i++) {
		p = stringsize(font, voltab[i].desc);
		voltab[i].label = allocimage(display, Rpt(ZP, p), GREY1, 0, DBlack);
		string(voltab[i].label, ZP, display->white, ZP, font, voltab[i].desc);
		rotate90(&voltab[i].label);
	}
	p = stringsize(font, "0");
	boxwid = p.y + 2*2;
	redraw(screen);
	einit(Emouse|Ekeyboard);

	for(;;) {
		switch(eread(Emouse|Ekeyboard, &e)) {
		case Ekeyboard:
			if(e.kbdc == 0x7F || e.kbdc == 'q')
				exits(0);
			break;
		case Emouse:
			if(e.mouse.buttons)
				click(e.mouse);
			break;
		}
	}
}

void
eresized(int new)
{
	if(new && getwindow(display, Refmesg) < 0)
		fprint(2,"can't reattach to window");
	redraw(screen);
}

