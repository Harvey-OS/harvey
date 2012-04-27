#define _WIN32_WINNT 0x0500
#include	<windows.h>

#undef Rectangle
#define Rectangle _Rectangle

#include "u.h"
#include "lib.h"
#include "kern/dat.h"
#include "kern/fns.h"
#include "error.h"
#include "user.h"
#include <draw.h>
#include <memdraw.h>
#include "screen.h"
#include "keyboard.h"

Memimage	*gscreen;
Screeninfo	screen;

extern int mousequeue;
static int depth;

static	HINSTANCE	inst;
static	HWND		window;
static	HPALETTE	palette;
static	LOGPALETTE	*logpal;
static  Lock		gdilock;
static 	BITMAPINFO	*bmi;
static	HCURSOR		hcursor;

static void	winproc(void *);
static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
static void	paletteinit(void);
static void	bmiinit(void);

static int readybit;
static Rendez	rend;

Point	ZP;

static int
isready(void*a)
{
	return readybit;
}

void
screeninit(void)
{
	int fmt;
	int dx, dy;

	memimageinit();
	if(depth == 0)
		depth = GetDeviceCaps(GetDC(NULL), BITSPIXEL);
	switch(depth){
	case 32:
		screen.dibtype = DIB_RGB_COLORS;
		screen.depth = 32;
		fmt = XRGB32;
		break;
	case 24:
		screen.dibtype = DIB_RGB_COLORS;
		screen.depth = 24;
		fmt = RGB24;
		break;
	case 16:
		screen.dibtype = DIB_RGB_COLORS;
		screen.depth = 16;
		fmt = RGB15;	/* [sic] */
		break;
	case 8:
	default:
		screen.dibtype = DIB_PAL_COLORS;
		screen.depth = 8;
		depth = 8;
		fmt = CMAP8;
		break;
	}
	dx = GetDeviceCaps(GetDC(NULL), HORZRES);
	dy = GetDeviceCaps(GetDC(NULL), VERTRES);

	gscreen = allocmemimage(Rect(0,0,dx,dy), fmt);
	kproc("winscreen", winproc, 0);
	ksleep(&rend, isready, 0);
}

uchar*
attachscreen(Rectangle *r, ulong *chan, int *depth, int *width, int *softscreen, void **X)
{
	*r = gscreen->r;
	*chan = gscreen->chan;
	*depth = gscreen->depth;
	*width = gscreen->width;
	*softscreen = 1;

	return gscreen->data->bdata;
}

void
flushmemscreen(Rectangle r)
{
	screenload(r, gscreen->depth, byteaddr(gscreen, ZP), ZP,
		gscreen->width*sizeof(ulong));
//	Sleep(100);
}

void
screenload(Rectangle r, int depth, uchar *p, Point pt, int step)
{
	int dx, dy, delx;
	HDC hdc;
	RECT winr;

	if(depth != gscreen->depth)
		panic("screenload: bad ldepth");

	/*
	 * Sometimes we do get rectangles that are off the
	 * screen to the negative axes, for example, when
	 * dragging around a window border in a Move operation.
	 */
	if(rectclip(&r, gscreen->r) == 0)
		return;

	if((step&3) != 0 || ((pt.x*depth)%32) != 0 || ((ulong)p&3) != 0)
		panic("screenload: bad params %d %d %ux", step, pt.x, p);
	dx = r.max.x - r.min.x;
	dy = r.max.y - r.min.y;

	if(dx <= 0 || dy <= 0)
		return;

	if(depth == 24)
		delx = r.min.x % 4;
	else
		delx = r.min.x & (31/depth);

	p += (r.min.y-pt.y)*step;
	p += ((r.min.x-delx-pt.x)*depth)>>3;

	if(GetWindowRect(window, &winr)==0)
		return;
	if(rectclip(&r, Rect(0, 0, winr.right-winr.left, winr.bottom-winr.top))==0)
		return;
	
	lock(&gdilock);

	hdc = GetDC(window);
	SelectPalette(hdc, palette, 0);
	RealizePalette(hdc);

//FillRect(hdc,(void*)&r, GetStockObject(BLACK_BRUSH));
//GdiFlush();
//Sleep(100);

	bmi->bmiHeader.biWidth = (step*8)/depth;
	bmi->bmiHeader.biHeight = -dy;	/* - => origin upper left */

	StretchDIBits(hdc, r.min.x, r.min.y, dx, dy,
		delx, 0, dx, dy, p, bmi, screen.dibtype, SRCCOPY);

	ReleaseDC(window, hdc);

	GdiFlush();
 
	unlock(&gdilock);
}

