/*
 * plan9.c -- a graphics driver for Plan 9
 * (c) 2003 Andrey Mirtchovski
 */

/* #define PLAN9_DEBUG /* */

#ifdef PLAN9_DEBUG
#define MESSAGE print
#endif

#include <u.h>
#include <libc.h>
#include <draw.h>
#include <event.h>
#include <sys/time.h>
#include <errno.h>

typedef struct graphics_driver Gfxdrvr;
typedef struct graphics_device Gfxdev;
typedef struct bitmap Libitmap;

struct bitmap {
	int	x, y;	/* Dimensions */
	int	skip;	/* Bytes between vertically consecutive pixels */
	void	*data;	/* Pointer to room for topleft pixel */
	void	*user;	/* For implementing LRU algor by higher layer or what*/
	void	*flags;	/* Allocation flags for the driver */
};

struct rect {
	int	x1, x2, y1, y2;
};
struct graphics_device {
        /* Only graphics driver is allowed to write to this */

        struct rect size;	/* Size of the window */
        /* int left, right, top, bottom; */
	struct rect clip;
        /* right, bottom are coords of the first point outside clipping area */

        Gfxdrvr	*drv;
        void	*driver_data;

        /* Only user is allowed to write here, driver inits to zero's */
        void	*user_data;
        void	(*redraw_handler)(Gfxdev *dev, struct rect *r);
        void	(*resize_handler)(Gfxdev *dev);
        void	(*keyboard_handler)(Gfxdev *dev, int key, int flags);
        void	(*mouse_handler)(Gfxdev *dev, int x, int y, int buttons);
};

struct graphics_driver {
	uchar	*name;

	/* param is get from get_driver_param and saved into configure file */
	uchar *(*init_driver)(uchar *param, uchar *display);

	/* Creates new device and returns pointer to it */
	Gfxdev	*(* init_device)(void);
	void	(*shutdown_device)(Gfxdev *dev);  /* Destroys the device */
	void	(*shutdown_driver)(void);

	/* returns allocated string with parameter given to init_driver() */
	uchar	*(*get_driver_param)(void);

	/* dest must have x and y filled in when get_empty_bitmap is called */
	int	(*get_empty_bitmap)(Libitmap *dest);

	/* dest must have x & y filled in when get_filled_bitmap is called. */
	/* bitmap is created, pre-filled with n_bytes of pattern & registered */
	int	(*get_filled_bitmap)(Libitmap *dest, long color);
	void	(*register_bitmap)(Libitmap *bmp);
	void	*(*prepare_strip)(Libitmap *bmp, int top, int lines);
	void	(*commit_strip)(Libitmap *bmp, int top, int lines);

	/* Must not touch x and y. Suitable for re-registering. */
	void	(*unregister_bitmap)(Libitmap *bmp);
	void	(*draw_bitmap)(Gfxdev *dev, Libitmap *hndl, int x, int y);
	void	(*draw_bitmaps)(Gfxdev *dev, Libitmap **hndls, int n, int x, int y);

	/*
	 * rgb has gamma 1/display_gamma; 255 means exactly the largest
	 * sample the display is able to produce.
	 * Thus, if we have 3 bits for red, we will perform this code:
	 *	red = (red*7 +127)/255;
	 */
	long	(*get_color)(int rgb);
	void	(*fill_area)(Gfxdev *dev, int x1, int y1, int x2, int y2, long color);
	void	(*draw_hline)(Gfxdev *dev, int left, int y, int right, long color);
	void	(*draw_vline)(Gfxdev *dev, int x, int top, int bottom, long color);
	int	(*hscroll)(Gfxdev *dev, struct rect_set **set, int sc);
	int	(*vscroll)(Gfxdev *dev, struct rect_set **set, int sc);

	/*
	 * When scrolling, empty spaces will have undefined contents. returns:
	 *  0 - the caller should not care about redrawing, redraw will be sent
	 *  1 - the caller should redraw uncovered area.
	 * when set is not NULL, rectangles in the set (uncovered area)
	 * should be redrawn.
	 */
	void	(*set_clip_area)(Gfxdev *dev, struct rect *r);

