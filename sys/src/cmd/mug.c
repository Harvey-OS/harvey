#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>
#include <cursor.h>

#define initstate muginitstate

typedef struct State State;
struct State {
	double black;
	double white;
	double stretch;
	double gamma;
	int depth;
	int gtab[1001];
	Rectangle selr;
};

typedef struct Face Face;
struct Face {
	Rectangle r;
	State state;
	Image *small;
};

double GAMMA = 1.0;		/* theory tells me this should be 2.2, but 1.0 sure looks better */
enum {
	Left=0,
	Right,
	Top,
	Bottom,

	RTopLeft=0,
	RTop,
	RTopRight,
	RLeft,
	RMiddle,
	RRight,
	RBotLeft,
	RBot,
	RBotRight,
};

void*
emalloc(ulong sz)
{
	void *v;

	v = malloc(sz);
	if(v == nil)
		sysfatal("malloc %lud fails", sz);
	memset(v, 0, sz);
	return v;
}

Face *face[8];
int nface;
uchar grey2cmap[256];
Image *bkgd;
Image *orig;
Image *ramp, *small, *osmall, *tmp8, *red, *green, *blue;
State state, ostate;
uchar val2cmap[256];
uchar clamp[3*256];
Rectangle rbig, rramp, rface[nelem(face)], rsmall;
double *rdata;
int sdy, sdx;

void
geometry(Rectangle r)
{
	int i;
	Rectangle fr[9];

	rramp.min = addpt(r.min, Pt(4,4));
	rramp.max = addpt(rramp.min, Pt(256,256));

	rbig.min = Pt(rramp.max.x+6, rramp.min.y);
	rbig.max = addpt(rbig.min, Pt(Dx(orig->r), Dy(orig->r)));

	for(i=0; i<9; i++)
		fr[i] = rectaddpt(Rect(0,0,48,48), Pt(rramp.min.x+48+56*(i%3), rramp.max.y+6+56*(i/3)));

	rsmall = fr[4];
	for(i=0; i<4; i++)
		rface[i] = fr[i];
	for(i=4; i<8; i++)
		rface[i] = fr[i+1];
}

double
y2gamma(int y)
{
	double g;

	g = (double)y / 128.0;
	return 0.5+g*g;		/* gamma from 0.5 to 4.5, with 1.0 near the middle */
}

int
gamma2y(double g)
{
	g -= 0.5;
	return (int)(128.0*sqrt(g)+0.5);
}

void
drawface(int i)
{
	if(i==-1){
		border(screen, rsmall, -3, blue, ZP);
		draw(screen, rsmall, small, nil, ZP);
		return;
	}
	border(screen, rface[i], -1, display->black, ZP);
	if(face[i])
		draw(screen, rface[i], face[i]->small, nil, ZP);
	else
		draw(screen, rface[i], display->white, nil, ZP);
}

void
drawrampbar(Image *color, State *s)
{
	Rectangle liner, r;
	static Rectangle br;

	if(Dx(br))
		draw(screen, br, ramp, nil, subpt(br.min, rramp.min));

	r = rramp;
	r.max.x = r.min.x + (int)(s->white*255.0);
	r.min.x += (int)(s->black*255.0);
	r.min.y += gamma2y(s->gamma);
	r.max.y = r.min.y+1; 
	rectclip(&r, rramp);
	draw(screen, r, color, nil, ZP);
	br = r;

	r.min.y -= 2;
	r.max.y += 2;
	
	liner = r;
	r.min.x += Dx(liner)/3;
	r.max.x -= Dx(liner)/3;
	rectclip(&r, rramp);
	draw(screen, r, color, nil, ZP);
	combinerect(&br, r);

	r = liner;
	r.max.x = r.min.x+3;
	rectclip(&r, rramp);
	draw(screen, r, color, nil, ZP);
	combinerect(&br, r);

	r = liner;
	r.min.x = r.max.x-3;
	rectclip(&r, rramp);
	draw(screen, r, color, nil, ZP);
	combinerect(&br, r);
}

void
drawscreen(int clear)
{
	int i;

	if(clear){
		geometry(screen->r);
		draw(screen, screen->r, bkgd, nil, ZP);
	}

	border(screen, rbig, -1, display->black, ZP);
	draw(screen, rbig, orig, nil, orig->r.min);

	border(screen, rramp, -1, display->black, ZP);
	draw(screen, rramp, ramp, nil, ramp->r.min);
	drawrampbar(red, &state);

	border(screen, rectaddpt(state.selr, subpt(rbig.min, orig->r.min)), -2, red, ZP);
	if(clear){
		drawface(-1);
		for(i=0; i<nelem(face); i++)
			drawface(i);
	}
}

