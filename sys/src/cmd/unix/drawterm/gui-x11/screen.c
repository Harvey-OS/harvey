#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "keysym2ucs.h"

/*
 * alias defs for image types to overcome name conflicts
 */
#define	Point	IPoint
#define	Rectangle	IRectangle
#define	Display	IDisplay
#define	Font	IFont
#define	Screen	IScreen

#include	"u.h"
#include	"lib.h"
#include	"dat.h"
#include	"fns.h"
#include	"user.h"
#include	"draw.h"
#include	"memdraw.h"
#include	"keyboard.h"
#include	"screen.h"

#undef time
#undef Point
#undef Rectangle
#undef Display
#undef Font
#undef Screen

typedef struct ICursor ICursor;
struct ICursor
{
	int	w;
	int	h;
	int	hotx;
	int	hoty;
	char	*src;
	char	*mask;
};


#define ABS(x) ((x) < 0 ? -(x) : (x))

enum
{
	DblTime	= 300		/* double click time in msec */
};

XColor			map[256];	/* Plan 9 colormap array */
XColor			map7[128];	/* Plan 9 colormap array */
uchar			map7to8[128][2];
Colormap		xcmap;		/* Default shared colormap  */
int 			plan9tox11[256]; /* Values for mapping between */
int 			x11toplan9[256]; /* X11 and Plan 9 */
int				x24bitswap = 0;	/* swap endian for 24bit RGB */
int				xtblbit;
extern int mousequeue;

/* for copy/paste, lifted from plan9ports */
Atom clipboard; 
Atom utf8string;
Atom targets;
Atom text;
Atom compoundtext;

static	Drawable	xdrawable;
/* static	Atom		wm_take_focus; */
static	void		xexpose(XEvent*);
static	void		xmouse(XEvent*);
static	void		xkeyboard(XEvent*);
static	void		xmapping(XEvent*);
static	void		xdestroy(XEvent*);
static	void		xselect(XEvent*, Display*);
static	void		xproc(void*);
static	Memimage*		xinitscreen(void);
static	void		initmap(Window);
static	GC		creategc(Drawable);
static	void		graphicscmap(XColor*);
	int		xscreendepth;
	Drawable	xscreenid;
	Display*	xdisplay;	/* used holding draw lock */
	Display*	xkmcon;	/* used only in xproc */
	Display*	xsnarfcon;	/* used holding clip.lk */
	Visual		*xvis;
	GC		xgcfill, xgccopy, xgcsimplesrc, xgczero, xgcreplsrc;
	GC		xgcfill0, xgccopy0, xgcsimplesrc0, xgczero0, xgcreplsrc0;
	ulong		xblack;
	ulong		xwhite;
	ulong	xscreenchan;

static int putsnarf, assertsnarf;

extern Memimage* xallocmemimage(IRectangle, ulong, int);
Memimage *gscreen;
Screeninfo screen;
XImage *ximage;

void
screeninit(void)
{
	_memmkcmap();

	gscreen = xinitscreen();
	kproc("xscreen", xproc, nil);

	memimageinit();
	terminit();
	drawqlock();
	flushmemscreen(gscreen->r);
	drawqunlock();
}

uchar*
attachscreen(IRectangle *r, ulong *chan, int *depth,
	int *width, int *softscreen, void **X)
{
	*r = gscreen->r;
	*chan = gscreen->chan;
	*depth = gscreen->depth;
	*width = gscreen->width;
	*X = gscreen->X;
	*softscreen = 1;

	return gscreen->data->bdata;
}

void
flushmemscreen(IRectangle r)
{
	assert(!drawcanqlock());
	if(r.min.x >= r.max.x || r.min.y >= r.max.y)
		return;
	XCopyArea(xdisplay, xscreenid, xdrawable, xgccopy, r.min.x, r.min.y, Dx(r), Dy(r), r.min.x, r.min.y);
	XFlush(xdisplay);
}

static int
revbyte(int b)
{
	int r;

	r = 0;
	r |= (b&0x01) << 7;
	r |= (b&0x02) << 5;
	r |= (b&0x04) << 3;
	r |= (b&0x08) << 1;
	r |= (b&0x10) >> 1;
	r |= (b&0x20) >> 3;
	r |= (b&0x40) >> 5;
	r |= (b&0x80) >> 7;
	return r;
}