	/* restore old videomode and disable drawing on terminal */
	int	(*block)(Gfxdev *dev);
	void	(*unblock)(Gfxdev *dev);	/* reenable the terminal */
	/*
	 * set window title. title is in utf-8 encoding -- you should recode
	 * it to device charset. if device doesn't support titles (svgalib,
	 * framebuffer), this should be NULL, not empty function!
	 */
	void	(*set_title)(Gfxdev *dev, uchar *title);

	/*
	 * -if !NULL executes command on this graphics device,
	 * -if NULL links uses generic (console) command executing functions
	 * -return value is the same as of the 'system' syscall
	 * -if flag is !0, run command in separate shell
	 * else run command directly
	 */
	int	(*exec)(uchar *command, int flag);

	/*
	 * Data layout
	 * depth
	 *  8 7 6 5 4 3 2 1 0
	 * +-+---------+-----+
	 * | |         |     |
	 * +-+---------+-----+
	 *
	 * 0 - 2 Number of bytes per pixel in passed bitmaps
	 * 3 - 7 Number of significant bits per pixel -- 1, 4, 8, 15, 16, 24
	 * 8   0-- normal order, 1-- misordered.  Has the same value as
	 *	vga_misordered from the VGA mode.
	 *
	 * This number is to be used by the layer that generates images.
	 * Memory layout for 1 bytes per pixel is:
	 * 2 colors:
	 *  7 6 5 4 3 2 1 0
	 * +-------------+-+
	 * |      0      |B| B is The Bit. 0 black, 1 white
	 * +-------------+-+
	 *
	 * 16 colors:
	 *  7 6 5 4 3 2 1 0
	 * +-------+-------+
	 * |   0   | PIXEL | Pixel is 4-bit index into palette
	 * +-------+-------+
	 *
	 * 256 colors:
	 *  7 6 5 4 3 2 1 0
	 * +---------------+
	 * |  --PIXEL--    | Pixels is 8-bit index into palette
	 * +---------------+
	 */
	int	depth;

	/* size of screen. only for drivers that use virtual devices */
	int	x, y;
	int	flags;			/* GD_xxx flags */
	int	codepage;
	/*
	 * -if exec is NULL string is unused
	 * -otherwise this string describes shell to be executed by the
	 * exec function, the '%' char means string to be executed
	 * -shell cannot be NULL; if exec is !NULL and shell is empty,
	 * exec should use some default shell (e.g. "xterm -e %")
	 */
	uchar	*shell;
};

struct style{
	int refcount;
	unsigned char r0, g0, b0, r1, g1, b1;
       	/* ?0 are background, ?1 foreground.
	 * These are unrounded 8-bit sRGB space 
	 */
	int height;
	int flags; /* non-zero means underline */
	long underline_color; /* Valid only if flags are nonzero */
	int *table; /* First is refcount, then n_fonts entries. Total
                     * size is n_fonts+1 integers.
		     */
	int mono_space; /* -1 if the font is not monospaced
			 * width of the space otherwise
			 */
	int mono_height; /* Height of the space if mono_space is >=0
			  * undefined otherwise
			  */
};

#define KBD_SHIFT	1
#define KBD_CTRL	2
#define KBD_ALT		4

#define KBD_ENTER	-0x100
#define KBD_BS		-0x101
#define KBD_TAB		-0x102
#define KBD_ESC		-0x103
#define KBD_LEFT	-0x104
#define KBD_RIGHT	-0x105
#define KBD_UP		-0x106
#define KBD_DOWN	-0x107
#define KBD_INS		-0x108
#define KBD_DEL		-0x109
#define KBD_HOME	-0x10a
#define KBD_END		-0x10b
#define KBD_PAGE_UP	-0x10c
#define KBD_PAGE_DOWN	-0x10d

#define BM_BUTT		15
#define B_LEFT		0
#define B_MIDDLE	1
#define B_RIGHT		2

#define BM_ACT		48
#define B_DOWN		0
#define B_UP		16
#define B_DRAG		32
#define B_MOVE		48

static Image *colors[256];

enum{
	Font13 = 0,
	Font15,
	Font19,
	Font22,
	Font30,
	FontMono,
	FontLast,
};
Font *fnt;	/* monospace and normal */