void
moveframe(Rectangle old, Rectangle new)
{
	border(screen, rectaddpt(old, subpt(rbig.min, orig->r.min)), -2, orig, old.min);
	border(screen, rectaddpt(new, subpt(rbig.min, orig->r.min)), -2, red, ZP);
}


/*
 * Initialize gamma ramp; should dither for
 * benefit of non-true-color displays.
 */
void
initramp(void)
{
	int k, x, y;
	uchar dat[256*256];
	double g;

	k = 0;
	for(y=0; y<256; y++) {
		g = y2gamma(y);
		for(x=0; x<256; x++)
			dat[k++] = 255.0 * pow(x/255.0, g);
	}
	assert(k == sizeof dat);

	ramp = allocimage(display, Rect(0,0,256,256), GREY8, 0, DNofill);
	if(ramp == nil)
		sysfatal("allocimage: %r");

	if(loadimage(ramp, ramp->r, dat, sizeof dat) != sizeof dat)
		sysfatal("loadimage: %r");
}

void
initclamp(void)
{
	int i;

	for(i=0; i<256; i++) {
		clamp[i] = 0;
		clamp[256+i] = i;
		clamp[512+i] = 255;
	}
}

void
changestretch(double stretch)
{
	state.stretch = stretch;
}

/*
 * There is greyscale data for the rectangle datar in data;
 * extract square r and write it into the 48x48 pixel image small.
 */
void
process(double *data, Rectangle datar, Rectangle r, Image *small)
{
	double black, center, delta, *k, shrink, sum, *tmp[48], *tt, w, white, x;
	int datadx, dp, dx, dy, error, i, ii, j, jj;
	int ksize, ksizeby2, sdata[48*48], sd, sh, sm, sv, u, uu, uuu, v, vv;
	uchar bdata[48*48];

	datadx = Dx(datar);
	dx = Dx(r);
	dy = Dy(r);
	shrink = dx/48.0;

	ksize = 1+2*(int)(shrink/2.0);
	if(ksize <= 2)
		return;

	k = emalloc(ksize*sizeof(k[0]));

	/* center of box */
	for(i=1; i<ksize-1; i++)
		k[i] = 1.0;

	/* edges */
	x = shrink - floor(shrink);
	k[0] = x;
	k[ksize-1] = x;

	sum = 0.0;
	for(i=0; i<ksize; i++)
		sum += k[i];

	for(i=0; i<ksize; i++)
		k[i] /= sum;

	ksizeby2 = ksize/2;

	for(i=0; i<48; i++)
		tmp[i] = emalloc(datadx*sizeof(tmp[i][0]));

	/* squeeze vertically */
	for(i=0; i<48; i++) {
		ii = r.min.y+i*dy/48;
		tt = tmp[i];
		uu = ii - ksizeby2;
		for(j=r.min.x-ksize; j<r.max.x+ksize; j++) {
			if(j<datar.min.x || j>=datar.max.x)
				continue;
			w = 0.0;

			uuu = uu*datadx+j;
			if(uu>=datar.min.y && uu+ksize<datar.max.y)
				for(u=0; u<ksize; u++){
					w += k[u]*data[uuu];
					uuu += datadx;
				}
			else
				for(u=0; u<ksize; u++){
					if(uu+u>=datar.min.y && uu+u<datar.max.y)
						w += k[u]*data[uuu];
					uuu+=datadx;
				}
			tt[j-datar.min.x] = w;
		}
	}

	/* stretch value scale */
	center = (state.black+state.white)/2;
	delta = state.stretch*(state.white-state.black)/2;
	black = center - delta;
	white = center + delta;

	/* squeeze horizontally */
	for(i=0; i<48; i++) {
		tt = tmp[i];
		for(j=0; j<48; j++) {
			jj = r.min.x+j*dx/48;
			w = 0.0;
			for(v=0; v<ksize; v++) {
				vv = jj - ksizeby2 + v;
				if(vv<datar.min.x || vv>=datar.max.x) {
					w += k[v];		/* assume white surround */
					continue;
				}
				w += k[v]*tt[vv-datar.min.x];
			}
			if(w < black || black==white)
				w = 0.0;
			else if(w > white)
				w = 1.0;
			else
				w = (w-black)/(white-black);
			sdata[i*48+j] = state.gtab[(int)(1000.0*w)];
		}
	}

	/* dither to lower depth before copying into GREY8 version */
	if(small->chan != GREY8) {
		u = 0;
		dp = small->depth;
		for(i=0; i<48; i++) {
			sm = 0xFF ^ (0xFF>>dp);
			sh = 0;
			v = 0;
			for(j=0; j<48; j++) {
				ii = 48*i+j;
				sd = clamp[sdata[ii]+256];
				sv = sd&sm;
				v |= sv>>sh;
				sh += dp;
				if(sh == 8) {
					bdata[u++] = v;
					v = 0;
					sh = 0;
				}

				/* propagate error, with decay (sum errors < 1) */
				error = sd - sv;
				if(ii+49 < 48*48) {	/* one test is enough, really */
					sdata[ii+1] = sdata[ii+1]+((3*error)>>4);
					sdata[ii+48] = sdata[ii+48]+((3*error)>>4);
					sdata[ii+49] = sdata[ii+49]+((3*error)>>3);
				}

				/* produce correct color map value by copying bits */
				switch(dp){
				case 1:
					sv |= sv>>1;
				case 2:
					sv |= sv>>2;
				case 4:
					sv |= sv>>4;
				}
				sdata[ii] = sv;
			}
		}
		for(i=0; i<nelem(bdata); i++)
			bdata[i] = sdata[i];
		if(loadimage(tmp8, tmp8->r, bdata, sizeof bdata) != sizeof bdata)
			sysfatal("loadimage: %r");
		draw(small, small->r, tmp8, nil, tmp8->r.min);
	} else {
		for(i=0; i<nelem(bdata); i++)
			bdata[i] = sdata[i];
		if(loadimage(small, small->r, bdata, sizeof bdata) != sizeof bdata)
			sysfatal("loadimage: %r");
	}

	free(k);
	for(i=0; i<48; i++)
		free(tmp[i]);
}