static void
winproc(void *a)
{
	WNDCLASS wc;
	MSG msg;

	inst = GetModuleHandle(NULL);

	paletteinit();
	bmiinit();
	terminit();

	wc.style = 0;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = inst;
	wc.hIcon = LoadIcon(inst, NULL);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"9pmgraphics";
	RegisterClass(&wc);

	window = CreateWindowEx(
		0,			/* extended style */
		L"9pmgraphics",		/* class */
		L"drawterm screen",		/* caption */
		WS_OVERLAPPEDWINDOW,    /* style */
		CW_USEDEFAULT,		/* init. x pos */
		CW_USEDEFAULT,		/* init. y pos */
		CW_USEDEFAULT,		/* init. x size */
		CW_USEDEFAULT,		/* init. y size */
		NULL,			/* parent window (actually owner window for overlapped)*/
		NULL,			/* menu handle */
		inst,			/* program handle */
		NULL			/* create parms */
		);

	if(window == nil)
		panic("can't make window\n");

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);

	readybit = 1;
	wakeup(&rend);

	screen.reshaped = 0;

	while(GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
//	MessageBox(0, "winproc", "exits", MB_OK);
	ExitProcess(0);
}

int
col(int v, int n)
{
	int i, c;

	c = 0;
	for(i = 0; i < 8; i += n)
		c |= v << (16-(n+i));
	return c >> 8;
}


void
paletteinit(void)
{
	PALETTEENTRY *pal;
	int r, g, b, cr, cg, cb, v;
	int num, den;
	int i, j;

	logpal = mallocz(sizeof(LOGPALETTE) + 256*sizeof(PALETTEENTRY), 1);
	if(logpal == nil)
		panic("out of memory");
	logpal->palVersion = 0x300;
	logpal->palNumEntries = 256;
	pal = logpal->palPalEntry;

	for(r=0,i=0; r<4; r++) {
		for(v=0; v<4; v++,i+=16){
			for(g=0,j=v-r; g<4; g++) {
				for(b=0; b<4; b++,j++){
					den=r;
					if(g>den)
						den=g;
					if(b>den)
						den=b;
					/* divide check -- pick grey shades */
					if(den==0)
						cr=cg=cb=v*17;
					else{
						num=17*(4*den+v);
						cr=r*num/den;
						cg=g*num/den;
						cb=b*num/den;
					}
					pal[i+(j&15)].peRed = cr;
					pal[i+(j&15)].peGreen = cg;
					pal[i+(j&15)].peBlue = cb;
					pal[i+(j&15)].peFlags = 0;
				}
			}
		}
	}
	palette = CreatePalette(logpal);
}


void
getcolor(ulong i, ulong *r, ulong *g, ulong *b)
{
	PALETTEENTRY *pal;

	pal = logpal->palPalEntry;
	*r = pal[i].peRed;
	*g = pal[i].peGreen;
	*b = pal[i].peBlue;
}