extern Gfxdrvr plan9_driver;
Gfxdev gd;			/* only a single window! */

char *buttons[] =
{
	"exit",
	0
};

Menu menu =
{
	buttons
};
Mouse m;

void
eresized(int new)
{
	if(new) {
		if(getwindow(display, Refnone) < 0)
			exits("can't reattach to window");
		gd.size.x2 = Dx(screen->r);
		gd.size.y2 = Dy(screen->r);
		gd.clip = gd.size;
		gd.resize_handler(&gd);
	}
}

static int	
plan9_translate_key(int key, int *k, int *f)
{
	*f = 0;
	if (key == 0177)		/* delete */
		exits(0);

	switch (key) {
	case '\b':	
		*k = KBD_BS;
		break;
	case '\t':	
		*k = KBD_TAB;
		break;
	case '\n':	
		*k = KBD_ENTER;
		break;
	case 13:	
		*k = KBD_END;
		break;
	case 033:	
		*k = KBD_ESC;
		break;
	case 0x80:
		*k = KBD_DOWN;
		break;

	/* within Unicode private-use space */
	case 0xf00d:
		*k = KBD_HOME;
		break;
	case 0xf00e:
		*k = KBD_UP;
		break;
	case 0xf00f:
		*k = KBD_PAGE_UP;
		break;
	case 0xf011:
		*k = KBD_LEFT;
		break;
	case 0xf012:
		*k = KBD_RIGHT;
		break;
	case 0xf013:
		*k = KBD_PAGE_DOWN;
		break;
	case 0xf014:
		/* Ins, simulates '\f': refresh */
		*k = 'L';
		*f = KBD_CTRL;
		break;

	default:
		*k = key;
		break;
	}
	return 1;
}

static void
plan9_process_kbd(Event *ep)
{
	int key = ep->kbdc, k, f;

#ifdef PLAN9_DEBUG
MESSAGE("plan9_process_kbd: %c\n", key);
#endif
	if (plan9_translate_key(key, &k, &f))
		gd.keyboard_handler(&gd, k, f);
}

static Mouse oldm;

static void
plan9_process_mouse(Event *ep)
{
	Mouse m;
	int but = 0, evt = 0;

	m = ep->mouse;
	m.xy.x = m.xy.x - screen->r.min.x;
	m.xy.y = m.xy.y - screen->r.min.y;
#ifdef PLAN9_DEBUG
MESSAGE("plan9_process_mouse: %P, %d\n", m.xy, m.buttons);
#endif
	evt = B_MOVE;

	if(m.buttons & 1) {
		but = B_LEFT;
		if(oldm.buttons & 1)
			evt = B_DRAG;
		 else
			evt = B_DOWN;
	} else if(oldm.buttons & 1) {
		but = B_LEFT;
		evt = B_UP;
	}

	if(m.buttons & 2) {
		but = B_MIDDLE;
		if(oldm.buttons & 2)
			evt = B_DRAG;
		 else
			evt = B_DOWN;
	} else if(oldm.buttons & 2) {
		but = B_MIDDLE;
		evt = B_UP;
	}

	if(m.buttons & 4) {
		but = B_RIGHT;
		if(oldm.buttons & 4)
			evt = B_DRAG;
		 else
			evt = B_DOWN;
	} else if(oldm.buttons & 4) {
		but = B_RIGHT;
		evt = B_UP;
	}

	gd.mouse_handler(&gd, m.xy.x, m.xy.y, but|evt);

	oldm.buttons = m.buttons;
	oldm.msec = m.msec;
	oldm.xy = m.xy;
}

static void
redraw(void)
{
	Gfxdev *dev = &gd;
	struct rect r = {0, dev->size.x2, 0, dev->size.y2};

	t_redraw(dev, &r);
}

/*
 * closedisplay() and drawshutdown() in the event library set display to nil.
 * damn it to hell.
 */
static volatile int einited;
int evpipe[2];

static struct shared {
	ulong	key;
	Event	ev;
} sh;
static struct shared *shp = &sh;

extern int shootme;