void
initval2cmap(void)
{
	int i;

	for(i=0; i<256; i++)
		val2cmap[i] = rgb2cmap(i, i, i);
}

void
setgtab(State *s)
{
	int i;

	for(i=0; i<=1000; i++)
		s->gtab[i] = val2cmap[(int)(255.0*pow((i/1000.0), 1.0/s->gamma))];
}

int
section(int x)
{
	int ib, iw;

	ib = state.black * 255.0;
	iw = state.white * 255.0;

	if(x<ib-5 || iw+5<x)
		return -1;

	iw -= ib;
	x -= ib;
	if(x < iw/3)
		return 0;
	if(x < 2*iw/3)
		return 1;
	return 2;
}

Image*
copyimage(Image *i)
{
	Image *n;

	if(i == nil)
		return nil;

	n = allocimage(display, i->r, i->chan, 0, DNofill);
	if(n == nil)
		sysfatal("allocimage: %r");

	draw(n, n->r, i, nil, i->r.min);
	return n;
}

Image*
grey8image(Image *i)
{
	Image *n;

	if(i->chan == GREY8)
		return i;

	n = allocimage(display, i->r, GREY8, 0, DNofill);
	if(n == nil)
		sysfatal("allocimage: %r");

	draw(n, n->r, i, nil, i->r.min);
	freeimage(i);
	return n;
}


void
mark(void)
{
	if(osmall != small){
		freeimage(osmall);
		osmall = small;
	}
	ostate = state;
}

void
undo(void)
{
	if(small != osmall){
		freeimage(small);
		small = osmall;
	}
	state = ostate;
	process(rdata, orig->r, state.selr, small);
	drawface(-1);
	drawscreen(0);
}

void
saveface(Face *f, int slot)
{
	if(slot == -1){
		mark();
		state = f->state;
		small = copyimage(f->small);
		drawface(-1);
		drawscreen(0);
		return;
	}

	if(face[slot]==nil)
		face[slot] = emalloc(sizeof(*face[slot]));
	else{
		freeimage(face[slot]->small);
		face[slot]->small = nil;
	}

	if(f == nil){
		face[slot]->small = copyimage(small);
		face[slot]->state = state;
	}else{
		face[slot]->small = copyimage(f->small);
		face[slot]->state = f->state;
	}
	drawface(slot);
}