void
mouseset(IPoint xy)
{
	drawqlock();
	XWarpPointer(xdisplay, None, xdrawable, 0, 0, 0, 0, xy.x, xy.y);
	XFlush(xdisplay);
	drawqunlock();
}

static Cursor xcursor;

void
setcursor(void)
{
	Cursor xc;
	XColor fg, bg;
	Pixmap xsrc, xmask;
	int i;
	uchar src[2*16], mask[2*16];

	for(i=0; i<2*16; i++){
		src[i] = revbyte(cursor.set[i]);
		mask[i] = revbyte(cursor.set[i] | cursor.clr[i]);
	}

	drawqlock();
	fg = map[0];
	bg = map[255];
	xsrc = XCreateBitmapFromData(xdisplay, xdrawable, (char*)src, 16, 16);
	xmask = XCreateBitmapFromData(xdisplay, xdrawable, (char*)mask, 16, 16);
	xc = XCreatePixmapCursor(xdisplay, xsrc, xmask, &fg, &bg, -cursor.offset.x, -cursor.offset.y);
	if(xc != 0) {
		XDefineCursor(xdisplay, xdrawable, xc);
		if(xcursor != 0)
			XFreeCursor(xdisplay, xcursor);
		xcursor = xc;
	}
	XFreePixmap(xdisplay, xsrc);
	XFreePixmap(xdisplay, xmask);
	XFlush(xdisplay);
	drawqunlock();
}

void
cursorarrow(void)
{
	drawqlock();
	if(xcursor != 0){
		XFreeCursor(xdisplay, xcursor);
		xcursor = 0;
	}
	XUndefineCursor(xdisplay, xdrawable);
	XFlush(xdisplay);
	drawqunlock();
}

static void
xproc(void *arg)
{
	ulong mask;
	XEvent event;

	mask = 	KeyPressMask|
		ButtonPressMask|
		ButtonReleaseMask|
		PointerMotionMask|
		Button1MotionMask|
		Button2MotionMask|
		Button3MotionMask|
		Button4MotionMask|
		Button5MotionMask|
		ExposureMask|
		StructureNotifyMask;

	XSelectInput(xkmcon, xdrawable, mask);
	for(;;) {
		//XWindowEvent(xkmcon, xdrawable, mask, &event);
		XNextEvent(xkmcon, &event);
		xselect(&event, xkmcon);
		xkeyboard(&event);
		xmouse(&event);
		xexpose(&event);
		xmapping(&event);
		xdestroy(&event);
	}
}

static int
shutup(Display *d, XErrorEvent *e)
{
	char buf[200];
	iprint("X error: error code=%d, request_code=%d, minor=%d\n", e->error_code, e->request_code, e->minor_code);
	XGetErrorText(d, e->error_code, buf, sizeof(buf));
	iprint("%s\n", buf);
	USED(d);
	USED(e);
	return 0;
}

static int
panicshutup(Display *d)
{
	panic("x error");
	return -1;
}