static int
watchevents(void)
{
	char c;
	int pid;

	einit(Ekeyboard|Emouse);
	pid = rfork(RFPROC|RFMEM);
	if (pid == 0) {			/* child: event-watching process */
		for (;;) {
			if(display == nil)
				exits(0);
			shp->key = event(&shp->ev);
			/* there are events! */
			if (apewrite(evpipe[0], "x", 1) != 1)
				break;
			/* wait for event processing */
			if (aperead(evpipe[0], &c, 1) != 1)
				break;
		}
		shootme = 0;
		einited = 0;
		_exits(0);
	}
	/* parent */
	shootme = pid;
	seteventbit(evpipe[1]);
	einited = 1;
}

static int
processevent(Event *ep, ulong key)
{
	switch (key) {
	case Emouse:
		plan9_process_mouse(ep);
		break;
	case Ekeyboard:
		plan9_process_kbd(ep);
		break;
	default:
		fprint(2, "martian input event!\n");
		break;
	}
}

static int
processevents(void)
{
	char c;

	if (aperead(evpipe[1], &c, 1) != 1) {	/* wait for first event */
		einited = 0;
		return;
	}
	processevent(&shp->ev, shp->key);
	while (ecankbd() || ecanmouse()) {
		shp->key = eread(Ekeyboard|Emouse, &shp->ev);
		processevent(&shp->ev, shp->key);
	}
	if (apewrite(evpipe[1], "y", 1) != 1)	/* events are processed */
		einited = 0;
}

static void
update(void)
{
	if (display != nil)
		flushimage(display, 1);
}

static void
repaint(void)
{
	static time_t lastdraw, now;

	if (display != nil) {
		time(&now);
		if (now - lastdraw >= 5) {	/* catch stragglers */
			redraw();
			lastdraw = now;
		}
		update();
	}
}

int
plan9_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	struct timeval *timeout)
{
	int fds;
	static int first = 1;

#ifdef PLAN9_DEBUG
	MESSAGE("plan9_select\n");
#endif
	if (first) {
		first = 0;
		repaint();
	}
	check_bottom_halves();
	fds = select(n, readfds, writefds, exceptfds, timeout);
	if (fds > 0 && einited && isseteventbit(evpipe[1]))
		processevents();
	repaint();		/* enforce periodic screen update */
	return fds;
}

static uchar *
plan9_init_driver(uchar *param, uchar *d)
{
	int i;

#ifdef PLAN9_DEBUG
	MESSAGE("plan9 init driver\n");
#endif
	if(initdraw(nil, nil, "flame") < 0)
		exits("initdraw failed");

	for (i = 0; i < 256; i++)
		colors[i] = allocimage(display, Rect(0, 0, 1, 1), screen->chan,
			1, cmap2rgba(i));

	fnt = openfont(display, "/lib/font/bit/lucm/unicode.9.font");

	eresized(0);
	watchevents();
	return nil;
}

static void
plan9_shutdown_driver(void)
{
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_shutdown_driver\n");
#endif
	exits(0);
}

static Gfxdev*
plan9_init_device(void)
{
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_init_device\n");
#endif
	gd.size.x1=0;
	gd.size.y1=0;
	gd.size.x2=Dx(screen->r);
	gd.size.y2=Dy(screen->r);

	gd.clip.x1=gd.size.x1;
	gd.clip.y1=gd.size.y1;
	gd.clip.x2=gd.size.x2;
	gd.clip.y2=gd.size.y2;
	gd.drv= &plan9_driver;
	gd.driver_data=nil;
	gd.user_data=0;

	return &gd;
}

/* close window */
static void
plan9_shutdown_device(Gfxdev *gd)
{
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_shutdown_device\n");
#endif
	exits("shutdown device");
}

static int
plan9_get_filled_bitmap(Libitmap *bmp, long color)
{
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_get_filled_bitmap\n");
#endif
	bmp->skip = bmp->x * 3;
	bmp->data = mallocz(bmp->skip * bmp->y, 0);
	bmp->flags = 0;
	return 0;
}

static int
plan9_get_empty_bitmap(Libitmap *bmp)
{
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_get_empty_bitmap\n");
#endif
	bmp->skip=bmp->x*3;
	bmp->data=malloc(bmp->skip*bmp->y);
	bmp->flags=0;
	return 0;
}

