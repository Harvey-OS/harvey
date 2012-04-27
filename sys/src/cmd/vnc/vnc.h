#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <memdraw.h>

typedef struct Pixfmt	Pixfmt;
typedef struct Colorfmt	Colorfmt;
typedef struct Vnc	Vnc;

struct Colorfmt {
	int		max;
	int		shift;
};

struct Pixfmt {
	int		bpp;
	int		depth;
	int		bigendian;
	int		truecolor;
	Colorfmt	red;
	Colorfmt	green;
	Colorfmt	blue;
};

struct Vnc {
	QLock;
	int		datafd;			/* for network connection */
	int		ctlfd;			/* control for network connection */

	Biobuf		in;
	Biobuf		out;

	Point		dim;
	Pixfmt;
	char		*name;	/* client only */
};

enum {
	/* authentication negotiation */
	AFailed		= 0,
	ANoAuth,
	AVncAuth,

	/* vnc auth negotiation */
	VncAuthOK	= 0,
	VncAuthFailed,
	VncAuthTooMany,
	VncChalLen	= 16,

	/* server to client */
	MFrameUpdate	= 0,
	MSetCmap,
	MBell,
	MSCut,
	MSAck,

	/* client to server */
	MPixFmt		= 0,
	MFixCmap,
	MSetEnc,
	MFrameReq,
	MKey,
	MMouse,
	MCCut,

	/* image encoding methods */
	EncRaw		= 0,
	EncCopyRect	= 1,
	EncRre		= 2,
	EncCorre	= 4,
	EncHextile	= 5,
	EncZlib		= 6,  /* 6,7,8 have been used by others */
	EncTight	= 7,
	EncZHextile	= 8,
	EncMouseWarp	= 9,

	/* paramaters for hextile encoding */
	HextileDim	= 16,
	HextileRaw	= 1,
	HextileBack	= 2,
	HextileFore	= 4,
	HextileRects	= 8,
	HextileCols	= 16
};

/*
 * we're only using the ulong as a place to store bytes,
 * and as something to compare against.
 * the bytes are stored in little-endian format.
 */
typedef ulong Color;

/* auth.c */
extern	int		vncauth(Vnc*, char*);
extern	int		vnchandshake(Vnc*);
extern	int		vncsrvauth(Vnc*);
extern	int		vncsrvhandshake(Vnc*);

/* proto.c */
extern	Vnc*		vncinit(int, int, Vnc*);
extern	uchar		vncrdchar(Vnc*);
extern	ushort		vncrdshort(Vnc*);
extern	ulong		vncrdlong(Vnc*);
extern	Point		vncrdpoint(Vnc*);
extern	Rectangle	vncrdrect(Vnc*);
extern	Rectangle	vncrdcorect(Vnc*);
extern	Pixfmt		vncrdpixfmt(Vnc*);
extern	void		vncrdbytes(Vnc*, void*, int);
extern	char*		vncrdstring(Vnc*);
extern	char*	vncrdstringx(Vnc*);
extern	void		vncwrstring(Vnc*, char*);
extern  void    	vncgobble(Vnc*, long);

extern	void		vncflush(Vnc*);
extern	void		vncterm(Vnc*);
extern	void		vncwrbytes(Vnc*, void*, int);
extern	void		vncwrlong(Vnc*, ulong);
extern	void		vncwrshort(Vnc*, ushort);
extern	void		vncwrchar(Vnc*, uchar);
extern	void		vncwrpixfmt(Vnc*, Pixfmt*);
extern	void		vncwrrect(Vnc*, Rectangle);
extern	void		vncwrpoint(Vnc*, Point);

extern	void		vnclock(Vnc*);		/* for writing */
extern	void		vncunlock(Vnc*);

extern	void		hexdump(void*, int);

/* implemented by clients of the io library */
extern	void		vnchungup(Vnc*);

extern	int		verbose;
extern	char*	serveraddr;