static Memimage*
xinitscreen(void)
{
	Memimage *gscreen;
	int i, xsize, ysize, pmid;
	char *argv[2];
	char *disp_val;
	Window rootwin;
	IRectangle r;
	XWMHints hints;
	Screen *screen;
	XVisualInfo xvi;
	int rootscreennum;
	XTextProperty name;
	XClassHint classhints;
	XSizeHints normalhints;
	XSetWindowAttributes attrs;
	XPixmapFormatValues *pfmt;
	int n;
 
	xscreenid = 0;
	xdrawable = 0;

	xdisplay = XOpenDisplay(NULL);
	if(xdisplay == 0){
		disp_val = getenv("DISPLAY");
		if(disp_val == 0)
			disp_val = "not set";
		iprint("drawterm: open %r, DISPLAY is %s\n", disp_val);
		exit(0);
	}

	XSetErrorHandler(shutup);
	XSetIOErrorHandler(panicshutup);
	rootscreennum = DefaultScreen(xdisplay);
	rootwin = DefaultRootWindow(xdisplay);
	
	xscreendepth = DefaultDepth(xdisplay, rootscreennum);
	if(XMatchVisualInfo(xdisplay, rootscreennum, 16, TrueColor, &xvi)
	|| XMatchVisualInfo(xdisplay, rootscreennum, 16, DirectColor, &xvi)){
		xvis = xvi.visual;
		xscreendepth = 16;
		xtblbit = 1;
	}
	else if(XMatchVisualInfo(xdisplay, rootscreennum, 24, TrueColor, &xvi)
	|| XMatchVisualInfo(xdisplay, rootscreennum, 24, DirectColor, &xvi)){
		xvis = xvi.visual;
		xscreendepth = 24;
		xtblbit = 1;
	}
	else if(XMatchVisualInfo(xdisplay, rootscreennum, 8, PseudoColor, &xvi)
	|| XMatchVisualInfo(xdisplay, rootscreennum, 8, StaticColor, &xvi)){
		if(xscreendepth > 8)
			panic("drawterm: can't deal with colormapped depth %d screens\n", xscreendepth);
		xvis = xvi.visual;
		xscreendepth = 8;
	}
	else{
		if(xscreendepth != 8)
			panic("drawterm: can't deal with depth %d screens\n", xscreendepth);
		xvis = DefaultVisual(xdisplay, rootscreennum);
	}

	/*
	 * xscreendepth is only the number of significant pixel bits,
	 * not the total.  We need to walk the display list to find
	 * how many actual bits are being used per pixel.
	 */
	xscreenchan = 0; /* not a valid channel */
	pfmt = XListPixmapFormats(xdisplay, &n);
	for(i=0; i<n; i++){
		if(pfmt[i].depth == xscreendepth){
			switch(pfmt[i].bits_per_pixel){
			case 1:	/* untested */
				xscreenchan = GREY1;
				break;
			case 2:	/* untested */
				xscreenchan = GREY2;
				break;
			case 4:	/* untested */
				xscreenchan = GREY4;
				break;
			case 8:
				xscreenchan = CMAP8;
				break;
			case 16: /* uses 16 rather than 15, empirically. */
				xscreenchan = RGB16;
				break;
			case 24: /* untested (impossible?) */
				xscreenchan = RGB24;
				break;
			case 32:
				xscreenchan = CHAN4(CIgnore, 8, CRed, 8, CGreen, 8, CBlue, 8);
				break;
			}
		}
	}
	if(xscreenchan == 0)
		panic("drawterm: unknown screen pixel format\n");
		
	screen = DefaultScreenOfDisplay(xdisplay);
	xcmap = DefaultColormapOfScreen(screen);

	if(xvis->class != StaticColor){
		graphicscmap(map);
		initmap(rootwin);
	}


	r.min = ZP;
	r.max.x = WidthOfScreen(screen);
	r.max.y = HeightOfScreen(screen);

	
	xsize = Dx(r)*3/4;
	ysize = Dy(r)*3/4;
	
	attrs.colormap = xcmap;
	attrs.background_pixel = 0;
	attrs.border_pixel = 0;
	/* attrs.override_redirect = 1;*/ /* WM leave me alone! |CWOverrideRedirect */
	xdrawable = XCreateWindow(xdisplay, rootwin, 0, 0, xsize, ysize, 0, 
		xscreendepth, InputOutput, xvis, CWBackPixel|CWBorderPixel|CWColormap, &attrs);

	/*
	 * set up property as required by ICCCM
	 */
	name.value = (uchar*)"drawterm";
	name.encoding = XA_STRING;
	name.format = 8;
	name.nitems = strlen((char*)name.value);
	normalhints.flags = USSize|PMaxSize;
	normalhints.max_width = Dx(r);
	normalhints.max_height = Dy(r);
	normalhints.width = xsize;
	normalhints.height = ysize;
	hints.flags = InputHint|StateHint;
	hints.input = 1;
	hints.initial_state = NormalState;
	classhints.res_name = "drawterm";
	classhints.res_class = "Drawterm";
	argv[0] = "drawterm";
	argv[1] = nil;
	XSetWMProperties(xdisplay, xdrawable,
		&name,			/* XA_WM_NAME property for ICCCM */
		&name,			/* XA_WM_ICON_NAME */
		argv,			/* XA_WM_COMMAND */
		1,			/* argc */
		&normalhints,		/* XA_WM_NORMAL_HINTS */
		&hints,			/* XA_WM_HINTS */
		&classhints);		/* XA_WM_CLASS */
	XFlush(xdisplay);
	
	/*
	 * put the window on the screen
	 */
	XMapWindow(xdisplay, xdrawable);
	XFlush(xdisplay);

	xscreenid = XCreatePixmap(xdisplay, xdrawable, Dx(r), Dy(r), xscreendepth);
	gscreen = xallocmemimage(r, xscreenchan, xscreenid);
	
	xgcfill = creategc(xscreenid);
	XSetFillStyle(xdisplay, xgcfill, FillSolid);
	xgccopy = creategc(xscreenid);
	xgcsimplesrc = creategc(xscreenid);
	XSetFillStyle(xdisplay, xgcsimplesrc, FillStippled);
	xgczero = creategc(xscreenid);
	xgcreplsrc = creategc(xscreenid);
	XSetFillStyle(xdisplay, xgcreplsrc, FillTiled);

	pmid = XCreatePixmap(xdisplay, xdrawable, 1, 1, 1);
	xgcfill0 = creategc(pmid);
	XSetForeground(xdisplay, xgcfill0, 0);
	XSetFillStyle(xdisplay, xgcfill0, FillSolid);
	xgccopy0 = creategc(pmid);
	xgcsimplesrc0 = creategc(pmid);
	XSetFillStyle(xdisplay, xgcsimplesrc0, FillStippled);
	xgczero0 = creategc(pmid);
	xgcreplsrc0 = creategc(pmid);
	XSetFillStyle(xdisplay, xgcreplsrc0, FillTiled);
	XFreePixmap(xdisplay, pmid);

	XSetForeground(xdisplay, xgccopy, plan9tox11[0]);
	XFillRectangle(xdisplay, xscreenid, xgccopy, 0, 0, xsize, ysize);

	xkmcon = XOpenDisplay(NULL);
	if(xkmcon == 0){
		disp_val = getenv("DISPLAY");
		if(disp_val == 0)
			disp_val = "not set";
		iprint("drawterm: open %r, DISPLAY is %s\n", disp_val);
		exit(0);
	}
	xsnarfcon = XOpenDisplay(NULL);
	if(xsnarfcon == 0){
		disp_val = getenv("DISPLAY");
		if(disp_val == 0)
			disp_val = "not set";
		iprint("drawterm: open %r, DISPLAY is %s\n", disp_val);
		exit(0);
	}

	clipboard = XInternAtom(xkmcon, "CLIPBOARD", False);
	utf8string = XInternAtom(xkmcon, "UTF8_STRING", False);
	targets = XInternAtom(xkmcon, "TARGETS", False);
	text = XInternAtom(xkmcon, "TEXT", False);
	compoundtext = XInternAtom(xkmcon, "COMPOUND_TEXT", False);

	xblack = screen->black_pixel;
	xwhite = screen->white_pixel;
	return gscreen;
}