static void
plan9_unregister_bitmap(Libitmap *bmp)
{
}

static void
plan9_fill_area(Gfxdev *dev, int x1, int y1, int x2, int y2, long color)
{
	Rectangle rt;
	int r, g, b;

	rt = rectaddpt(Rect(x1, y1, x2, y2), screen->r.min);
	b = (color>>16) & 0xff;
	g = (color>>8) & 0xff;
	r = (color) & 0xff;
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_fill_area: %R; color: %ld\n", rt, color);
#endif
	draw(screen, rt, colors[rgb2cmap(r, g, b)], nil, ZP);
	// update();
}

static void
plan9_draw_hline(Gfxdev *dev, int left, int y, int right, long color)
{
	Point p1, p2;
	int r, g, b;

	b = (color>>16) & 0xff;
	g = (color>>8) & 0xff;
	r = (color) & 0xff;

	p1 = addpt(screen->r.min, Pt(left, y));
	p2 = addpt(screen->r.min, Pt(right, y));
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_draw_hline: %R\n", Rpt(p1, p2));
#endif

	line(screen, p1, p2, Endsquare, Endsquare, 1, colors[rgb2cmap(r, g, b)], ZP);
	// update();

}

static void
plan9_draw_vline(Gfxdev *dev, int x, int top, int bottom, long color)
{
	Point p1, p2;
	int r, g, b;

	b = (color>>16) & 0xff;
	g = (color>>8) & 0xff;
	r = (color) & 0xff;

	p1 = addpt(screen->r.min, Pt(x, top));
	p2 = addpt(screen->r.min, Pt(x, bottom));

#ifdef PLAN9_DEBUG
	MESSAGE("plan9_draw_vline: %R\n", Rpt(p1, p2));
#endif

	line(screen, p1, p2, Endsquare, Endsquare, 1, colors[rgb2cmap(r, g, b)], ZP);
	// update();
}

static void
plan9_set_clip_area(Gfxdev *dev, struct rect *r)
{
	Rectangle cr;

	gd.clip = *r;
	cr = Rect(r->x1-1, r->y1-1, r->x2+1, r->y2+1);

#ifdef PLAN9_DEBUG
	print("plan9_set_clip_area: %R\n", cr);
#endif
	replclipr(screen, screen->repl, rectaddpt(cr, screen->r.min));
}

static void
draw_bitmap(Gfxdev *dev, Libitmap *bmp, int x, int y)
{
	Image *img;
	Rectangle r;
	ulong i;

	r = Rect(0, 0, bmp->x, bmp->y);
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_draw_bitmap: size: %R at %P\n", r, Pt(x, y));
#endif
	if (display == nil)
		return;
	img = allocimage(display, r, RGB24, 0, 0);
	if(!img)
		return;

	loadimage(img, r, bmp->data, bmp->y*bmp->skip);
	draw(screen, rectaddpt(r, addpt(Pt(x, y), screen->r.min)), img, nil, ZP);
	freeimage(img);
}

static void
plan9_draw_bitmap(Gfxdev *dev, Libitmap *bmp, int x, int y)
{
	draw_bitmap(dev, bmp, x, y);
	update();
}

static void
plan9_draw_bitmaps(Gfxdev *dev, Libitmap **bmps, int n, int x, int y)
{
	int a;
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_draw_bitmaps\n");
#endif
	print("plan9_draw_bitmaps\n");

	if (!bmps)
		return;
	for (a = 0; a < n; a++) {
		draw_bitmap(dev, bmps[a], x, y);
		x += bmps[a]->x;
	}
	update();
}

static void
plan9_register_bitmap(Libitmap *bmp)
{
}

int
plan9_hscroll(Gfxdev *dev, struct rect_set **set, int sc)
{
	Rectangle r;
	int i;

#ifdef PLAN9_DEBUG
	MESSAGE("hscroll\n");
#endif

	r = Rect(gd.clip.x1, gd.clip.y1, gd.clip.x2, gd.clip.y2);
	r = rectaddpt(r, screen->r.min);
	draw(screen, r, screen, nil, addpt(Pt(gd.clip.x1-sc, gd.clip.y1), screen->r.min));
	// update();
	return 1;
}