int
writeface(char *outfile, Image *image)
{
	int i, fd, rv, y;
	uchar data[48*48/2];

	if(outfile == nil)
		fd = 1;
	else{
		if((fd = create(outfile, OWRITE, 0666)) < 0) 
			return -1;
	}

	switch(image->chan) {
	default:
		rv = -1;
		break;

	case GREY1:
		if(unloadimage(image, image->r, data, 48*48/8) != 48*48/8)
			sysfatal("unloadimage: %r");
		for(y=0; y<48; y++) {
			for(i=0; i<3; i++)
				fprint(fd, "0x%.2x%.2x,", data[y*6+i*2+0], data[y*6+i*2+1]);
			fprint(fd, "\n");
		}
		rv = 0;
		break;
		
	case GREY2:
		if(unloadimage(image, image->r, data, 48*48/4) != 48*48/4)
			sysfatal("unloadimage: %r");
		for(y=0; y<48; y++) {
			for(i=0; i<3; i++)
				fprint(fd, "0x%.2x%.2x,%.2x%.2x,",
					data[y*12+i*4+0], data[y*12+i*4+1],
					data[y*12+i*4+2], data[y*12+i*4+3]);
			fprint(fd, "\n");
		}
		rv = 0;
		break;

	case GREY4:
	case GREY8:
		rv = writeimage(fd, image, 0);	/* dolock? */
		break;
	}

	if(outfile)
		close(fd);
	return rv;
}

void
room(Rectangle out, Rectangle in, int *a)
{
	a[Left] = out.min.x - in.min.x;
	a[Right] = out.max.x - in.max.x;
	a[Top] = out.min.y - in.min.y;
	a[Bottom] = out.max.y - in.max.y;
}

int
min(int a, int b)
{
	if(a < b)
		return a;
	return b;
}

int
max(int a, int b)
{
	if(a > b)
		return a;
	return b;
}

int
move(Rectangle r, Rectangle picr, Point d, int k, Rectangle *rp)
{
	int a[4], i;
	Rectangle oldr;
	static int toggle;

	oldr = r;
	room(picr, r, a);
	switch(k){
	case RTopLeft:
		i = (d.x+d.y)/2;
		if(i>=Dx(r) || i>=Dy(r))
			break;
		i = max(i, a[Left]);
		i = max(i, a[Top]);
		r.min.x += i;
		r.min.y += i;
		break;
	case RTop:
		i = d.y;
		if(i < 0){
			/*
			 * should really check i/2, but this is safe and feedback
			 * makes the control feel right
			 */
			i = -min(-i, a[Right]);
			i = max(i, a[Left]);
		}
		i = max(i, a[Top]);
		if(i >= Dy(r))
			break;
		r.min.y += i;
		/* divide the half bit equally */
		toggle = 1-toggle;
		if(toggle){
			r.min.x += i/2;
			r.max.x = r.min.x+Dy(r);
		}else{
			r.max.x -= i/2;
			r.min.x = r.max.x-Dy(r);
		}
		break;
	case RTopRight:
		i = (-d.x+d.y)/2;
		if(i>=Dx(r) || i>=Dy(r))
			break;
		i = -min(-i, a[Right]);
		i = max(i, a[Top]);
		r.max.x -= i;
		r.min.y += i;
		break;
	case RLeft:
		i = d.x;
		if(i < 0){
			i = -min(-i, a[Bottom]);
			i = max(i, a[Top]);
		}
		i = max(i, a[Left]);
		if(i >= Dx(r))
			break;
		r.min.x += i;
		/* divide the half bit equally */
		toggle = 1-toggle;
		if(toggle){
			r.min.y += i/2;
			r.max.y = r.min.y+Dx(r);
		}else{
			r.max.y -= i/2;
			r.min.y = r.max.y-Dx(r);
		}
		break;
	case RMiddle:
		if(d.x >= 0)
			d.x = min(d.x, a[Right]);
		else
			d.x = max(d.x, a[Left]);
		if(d.y >= 0)
			d.y = min(d.y, a[Bottom]);
		else
			d.y = max(d.y, a[Top]);
		r = rectaddpt(r, d);
		break;
	case RRight:
		i = d.x;
		if(i > 0){
			i = min(i, a[Bottom]);
			i = -max(-i, a[Top]);
		}
		i = min(i, a[Right]);
		if(-i >= Dx(r))
			break;
		r.max.x += i;
		/* divide the half bit equally */
		toggle = 1-toggle;
		if(toggle){
			r.min.y -= i/2;
			r.max.y = r.min.y+Dx(r);
		}else{
			r.max.y += i/2;
			r.min.y = r.max.y-Dx(r);
		}
		break;
	case RBotLeft:
		i = (d.x+-d.y)/2;
		if(i>=Dx(r) || i>=Dy(r))
			break;
		i = max(i, a[Left]);
		i = -min(-i, a[Bottom]);
		r.min.x += i;
		r.max.y -= i;
		break;
	case RBot:
		i = d.y;
		if(i > 0){
			i = min(i, a[Right]);
			i = -max(-i, a[Left]);
		}
		i = min(i, a[Bottom]);
		if(i >= Dy(r))
			break;
		r.max.y += i;
		/* divide the half bit equally */
		toggle = 1-toggle;
		if(toggle){
			r.min.x -= i/2;
			r.max.x = r.min.x+Dy(r);
		}else{
			r.max.x += i/2;
			r.min.x = r.max.x-Dy(r);
		}
		break;
	case RBotRight:
		i = (-d.x+-d.y)/2;
		if(i>=Dx(r) || i>=Dy(r))
			break;
		i = -min(-i, a[Right]);
		i = -min(-i, a[Bottom]);
		r.max.x -= i;
		r.max.y -= i;
		break;
	}
	if(Dx(r)<3 || Dy(r)<3){
		*rp = oldr;
		return 0;
	}
	*rp = r;
	return !eqrect(r, oldr);
}