static void
graphicscmap(XColor *map)
{
	int r, g, b, cr, cg, cb, v, num, den, idx, v7, idx7;

	for(r=0; r!=4; r++) {
		for(g = 0; g != 4; g++) {
			for(b = 0; b!=4; b++) {
				for(v = 0; v!=4; v++) {
					den=r;
					if(g > den)
						den=g;
					if(b > den)
						den=b;
					/* divide check -- pick grey shades */
					if(den==0)
						cr=cg=cb=v*17;
					else {
						num=17*(4*den+v);
						cr=r*num/den;
						cg=g*num/den;
						cb=b*num/den;
					}
					idx = r*64 + v*16 + ((g*4 + b + v - r) & 15);
					map[idx].red = cr*0x0101;
					map[idx].green = cg*0x0101;
					map[idx].blue = cb*0x0101;
					map[idx].pixel = idx;
					map[idx].flags = DoRed|DoGreen|DoBlue;

					v7 = v >> 1;
					idx7 = r*32 + v7*16 + g*4 + b;
					if((v & 1) == v7){
						map7to8[idx7][0] = idx;
						if(den == 0) { 		/* divide check -- pick grey shades */
							cr = ((255.0/7.0)*v7)+0.5;
							cg = cr;
							cb = cr;
						}
						else {
							num=17*15*(4*den+v7*2)/14;
							cr=r*num/den;
							cg=g*num/den;
							cb=b*num/den;
						}
						map7[idx7].red = cr*0x0101;
						map7[idx7].green = cg*0x0101;
						map7[idx7].blue = cb*0x0101;
						map7[idx7].pixel = idx7;
						map7[idx7].flags = DoRed|DoGreen|DoBlue;
					}
					else
						map7to8[idx7][1] = idx;
				}
			}
		}
	}
}

