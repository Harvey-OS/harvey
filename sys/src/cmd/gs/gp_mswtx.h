/* Copyright (C) 1993  Russell Lang

Permission to use, copy, modify, distribute, and sell this
software and its documentation for any purpose is hereby granted
without fee, provided that the above copyright notice and this
permission notice appear in all copies of the software and related
documentation.
*/

/* gp_mswtx.h */
/*
 * Microsoft Windows 3.n text window definitions.
 * Original version by Russell Lang.
 */

#ifdef _WINDOWS
#define _Windows
#endif

/* ================================== */
/* For WIN32 API's */
#ifdef __WIN32__
#define OFFSETOF(x)  (x)
#define SELECTOROF(x)  (x)
#endif
 
/* ================================== */
/* text window structure */
/* If an optional item is not specified it must be zero */
#define MAXFONTNAME 80
typedef void (*shutdown_ptr)();
typedef struct tagTW
{
	HINSTANCE hInstance;		/* required */
	HINSTANCE hPrevInstance;	/* required */
	LPSTR	Title;			/* required */
	POINT	ScreenSize;		/* optional */
	unsigned int KeyBufSize;	/* optional */
	LPSTR	DragPre;		/* optional */
	LPSTR	DragPost;		/* optional */
	int	nCmdShow;		/* optional */
	shutdown_ptr shutdown;		/* optional */
	HICON	hIcon;			/* optional */
	HWND	hWndText;
	BYTE FAR *ScreenBuffer;
	BYTE FAR *KeyBuf;
	BYTE FAR *KeyBufIn;
	BYTE FAR *KeyBufOut;
	BOOL	bFocus;
	BOOL	bGetCh;
	char	fontname[MAXFONTNAME];	/* font name */
	int	fontsize;		/* font size in pts */
	HFONT	hfont;
	int	CharAscent;
	int	CaretHeight;
	int	CursorFlag;
	POINT	CursorPos;
	POINT	ClientSize;
	POINT	CharSize;
	POINT	ScrollPos;
	POINT	ScrollMax;
} TW;
typedef TW FAR*  LPTW;

/* ================================== */
void TextMessage(void);
int TextInit(LPTW lptw);
void TextClose(LPTW lptw);
void TextToCursor(LPTW lptw);
int TextKBHit(LPTW);
int TextGetCh(LPTW);
int TextPutCh(LPTW lptw, BYTE ch);
void TextWriteBuf(LPTW lptw, LPSTR buf, int cnt);