void
rlist(Rectangle r, Rectangle *ra)
{
	Rectangle tr;

	tr = r;
	tr.max.y = r.min.y+Dy(r)/4;
	ra[0] = tr;
	ra[0].max.x = tr.min.x+Dx(tr)/4;
	ra[1] = tr;
	ra[1].min.x = ra[0].max.x;
	ra[1].max.x = tr.max.x-Dx(tr)/4;
	ra[2] = tr;
	ra[2].min.x = ra[1].max.x;

	tr.min.y = tr.max.y;
	tr.max.y = r.max.y-Dy(r)/4;
	ra[3] = tr;
	ra[3].max.x = tr.min.x+Dx(tr)/4;
	ra[4] = tr;
	ra[4].min.x = ra[3].max.x;
	ra[4].max.x = tr.max.x-Dx(tr)/4;
	ra[5] = tr;
	ra[5].min.x = ra[4].max.x;

	tr.min.y = tr.max.y;
	tr.max.y = r.max.y;
	ra[6] = tr;
	ra[6].max.x = tr.min.x+Dx(tr)/4;
	ra[7] = tr;
	ra[7].min.x = ra[6].max.x;
	ra[7].max.x = tr.max.x-Dx(tr)/4;
	ra[8] = tr;
	ra[8].min.x = ra[7].max.x;
}

int
abs(int a)
{
	if(a < 0)
		return -a;
	return a;
}

void
usage(void)
{
	fprint(2, "usage: mug [file.bit]\n");
	exits("usage");
}

void
eresized(int new)
{
	if(new && getwindow(display, Refmesg) < 0)
		fprint(2,"can't reattach to window");
	drawscreen(1);

}

/*
interface notes

cursor changes while in rbig to indicate region.
only button 1 works for resizing region
only button 1 works for moving thingy in ramp

button-3 menu: Reset, Depth, Undo, Save, Write
*/

Cursor tl = {
	{-4, -4},
	{0xfe, 0x00, 0x82, 0x00, 0x8c, 0x00, 0x87, 0xff, 
	 0xa0, 0x01, 0xb0, 0x01, 0xd0, 0x01, 0x11, 0xff, 
	 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 
	 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x1f, 0x00, },
	{0x00, 0x00, 0x7c, 0x00, 0x70, 0x00, 0x78, 0x00, 
	 0x5f, 0xfe, 0x4f, 0xfe, 0x0f, 0xfe, 0x0e, 0x00, 
	 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 
	 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x00, 0x00, }
};

Cursor t = {
	{-7, -8},
	{0x00, 0x00, 0x00, 0x00, 0x03, 0x80, 0x06, 0xc0, 
	 0x1c, 0x70, 0x10, 0x10, 0x0c, 0x60, 0xfc, 0x7f, 
	 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xff, 0xff, 
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, },
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 
	 0x03, 0x80, 0x0f, 0xe0, 0x03, 0x80, 0x03, 0x80, 
	 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 0x00, 0x00, 
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
};