/*
 * Initialize and install the drawterm colormap as a private colormap for this
 * application.  Drawterm gets the best colors here when it has the cursor focus.
 */  
static void 
initmap(Window w)
{
	XColor c;
	int i;
	ulong p, pp;
	char buf[30];

	if(xscreendepth <= 1)
		return;

	if(xscreendepth >= 24) {
		/* The pixel value returned from XGetPixel needs to
		 * be converted to RGB so we can call rgb2cmap()
		 * to translate between 24 bit X and our color. Unfortunately,
		 * the return value appears to be display server endian 
		 * dependant. Therefore, we run some heuristics to later
		 * determine how to mask the int value correctly.
		 * Yeah, I know we can look at xvis->byte_order but 
		 * some displays say MSB even though they run on LSB.
		 * Besides, this is more anal.
		 */
		if(xscreendepth != DefaultDepth(xdisplay, DefaultScreen(xdisplay)))
			xcmap = XCreateColormap(xdisplay, w, xvis, AllocNone);

		c = map[19];
		/* find out index into colormap for our RGB */
		if(!XAllocColor(xdisplay, xcmap, &c))
			panic("drawterm: screen-x11 can't alloc color");

		p  = c.pixel;
		pp = rgb2cmap((p>>16)&0xff,(p>>8)&0xff,p&0xff);
		if(pp!=map[19].pixel) {
			/* check if endian is other way */
			pp = rgb2cmap(p&0xff,(p>>8)&0xff,(p>>16)&0xff);
			if(pp!=map[19].pixel)
				panic("cannot detect x server byte order");
			switch(xscreenchan){
			case RGB24:
				xscreenchan = BGR24;
				break;
			case XRGB32:
				xscreenchan = XBGR32;
				break;
			default:
				panic("don't know how to byteswap channel %s", 
					chantostr(buf, xscreenchan));
				break;
			}
		}
	} else if(xvis->class == TrueColor || xvis->class == DirectColor) {
	} else if(xvis->class == PseudoColor) {
		if(xtblbit == 0){
			xcmap = XCreateColormap(xdisplay, w, xvis, AllocAll); 
			XStoreColors(xdisplay, xcmap, map, 256);
			for(i = 0; i < 256; i++) {
				plan9tox11[i] = i;
				x11toplan9[i] = i;
			}
		}
		else {
			for(i = 0; i < 128; i++) {
				c = map7[i];
				if(!XAllocColor(xdisplay, xcmap, &c)) {
					iprint("drawterm: can't alloc colors in default map, don't use -7\n");
					exit(0);
				}
				plan9tox11[map7to8[i][0]] = c.pixel;
				plan9tox11[map7to8[i][1]] = c.pixel;
				x11toplan9[c.pixel] = map7to8[i][0];
			}
		}
	}
	else
		panic("drawterm: unsupported visual class %d\n", xvis->class);
}

static void
xdestroy(XEvent *e)
{
	XDestroyWindowEvent *xe;
	if(e->type != DestroyNotify)
		return;
	xe = (XDestroyWindowEvent*)e;
	if(xe->window == xdrawable)
		exit(0);
}

static void
xmapping(XEvent *e)
{
	XMappingEvent *xe;

	if(e->type != MappingNotify)
		return;
	xe = (XMappingEvent*)e;
	USED(xe);
}


/*
 * Disable generation of GraphicsExpose/NoExpose events in the GC.
 */
static GC
creategc(Drawable d)
{
	XGCValues gcv;

	gcv.function = GXcopy;
	gcv.graphics_exposures = False;
	return XCreateGC(xdisplay, d, GCFunction|GCGraphicsExposures, &gcv);
}

static void
xexpose(XEvent *e)
{
	IRectangle r;
	XExposeEvent *xe;

	if(e->type != Expose)
		return;
	xe = (XExposeEvent*)e;
	r.min.x = xe->x;
	r.min.y = xe->y;
	r.max.x = xe->x + xe->width;
	r.max.y = xe->y + xe->height;
	drawflushr(r);
}

