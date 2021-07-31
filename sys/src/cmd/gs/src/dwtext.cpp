/* Copyright (C) 1996, Russell Lang.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/


// $Id: dwtext.cpp,v 1.1 2000/03/09 08:40:40 lpd Exp $


// Microsoft Windows 3.n text window for Ghostscript.

#include <stdlib.h>
#include <string.h> 	// use only far items
#include <ctype.h>

#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>

#include "dwtext.h"

#if defined(_MSC_VER) && defined(__WIN32__)
#define _export
#else
  /* Define  min and max, but make sure to use the identical definition */
  /* to the one that all the compilers seem to have.... */
#  ifndef min
#    define min(a, b) (((a) < (b)) ? (a) : (b))
#  endif
#  ifndef max
#    define max(a, b) (((a) > (b)) ? (a) : (b))
#  endif
#endif

#ifndef EOF
#define EOF (-1)
#endif

/* sysmenu */
#define M_COPY_CLIP 1

LRESULT CALLBACK _export WndTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static const char* TextWinClassName = "rjlTextWinClass";
static const POINT TextWinMinSize = {16, 4};

void
TextWindow::error(char *message)
{
    MessageBox((HWND)NULL,message,(LPSTR)NULL, MB_ICONHAND | MB_SYSTEMMODAL);
}

/* Bring Cursor into text window */
void
TextWindow::to_cursor(void)
{
int nXinc=0;
int nYinc=0;
int cxCursor;
int cyCursor;
	cyCursor = CursorPos.y * CharSize.y;
	if ( (cyCursor + CharSize.y > ScrollPos.y + ClientSize.y) 
//	  || (cyCursor < ScrollPos.y) ) {
// change to scroll to end of window instead of just making visible
// so that ALL of error message can be seen
	  || (cyCursor < ScrollPos.y+ClientSize.y) ) {
		nYinc = max(0, cyCursor + CharSize.y - ClientSize.y) - ScrollPos.y;
		nYinc = min(nYinc, ScrollMax.y - ScrollPos.y);
	}
	cxCursor = CursorPos.x * CharSize.x;
	if ( (cxCursor + CharSize.x > ScrollPos.x + ClientSize.x)
	  || (cxCursor < ScrollPos.x) ) {
		nXinc = max(0, cxCursor + CharSize.x - ClientSize.x/2) - ScrollPos.x;
		nXinc = min(nXinc, ScrollMax.x - ScrollPos.x);
	}
	if (nYinc || nXinc) {
		ScrollPos.y += nYinc;
		ScrollPos.x += nXinc;
		ScrollWindow(hwnd,-nXinc,-nYinc,NULL,NULL);
		SetScrollPos(hwnd,SB_VERT,ScrollPos.y,TRUE);
		SetScrollPos(hwnd,SB_HORZ,ScrollPos.x,TRUE);
		UpdateWindow(hwnd);
	}
}

void
TextWindow::new_line(void)
{
	CursorPos.x = 0;
	CursorPos.y++;
	if (CursorPos.y >= ScreenSize.y) {
	    int i =  ScreenSize.x * (ScreenSize.y - 1);
		_fmemmove(ScreenBuffer, ScreenBuffer+ScreenSize.x, i);
		_fmemset(ScreenBuffer + i, ' ', ScreenSize.x);
		CursorPos.y--;
		ScrollWindow(hwnd,0,-CharSize.y,NULL,NULL);
		UpdateWindow(hwnd);
	}
	if (CursorFlag)
		to_cursor();
//	TextMessage();
}

/* Update count characters in window at cursor position */
/* Updates cursor position */
void
TextWindow::update_text(int count)
{
HDC hdc;
int xpos, ypos;
	xpos = CursorPos.x*CharSize.x - ScrollPos.x;
	ypos = CursorPos.y*CharSize.y - ScrollPos.y;
	hdc = GetDC(hwnd);
	SelectFont(hdc, hfont);
	TextOut(hdc,xpos,ypos,
		(LPSTR)(ScreenBuffer + CursorPos.y*ScreenSize.x + CursorPos.x),
		count);
	(void)ReleaseDC(hwnd,hdc);
	CursorPos.x += count;
	if (CursorPos.x >= ScreenSize.x)
		new_line();
}