void
bmiinit(void)
{
	ushort *p;
	int i;

	bmi = mallocz(sizeof(BITMAPINFOHEADER) + 256*sizeof(RGBQUAD), 1);
	if(bmi == 0)
		panic("out of memory");
	bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi->bmiHeader.biWidth = 0;
	bmi->bmiHeader.biHeight = 0;	/* - => origin upper left */
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = depth;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biSizeImage = 0;
	bmi->bmiHeader.biXPelsPerMeter = 0;
	bmi->bmiHeader.biYPelsPerMeter = 0;
	bmi->bmiHeader.biClrUsed = 0;
	bmi->bmiHeader.biClrImportant = 0;	/* number of important colors: 0 means all */

	p = (ushort*)bmi->bmiColors;
	for(i = 0; i < 256; i++)
		p[i] = i;
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	PAINTSTRUCT paint;
	HDC hdc;
	LONG x, y, b;
	int i;
	Rectangle r;

	switch(msg) {
	case WM_CREATE:
		break;
	case WM_SETCURSOR:
		/* User set */
		if(hcursor != NULL) {
			SetCursor(hcursor);
			return 1;
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	case WM_MOUSEWHEEL:
		if ((int)(wparam & 0xFFFF0000)>0)
			b|=8;
		else
			b|=16;
	case WM_MOUSEMOVE:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		x = LOWORD(lparam);
		y = HIWORD(lparam);
		b = 0;
		if(wparam & MK_LBUTTON)
			b = 1;
		if(wparam & MK_MBUTTON)
			b |= 2;
		if(wparam & MK_RBUTTON) {
			if(wparam & MK_SHIFT)
				b |= 2;
			else
				b |= 4;
		}
		lock(&mouse.lk);
		i = mouse.wi;
		if(mousequeue) {
			if(i == mouse.ri || mouse.lastb != b || mouse.trans) {
				mouse.wi = (i+1)%Mousequeue;
				if(mouse.wi == mouse.ri)
					mouse.ri = (mouse.ri+1)%Mousequeue;
				mouse.trans = mouse.lastb != b;
			} else {
				i = (i-1+Mousequeue)%Mousequeue;
			}
		} else {
			mouse.wi = (i+1)%Mousequeue;
			mouse.ri = i;
		}
		mouse.queue[i].xy.x = x;
		mouse.queue[i].xy.y = y;
		mouse.queue[i].buttons = b;
		mouse.queue[i].msec = ticks();
		mouse.lastb = b;
		unlock(&mouse.lk);
		wakeup(&mouse.r);
		break;

	case WM_CHAR:
		/* repeat count is lparam & 0xf */
		switch(wparam){
		case '\n':
			wparam = '\r';
			break;
		case '\r':
			wparam = '\n';
			break;
		}
		kbdputc(kbdq, wparam);
		break;

	case WM_SYSKEYUP:
		break;
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		switch(wparam) {
		case VK_MENU:
			kbdputc(kbdq, Kalt);
			break;
		case VK_INSERT:
			kbdputc(kbdq, Kins);
			break;
		case VK_DELETE:
//			kbdputc(kbdq, Kdel);
			kbdputc(kbdq, 0x7f);	// should have Kdel in keyboard.h
			break;
		case VK_UP:
			kbdputc(kbdq, Kup);
			break;
		case VK_DOWN:
			kbdputc(kbdq, Kdown);
			break;
		case VK_LEFT:
			kbdputc(kbdq, Kleft);
			break;
		case VK_RIGHT:
			kbdputc(kbdq, Kright);
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_PALETTECHANGED:
		if((HWND)wparam == hwnd)
			break;
	/* fall through */
	case WM_QUERYNEWPALETTE:
		hdc = GetDC(hwnd);
		SelectPalette(hdc, palette, 0);
		if(RealizePalette(hdc) != 0)
			InvalidateRect(hwnd, nil, 0);
		ReleaseDC(hwnd, hdc);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &paint);
		r.min.x = paint.rcPaint.left;
		r.min.y = paint.rcPaint.top;
		r.max.x = paint.rcPaint.right;
		r.max.y = paint.rcPaint.bottom;
		flushmemscreen(r);
		EndPaint(hwnd, &paint);
		break;
	case WM_COMMAND:
	case WM_SETFOCUS:
	case WM_DEVMODECHANGE:
	case WM_WININICHANGE:
	case WM_INITMENU:
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}

void
mouseset(Point xy)
{
	POINT pt;

	pt.x = xy.x;
	pt.y = xy.y;
	MapWindowPoints(window, 0, &pt, 1);
	SetCursorPos(pt.x, pt.y);
}

void
setcursor(void)
{
	HCURSOR nh;
	int x, y, h, w;
	uchar *sp, *cp;
	uchar *and, *xor;

	h = GetSystemMetrics(SM_CYCURSOR);
	w = (GetSystemMetrics(SM_CXCURSOR)+7)/8;

	and = mallocz(h*w, 1);
	memset(and, 0xff, h*w);
	xor = mallocz(h*w, 1);
	
	lock(&cursor.lk);
	for(y=0,sp=cursor.set,cp=cursor.clr; y<16; y++) {
		for(x=0; x<2; x++) {
			and[y*w+x] = ~(*sp|*cp);
			xor[y*w+x] = ~*sp & *cp;
			cp++;
			sp++;
		}
	}
	nh = CreateCursor(inst, -cursor.offset.x, -cursor.offset.y,
			GetSystemMetrics(SM_CXCURSOR), h,
			and, xor);
	if(nh != NULL) {
		SetCursor(nh);
		if(hcursor != NULL)
			DestroyCursor(hcursor);
		hcursor = nh;
	}
	unlock(&cursor.lk);

	free(and);
	free(xor);

	PostMessage(window, WM_SETCURSOR, (int)window, 0);
}

void
cursorarrow(void)
{
	if(hcursor != 0) {
		DestroyCursor(hcursor);
		hcursor = 0;
	}
	SetCursor(LoadCursor(0, IDC_ARROW));
	PostMessage(window, WM_SETCURSOR, (int)window, 0);
}


void
setcolor(ulong index, ulong red, ulong green, ulong blue)
{
}


uchar*
clipreadunicode(HANDLE h)
{
	Rune *p;
	int n;
	uchar *q;
	
	p = GlobalLock(h);
	n = wstrutflen(p)+1;
	q = malloc(n);
	wstrtoutf(q, p, n);
	GlobalUnlock(h);

	return q;
}

uchar *
clipreadutf(HANDLE h)
{
	uchar *p;

	p = GlobalLock(h);
	p = strdup(p);
	GlobalUnlock(h);
	
	return p;
}

char*
clipread(void)
{
	HANDLE h;
	uchar *p;

	if(!OpenClipboard(window)) {
		oserror();
		return strdup("");
	}

	if((h = GetClipboardData(CF_UNICODETEXT)))
		p = clipreadunicode(h);
	else if((h = GetClipboardData(CF_TEXT)))
		p = clipreadutf(h);
	else {
		oserror();
		p = strdup("");
	}
	
	CloseClipboard();
	return p;
}

int
clipwrite(char *buf)
{
	HANDLE h;
	char *p, *e;
	Rune *rp;
	int n = strlen(buf);

	if(!OpenClipboard(window)) {
		oserror();
		return -1;
	}

	if(!EmptyClipboard()) {
		oserror();
		CloseClipboard();
		return -1;
	}

	h = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, (n+1)*sizeof(Rune));
	if(h == NULL)
		panic("out of memory");
	rp = GlobalLock(h);
	p = buf;
	e = p+n;
	while(p<e)
		p += chartorune(rp++, p);
	*rp = 0;
	GlobalUnlock(h);

	SetClipboardData(CF_UNICODETEXT, h);

	h = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, n+1);
	if(h == NULL)
		panic("out of memory");
	p = GlobalLock(h);
	memcpy(p, buf, n);
	p[n] = 0;
	GlobalUnlock(h);
	
	SetClipboardData(CF_TEXT, h);

	CloseClipboard();
	return n;
}

int
atlocalconsole(void)
{
	return 1;
}