static void
xkeyboard(XEvent *e)
{
	KeySym k;

	/*
	 * I tried using XtGetActionKeysym, but it didn't seem to
	 * do case conversion properly
	 * (at least, with Xterminal servers and R4 intrinsics)
	 */
	if(e->xany.type != KeyPress)
		return;


	XLookupString((XKeyEvent*)e, NULL, 0, &k, NULL);

	if(k == XK_Multi_key || k == NoSymbol)
		return;
	if(k&0xFF00){
		switch(k){
		case XK_BackSpace:
		case XK_Tab:
		case XK_Escape:
		case XK_Delete:
		case XK_KP_0:
		case XK_KP_1:
		case XK_KP_2:
		case XK_KP_3:
		case XK_KP_4:
		case XK_KP_5:
		case XK_KP_6:
		case XK_KP_7:
		case XK_KP_8:
		case XK_KP_9:
		case XK_KP_Divide:
		case XK_KP_Multiply:
		case XK_KP_Subtract:
		case XK_KP_Add:
		case XK_KP_Decimal:
			k &= 0x7F;
			break;
		case XK_Linefeed:
			k = '\r';
			break;
		case XK_KP_Space:
			k = ' ';
			break;
		case XK_Home:
		case XK_KP_Home:
			k = Khome;
			break;
		case XK_Left:
		case XK_KP_Left:
			k = Kleft;
			break;
		case XK_Up:
		case XK_KP_Up:
			k = Kup;
			break;
		case XK_Down:
		case XK_KP_Down:
			k = Kdown;
			break;
		case XK_Right:
		case XK_KP_Right:
			k = Kright;
			break;
		case XK_Page_Down:
		case XK_KP_Page_Down:
			k = Kpgdown;
			break;
		case XK_End:
		case XK_KP_End:
			k = Kend;
			break;
		case XK_Page_Up:	
		case XK_KP_Page_Up:
			k = Kpgup;
			break;
		case XK_Insert:
		case XK_KP_Insert:
			k = Kins;
			break;
		case XK_KP_Enter:
		case XK_Return:
			k = '\n';
			break;
		case XK_Alt_L:
		case XK_Alt_R:
			k = Kalt;
			break;
		case XK_Shift_L:
		case XK_Shift_R:
		case XK_Control_L:
		case XK_Control_R:
		case XK_Caps_Lock:
		case XK_Shift_Lock:

		case XK_Meta_L:
		case XK_Meta_R:
		case XK_Super_L:
		case XK_Super_R:
		case XK_Hyper_L:
		case XK_Hyper_R:
			return;
		default:		/* not ISO-1 or tty control */
  			if(k>0xff){
				k = keysym2ucs(k); /* supplied by X */
				if(k == -1)
					return;
			}
			break;
		}
	}

	/* Compensate for servers that call a minus a hyphen */
	if(k == XK_hyphen)
		k = XK_minus;
	/* Do control mapping ourselves if translator doesn't */
	if(e->xkey.state&ControlMask)
		k &= 0x9f;
	if(k == NoSymbol) {
		return;
	}

	kbdputc(kbdq, k);
}

