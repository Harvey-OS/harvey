/* Copyright (C) 1996-2001 Ghostgum Software Pty Ltd.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/


/* $Id: dwtext.c,v 1.9 2005/03/04 10:27:39 ghostgum Exp $ */

/* Microsoft Windows text window for Ghostscript.

#include <stdlib.h>
#include <string.h> 	/* use only far items */
#include <ctype.h>

#define STRICT
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <shellapi.h>

#include "dwtext.h"

/* Define  min and max, but make sure to use the identical definition */
/* to the one that all the compilers seem to have.... */
#ifndef min
#  define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#  define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef EOF
#define EOF (-1)
#endif

/* sysmenu */
#define M_COPY_CLIP 1
#define M_PASTE_CLIP 2

LRESULT CALLBACK WndTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void text_error(char *message);
static void text_new_line(TW *tw);
static void text_update_text(TW *tw, int count);
static void text_drag_drop(TW *tw, HDROP hdrop);
static void text_copy_to_clipboard(TW *tw);
static void text_paste_from_clipboard(TW *tw);

static const char* TextWinClassName = "rjlTextWinClass";
static const POINT TextWinMinSize = {16, 4};

static void
text_error(char *message)
{
    MessageBox((HWND)NULL,message,(LPSTR)NULL, MB_ICONHAND | MB_SYSTEMMODAL);
}

/* Bring Cursor into text window */
void
text_to_cursor(TW *tw)
{
int nXinc=0;
int nYinc=0;
int cxCursor;
int cyCursor;
	cyCursor = tw->CursorPos.y * tw->CharSize.y;
	if ( (cyCursor + tw->CharSize.y > tw->ScrollPos.y + tw->ClientSize.y) 
/*	  || (cyCursor < tw->ScrollPos.y) ) { */
/* change to scroll to end of window instead of just making visible */
/* so that ALL of error message can be seen */
	  || (cyCursor < tw->ScrollPos.y+tw->ClientSize.y) ) {
		nYinc = max(0, cyCursor + tw->CharSize.y 
			- tw->ClientSize.y) - tw->ScrollPos.y;
		nYinc = min(nYinc, tw->ScrollMax.y - tw->ScrollPos.y);
	}
	cxCursor = tw->CursorPos.x * tw->CharSize.x;
	if ( (cxCursor + tw->CharSize.x > tw->ScrollPos.x + tw->ClientSize.x)
	  || (cxCursor < tw->ScrollPos.x) ) {
		nXinc = max(0, cxCursor + tw->CharSize.x 
			- tw->ClientSize.x/2) - tw->ScrollPos.x;
		nXinc = min(nXinc, tw->ScrollMax.x - tw->ScrollPos.x);
	}
	if (nYinc || nXinc) {
		tw->ScrollPos.y += nYinc;
		tw->ScrollPos.x += nXinc;
		ScrollWindow(tw->hwnd,-nXinc,-nYinc,NULL,NULL);
		SetScrollPos(tw->hwnd,SB_VERT,tw->ScrollPos.y,TRUE);
		SetScrollPos(tw->hwnd,SB_HORZ,tw->ScrollPos.x,TRUE);
		UpdateWindow(tw->hwnd);
	}
}

static void
text_new_line(TW *tw)
{
	tw->CursorPos.x = 0;
	tw->CursorPos.y++;
	if (tw->CursorPos.y >= tw->ScreenSize.y) {
	    int i =  tw->ScreenSize.x * (tw->ScreenSize.y - 1);
		memmove(tw->ScreenBuffer, tw->ScreenBuffer+tw->ScreenSize.x, i);
		memset(tw->ScreenBuffer + i, ' ', tw->ScreenSize.x);
		tw->CursorPos.y--;
		ScrollWindow(tw->hwnd,0,-tw->CharSize.y,NULL,NULL);
		UpdateWindow(tw->hwnd);
	}
	if (tw->CursorFlag)
		text_to_cursor(tw);

/*	TextMessage(); */
}