int plan9_vscroll(Gfxdev *dev, struct rect_set **set, int sc)
{
	Rectangle r;
	int i;

#ifdef PLAN9_DEBUG
	MESSAGE("vscroll\n");
#endif

	r = Rect(gd.clip.x1, gd.clip.y1, gd.clip.x2, gd.clip.y2);
	r = rectaddpt(r, screen->r.min);
	draw(screen, r, screen, nil, addpt(Pt(gd.clip.x1, gd.clip.y1-sc), screen->r.min));
	// update();
	return 1;
}


void *
plan9_prepare_strip(Libitmap *bmp, int top, int lines)
{
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_prepare_strip\n");
#endif
	return (char *)bmp->data + bmp->skip*top;
}

void plan9_commit_strip(Libitmap *bmp, int top, int lines)
{
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_commit_strip\n");
#endif
}

void
plan9_set_window_title(Gfxdev *gd, uchar *title)
{
	int fd;

#ifdef PLAN9_DEBUG
	MESSAGE("plan9_set_window_title: %s\n", title);
#endif
	if(title == nil)
		return;
	fd = open("/dev/label", OWRITE);
	if(fd >= 0) {
		write(fd, title, strlen((char *)title));
		close(fd);
	}
}

void
plan9_set_clipboard_text(Gfxdev *gd, uchar *text)
{
	int fd;

	if(text == nil)
		return;
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_set_clipboard_text: %s\n", text);
#endif
	fd = open("/dev/snarf", OWRITE);
	if(fd >= 0) {
		write(fd, text, utflen((char *)text));
		close(fd);
	}
}

static uchar *
plan9_get_driver_param(void)
{
	return nil;
}

static int
plan9_exec(uchar *command, int fg)
{
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_exec: %s\n", command);
#endif
	return 0;
}

static int
plan9_block(Gfxdev *)
{
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_block\n");
#endif
	return 0;
}

static void
plan9_unblock(Gfxdev *)
{
#ifdef PLAN9_DEBUG
	MESSAGE("plan9_unblock\n");
#endif
}

static Font * 
getfont(int size)
{
	return fnt;

}

/* return the width of a character */
int 
plan9_charsize(struct style *s, char c)
{
	return stringnwidth(fnt, &c, 1);
}

/* return the length of a string */
int
plan9_stringsize(struct style *s, char *text)
{
	return stringwidth(fnt, text);
}

/* should return the width (in pixels) that the text occupies */
int 
plan9_print_text(struct graphics_driver *gd, struct
	graphics_device *device, int x, int y, struct style *style,
	char *text)

{	
	Point p;
	Font *f;
	char *c;

	while((c = strchr(text, 0x01)) != nil) 
		*c = ' ';

	f = getfont(style->height);

	p = addpt(Pt(x, y), screen->r.min);

	stringbg(screen, p, 
		colors[rgb2cmap(style->r1, style->g1, style->b1)],
		ZP, f, text,
		colors[rgb2cmap(style->r0, style->g0, style->b0)],
		ZP);
	
	flushimage(display, 1);


	p = stringsize(f, text);
	return p.x;
}



extern long (*color_pass_rgb)(int);

Gfxdrvr plan9_driver={
	(uchar *)"plan9",
	plan9_init_driver,
	plan9_init_device,
	plan9_shutdown_device,
	plan9_shutdown_driver,
	plan9_get_driver_param,
	plan9_get_empty_bitmap,
	plan9_get_filled_bitmap,
	plan9_register_bitmap,
	plan9_prepare_strip,
	plan9_commit_strip,
	plan9_unregister_bitmap,
	plan9_draw_bitmap,
	plan9_draw_bitmaps,
	color_pass_rgb,
	plan9_fill_area,
	plan9_draw_hline,
	plan9_draw_vline,
	plan9_hscroll,
	plan9_vscroll,
	plan9_set_clip_area,
	plan9_block,
	plan9_unblock,
	plan9_set_window_title,
	plan9_exec,
	195,			/* depth */
	0, 0,			/* size  */
	0,			/* flags */
	0,			/* codepage */
	(uchar *)"/bin/rc",	/* shell */
};