static void
xmouse(XEvent *e)
{
	Mousestate ms;
	int i, s;
	XButtonEvent *be;
	XMotionEvent *me;

	if(putsnarf != assertsnarf){
		assertsnarf = putsnarf;
		XSetSelectionOwner(xkmcon, XA_PRIMARY, xdrawable, CurrentTime);
		if(clipboard != None)
			XSetSelectionOwner(xkmcon, clipboard, xdrawable, CurrentTime);
		XFlush(xkmcon);
	}

	switch(e->type){
	case ButtonPress:
		be = (XButtonEvent *)e;
		/* 
		 * Fake message, just sent to make us announce snarf.
		 * Apparently state and button are 16 and 8 bits on
		 * the wire, since they are truncated by the time they
		 * get to us.
		 */
		if(be->send_event
		&& (~be->state&0xFFFF)==0
		&& (~be->button&0xFF)==0)
			return;
		ms.xy.x = be->x;
		ms.xy.y = be->y;
		s = be->state;
		ms.msec = be->time;
		switch(be->button){
		case 1:
			s |= Button1Mask;
			break;
		case 2:
			s |= Button2Mask;
			break;
		case 3:
			s |= Button3Mask;
			break;
		case 4:
			s |= Button4Mask;
			break;
		case 5:
			s |= Button5Mask;
			break;
		}
		break;
	case ButtonRelease:
		be = (XButtonEvent *)e;
		ms.xy.x = be->x;
		ms.xy.y = be->y;
		ms.msec = be->time;
		s = be->state;
		switch(be->button){
		case 1:
			s &= ~Button1Mask;
			break;
		case 2:
			s &= ~Button2Mask;
			break;
		case 3:
			s &= ~Button3Mask;
			break;
		case 4:
			s &= ~Button4Mask;
			break;
		case 5:
			s &= ~Button5Mask;
			break;
		}
		break;
	case MotionNotify:
		me = (XMotionEvent *)e;
		s = me->state;
		ms.xy.x = me->x;
		ms.xy.y = me->y;
		ms.msec = me->time;
		break;
	default:
		return;
	}

	ms.buttons = 0;
	if(s & Button1Mask)
		ms.buttons |= 1;
	if(s & Button2Mask)
		ms.buttons |= 2;
	if(s & Button3Mask)
		ms.buttons |= 4;
	if(s & Button4Mask)
		ms.buttons |= 8;
	if(s & Button5Mask)
		ms.buttons |= 16;

	lock(&mouse.lk);
	i = mouse.wi;
	if(mousequeue) {
		if(i == mouse.ri || mouse.lastb != ms.buttons || mouse.trans) {
			mouse.wi = (i+1)%Mousequeue;
			if(mouse.wi == mouse.ri)
				mouse.ri = (mouse.ri+1)%Mousequeue;
			mouse.trans = mouse.lastb != ms.buttons;
		} else {
			i = (i-1+Mousequeue)%Mousequeue;
		}
	} else {
		mouse.wi = (i+1)%Mousequeue;
		mouse.ri = i;
	}
	mouse.queue[i] = ms;
	mouse.lastb = ms.buttons;
	unlock(&mouse.lk);
	wakeup(&mouse.r);
}

void
getcolor(ulong i, ulong *r, ulong *g, ulong *b)
{
	ulong v;
	
	v = cmap2rgb(i);
	*r = (v>>16)&0xFF;
	*g = (v>>8)&0xFF;
	*b = v&0xFF;
}

void
setcolor(ulong i, ulong r, ulong g, ulong b)
{
	/* no-op */
}

int
atlocalconsole(void)
{
	char *p, *q;
	char buf[128];

	p = getenv("DRAWTERM_ATLOCALCONSOLE");
	if(p && atoi(p) == 1)
		return 1;

	p = getenv("DISPLAY");
	if(p == nil)
		return 0;

	/* extract host part */
	q = strchr(p, ':');
	if(q == nil)
		return 0;
	*q = 0;

	if(strcmp(p, "") == 0)
		return 1;

	/* try to match against system name (i.e. for ssh) */
	if(gethostname(buf, sizeof buf) == 0){
		if(strcmp(p, buf) == 0)
			return 1;
		if(strncmp(p, buf, strlen(p)) == 0 && buf[strlen(p)]=='.')
			return 1;
	}

	return 0;
}

/*
 * Cut and paste.  Just couldn't stand to make this simple...
 */

typedef struct Clip Clip;
struct Clip
{
	char buf[SnarfSize];
	QLock lk;
};
Clip clip;

#undef long	/* sic */
#undef ulong