Cursor tr = {
	{-11, -4},
	{0x00, 0x7f, 0x00, 0x41, 0x00, 0x31, 0xff, 0xe1, 
	 0x80, 0x05, 0x80, 0x0d, 0x80, 0x0b, 0xff, 0x88, 
	 0x00, 0x88, 0x0, 0x88, 0x00, 0x88, 0x00, 0x88, 
	 0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 0x00, 0xf8, },
	{0x00, 0x00, 0x00, 0x3e, 0x00, 0x0e, 0x00, 0x1e, 
	 0x7f, 0xfa, 0x7f, 0xf2, 0x7f, 0xf0, 0x00, 0x70, 
	 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 
	 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 0x00, 0x00, }
};

Cursor r = {
	{-8, -7},
	{0x07, 0xc0, 0x04, 0x40, 0x04, 0x40, 0x04, 0x58, 
	 0x04, 0x68, 0x04, 0x6c, 0x04, 0x06, 0x04, 0x02, 
	 0x04, 0x06, 0x04, 0x6c, 0x04, 0x68, 0x04, 0x58, 
	 0x04, 0x40, 0x04, 0x40, 0x04, 0x40, 0x07, 0xc0, },
	{0x00, 0x00, 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 
	 0x03, 0x90, 0x03, 0x90, 0x03, 0xf8, 0x03, 0xfc, 
	 0x03, 0xf8, 0x03, 0x90, 0x03, 0x90, 0x03, 0x80, 
	 0x03, 0x80, 0x03, 0x80, 0x03, 0x80, 0x00, 0x00, }
};

Cursor br = {
	{-11, -11},
	{0x00, 0xf8, 0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 
	 0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 0x00, 0x88, 
	 0xff, 0x88, 0x80, 0x0b, 0x80, 0x0d, 0x80, 0x05, 
	 0xff, 0xe1, 0x00, 0x31, 0x00, 0x41, 0x00, 0x7f, },
	{0x00, 0x00, 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 
	 0x0, 0x70, 0x00, 0x70, 0x00, 0x70, 0x00, 0x70, 
	 0x00, 0x70, 0x7f, 0xf0, 0x7f, 0xf2, 0x7f, 0xfa, 
	 0x00, 0x1e, 0x00, 0x0e, 0x00, 0x3e, 0x00, 0x00, }
};

Cursor b = {
	{-7, -7},
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	 0xff, 0xff, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 
	 0xfc, 0x7f, 0x0c, 0x60, 0x10, 0x10, 0x1c, 0x70, 
	 0x06, 0xc0, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, },
	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	 0x00, 0x00, 0x7f, 0xfe, 0x7f, 0xfe, 0x7f, 0xfe, 
	 0x03, 0x80, 0x03, 0x80, 0x0f, 0xe0, 0x03, 0x80, 
	 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, }
};

Cursor bl = {
	{-4, -11},
	{0x1f, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 
	 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 0x11, 0x00, 
	 0x11, 0xff, 0xd0, 0x01, 0xb0, 0x01, 0xa0, 0x01, 
	 0x87, 0xff, 0x8c, 0x00, 0x82, 0x00, 0xfe, 0x00, },
	{0x00, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 
	 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x0e, 0x00, 
	 0x0e, 0x00, 0x0f, 0xfe, 0x4f, 0xfe, 0x5f, 0xfe, 
	 0x78, 0x00, 0x70, 0x00, 0x7c, 0x00, 0x00, 0x0, }
};

Cursor l = {
	{-7, -7},
	{0x03, 0xe0, 0x02, 0x20, 0x02, 0x20, 0x1a, 0x20, 
	 0x16, 0x20, 0x36, 0x20, 0x60, 0x20, 0x40, 0x20, 
	 0x60, 0x20, 0x36, 0x20, 0x16, 0x20, 0x1a, 0x20, 
	 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x03, 0xe0, },
	{0x00, 0x00, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 
	 0x09, 0xc0, 0x09, 0xc0, 0x1f, 0xc0, 0x3f, 0xc0, 
	 0x1f, 0xc0, 0x09, 0xc0, 0x09, 0xc0, 0x01, 0xc0, 
	 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x00, 0x00, }
};

Cursor boxcursor = {
	{-7, -7},
	{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F,
	 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xFF, 0xFF,
	 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, },
	{0x00, 0x00, 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E, 0x70, 0x0E,
	 0x7F, 0xFE, 0x7F, 0xFE, 0x7F, 0xFE, 0x00, 0x00, }
};

Cursor clearcursor;

Cursor *corners[10] = {
	&tl,	&t,	&tr,
	&l,	&boxcursor,	&r,
	&bl,	&b,	&br,
	nil,	/* default arrow */
};