/* Update count characters in window at cursor position */
/* Updates cursor position */
static void
text_update_text(TW *tw, int count)
{
HDC hdc;
int xpos, ypos;
	xpos = tw->CursorPos.x*tw->CharSize.x - tw->ScrollPos.x;
	ypos = tw->CursorPos.y*tw->CharSize.y - tw->ScrollPos.y;
	hdc = GetDC(tw->hwnd);
	SelectFont(hdc, tw->hfont);
	TextOut(hdc,xpos,ypos,
		(LPSTR)(tw->ScreenBuffer + tw->CursorPos.y*tw->ScreenSize.x 
		+ tw->CursorPos.x), count);
	(void)ReleaseDC(tw->hwnd,hdc);
	tw->CursorPos.x += count;
	if (tw->CursorPos.x >= tw->ScreenSize.x)
		text_new_line(tw);
}


void
text_size(TW *tw, int width, int height)
{
    tw->ScreenSize.x =  max(width, TextWinMinSize.x);
    tw->ScreenSize.y =  max(height, TextWinMinSize.y);
}

void
text_font(TW *tw, const char *name, int size)
{
    /* make a new font */
    LOGFONT lf;
    TEXTMETRIC tm;
    LPSTR p;
    HDC hdc;
	
    /* reject inappropriate arguments */
    if (name == NULL)
	return;
    if (size < 4)
	return;

    /* set new name and size */
    if (tw->fontname)
	free(tw->fontname);
    tw->fontname = (char *)malloc(strlen(name)+1);
    if (tw->fontname == NULL)
	return;
    strcpy(tw->fontname, name);
    tw->fontsize = size;

    /* if window not open, hwnd == 0 == HWND_DESKTOP */
    hdc = GetDC(tw->hwnd);
    memset(&lf, 0, sizeof(LOGFONT));
    strncpy(lf.lfFaceName,tw->fontname,LF_FACESIZE);
    lf.lfHeight = -MulDiv(tw->fontsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
    lf.lfPitchAndFamily = FIXED_PITCH;
    lf.lfCharSet = DEFAULT_CHARSET;
    if ( (p = strstr(tw->fontname," Italic")) != (LPSTR)NULL ) {
	lf.lfFaceName[ (unsigned int)(p-tw->fontname) ] = '\0';
	lf.lfItalic = TRUE;
    }
    if ( (p = strstr(tw->fontname," Bold")) != (LPSTR)NULL ) {
	lf.lfFaceName[ (unsigned int)(p-tw->fontname) ] = '\0';
	lf.lfWeight = FW_BOLD;
    }
    if (tw->hfont)
	DeleteFont(tw->hfont);

    tw->hfont = CreateFontIndirect((LOGFONT FAR *)&lf);

    /* get text size */
    SelectFont(hdc, tw->hfont);
    GetTextMetrics(hdc,(TEXTMETRIC FAR *)&tm);
    tw->CharSize.y = tm.tmHeight;
    tw->CharSize.x = tm.tmAveCharWidth;
    tw->CharAscent = tm.tmAscent;
    if (tw->bFocus)
	CreateCaret(tw->hwnd, 0, tw->CharSize.x, 2+tw->CaretHeight);
    ReleaseDC(tw->hwnd, hdc);

    /* redraw window if necessary */
    if (tw->hwnd != HWND_DESKTOP) {
	/* INCOMPLETE */
    }
}



/* Set drag strings */
void
text_drag(TW *tw, const char *pre, const char *post)
{
    /* remove old strings */
    if (tw->DragPre)
	free((char *)tw->DragPre);
    tw->DragPre = NULL;
    if (tw->DragPost)
	free((char *)tw->DragPost);
    tw->DragPost = NULL;

    /* add new strings */
    tw->DragPre = malloc(strlen(pre)+1);
    if (tw->DragPre)
	strcpy(tw->DragPre, pre);
    tw->DragPost = malloc(strlen(post)+1);
    if (tw->DragPost)
	strcpy(tw->DragPost, post);
}

/* Set the window position and size */
void 
text_setpos(TW *tw, int x, int y, int cx, int cy)
{
    tw->x = x;
    tw->y = y;
    tw->cx = cx;
    tw->cy = cy;
}

/* Get the window position and size */
int text_getpos(TW *tw, int *px, int *py, int *pcx, int *pcy)
{
    *px = tw->x;
    *py = tw->y;
    *pcx = tw->cx;
    *pcy = tw->cy;
    return 0;
}

/* Allocate new text window */
TW *
text_new(void)
{
    TW *tw;
    tw = (TW *)malloc(sizeof(TW));
    if (tw == NULL)
	return NULL;
    /* make sure everything is null */
    memset(tw, 0, sizeof(TW));

    /* set some defaults */
    text_font(tw, "Courier New", 10);
    text_size(tw, 80, 24);
    tw->KeyBufSize = 2048;
    tw->CursorFlag = 1;	/* scroll to cursor after \n or \r */
    tw->hwnd = HWND_DESKTOP;

    tw->line_end = 0;
    tw->line_start = 0;
    tw->line_complete = FALSE;
    tw->line_eof = FALSE;

    tw->x = CW_USEDEFAULT;
    tw->y = CW_USEDEFAULT;
    tw->cx = CW_USEDEFAULT;
    tw->cy = CW_USEDEFAULT;
    return tw;
}

/* Destroy window and deallocate text window structure */
void
text_destroy(TW *tw)
{
    if (tw->hwnd)
        DestroyWindow(tw->hwnd);
    tw->hwnd = HWND_DESKTOP;

    if (tw->hfont)
	DeleteFont(tw->hfont);
    tw->hfont = NULL;

    if (tw->KeyBuf)
	free((char *)tw->KeyBuf);
    tw->KeyBuf = NULL;

    if (tw->ScreenBuffer)
	free((char *)tw->ScreenBuffer);
    tw->ScreenBuffer = NULL;

    if (tw->DragPre)
	free((char *)tw->DragPre);
    tw->DragPre = NULL;

    if (tw->DragPost)
	free((char *)tw->DragPost);
    tw->DragPost = NULL;

    if (tw->fontname)
	free((char *)tw->fontname);
    tw->fontname = NULL;
}


/* register the window class */
int
text_register_class(TW *tw, HICON hicon)
{
    WNDCLASS wndclass;
    HINSTANCE hInstance = GetModuleHandle(NULL);
    tw->hIcon = hicon;

    /* register window class */
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndTextProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = sizeof(void *);
    wndclass.hInstance = hInstance;
    wndclass.hIcon = tw->hIcon ? tw->hIcon : LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = GetStockBrush(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = TextWinClassName;
    return RegisterClass(&wndclass);
}

/* Show the window */
int
text_create(TW *tw, const char *app_name, int show_cmd)
{
    HMENU sysmenu;
    HINSTANCE hInstance = GetModuleHandle(NULL);

    tw->Title = app_name;
    tw->nCmdShow = show_cmd;
    tw->quitnow = FALSE;

    /* make sure we have some sensible defaults */
    if (tw->KeyBufSize < 256)
	tw->KeyBufSize = 256;

    tw->CursorPos.x = tw->CursorPos.y = 0;
    tw->bFocus = FALSE;
    tw->bGetCh = FALSE;
    tw->CaretHeight = 0;


    /* allocate buffers */
    tw->KeyBufIn = tw->KeyBufOut = tw->KeyBuf = malloc(tw->KeyBufSize);
    if (tw->KeyBuf == NULL) {
	text_error("Out of memory");
	return 1;
    }
    tw->ScreenBuffer = malloc(tw->ScreenSize.x * tw->ScreenSize.y);
    if (tw->ScreenBuffer == NULL) {
	text_error("Out of memory");
	return 1;
    }
    memset(tw->ScreenBuffer, ' ', tw->ScreenSize.x * tw->ScreenSize.y);

    tw->hwnd = CreateWindow(TextWinClassName, tw->Title,
		  WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
		  tw->x, tw->y, tw->cx, tw->cy,
		  NULL, NULL, hInstance, tw);

    if (tw->hwnd == NULL) {
	MessageBox((HWND)NULL,"Couldn't open text window",(LPSTR)NULL, MB_ICONHAND | MB_SYSTEMMODAL);
	return 1;
    }

    ShowWindow(tw->hwnd, tw->nCmdShow);
    sysmenu = GetSystemMenu(tw->hwnd,0);	/* get the sysmenu */
    AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(sysmenu, MF_STRING, M_COPY_CLIP, "Copy to Clip&board");
    AppendMenu(sysmenu, MF_STRING, M_PASTE_CLIP, "&Paste");

    return 0;
}


int
text_putch(TW *tw, int ch)
{
int pos;
int n;
    if (tw->quitnow)
	return ch;	/* don't write error message as we shut down */
    switch(ch) {
	case '\r':
		tw->CursorPos.x = 0;
		if (tw->CursorFlag)
		    text_to_cursor(tw);
		break;
	case '\n':
		text_new_line(tw);
		break;
	case 7:
		MessageBeep(-1);
		if (tw->CursorFlag)
		    text_to_cursor(tw);
		break;
	case '\t':
		{
		    for (n = 8 - (tw->CursorPos.x % 8); n>0; n-- )
			    text_putch(tw, ' ');
		}
		break;
	case 0x08:
	case 0x7f:
		tw->CursorPos.x--;
		if (tw->CursorPos.x < 0) {
		    tw->CursorPos.x = tw->ScreenSize.x - 1;
		    tw->CursorPos.y--;
		}
		if (tw->CursorPos.y < 0)
		    tw->CursorPos.y = 0;
		break;
	default:
		pos = tw->CursorPos.y*tw->ScreenSize.x + tw->CursorPos.x;
		tw->ScreenBuffer[pos] = ch;
		text_update_text(tw, 1);
    }
    return ch;
}

void 
text_write_buf(TW *tw, const char *str, int cnt)
{
BYTE *p;
int count, limit;
int n;
    if (tw->quitnow)
	return;		/* don't write error message as we shut down */
    while (cnt>0) {
	p = tw->ScreenBuffer + tw->CursorPos.y*tw->ScreenSize.x + tw->CursorPos.x;
	limit = tw->ScreenSize.x - tw->CursorPos.x;
	for (count=0; (count < limit) && (cnt>0) && 
	    (isprint((unsigned char)(*str)) || *str=='\t'); count++) {
	    if (*str=='\t') {
		for (n = 8 - ((tw->CursorPos.x+count) % 8); (count < limit) & (n>0); n--, count++ )
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
	    text_update_text(tw, count);
	}
	if (cnt > 0) {
	    if (*str=='\n') {
		text_new_line(tw);
		str++;
		cnt--;
	    }
	    else if (!isprint((unsigned char)(*str)) && *str!='\t') {
		text_putch(tw, *str++);
		cnt--;
	    }
	}
    }
}

/* Put string to window */
void
text_puts(TW *tw, const char *str)
{
    text_write_buf(tw, str, strlen(str));
}


/* TRUE if key hit, FALSE if no key */
int
text_kbhit(TW *tw)
{
    return (tw->KeyBufIn != tw->KeyBufOut);
}

/* get character from keyboard, no echo */
/* need to add extended codes */
int
text_getch(TW *tw)
{
    MSG msg;
    int ch;
    text_to_cursor(tw);
    tw->bGetCh = TRUE;
    if (tw->bFocus) {
	SetCaretPos(tw->CursorPos.x*tw->CharSize.x - tw->ScrollPos.x,
	    tw->CursorPos.y*tw->CharSize.y + tw->CharAscent 
	    - tw->CaretHeight - tw->ScrollPos.y);
	ShowCaret(tw->hwnd);
    }

    while (PeekMessage(&msg, (HWND)NULL, 0, 0, PM_NOREMOVE)) {
	if (GetMessage(&msg, (HWND)NULL, 0, 0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
    }
    if (tw->quitnow)
       return EOF;	/* window closed */

    while (!text_kbhit(tw)) {
        if (!tw->quitnow) {
	    if (GetMessage(&msg, (HWND)NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	    }
	}
	else
	   return EOF;	/* window closed */
    }

    ch = *tw->KeyBufOut++;
    if (ch=='\r')
	ch = '\n';
    if (tw->KeyBufOut - tw->KeyBuf >= tw->KeyBufSize)
	tw->KeyBufOut = tw->KeyBuf;		/* wrap around */
    if (tw->bFocus)
	HideCaret(tw->hwnd);
    tw->bGetCh = FALSE;
    return ch;
}

/* Read line from keyboard using buffered input
 * Return at most 'len' characters
 * Does NOT add null terminating character
 * This is NOT the same as fgets()
 * Do not mix this with calls to text_getch()
 */
int
text_read_line(TW *tw, char *line, int len)
{	
int ch;
    if (tw->line_eof)
	return 0;

    while (!tw->line_complete) {
	/* we have not yet collected a full line */
        ch = text_getch(tw);
	switch(ch) {
	    case EOF:
	    case 26:	/* ^Z == EOF */
		tw->line_eof = TRUE;
		tw->line_complete = TRUE;
		break;
	    case '\b':	/* ^H */
	    case 0x7f:  /* DEL */
		if (tw->line_end) {
		    text_putch(tw, '\b');
		    text_putch(tw, ' ');
		    text_putch(tw, '\b');
		    --(tw->line_end);
		}
		break;
	    case 21:	/* ^U */
		while (tw->line_end) {
		    text_putch(tw, '\b');
		    text_putch(tw, ' ');
		    text_putch(tw, '\b');
		    --(tw->line_end);
		}
		break;
	    case '\r':
	    case '\n':
		tw->line_complete = TRUE;
		/* fall through */
	    default:
		tw->line_buf[tw->line_end++] = ch;
		text_putch(tw, ch);
		break;
	}
	if (tw->line_end >= sizeof(tw->line_buf))
	    tw->line_complete = TRUE;
    }

    if (tw->quitnow)
	return -1;

    if (tw->line_complete) {
	/* We either filled the buffer or got CR, LF or EOF */
	int count = min(len, tw->line_end - tw->line_start);
	memcpy(line, tw->line_buf + tw->line_start, count);
	tw->line_start += count;
	if (tw->line_start == tw->line_end) {
	    tw->line_start = tw->line_end = 0;
	    tw->line_complete = FALSE;
	}
	return count;
    }

    return 0;
}

/* Read a string from the keyboard, of up to len characters */
/* (not including trailing NULL) */
int
text_gets(TW *tw, char *line, int len)
{
LPSTR dest = line;
LPSTR limit = dest + len;  /* don't leave room for '\0' */
int ch;
    do {
	if (dest >= limit)
	    break;
	ch = text_getch(tw);
	switch(ch) {
	    case 26:	/* ^Z == EOF */
		return 0;
	    case '\b':	/* ^H */
	    case 0x7f:  /* DEL */
		if (dest > line) {
		    text_putch(tw, '\b');
		    text_putch(tw, ' ');
		    text_putch(tw, '\b');
		    --dest;
		}
		break;
	    case 21:	/* ^U */
		while (dest > line) {
		    text_putch(tw, '\b');
		    text_putch(tw, ' ');
		    text_putch(tw, '\b');
		    --dest;
		}
		break;
	    default:
		*dest++ = ch;
		text_putch(tw, ch);
		break;
	}
    } while (ch != '\n');

    *dest = '\0';
    return (dest-line);
}


/* Windows 3.1 drag-drop feature */
void
text_drag_drop(TW *tw, HDROP hdrop)
{
    char szFile[256];
    int i, cFiles;
    const char *p;
    if ( (tw->DragPre==NULL) || (tw->DragPost==NULL) )
	    return;

    cFiles = DragQueryFile(hdrop, (UINT)(-1), (LPSTR)NULL, 0);
    for (i=0; i<cFiles; i++) {
	DragQueryFile(hdrop, i, szFile, 80);
	for (p=tw->DragPre; *p; p++)
		SendMessage(tw->hwnd,WM_CHAR,*p,1L);
	for (p=szFile; *p; p++) {
	    if (*p == '\\')
		SendMessage(tw->hwnd,WM_CHAR,'/',1L);
	    else 
		SendMessage(tw->hwnd,WM_CHAR,*p,1L);
	}
	for (p=tw->DragPost; *p; p++)
		SendMessage(tw->hwnd,WM_CHAR,*p,1L);
    }
    DragFinish(hdrop);
}


void
text_copy_to_clipboard(TW *tw)
{
    int size, count;
    HGLOBAL hGMem;
    LPSTR cbuf, cp;
    TEXTMETRIC tm;
    UINT type;
    HDC hdc;
    int i;

    size = tw->ScreenSize.y * (tw->ScreenSize.x + 2) + 1;
    hGMem = GlobalAlloc(GHND | GMEM_SHARE, (DWORD)size);
    cbuf = cp = (LPSTR)GlobalLock(hGMem);
    if (cp == (LPSTR)NULL)
	return;
    
    for (i=0; i<tw->ScreenSize.y; i++) {
	count = tw->ScreenSize.x;
	memcpy(cp, tw->ScreenBuffer + tw->ScreenSize.x*i, count);
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
    size = strlen(cbuf) + 1;
    GlobalUnlock(hGMem);
    hGMem = GlobalReAlloc(hGMem, (DWORD)size, GHND | GMEM_SHARE);
    /* find out what type to put into clipboard */
    hdc = GetDC(tw->hwnd);
    SelectFont(hdc, tw->hfont);
    GetTextMetrics(hdc,(TEXTMETRIC FAR *)&tm);
    if (tm.tmCharSet == OEM_CHARSET)
	type = CF_OEMTEXT;
    else
	type = CF_TEXT;
    ReleaseDC(tw->hwnd, hdc);
    /* give buffer to clipboard */
    OpenClipboard(tw->hwnd);
    EmptyClipboard();
    SetClipboardData(type, hGMem);
    CloseClipboard();
}

void
text_paste_from_clipboard(TW *tw)
{
    HGLOBAL hClipMemory;
    BYTE *p;
    long count;
    OpenClipboard(tw->hwnd);
    if (IsClipboardFormatAvailable(CF_TEXT)) {
	hClipMemory = GetClipboardData(CF_TEXT);
	p = GlobalLock(hClipMemory);
	while (*p) {
	    /* transfer to keyboard circular buffer */
	    count = tw->KeyBufIn - tw->KeyBufOut;
	    if (count < 0) 
		count += tw->KeyBufSize;
	    if (count < tw->KeyBufSize-1) {
		*tw->KeyBufIn++ = *p;
		if (tw->KeyBufIn - tw->KeyBuf >= tw->KeyBufSize)
		    tw->KeyBufIn = tw->KeyBuf;	/* wrap around */
	    }
	    p++;
	}
	GlobalUnlock(hClipMemory);
    }
    CloseClipboard();
}

/* text window */
LRESULT CALLBACK
WndTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    int nYinc, nXinc;
    TW *tw;
    if (message == WM_CREATE) {
	/* Object is stored in window extra data.
	 * Nothing must try to use the object before WM_CREATE
	 * initializes it here.
	 */
	tw = (TW *)(((CREATESTRUCT FAR *)lParam)->lpCreateParams);
	SetWindowLong(hwnd, 0, (LONG)tw);
    }
    tw = (TW *)GetWindowLong(hwnd, 0);

    switch(message) {
	case WM_SYSCOMMAND:
	    switch(LOWORD(wParam)) {
		case M_COPY_CLIP:
		    text_copy_to_clipboard(tw);
		    return 0;
		case M_PASTE_CLIP:
		    text_paste_from_clipboard(tw);
		    return 0;
	    }
	    break;
	case WM_SETFOCUS: 
	    tw->bFocus = TRUE;
	    CreateCaret(hwnd, 0, tw->CharSize.x, 2+tw->CaretHeight);
	    SetCaretPos(tw->CursorPos.x*tw->CharSize.x - tw->ScrollPos.x,
		    tw->CursorPos.y*tw->CharSize.y + tw->CharAscent
		     - tw->CaretHeight - tw->ScrollPos.y);
	    if (tw->bGetCh)
		    ShowCaret(hwnd);
	    break;
	case WM_KILLFOCUS: 
	    DestroyCaret();
	    tw->bFocus = FALSE;
	    break;
	case WM_MOVE:
	    if (!IsIconic(hwnd) && !IsZoomed(hwnd)) {
		GetWindowRect(hwnd, &rect);
		tw->x = rect.left;
		tw->y = rect.top;
	    }
	    break;
	case WM_SIZE:
	    if (wParam == SIZE_MINIMIZED)
		    return(0);

	    /* remember current window size */
	    if (wParam != SIZE_MAXIMIZED) {
		GetWindowRect(hwnd, &rect);
		tw->cx = rect.right - rect.left;
		tw->cy = rect.bottom - rect.top;
		tw->x = rect.left;
		tw->y = rect.top;
	    }

	    tw->ClientSize.y = HIWORD(lParam);
	    tw->ClientSize.x = LOWORD(lParam);

	    tw->ScrollMax.y = max(0, tw->CharSize.y*tw->ScreenSize.y - tw->ClientSize.y);
	    tw->ScrollPos.y = min(tw->ScrollPos.y, tw->ScrollMax.y);

	    SetScrollRange(hwnd, SB_VERT, 0, tw->ScrollMax.y, FALSE);
	    SetScrollPos(hwnd, SB_VERT, tw->ScrollPos.y, TRUE);

	    tw->ScrollMax.x = max(0, tw->CharSize.x*tw->ScreenSize.x - tw->ClientSize.x);
	    tw->ScrollPos.x = min(tw->ScrollPos.x, tw->ScrollMax.x);

	    SetScrollRange(hwnd, SB_HORZ, 0, tw->ScrollMax.x, FALSE);
	    SetScrollPos(hwnd, SB_HORZ, tw->ScrollPos.x, TRUE);

	    if (tw->bFocus && tw->bGetCh) {
		SetCaretPos(tw->CursorPos.x*tw->CharSize.x - tw->ScrollPos.x,
			    tw->CursorPos.y*tw->CharSize.y + tw->CharAscent 
			    - tw->CaretHeight - tw->ScrollPos.y);
		ShowCaret(hwnd);
	    }
	    return(0);
	case WM_VSCROLL:
	    switch(LOWORD(wParam)) {
		case SB_TOP:
		    nYinc = -tw->ScrollPos.y;
		    break;
		case SB_BOTTOM:
		    nYinc = tw->ScrollMax.y - tw->ScrollPos.y;
		    break;
		case SB_LINEUP:
		    nYinc = -tw->CharSize.y;
		    break;
		case SB_LINEDOWN:
		    nYinc = tw->CharSize.y;
		    break;
		case SB_PAGEUP:
		    nYinc = min(-1,-tw->ClientSize.y);
		    break;
		case SB_PAGEDOWN:
		    nYinc = max(1,tw->ClientSize.y);
		    break;
		case SB_THUMBPOSITION:
		    nYinc = HIWORD(wParam) - tw->ScrollPos.y;
		    break;
		default:
		    nYinc = 0;
	    }
	    if ( (nYinc = max(-tw->ScrollPos.y, 
		    min(nYinc, tw->ScrollMax.y - tw->ScrollPos.y)))
		    != 0 ) {
		    tw->ScrollPos.y += nYinc;
		    ScrollWindow(hwnd,0,-nYinc,NULL,NULL);
		    SetScrollPos(hwnd,SB_VERT,tw->ScrollPos.y,TRUE);
		    UpdateWindow(hwnd);
	    }
	    return(0);
	case WM_HSCROLL:
	    switch(LOWORD(wParam)) {
		case SB_LINEUP:
		    nXinc = -tw->CharSize.x;
		    break;
		case SB_LINEDOWN:
		    nXinc = tw->CharSize.x;
		    break;
		case SB_PAGEUP:
		    nXinc = min(-1,-tw->ClientSize.x);
		    break;
		case SB_PAGEDOWN:
		    nXinc = max(1,tw->ClientSize.x);
		    break;
		case SB_THUMBPOSITION:
		    nXinc = HIWORD(wParam) - tw->ScrollPos.x;
		    break;
		default:
		    nXinc = 0;
	    }
	    if ( (nXinc = max(-tw->ScrollPos.x, 
		    min(nXinc, tw->ScrollMax.x - tw->ScrollPos.x)))
		    != 0 ) {
		    tw->ScrollPos.x += nXinc;
		    ScrollWindow(hwnd,-nXinc,0,NULL,NULL);
		    SetScrollPos(hwnd,SB_HORZ,tw->ScrollPos.x,TRUE);
		    UpdateWindow(hwnd);
	    }
	    return(0);
	case WM_KEYDOWN:
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
	    break;
	case WM_CHAR:
	    { /* store key in circular buffer */
		long count = tw->KeyBufIn - tw->KeyBufOut;
		if (count < 0) count += tw->KeyBufSize;
		if (count < tw->KeyBufSize-1) {
		    *tw->KeyBufIn++ = wParam;
		    if (tw->KeyBufIn - tw->KeyBuf >= tw->KeyBufSize)
			tw->KeyBufIn = tw->KeyBuf;	/* wrap around */
		}
	    }
	    return(0);
	case WM_PAINT:
	    {
	    POINT source, width, dest;
	    hdc = BeginPaint(hwnd, &ps);
	    SelectFont(hdc, tw->hfont);
	    SetMapMode(hdc, MM_TEXT);
	    SetBkMode(hdc,OPAQUE);
	    GetClientRect(hwnd, &rect);
	    source.x = (rect.left + tw->ScrollPos.x) / tw->CharSize.x; /* source */
	    source.y = (rect.top + tw->ScrollPos.y) / tw->CharSize.y;
	    dest.x = source.x * tw->CharSize.x - tw->ScrollPos.x; /* destination */
	    dest.y = source.y * tw->CharSize.y - tw->ScrollPos.y;
	    width.x = ((rect.right  + tw->ScrollPos.x + tw->CharSize.x - 1) / tw->CharSize.x) - source.x; /* width */
	    width.y = ((rect.bottom + tw->ScrollPos.y + tw->CharSize.y - 1) / tw->CharSize.y) - source.y;
	    if (source.x < 0)
		    source.x = 0;
	    if (source.y < 0)
		    source.y = 0;
	    if (source.x+width.x > tw->ScreenSize.x)
		    width.x = tw->ScreenSize.x - source.x;
	    if (source.y+width.y > tw->ScreenSize.y)
		    width.y = tw->ScreenSize.y - source.y;
	    /* for each line */
	    while (width.y>0) {
		    TextOut(hdc,dest.x,dest.y,
			(LPSTR)(tw->ScreenBuffer + source.y*tw->ScreenSize.x + source.x),
			width.x);
		    dest.y += tw->CharSize.y;
		    source.y++;
		    width.y--;
	    }
	    EndPaint(hwnd, &ps);
	    return 0;
	    }
	case WM_DROPFILES:
	    text_drag_drop(tw, (HDROP)wParam);
	    break;
	case WM_CREATE:
	    {
	    RECT crect, wrect;
	    int cx, cy;

	    tw->hwnd = hwnd;

	    /* make window no larger than screen buffer */
	    GetWindowRect(hwnd, &wrect);
	    GetClientRect(hwnd, &crect);
	    cx = min(tw->CharSize.x*tw->ScreenSize.x, crect.right);
	    cy = min(tw->CharSize.y*tw->ScreenSize.y, crect.bottom);
	    MoveWindow(hwnd, wrect.left, wrect.top,
		 wrect.right-wrect.left + (cx - crect.right),
		 wrect.bottom-wrect.top + (cy - crect.bottom),
		     TRUE);

	    if ( (tw->DragPre!=(LPSTR)NULL) && (tw->DragPost!=(LPSTR)NULL) )
		DragAcceptFiles(hwnd, TRUE);
	    }
	    break;
	case WM_CLOSE:
	    /* Tell user that we heard them */
	    if (!tw->quitnow) {
		char title[256];
		int count = GetWindowText(hwnd, title, sizeof(title)-11);
		strcpy(title+count, " - closing");
		SetWindowText(hwnd, title);
	    }
	    tw->quitnow = TRUE;
	    /* wait until Ghostscript exits before destroying window */
	    return 0;	
	case WM_DESTROY:
	    DragAcceptFiles(hwnd, FALSE);
	    if (tw->hfont)
		DeleteFont(tw->hfont);
	    tw->hfont = (HFONT)0;
	    tw->quitnow = TRUE;
	    PostQuitMessage(0);
	    break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}


HWND text_get_handle(TW *tw)
{
    return tw->hwnd;
}


#ifdef NOTUSED
/* test program */
#pragma argsused

int PASCAL 
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{

	/* make a test window */
	tw = text_new();

	if (!hPrevInstance) {
	    HICON hicon = LoadIcon(NULL, IDI_APPLICATION);
	    text_register_class(hicon);
	}
	text_font(tw, "Courier New", 10);
	text_size(tw, 80, 80);
	text_drag(tw, "(", ") run\r");

	/* show the text window */
	if (!text_create(tw, "Application Name", nCmdShow)) {
	    /* do the real work here */
	    /* TESTING */
	    int ch;
	    int len;
	    char *line = new char[256];
	    while ( (len = text_read_line(tw, line, 256-1)) != 0 ) {
		text_write_buf(tw, line, len);
	    }
/*
	    while ( text_gets(tw, line, 256-1) ) {
		text_puts(tw, line);
	    }
*/
/*
	    while ( (ch = text_getch(tw, )) != 4 )
		text_putch(tw, ch);
*/
	}
	else {
	    
	}

	/* clean up */
	text_destroy(tw);
	
	/* end program */
	return 0;
}
#endif