static char*
_xgetsnarf(Display *xd)
{
	uchar *data, *xdata;
	Atom clipboard, type, prop;
	unsigned long len, lastlen, dummy;
	int fmt, i;
	Window w;

	qlock(&clip.lk);
	/*
	 * Have we snarfed recently and the X server hasn't caught up?
	 */
	if(putsnarf != assertsnarf)
		goto mine;

	/*
	 * Is there a primary selection (highlighted text in an xterm)?
	 */
	clipboard = XA_PRIMARY;
	w = XGetSelectionOwner(xd, XA_PRIMARY);
	if(w == xdrawable){
	mine:
		data = (uchar*)strdup(clip.buf);
		goto out;
	}

	/*
	 * If not, is there a clipboard selection?
	 */
	if(w == None && clipboard != None){
		clipboard = clipboard;
		w = XGetSelectionOwner(xd, clipboard);
		if(w == xdrawable)
			goto mine;
	}

	/*
	 * If not, give up.
	 */
	if(w == None){
		data = nil;
		goto out;
	}
		
	/*
	 * We should be waiting for SelectionNotify here, but it might never
	 * come, and we have no way to time out.  Instead, we will clear
	 * local property #1, request our buddy to fill it in for us, and poll
	 * until he's done or we get tired of waiting.
	 *
	 * We should try to go for utf8string instead of XA_STRING,
	 * but that would add to the polling.
	 */
	prop = 1;
	XChangeProperty(xd, xdrawable, prop, XA_STRING, 8, PropModeReplace, (uchar*)"", 0);
	XConvertSelection(xd, clipboard, XA_STRING, prop, xdrawable, CurrentTime);
	XFlush(xd);
	lastlen = 0;
	for(i=0; i<10 || (lastlen!=0 && i<30); i++){
		usleep(100*1000);
		XGetWindowProperty(xd, xdrawable, prop, 0, 0, 0, AnyPropertyType,
			&type, &fmt, &dummy, &len, &data);
		if(lastlen == len && len > 0)
			break;
		lastlen = len;
	}
	if(i == 10){
		data = nil;
		goto out;
	}
	/* get the property */
	data = nil;
	XGetWindowProperty(xd, xdrawable, prop, 0, SnarfSize/sizeof(unsigned long), 0, 
		AnyPropertyType, &type, &fmt, &len, &dummy, &xdata);
	if((type != XA_STRING && type != utf8string) || len == 0){
		if(xdata)
			XFree(xdata);
		data = nil;
	}else{
		if(xdata){
			data = (uchar*)strdup((char*)xdata);
			XFree(xdata);
		}else
			data = nil;
	}
out:
	qunlock(&clip.lk);
	return (char*)data;
}

static void
_xputsnarf(Display *xd, char *data)
{
	XButtonEvent e;

	if(strlen(data) >= SnarfSize)
		return;
	qlock(&clip.lk);
	strcpy(clip.buf, data);

	/* leave note for mouse proc to assert selection ownership */
	putsnarf++;

	/* send mouse a fake event so snarf is announced */
	memset(&e, 0, sizeof e);
	e.type = ButtonPress;
	e.window = xdrawable;
	e.state = ~0;
	e.button = ~0;
	XSendEvent(xd, xdrawable, True, ButtonPressMask, (XEvent*)&e);
	XFlush(xd);
	qunlock(&clip.lk);
}

static void
xselect(XEvent *e, Display *xd)
{
	char *name;
	XEvent r;
	XSelectionRequestEvent *xe;
	Atom a[4];

	if(e->xany.type != SelectionRequest)
		return;

	memset(&r, 0, sizeof r);
	xe = (XSelectionRequestEvent*)e;
if(0) iprint("xselect target=%d requestor=%d property=%d selection=%d\n",
	xe->target, xe->requestor, xe->property, xe->selection);
	r.xselection.property = xe->property;
	if(xe->target == targets){
		a[0] = XA_STRING;
		a[1] = utf8string;
		a[2] = text;
		a[3] = compoundtext;

		XChangeProperty(xd, xe->requestor, xe->property, xe->target,
			8, PropModeReplace, (uchar*)a, sizeof a);
	}else if(xe->target == XA_STRING || xe->target == utf8string || xe->target == text || xe->target == compoundtext){
		/* if the target is STRING we're supposed to reply with Latin1 XXX */
		qlock(&clip.lk);
		XChangeProperty(xd, xe->requestor, xe->property, xe->target,
			8, PropModeReplace, (uchar*)clip.buf, strlen(clip.buf));
		qunlock(&clip.lk);
	}else{
		iprint("get %d\n", xe->target);
		name = XGetAtomName(xd, xe->target);
		if(name == nil)
			iprint("XGetAtomName failed\n");
		else if(strcmp(name, "TIMESTAMP") != 0)
			iprint("%s: cannot handle selection request for '%s' (%d)\n", argv0, name, (int)xe->target);
		r.xselection.property = None;
	}

	r.xselection.display = xe->display;
	/* r.xselection.property filled above */
	r.xselection.target = xe->target;
	r.xselection.type = SelectionNotify;
	r.xselection.requestor = xe->requestor;
	r.xselection.time = xe->time;
	r.xselection.send_event = True;
	r.xselection.selection = xe->selection;
	XSendEvent(xd, xe->requestor, False, 0, &r);
	XFlush(xd);
}

char*
clipread(void)
{
	return _xgetsnarf(xsnarfcon);
}

int
clipwrite(char *buf)
{
	_xputsnarf(xsnarfcon, buf);
	return 0;
}