char *item[] = {
	"Reset",
	"Depth",
	"Undo",
	"Write",
	"Exit",
	nil
};

Menu menu = {
	item, 
	nil,
	2
};

/*BUG make less flashy */
void
moveface(Image *back, Point lastp, Image *face, Point p, Point d)
{
	draw(screen, rectaddpt(back->r, subpt(lastp, d)), back, nil, back->r.min);
	draw(back, back->r, screen, nil, addpt(back->r.min, subpt(p, d)));
	border(screen, rectaddpt(face->r, subpt(p, d)),
		 -1, display->black, ZP);
	draw(screen, rectaddpt(face->r, subpt(p, d)), 
		face, nil, face->r.min);
}

int
dragface(Mouse *m, Image *im, Point d, int x)
{
	int i;
	Point lastp;
	static Image *back;

	if(back == nil){
		back = allocimage(display, Rect(-1,-1,49,49), display->image->chan, 0, DNofill);
		if(back == nil)
			sysfatal("dragface backing store: %r");
	}

	lastp = m->xy;
	draw(back, back->r, screen, nil, addpt(back->r.min, subpt(lastp, d)));
	esetcursor(&clearcursor);
	do{
		moveface(back, lastp, im, m->xy, d);
		lastp = m->xy;
	}while(*m=emouse(), m->buttons==1);

	draw(screen, rectaddpt(back->r, subpt(lastp, d)), back, nil, back->r.min);
	esetcursor(nil);
	if(m->buttons==0){
		for(i=0; i<nelem(face); i++)
			if(ptinrect(m->xy, rface[i]))
				return i;
		if(ptinrect(m->xy, rsmall))
			return -1;
		return x;
	}
	while(*m=emouse(), m->buttons)
		;
	return x;
}

void
initstate(void)
{
	state.black = 0.0;
	state.white = 1.0;
	state.stretch = 1.0;
	state.depth = 4;
	state.gamma = 1.0;
	setgtab(&state);
	state.selr = insetrect(orig->r, 5);
	sdx = Dx(state.selr);
	sdy = Dy(state.selr);
	if(sdx > sdy)
		state.selr.max.x = state.selr.min.x+sdy;
	else
		state.selr.max.y = state.selr.min.y+sdx;
}

