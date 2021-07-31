#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <thread.h>

typedef struct Pixfmt Pixfmt;
typedef struct Colorfmt Colorfmt;
typedef struct Serverinfo Serverinfo;
typedef struct Vnc Vnc;

struct Colorfmt {
	int max;
	int shift;
};

struct Pixfmt {
	int bpp;
	int depth;
	int bigendian;
	int truecolor;
	Colorfmt red;
	Colorfmt green;
	Colorfmt blue;
};

struct Serverinfo {
	Point dim;
	Pixfmt;
	char *name;
};

struct Vnc {
	QLock;

	Biobuf in;
	Biobuf out;

	Serverinfo;

};

enum {
	rfbProtocolMajorVersion = 3,
	rfbProtocolMinorVersion = 3,

	rfbConnFailed = 0,
	rfbNoAuth,
	rfbVncAuth,

	rfbVncAuthOK = 0,
	rfbVncAuthFailed,
	rfbVncAuthTooMany,

	rfbChalLen = 16,

	/* server to client */
	rfbMsgFramebufferUpdate = 0,
	rfbMsgSetColorMapEntries,
	rfbMsgBell,
	rfbMsgServerCutText,

	/* client to server */
	rfbMsgSetPixelFormat = 0,
	rfbMsgFixColorMapEntries,
	rfbMsgSetEncodings,
	rfbMsgFramebufferUpdateRequest,
	rfbMsgKeyEvent,
	rfbMsgPointerEvent,
	rfbMsgClientCutText,

	rfbEncodingRaw = 0,
	rfbEncodingCopyRect,
	rfbEncodingRRE,
	rfbEncodingCoRRE = 4,
	rfbEncodingHextile,

	rfbHextileRaw = 1<<0,
	rfbHextileBackground = 1<<1,
	rfbHextileForeground = 1<<2,
	rfbHextileAnySubrects = 1<<3,
	rfbHextileSubrectsColored = 1<<4,
};

/*
 * we're only using the ulong as a place to store bytes,
 * and as something to compare against.
 * the bytes are stored in little-endian format.
 */
typedef ulong Color;

/* auth.c */
extern	int		vncauth(Vnc*);

/* color.c */
extern	void		choosecolor(Vnc*);
extern	void		(*cvtpixels)(uchar*, int);

/* dial.c */
extern	char*	netmkvncaddr(char*);

/* draw.c */
extern	void		sendencodings(Vnc*);
extern	void		requestupdate(Vnc*, int);
extern	void		readfromserver(Vnc*);
extern	void		updatescreen(Rectangle);

/*
 * Vncscreen is a pointer to the backing store for
 * the full screen.  If the window is big enough, vncscreen == screen.
 * Otherwise, we have a separate image that we copy to the
 * screen as it is updated.  Viewr defines the rectangle
 * that is visible in screen.  ZP means the upper left corner.
 */
extern	Image*	vncscreen;
extern	Rectangle		viewr;	/* protected by lockdisplay */

/* proto.c */
extern	Image*	colorimage(Color);
extern	Vnc*		openvnc(int);

extern	void 		Verror(char*, ...);

extern	uchar	Vrdchar(Vnc*);
extern	ushort	Vrdshort(Vnc*);
extern	ulong	Vrdlong(Vnc*);
extern	Point		Vrdpoint(Vnc*);
extern	Rectangle	Vrdrect(Vnc*);
extern	Rectangle	Vrdcorect(Vnc*);
extern	Pixfmt	Vrdpixfmt(Vnc*);
extern	void		Vrdbytes(Vnc*, void*, int);
extern	char*	Vrdstring(Vnc*);
extern	Color	Vrdcolor(Vnc*);

extern	void		Vflush(Vnc*);
extern	void		Vwrbytes(Vnc*, void*, int);
extern	void		Vwrlong(Vnc*, ulong);
extern	void		Vwrshort(Vnc*, ushort);
extern	void		Vwrchar(Vnc*, uchar);
extern	void		Vwrpixfmt(Vnc*, Pixfmt*);
extern	void		Vwrrect(Vnc*, Rectangle);
extern	void		Vwrpoint(Vnc*, Point);

extern	void		Vlock(Vnc*);		/* for writing */
extern	void		Vunlock(Vnc*);

extern	void		hexdump(void*, int);

extern	uchar	zero[];

/* viewer.c */
extern	char*	encodings;
extern	int		bpp12;
extern	int		shared;
extern	int		verbose;
extern	Vnc*		vnc;

/* wsys.c */
extern	void		initwindow(Vnc*);
extern	void		readkbd(Vnc*);
extern	void		readmouse(Vnc*);

typedef struct Mouse Mouse;
struct Mouse {
	int buttons;
	Point xy;
};