void
TextWindow::size(int width, int height)
{
    ScreenSize.x =  max(width, TextWinMinSize.x);
    ScreenSize.y =  max(height, TextWinMinSize.y);
}

void
TextWindow::font(const char *name, int size)
{
    // reject inappropriate arguments
    if (name == NULL)
	return;
    if (size < 4)
	return;

    // set new name and size
    fontname = new char[strlen(name)+1];
    if (fontname == NULL)
	return;
    strcpy(fontname, name);
    fontsize = size;

    // make a new font
    LOGFONT lf;
    TEXTMETRIC tm;
    LPSTR p;
    HDC hdc;
	
    // if window not open, hwnd == 0 == HWND_DESKTOP
    hdc = GetDC(hwnd);
    _fmemset(&lf, 0, sizeof(LOGFONT));
    _fstrncpy(lf.lfFaceName,fontname,LF_FACESIZE);
    lf.lfHeight = -MulDiv(fontsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    lf.lfPitchAndFamily = FIXED_PITCH;
    lf.lfCharSet = DEFAULT_CHARSET;
    if ( (p = _fstrstr(fontname," Italic")) != (LPSTR)NULL ) {
	lf.lfFaceName[ (unsigned int)(p-fontname) ] = '\0';
	lf.lfItalic = TRUE;
    }
    if ( (p = _fstrstr(fontname," Bold")) != (LPSTR)NULL ) {
	lf.lfFaceName[ (unsigned int)(p-fontname) ] = '\0';
	lf.lfWeight = FW_BOLD;
    }
    if (hfont)
	DeleteFont(hfont);

    hfont = CreateFontIndirect((LOGFONT FAR *)&lf);

    /* get text size */
    SelectFont(hdc, hfont);
    GetTextMetrics(hdc,(TEXTMETRIC FAR *)&tm);
    CharSize.y = tm.tmHeight;
    CharSize.x = tm.tmAveCharWidth;
    CharAscent = tm.tmAscent;
    if (bFocus)
	CreateCaret(hwnd, 0, CharSize.x, 2+CaretHeight);
    ReleaseDC(hwnd, hdc);

    // redraw window if necessary
    if (hwnd != HWND_DESKTOP) {
	// INCOMPLETE
    }
}



// Set drag strings
void
TextWindow::drag(const char *pre, const char *post)
{
    // remove old strings
    delete DragPre;
    DragPre = NULL;
    delete DragPost;
    DragPost = NULL;

    // add new strings
    DragPre = new char[strlen(pre)+1];
    if (DragPre)
	strcpy(DragPre, pre);
    DragPost = new char[strlen(post)+1];
    if (DragPost)
	strcpy(DragPost, post);
}

// Text Window constructor
TextWindow::TextWindow(void)
{
    // make sure everything is null
    memset(this, 0, sizeof(TextWindow));

    // set some defaults
    font("Courier New", 10);
    size(80, 24);
    KeyBufSize = 2048;
    CursorFlag = 1;	// scroll to cursor after \n or \r
}

// TextWindow destructor
TextWindow::~TextWindow(void)
{
    if (hwnd)
        DestroyWindow(hwnd);
    if (hfont)
	DeleteFont(hfont);

    delete KeyBuf;
    delete ScreenBuffer;
    delete DragPre;
    delete DragPost;
}

// register the window class
int
TextWindow::register_class(HINSTANCE hinst, HICON hicon)
{
    hInstance = hinst;
    hIcon = hicon;

    // register window class
    WNDCLASS wndclass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndTextProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = sizeof(void FAR *);
    wndclass.hInstance = hInstance;
    wndclass.hIcon = hIcon ? hIcon : LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = GetStockBrush(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = TextWinClassName;
    return RegisterClass(&wndclass);
}

// Show the window
int
TextWindow::create(LPSTR app_name, int show_cmd)
{
    HMENU sysmenu;

    Title = app_name;
    nCmdShow = show_cmd;
    quitnow = FALSE;

    // make sure we have some sensible defaults
    if (KeyBufSize < 256)
	KeyBufSize = 256;

    CursorPos.x = CursorPos.y = 0;
    bFocus = FALSE;
    bGetCh = FALSE;
    CaretHeight = 0;


    // allocate buffers
    KeyBufIn = KeyBufOut = KeyBuf = new BYTE[KeyBufSize];
    if (KeyBuf == NULL) {
	error("Out of memory");
	return 1;
    }
    ScreenBuffer = new BYTE[ScreenSize.x * ScreenSize.y];
    if (ScreenBuffer == NULL) {
	error("Out of memory");
	return 1;
    }
    _fmemset(ScreenBuffer, ' ', ScreenSize.x * ScreenSize.y);

    hwnd = CreateWindow(TextWinClassName, Title,
		  WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
		  CW_USEDEFAULT, CW_USEDEFAULT,
		  CW_USEDEFAULT, CW_USEDEFAULT,
		  NULL, NULL, hInstance, this);

    if (hwnd == NULL) {
	MessageBox((HWND)NULL,"Couldn't open text window",(LPSTR)NULL, MB_ICONHAND | MB_SYSTEMMODAL);
	return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    sysmenu = GetSystemMenu(hwnd,0);	/* get the sysmenu */
    AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(sysmenu, MF_STRING, M_COPY_CLIP, "Copy to Clip&board");

    return 0;
}

int
TextWindow::destroy(void)
{
    if (hwnd) {
        DestroyWindow(hwnd);
	hwnd = (HWND)NULL;
    }
    return 0;
}



int
TextWindow::putch(int ch)
{
int pos;
    switch(ch) {
	case '\r':
		CursorPos.x = 0;
		if (CursorFlag)
		    to_cursor();
		break;
	case '\n':
		new_line();
		break;
	case 7:
		MessageBeep(-1);
		if (CursorFlag)
		    to_cursor();
		break;
	case '\t':
		{
		    for ( int n = 8 - (CursorPos.x % 8); n>0; n-- )
			    putch(' ');
		}
		break;
	case 0x08:
	case 0x7f:
		CursorPos.x--;
		if (CursorPos.x < 0) {
		    CursorPos.x = ScreenSize.x - 1;
		    CursorPos.y--;
		}
		if (CursorPos.y < 0)
		    CursorPos.y = 0;
		break;
	default:
		pos = CursorPos.y*ScreenSize.x + CursorPos.x;
		ScreenBuffer[pos] = ch;
		update_text(1);
    }
    return ch;
}

void 
TextWindow::write_buf(LPSTR str, int cnt)
{
BYTE FAR *p;
int count, limit;
    while (cnt>0) {
	p = ScreenBuffer + CursorPos.y*ScreenSize.x + CursorPos.x;
	limit = ScreenSize.x - CursorPos.x;
	for (count=0; (count < limit) && (cnt>0) && (isprint(*str) || *str=='\t'); count++) {
	    if (*str=='\t') {
		for ( int n = 8 - ((CursorPos.x+count) % 8); (count < limit) & (n>0); n--, count++ )
		    *p++ = ' ';
		str++;
		count--;
   	    }
	    else {
		*p++ = *str++;
	    }
	    cnt--;
	}
	if (count>0) {
	    update_text(count);
	}
	if (cnt > 0) {
	    if (*str=='\n') {
		new_line();
		str++;
		cnt--;
	    }
	    else if (!isprint(*str) && *str!='\t') {
		putch(*str++);
		cnt--;
	    }
	}
    }
}

// Put string to window
void
TextWindow::puts(LPSTR str)
{
    write_buf(str, lstrlen(str));
}


/* TRUE if key hit, FALSE if no key */
inline int
TextWindow::kbhit(void)
{
    return (KeyBufIn != KeyBufOut);
}

/* get character from keyboard, no echo */
/* need to add extended codes */
int
TextWindow::getch(void)
{
    int ch;
    to_cursor();
    bGetCh = TRUE;
    if (bFocus) {
	SetCaretPos(CursorPos.x*CharSize.x - ScrollPos.x,
	    CursorPos.y*CharSize.y + CharAscent - CaretHeight - ScrollPos.y);
	ShowCaret(hwnd);
    }

    MSG msg;
    do {
        if (!quitnow && GetMessage(&msg, (HWND)NULL, 0, 0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
	else
	   return EOF;	/* window closed */
    } while (!kbhit());

    ch = *KeyBufOut++;
    if (ch=='\r')
	ch = '\n';
    if (KeyBufOut - KeyBuf >= KeyBufSize)
	KeyBufOut = KeyBuf;		// wrap around
    if (bFocus)
	HideCaret(hwnd);
    bGetCh = FALSE;
    return ch;
}

// Read line from keyboard
// Return at most 'len' characters
// Does NOT add null terminating character
// This does is NOT the same as fgets
int
TextWindow::read_line(LPSTR line, int len)
{	
LPSTR dest = line;
LPSTR limit = dest + len - 1;  // leave room for '\n'
int ch;
    if (dest > limit)
	return 0;
    while ( (ch = getch()) != '\n' ) {
	switch(ch) {
	    case EOF:
	    case 26:	// ^Z == EOF
		return 0;
	    case '\b':	// ^H
	    case 0x7f:  // DEL
		if (dest > line) {
		    putch('\b');
		    putch(' ');
		    putch('\b');
		    --dest;
		}
		break;
	    case 21:	// ^U
		while (dest > line) {
		    putch('\b');
		    putch(' ');
		    putch('\b');
		    --dest;
		}
		break;
	    default:
		if (dest == limit)
		    MessageBeep(-1);
		else {
		    *dest++ = ch;
		    putch(ch);
		}
		break;
	}
    }
    *dest++ = ch;
    putch(ch);
    return (dest-line);
}

// Read a string from the keyboard, of up to len characters
// (not including trailing NULL)
int
TextWindow::gets(LPSTR line, int len)
{
LPSTR dest = line;
LPSTR limit = dest + len;  // don't leave room for '\0'
int ch;
    do {
	if (dest >= limit)
	    break;
	ch = getch();
	switch(ch) {
	    case 26:	// ^Z == EOF
		return 0;
	    case '\b':	// ^H
	    case 0x7f:  // DEL
		if (dest > line) {
		    putch('\b');
		    putch(' ');
		    putch('\b');
		    --dest;
		}
		break;
	    case 21:	// ^U
		while (dest > line) {
		    putch('\b');
		    putch(' ');
		    putch('\b');
		    --dest;
		}
		break;
	    default:
		*dest++ = ch;
		putch(ch);
		break;
	}
    } while (ch != '\n');

    *dest = '\0';
    return (dest-line);
}


/* Windows 3.1 drag-drop feature */
void
TextWindow::drag_drop(HDROP hdrop)
{
    int i, cFiles;
    LPSTR p;
    if ( (DragPre==(LPSTR)NULL) || (DragPost==(LPSTR)NULL) )
	    return;
    char szFile[256];

    cFiles = DragQueryFile(hdrop, (UINT)(-1), (LPSTR)NULL, 0);
    for (i=0; i<cFiles; i++) {
	DragQueryFile(hdrop, i, szFile, 80);
	for (p=DragPre; *p; p++)
		SendMessage(hwnd,WM_CHAR,*p,1L);
	for (p=szFile; *p; p++) {
	    if (*p == '\\')
		SendMessage(hwnd,WM_CHAR,'/',1L);
	    else 
		SendMessage(hwnd,WM_CHAR,*p,1L);
	}
	for (p=DragPost; *p; p++)
		SendMessage(hwnd,WM_CHAR,*p,1L);
    }
    DragFinish(hdrop);
}


void
TextWindow::copy_to_clipboard(void)
{
    int size, count;
    HGLOBAL hGMem;
    LPSTR cbuf, cp;
    TEXTMETRIC tm;
    UINT type;
    HDC hdc;
    int i;

    size = ScreenSize.y * (ScreenSize.x + 2) + 1;
    hGMem = GlobalAlloc(GHND | GMEM_SHARE, (DWORD)size);
    cbuf = cp = (LPSTR)GlobalLock(hGMem);
    if (cp == (LPSTR)NULL)
	return;
    
    for (i=0; i<ScreenSize.y; i++) {
	count = ScreenSize.x;
	_fmemcpy(cp, ScreenBuffer + ScreenSize.x*i, count);
	/* remove trailing spaces */
	for (count=count-1; count>=0; count--) {
		if (cp[count]!=' ')
			break;
		cp[count] = '\0';
	}
	cp[++count] = '\r';
	cp[++count] = '\n';
	cp[++count] = '\0';
	cp += count;
    }
    size = _fstrlen(cbuf) + 1;
    GlobalUnlock(hGMem);
    hGMem = GlobalReAlloc(hGMem, (DWORD)size, GHND | GMEM_SHARE);
    /* find out what type to put into clipboard */
    hdc = GetDC(hwnd);
    SelectFont(hdc, hfont);
    GetTextMetrics(hdc,(TEXTMETRIC FAR *)&tm);
    if (tm.tmCharSet == OEM_CHARSET)
	type = CF_OEMTEXT;
    else
	type = CF_TEXT;
    ReleaseDC(hwnd, hdc);
    /* give buffer to clipboard */
    OpenClipboard(hwnd);
    EmptyClipboard();
    SetClipboardData(type, hGMem);
    CloseClipboard();
}

/* text window */
LRESULT CALLBACK _export
WndTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TextWindow *tw;
    if (message == WM_CREATE) {
	// Object is stored in window extra data.
	// Nothing must try to use the object before WM_CREATE
	// initializes it here.
	tw = (TextWindow *)(((CREATESTRUCT FAR *)lParam)->lpCreateParams);
	SetWindowLong(hwnd, 0, (LONG)tw);
    }

    // call the object window procedure
    tw = (TextWindow *)GetWindowLong(hwnd, 0);
    return tw->WndProc(hwnd, message, wParam, lParam);
}


// member window procedure
LRESULT
TextWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    int nYinc, nXinc;

    switch(message) {
	case WM_SYSCOMMAND:
	    switch(LOWORD(wParam)) {
		case M_COPY_CLIP:
		    copy_to_clipboard();
		    return 0;
	    }
	    break;
	case WM_SETFOCUS: 
	    bFocus = TRUE;
	    CreateCaret(hwnd, 0, CharSize.x, 2+CaretHeight);
	    SetCaretPos(CursorPos.x*CharSize.x - ScrollPos.x,
		    CursorPos.y*CharSize.y + CharAscent
		     - CaretHeight - ScrollPos.y);
	    if (bGetCh)
		    ShowCaret(hwnd);
	    break;
	case WM_KILLFOCUS: 
	    DestroyCaret();
	    bFocus = FALSE;
	    break;
	case WM_SIZE:
	    ClientSize.y = HIWORD(lParam);
	    ClientSize.x = LOWORD(lParam);

	    ScrollMax.y = max(0, CharSize.y*ScreenSize.y - ClientSize.y);
	    ScrollPos.y = min(ScrollPos.y, ScrollMax.y);

	    SetScrollRange(hwnd, SB_VERT, 0, ScrollMax.y, FALSE);
	    SetScrollPos(hwnd, SB_VERT, ScrollPos.y, TRUE);

	    ScrollMax.x = max(0, CharSize.x*ScreenSize.x - ClientSize.x);
	    ScrollPos.x = min(ScrollPos.x, ScrollMax.x);

	    SetScrollRange(hwnd, SB_HORZ, 0, ScrollMax.x, FALSE);
	    SetScrollPos(hwnd, SB_HORZ, ScrollPos.x, TRUE);

	    if (bFocus && bGetCh) {
		SetCaretPos(CursorPos.x*CharSize.x - ScrollPos.x,
			    CursorPos.y*CharSize.y + CharAscent 
			    - CaretHeight - ScrollPos.y);
		ShowCaret(hwnd);
	    }
	    return(0);
	case WM_VSCROLL:
	    switch(LOWORD(wParam)) {
		case SB_TOP:
		    nYinc = -ScrollPos.y;
		    break;
		case SB_BOTTOM:
		    nYinc = ScrollMax.y - ScrollPos.y;
		    break;
		case SB_LINEUP:
		    nYinc = -CharSize.y;
		    break;
		case SB_LINEDOWN:
		    nYinc = CharSize.y;
		    break;
		case SB_PAGEUP:
		    nYinc = min(-1,-ClientSize.y);
		    break;
		case SB_PAGEDOWN:
		    nYinc = max(1,ClientSize.y);
		    break;
		case SB_THUMBPOSITION:
#ifdef __WIN32__
		    nYinc = HIWORD(wParam) - ScrollPos.y;
#else
		    nYinc = LOWORD(lParam) - ScrollPos.y;
#endif
		    break;
		default:
		    nYinc = 0;
	    }
	    if ( (nYinc = max(-ScrollPos.y, 
		    min(nYinc, ScrollMax.y - ScrollPos.y)))
		    != 0 ) {
		    ScrollPos.y += nYinc;
		    ScrollWindow(hwnd,0,-nYinc,NULL,NULL);
		    SetScrollPos(hwnd,SB_VERT,ScrollPos.y,TRUE);
		    UpdateWindow(hwnd);
	    }
	    return(0);
	case WM_HSCROLL:
	    switch(LOWORD(wParam)) {
		case SB_LINEUP:
		    nXinc = -CharSize.x;
		    break;
		case SB_LINEDOWN:
		    nXinc = CharSize.x;
		    break;
		case SB_PAGEUP:
		    nXinc = min(-1,-ClientSize.x);
		    break;
		case SB_PAGEDOWN:
		    nXinc = max(1,ClientSize.x);
		    break;
		case SB_THUMBPOSITION:
#ifdef __WIN32__
		    nXinc = HIWORD(wParam) - ScrollPos.x;
#else
		    nXinc = LOWORD(lParam) - ScrollPos.x;
#endif
		    break;
		default:
		    nXinc = 0;
	    }
	    if ( (nXinc = max(-ScrollPos.x, 
		    min(nXinc, ScrollMax.x - ScrollPos.x)))
		    != 0 ) {
		    ScrollPos.x += nXinc;
		    ScrollWindow(hwnd,-nXinc,0,NULL,NULL);
		    SetScrollPos(hwnd,SB_HORZ,ScrollPos.x,TRUE);
		    UpdateWindow(hwnd);
	    }
	    return(0);
	case WM_KEYDOWN:
	    if (GetKeyState(VK_SHIFT) < 0) {
	      switch(wParam) {
		case VK_HOME:
			SendMessage(hwnd, WM_VSCROLL, SB_TOP, (LPARAM)0);
			break;
		case VK_END:
			SendMessage(hwnd, WM_VSCROLL, SB_BOTTOM, (LPARAM)0);
			break;
		case VK_PRIOR:
			SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, (LPARAM)0);
			break;
		case VK_NEXT:
			SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, (LPARAM)0);
			break;
		case VK_UP:
			SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, (LPARAM)0);
			break;
		case VK_DOWN:
			SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, (LPARAM)0);
			break;
		case VK_LEFT:
			SendMessage(hwnd, WM_HSCROLL, SB_LINEUP, (LPARAM)0);
			break;
		case VK_RIGHT:
			SendMessage(hwnd, WM_HSCROLL, SB_LINEDOWN, (LPARAM)0);
			break;
	      }
	    }
	    else {
	        switch(wParam) {
		    case VK_HOME:
		    case VK_END:
		    case VK_PRIOR:
		    case VK_NEXT:
		    case VK_UP:
		    case VK_DOWN:
		    case VK_LEFT:
		    case VK_RIGHT:
		    case VK_DELETE:
		    { /* store key in circular buffer */
			long count = KeyBufIn - KeyBufOut;
			if (count < 0) count += KeyBufSize;
			if (count < KeyBufSize-2) {
			    *KeyBufIn++ = 0;
			    if (KeyBufIn - KeyBuf >= KeyBufSize)
				KeyBufIn = KeyBuf;	/* wrap around */
			    *KeyBufIn++ = HIWORD(lParam) & 0xff;
			    if (KeyBufIn - KeyBuf >= KeyBufSize)
				KeyBufIn = KeyBuf;	/* wrap around */
			}
		    }
	        }
	    }
	    break;
	case WM_CHAR:
	    { /* store key in circular buffer */
		long count = KeyBufIn - KeyBufOut;
		if (count < 0) count += KeyBufSize;
		if (count < KeyBufSize-1) {
		    *KeyBufIn++ = wParam;
		    if (KeyBufIn - KeyBuf >= KeyBufSize)
			KeyBufIn = KeyBuf;	/* wrap around */
		}
	    }
	    return(0);
	case WM_PAINT:
	    {
	    POINT source, width, dest;
	    hdc = BeginPaint(hwnd, &ps);
	    SelectFont(hdc, hfont);
	    SetMapMode(hdc, MM_TEXT);
	    SetBkMode(hdc,OPAQUE);
	    GetClientRect(hwnd, &rect);
	    source.x = (rect.left + ScrollPos.x) / CharSize.x;		/* source */
	    source.y = (rect.top + ScrollPos.y) / CharSize.y;
	    dest.x = source.x * CharSize.x - ScrollPos.x; 				/* destination */
	    dest.y = source.y * CharSize.y - ScrollPos.y;
	    width.x = ((rect.right  + ScrollPos.x + CharSize.x - 1) / CharSize.x) - source.x; /* width */
	    width.y = ((rect.bottom + ScrollPos.y + CharSize.y - 1) / CharSize.y) - source.y;
	    if (source.x < 0)
		    source.x = 0;
	    if (source.y < 0)
		    source.y = 0;
	    if (source.x+width.x > ScreenSize.x)
		    width.x = ScreenSize.x - source.x;
	    if (source.y+width.y > ScreenSize.y)
		    width.y = ScreenSize.y - source.y;
	    /* for each line */
	    while (width.y>0) {
		    TextOut(hdc,dest.x,dest.y,
			(LPSTR)(ScreenBuffer + source.y*ScreenSize.x + source.x),
			width.x);
		    dest.y += CharSize.y;
		    source.y++;
		    width.y--;
	    }
	    EndPaint(hwnd, &ps);
	    return 0;
	    }
	case WM_DROPFILES:
	    drag_drop((HDROP)wParam);
	    break;
	case WM_CREATE:
	    {
	    RECT crect, wrect;

	    TextWindow::hwnd = hwnd;

	    GetClientRect(hwnd, &crect);
	    if ( (CharSize.y*ScreenSize.y < crect.bottom)
	      || (CharSize.x*ScreenSize.x < crect.right) ) {
		    /* shrink size */
		GetWindowRect(hwnd,&wrect);
		MoveWindow(hwnd, wrect.left, wrect.top,
		     wrect.right-wrect.left + (CharSize.x*ScreenSize.x - crect.right),
		     wrect.bottom-wrect.top + (CharSize.y*ScreenSize.y - crect.bottom),
		     TRUE);
	    }
	    if ( (DragPre!=(LPSTR)NULL) && (DragPost!=(LPSTR)NULL) )
		DragAcceptFiles(hwnd, TRUE);
	    }
	    break;
	case WM_CLOSE:
	    break;
	case WM_DESTROY:
	    DragAcceptFiles(hwnd, FALSE);
	    if (hfont)
		DeleteFont(hfont);
	    hfont = (HFONT)0;
	    quitnow = TRUE;
	    PostQuitMessage(0);
	    break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}




#ifdef NOTUSED
// test program
#pragma argsused

int PASCAL 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{

// make a test window
TextWindow* textwin = new TextWindow(hInstance);
	if (!hPrevInstance) {
	    HICON hicon = LoadIcon(NULL, IDI_APPLICATION);
	    textwin->register_class(hicon);
	}
	textwin->font("Courier New", 10);
	textwin->size(80, 80);
	textwin->drag("(", ") run\r");

	// show the text window
	if (!textwin->show("Application Name", nCmdShow)) {
	    // do the real work here
	    // TESTING
	    int ch;
	    int len;
	    char *line = new char[256];
	    while ( (len = textwin->read_line(line, 256-1)) != 0 ) {
		textwin->write_buf(line, len);
	    }
/*
	    while ( textwin->gets(line, 256-1) ) {
		textwin->puts(line);
	    }
*/
/*
	    while ( (ch = textwin->getch()) != 4 )
		textwin->putch(ch);
*/
	}
	else {
	    
	}

	// clean up
	delete textwin;
	
	// end program
	return 0;
}
#endif