void
main(int argc, char **argv)
{
	int ccursor, i, fd, k, n, y;
	uchar *data;
	double gammatab[256];
	Event e;
	Mouse m;
	Point lastp, p;
	Rectangle nselr, rbig9[9];

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc > 1)
		usage();
	if(argc == 1){
		if((fd = open(argv[0], OREAD)) < 0)
			sysfatal("open %s: %r", argv[0]);
	}else
		fd = 0;

	if (initdraw(0, 0, "mug") < 0)
		sysfatal("initdraw failed");

	if((orig = readimage(display, fd, 0)) == nil)
		sysfatal("readimage: %r");

	orig = grey8image(orig);

	initramp();
	initclamp();
	initval2cmap();
	bkgd = allocimagemix(display, DPaleyellow, DWhite);
	small = allocimage(display, Rect(0,0,48,48), GREY4, 0, DWhite);
	tmp8 = allocimage(display, Rect(0,0,48,48), GREY8, 0, DWhite);
	red = allocimage(display, Rect(0,0,1,1), display->image->chan, 1, DRed);
	green = allocimage(display, Rect(0,0,1,1), display->image->chan, 1, DGreen);
	blue = allocimage(display, Rect(0,0,1,1), display->image->chan, 1, DBlue);
	if(bkgd==nil || small==nil || tmp8==nil || red==nil || green==nil || blue==nil)
		sysfatal("allocimage: %r");

	n = Dx(orig->r)*Dy(orig->r);
	data = emalloc(n*sizeof data[0]);
	rdata = emalloc(n*sizeof rdata[0]);

	if(unloadimage(orig, orig->r, data, n) != n)
		sysfatal("unloadimage: %r");
	
	for(i=0; i<256; i++)
		gammatab[i] = pow((255-i)/(double)255.0, GAMMA);

	for(i=0; i<n; i++)
		rdata[i] = gammatab[255-data[i]];

	initstate();
	process(rdata, orig->r, state.selr, small);
	drawscreen(1);
	flushimage(display, 1);
	einit(Emouse|Ekeyboard);
	ccursor = 9;
	for(;;){
		if((n=eread(Emouse|Ekeyboard, &e))==Ekeyboard)
			continue;
		if(n != Emouse)
			break;

		m = e.mouse;
		if(m.buttons&4){
			ccursor = 9;
			esetcursor(corners[ccursor]);
			switch(emenuhit(3, &m, &menu)){
			case -1:
				continue;
			case 0:	/* Reset */
				mark();
				initstate();
				small = allocimage(display, Rect(0,0,48,48), CHAN1(CGrey, state.depth), 0, DWhite);
				if(small == nil)
					sysfatal("allocimage: %r");
				process(rdata, orig->r, state.selr, small);
				drawface(-1);
				drawscreen(0);
				break;
			case 1:	/* Depth */
				mark();
				/* osmall = small, so no freeimage */
				state.depth /= 2;
				if(state.depth == 0)
					state.depth = 8;
				small = allocimage(display, Rect(0,0,48,48), CHAN1(CGrey, state.depth), 0, DWhite);
				if(small == nil)
					sysfatal("allocimage: %r");
				process(rdata, orig->r, state.selr, small);
				drawface(-1);
				break;
			case 2:	/* Undo */
				undo();
				break;
			case 3:	/* Write */
				writeface(nil, small);
				break;
			case 4:	/* Exit */
				exits(nil);
				break;
			}
		}
			
		if(ptinrect(m.xy, rbig)){
			rlist(rectaddpt(state.selr, subpt(rbig.min, orig->r.min)), rbig9);
			for(i=0; i<9; i++)
				if(ptinrect(m.xy, rbig9[i]))
					break;
			if(i != ccursor){
				ccursor = i;
				esetcursor(corners[ccursor]);
			}
			if(i==9)
				continue;

			if(m.buttons & 1){
				mark();
				lastp = m.xy;
				while(m=emouse(), m.buttons&1){
					if(move(state.selr, orig->r, subpt(m.xy, lastp), i, &nselr)){
						moveframe(state.selr, nselr);
						state.selr = nselr;
						lastp = m.xy;
						process(rdata, orig->r, state.selr, small);
						drawface(-1);
					}
				}
			}
			continue;
		}

		if(ccursor != 9){	/* default cursor */
			ccursor = 9;
			esetcursor(corners[ccursor]);
		}

		if(ptinrect(m.xy, rramp)){
			if(m.buttons != 1)
				continue;
			mark();
			y = gamma2y(state.gamma);
			if(abs(y-(m.xy.y-rramp.min.y)) > 5)
				continue;
			k = section(m.xy.x-rramp.min.x);
			drawrampbar(green, &state);
			lastp = m.xy;
			while(m=emouse(), m.buttons&1){
				if(!ptinrect(m.xy, rramp))
					continue;
				switch(k){
				case -1:
					continue;
				case 0:
					if((m.xy.x-rramp.min.x)/255.0 < state.white){
						state.black = (m.xy.x-rramp.min.x)/255.0;
						break;
					}
					continue;
				case 1:
					state.gamma = y2gamma(m.xy.y-rramp.min.y);
					setgtab(&state);
					break;
				case 2:
					if((m.xy.x-rramp.min.x)/255.0 > state.black){
						state.white = (m.xy.x-rramp.min.x)/255.0;
						break;
					}
					continue;
				case 10:
					state.black += (m.xy.x-lastp.x)/255.0;
					state.white += (m.xy.x-lastp.x)/255.0;
					state.gamma = y2gamma(p.y);
					break;
				}
				process(rdata, orig->r, state.selr, small);
				drawface(-1);
				drawrampbar(green, &state);
			}
			if(m.buttons == 0){
				process(rdata, orig->r, state.selr, small);
				drawface(-1);
				drawrampbar(red, &state);
			}else
				undo();
			continue;
		}
	
		if(ptinrect(m.xy, rsmall)){
			if(m.buttons != 1)
				continue;
			n=dragface(&m, small, subpt(m.xy, rsmall.min), -1);
			if(n == -1)
				continue;
			saveface(nil, n);
		}
	
		for(i=0; i<nelem(face); i++)
			if(ptinrect(m.xy, rface[i]))
				break;
		if(i<nelem(face) && face[i] != nil){
			if(m.buttons != 1)
				continue;
			n=dragface(&m, face[i]->small, subpt(m.xy, rface[i].min), i);
			if(n == i)
				continue;
			saveface(face[i], n);
			continue;
		}

		do
			m = emouse();
		while(m.buttons==1);
	}
	exits(nil);
}
