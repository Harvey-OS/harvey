/* Copyright (C) 1993  Russell Lang

Permission to use, copy, modify, distribute, and sell this
software and its documentation for any purpose is hereby granted
without fee, provided that the above copyright notice and this
permission notice appear in all copies of the software and related
documentation.
*/

/* gp_mswtx.c */
/*
 * Microsoft Windows 3.n text window for Ghostscript.
 * Original version by Russell Lang
 */

#include <stdlib.h>
#include "string_.h"	/* use only far items */
#include "memory_.h"
#include "dos_.h"
#include "ctype_.h"

#define STRICT
#include "windows_.h"
#include <windowsx.h>
#if WINVER >= 0x030a
#include <commdlg.h>
#include <shellapi.h>
#endif

#include "gp_mswin.h"
#include "gp_mswtx.h"

/* sysmenu */
#define M_COPY_CLIP 1
/* font stuff */
#define TEXTFONTSIZE 9
#define TEXTFONTNAME "Terminal"

/* limits */
POINT ScreenMinSize = {16,4};
LRESULT CALLBACK _export WndTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

char szNoMemory[] = "out of memory";
LPSTR szTextClass = "gstext_class";

void
TextMessage(void)
{
    MSG msg;
    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
	{
	TranslateMessage(&msg);
	DispatchMessage(&msg);
	}
    return;
}


void
CreateTextClass(LPTW lptw)
{
WNDCLASS wndclass;
static BOOL init = FALSE;
	if (init)
		return;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndTextProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = sizeof(void FAR *);
	wndclass.hInstance = lptw->hInstance;
	if (lptw->hIcon)
		wndclass.hIcon = lptw->hIcon;
	else
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = GetStockBrush(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szTextClass;
	RegisterClass(&wndclass);
	init = TRUE;
}


/* make text window */
int
TextInit(LPTW lptw)
{
	HMENU sysmenu;
	HGLOBAL hglobal;
	
	if (!lptw->hPrevInstance)
		CreateTextClass(lptw);

	if (lptw->KeyBufSize == 0)
		lptw->KeyBufSize = 256;

	if (lptw->ScreenSize.x < ScreenMinSize.x)
		lptw->ScreenSize.x = ScreenMinSize.x;
	if (lptw->ScreenSize.y < ScreenMinSize.y)
		lptw->ScreenSize.y = ScreenMinSize.y;

	lptw->CursorPos.x = lptw->CursorPos.y = 0;
	lptw->bFocus = FALSE;
	lptw->bGetCh = FALSE;
	lptw->CaretHeight = 0;
	if (!lptw->nCmdShow)
		lptw->nCmdShow = SW_SHOWNORMAL;

	hglobal = GlobalAlloc(GHND, lptw->ScreenSize.x * lptw->ScreenSize.y);
	lptw->ScreenBuffer = (BYTE FAR *)GlobalLock(hglobal);
	if (lptw->ScreenBuffer == (BYTE FAR *)NULL) {
		MessageBox((HWND)NULL,szNoMemory,(LPSTR)NULL, MB_ICONHAND | MB_SYSTEMMODAL);
		return(1);
	}
	_fmemset(lptw->ScreenBuffer, ' ', lptw->ScreenSize.x * lptw->ScreenSize.y);
	hglobal = GlobalAlloc(LHND, lptw->KeyBufSize);
	lptw->KeyBuf = (BYTE FAR *)GlobalLock(hglobal);
	if (lptw->KeyBuf == (BYTE FAR *)NULL) {
		MessageBox((HWND)NULL,szNoMemory,(LPSTR)NULL, MB_ICONHAND | MB_SYSTEMMODAL);
		return(1);
	}
	lptw->KeyBufIn = lptw->KeyBufOut = lptw->KeyBuf;

	lptw->hWndText = CreateWindow(szTextClass, lptw->Title,
		  WS_OVERLAPPEDWINDOW | WS_VSCROLL | WS_HSCROLL,
		  CW_USEDEFAULT, CW_USEDEFAULT,
		  CW_USEDEFAULT, CW_USEDEFAULT,
		  NULL, NULL, lptw->hInstance, lptw);
	if (lptw->hWndText == (HWND)NULL) {
		MessageBox((HWND)NULL,"Couldn't open text window",(LPSTR)NULL, MB_ICONHAND | MB_SYSTEMMODAL);
		return(1);
	}
	ShowWindow(lptw->hWndText, lptw->nCmdShow);
	sysmenu = GetSystemMenu(lptw->hWndText,0);	/* get the sysmenu */
	AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(sysmenu, MF_STRING, M_COPY_CLIP, "Copy to Clip&board");

	return(0);
}

/* close a text window */
void
TextClose(LPTW lptw)
{
	HGLOBAL hglobal;

	/* close window */
	if (lptw->hWndText)
		DestroyWindow(lptw->hWndText);
	TextMessage();

	hglobal = (HGLOBAL)GlobalHandle( SELECTOROF(lptw->ScreenBuffer) );
	if (hglobal) {
		GlobalUnlock(hglobal);
		GlobalFree(hglobal);
	}
	hglobal = (HGLOBAL)GlobalHandle( SELECTOROF(lptw->KeyBuf) );
	if (hglobal) {
		GlobalUnlock(hglobal);
		GlobalFree(hglobal);
	}
	lptw->hWndText = (HWND)NULL;
}
	

/* Bring Cursor into text window */
void
TextToCursor(LPTW lptw)
{
int nXinc=0;
int nYinc=0;
int cxCursor;
int cyCursor;
	cyCursor = lptw->CursorPos.y * lptw->CharSize.y;
	if ( (cyCursor + lptw->CharSize.y > lptw->ScrollPos.y + lptw->ClientSize.y) 
	  || (cyCursor < lptw->ScrollPos.y) ) {
		nYinc = max(0, cyCursor + lptw->CharSize.y - lptw->ClientSize.y) - lptw->ScrollPos.y;
		nYinc = min(nYinc, lptw->ScrollMax.y - lptw->ScrollPos.y);
	}
	cxCursor = lptw->CursorPos.x * lptw->CharSize.x;
	if ( (cxCursor + lptw->CharSize.x > lptw->ScrollPos.x + lptw->ClientSize.x)
	  || (cxCursor < lptw->ScrollPos.x) ) {
		nXinc = max(0, cxCursor + lptw->CharSize.x - lptw->ClientSize.x/2) - lptw->ScrollPos.x;
		nXinc = min(nXinc, lptw->ScrollMax.x - lptw->ScrollPos.x);
	}
	if (nYinc || nXinc) {
		lptw->ScrollPos.y += nYinc;
		lptw->ScrollPos.x += nXinc;
		ScrollWindow(lptw->hWndText,-nXinc,-nYinc,NULL,NULL);
		SetScrollPos(lptw->hWndText,SB_VERT,lptw->ScrollPos.y,TRUE);
		SetScrollPos(lptw->hWndText,SB_HORZ,lptw->ScrollPos.x,TRUE);
		UpdateWindow(lptw->hWndText);
	}
}

void
NewLine(LPTW lptw)
{
	lptw->CursorPos.x = 0;
	lptw->CursorPos.y++;
	if (lptw->CursorPos.y >= lptw->ScreenSize.y) {
	    int i =  lptw->ScreenSize.x * (lptw->ScreenSize.y - 1);
		_fmemmove(lptw->ScreenBuffer, lptw->ScreenBuffer+lptw->ScreenSize.x, i);
		_fmemset(lptw->ScreenBuffer + i, ' ', lptw->ScreenSize.x);
		lptw->CursorPos.y--;
		ScrollWindow(lptw->hWndText,0,-lptw->CharSize.y,NULL,NULL);
		UpdateWindow(lptw->hWndText);
	}
	if (lptw->CursorFlag)
		TextToCursor(lptw);
	TextMessage();
}

/* Update count characters in window at cursor position */
/* Updates cursor position */
void
UpdateText(LPTW lptw, int count)
{
HDC hdc;
int xpos, ypos;
	xpos = lptw->CursorPos.x*lptw->CharSize.x - lptw->ScrollPos.x;
	ypos = lptw->CursorPos.y*lptw->CharSize.y - lptw->ScrollPos.y;
	hdc = GetDC(lptw->hWndText);
	SelectFont(hdc, lptw->hfont);
	TextOut(hdc,xpos,ypos,
		(LPSTR)(lptw->ScreenBuffer + lptw->CursorPos.y*lptw->ScreenSize.x + 
		lptw->CursorPos.x), count);
	(void)ReleaseDC(lptw->hWndText,hdc);
	lptw->CursorPos.x += count;
	if (lptw->CursorPos.x >= lptw->ScreenSize.x)
		NewLine(lptw);
}

int
TextPutCh(LPTW lptw, BYTE ch)
{
int pos;
	switch(ch) {
		case '\r':
			lptw->CursorPos.x = 0;
			if (lptw->CursorFlag)
				TextToCursor(lptw);
			break;
		case '\n':
			NewLine(lptw);
			break;
		case 7:
			MessageBeep(-1);
			if (lptw->CursorFlag)
				TextToCursor(lptw);
			break;
		case '\t':
			{
			int n;
				for ( n = 8 - (lptw->CursorPos.x % 8); n>0; n-- )
					TextPutCh(lptw, ' ');
			}
			break;
		case 0x08:
		case 0x7f:
			lptw->CursorPos.x--;
			if (lptw->CursorPos.x < 0) {
				lptw->CursorPos.x = lptw->ScreenSize.x - 1;
				lptw->CursorPos.y--;
			}
			if (lptw->CursorPos.y < 0)
				lptw->CursorPos.y = 0;
			break;
		default:
			pos = lptw->CursorPos.y*lptw->ScreenSize.x + lptw->CursorPos.x;
			lptw->ScreenBuffer[pos] = ch;
			UpdateText(lptw, 1);
	}
	return ch;
}

void 
TextWriteBuf(LPTW lptw, LPSTR str, int cnt)
{
BYTE FAR *p;
int count, limit;
    while (cnt>0) {
	p = lptw->ScreenBuffer + lptw->CursorPos.y*lptw->ScreenSize.x + lptw->CursorPos.x;
	limit = lptw->ScreenSize.x - lptw->CursorPos.x;
	for (count=0; (count < limit) && (cnt>0) && (isprint(*str) || *str=='\t'); count++) {
	    if (*str=='\t') {
		int n;
		for ( n = 8 - ((lptw->CursorPos.x+count) % 8); (count < limit) & (n>0); n--, count++ ) {
			*p++ = ' ';
		}
		str++;
		count--;
   	    }
	    else {
		*p++ = *str++;
	    }
	    cnt--;
	}
	if (count>0) {
	    UpdateText(lptw, count);
	}
	if (cnt > 0) {
	    if (*str=='\n') {
		NewLine(lptw);
		str++;
		cnt--;
	    }
	    else if (!isprint(*str) && *str!='\t') {
		TextPutCh(lptw, *str++);
		cnt--;
	    }
	}
    }
}


/* TRUE if key hit, FALSE if no key */
int
TextKBHit(LPTW lptw)
{
	return (lptw->KeyBufIn != lptw->KeyBufOut);
}

/* get character from keyboard, no echo */
/* need to add extended codes */
int
TextGetCh(LPTW lptw)
{
	int ch;
	TextToCursor(lptw);
	lptw->bGetCh = TRUE;
	if (lptw->bFocus) {
		SetCaretPos(lptw->CursorPos.x*lptw->CharSize.x - lptw->ScrollPos.x,
			lptw->CursorPos.y*lptw->CharSize.y + lptw->CharAscent 
			- lptw->CaretHeight - lptw->ScrollPos.y);
		ShowCaret(lptw->hWndText);
	}
	do {
		TextMessage();
	} while (!TextKBHit(lptw));
	ch = *lptw->KeyBufOut++;
	if (ch=='\r')
		ch = '\n';
	if (lptw->KeyBufOut - lptw->KeyBuf >= lptw->KeyBufSize)
		lptw->KeyBufOut = lptw->KeyBuf;	/* wrap around */
	if (lptw->bFocus)
		HideCaret(lptw->hWndText);
	lptw->bGetCh = FALSE;
	return ch;
}

#if WINVER >= 0x030a
/* Windows 3.1 drag-drop feature */
char szFile[80];
void
DragFunc(LPTW lptw, HDROP hdrop)
{
	int i, cFiles;
	LPSTR p;
	if ( (lptw->DragPre==(LPSTR)NULL) || (lptw->DragPost==(LPSTR)NULL) )
		return;
	cFiles = DragQueryFile(hdrop, 0xffff, (LPSTR)NULL, 0);
	for (i=0; i<cFiles; i++) {
		DragQueryFile(hdrop, i, szFile, 80);
		for (p=lptw->DragPre; *p; p++)
			SendMessage(lptw->hWndText,WM_CHAR,*p,1L);
		for (p=szFile; *p; p++) {
		    if (*p == '\\')
			SendMessage(lptw->hWndText,WM_CHAR,'/',1L);
		    else 
			SendMessage(lptw->hWndText,WM_CHAR,*p,1L);
		}
		for (p=lptw->DragPost; *p; p++)
			SendMessage(lptw->hWndText,WM_CHAR,*p,1L);
	}
	DragFinish(hdrop);
}
#endif


void
TextMakeFont(LPTW lptw)
{
	LOGFONT lf;
	TEXTMETRIC tm;
	LPSTR p;
	HDC hdc;
	
	if ((lptw->fontname[0]=='\0') || (lptw->fontsize==0)) {
		_fstrcpy(lptw->fontname,TEXTFONTNAME);
		lptw->fontsize = TEXTFONTSIZE;
	}

	hdc = GetDC(lptw->hWndText);
	_fmemset(&lf, 0, sizeof(LOGFONT));
	_fstrncpy(lf.lfFaceName,lptw->fontname,LF_FACESIZE);
	lf.lfHeight = -MulDiv(lptw->fontsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	lf.lfPitchAndFamily = FIXED_PITCH;
	lf.lfCharSet = DEFAULT_CHARSET;
	if ( (p = _fstrstr(lptw->fontname," Italic")) != (LPSTR)NULL ) {
		lf.lfFaceName[ (unsigned int)(p-lptw->fontname) ] = '\0';
		lf.lfItalic = TRUE;
	}
	if ( (p = _fstrstr(lptw->fontname," Bold")) != (LPSTR)NULL ) {
		lf.lfFaceName[ (unsigned int)(p-lptw->fontname) ] = '\0';
		lf.lfWeight = FW_BOLD;
	}
	if (lptw->hfont != 0)
		DeleteFont(lptw->hfont);
	lptw->hfont = CreateFontIndirect((LOGFONT FAR *)&lf);
	/* get text size */
	SelectFont(hdc, lptw->hfont);
	GetTextMetrics(hdc,(TEXTMETRIC FAR *)&tm);
	lptw->CharSize.y = tm.tmHeight;
	lptw->CharSize.x = tm.tmAveCharWidth;
	lptw->CharAscent = tm.tmAscent;
	if (lptw->bFocus)
		CreateCaret(lptw->hWndText, 0, lptw->CharSize.x, 2+lptw->CaretHeight);
	ReleaseDC(lptw->hWndText, hdc);
	return;
}

void
TextCopyClip(LPTW lptw)
{
	int size, count;
	HGLOBAL hGMem;
	LPSTR cbuf, cp;
	TEXTMETRIC tm;
	UINT type;
	HDC hdc;
	int i;

	size = lptw->ScreenSize.y * (lptw->ScreenSize.x + 2) + 1;
	hGMem = GlobalAlloc(GHND | GMEM_SHARE, (DWORD)size);
	cbuf = cp = (LPSTR)GlobalLock(hGMem);
	if (cp == (LPSTR)NULL)
		return;
	
	for (i=0; i<lptw->ScreenSize.y; i++) {
		count = lptw->ScreenSize.x;
		_fmemcpy(cp, lptw->ScreenBuffer + lptw->ScreenSize.x*i, count);
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
	hdc = GetDC(lptw->hWndText);
	SelectFont(hdc, lptw->hfont);
	GetTextMetrics(hdc,(TEXTMETRIC FAR *)&tm);
	if (tm.tmCharSet == OEM_CHARSET)
		type = CF_OEMTEXT;
	else
		type = CF_TEXT;
	ReleaseDC(lptw->hWndText, hdc);
	/* give buffer to clipboard */
	OpenClipboard(lptw->hWndText);
	EmptyClipboard();
	SetClipboardData(type, hGMem);
	CloseClipboard();
}


/* text window */
LRESULT CALLBACK _export
WndTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	int nYinc, nXinc;
	LPTW lptw;

	lptw = (LPTW)GetWindowLong(hwnd, 0);

	switch(message) {
		case WM_GSVIEW:
			if (wParam != PIPE_DATA)
				return 0;
#ifdef __WIN32__
			if (is_winnt) {
			    if (pipe_mapptr != NULL) {
				/* EOF if block length is zero */
	    			pipe_eof = ( *((WORD *)pipe_mapptr) == 0 );
				if (pipe_eof)
				    pipe_count = 0;
			    }
			    if (!pipe_eof && pipe_count) {
				MessageBox(hwnd, "pipe overflow", szAppName, MB_OK);
				pipe_count = 0;
			    }
			}
			else
#endif
			{
			    if (pipe_count) {
				MessageBox(hwnd, "pipe overflow", szAppName, MB_OK);
				pipe_count = 0;
				if (pipe_lpbyte != (LPBYTE)NULL)
				    GlobalUnlock(pipe_hglobal);
				if (pipe_hglobal != (HGLOBAL)NULL)
				    GlobalFree(pipe_hglobal);
			    }
			}
			pipe_hglobal = (HGLOBAL)lParam;
			return 0;
		case WM_SYSCOMMAND:
			switch(LOWORD(wParam)) {
				case M_COPY_CLIP:
					TextCopyClip(lptw);
					return 0;
			}
			break;
		case WM_SETFOCUS: 
			lptw->bFocus = TRUE;
			CreateCaret(hwnd, 0, lptw->CharSize.x, 2+lptw->CaretHeight);
			SetCaretPos(lptw->CursorPos.x*lptw->CharSize.x - lptw->ScrollPos.x,
				lptw->CursorPos.y*lptw->CharSize.y + lptw->CharAscent
				 - lptw->CaretHeight - lptw->ScrollPos.y);
			if (lptw->bGetCh)
				ShowCaret(hwnd);
			break;
		case WM_KILLFOCUS: 
			DestroyCaret();
			lptw->bFocus = FALSE;
			break;
		case WM_SIZE:
			lptw->ClientSize.y = HIWORD(lParam);
			lptw->ClientSize.x = LOWORD(lParam);

			lptw->ScrollMax.y = max(0, lptw->CharSize.y*lptw->ScreenSize.y - lptw->ClientSize.y);
			lptw->ScrollPos.y = min(lptw->ScrollPos.y, lptw->ScrollMax.y);

			SetScrollRange(hwnd, SB_VERT, 0, lptw->ScrollMax.y, FALSE);
			SetScrollPos(hwnd, SB_VERT, lptw->ScrollPos.y, TRUE);

			lptw->ScrollMax.x = max(0, lptw->CharSize.x*lptw->ScreenSize.x - lptw->ClientSize.x);
			lptw->ScrollPos.x = min(lptw->ScrollPos.x, lptw->ScrollMax.x);

			SetScrollRange(hwnd, SB_HORZ, 0, lptw->ScrollMax.x, FALSE);
			SetScrollPos(hwnd, SB_HORZ, lptw->ScrollPos.x, TRUE);

			if (lptw->bFocus && lptw->bGetCh) {
				SetCaretPos(lptw->CursorPos.x*lptw->CharSize.x - lptw->ScrollPos.x,
					lptw->CursorPos.y*lptw->CharSize.y + lptw->CharAscent 
					- lptw->CaretHeight - lptw->ScrollPos.y);
				ShowCaret(hwnd);
			}
			return(0);
		case WM_VSCROLL:
			switch(LOWORD(wParam)) {
				case SB_TOP:
					nYinc = -lptw->ScrollPos.y;
					break;
				case SB_BOTTOM:
					nYinc = lptw->ScrollMax.y - lptw->ScrollPos.y;
					break;
				case SB_LINEUP:
					nYinc = -lptw->CharSize.y;
					break;
				case SB_LINEDOWN:
					nYinc = lptw->CharSize.y;
					break;
				case SB_PAGEUP:
					nYinc = min(-1,-lptw->ClientSize.y);
					break;
				case SB_PAGEDOWN:
					nYinc = max(1,lptw->ClientSize.y);
					break;
				case SB_THUMBPOSITION:
#ifdef __WIN32__
					nYinc = HIWORD(wParam) - lptw->ScrollPos.y;
#else
					nYinc = LOWORD(lParam) - lptw->ScrollPos.y;
#endif
					break;
				default:
					nYinc = 0;
				}
			if ( (nYinc = max(-lptw->ScrollPos.y, 
				min(nYinc, lptw->ScrollMax.y - lptw->ScrollPos.y)))
				!= 0 ) {
				lptw->ScrollPos.y += nYinc;
				ScrollWindow(hwnd,0,-nYinc,NULL,NULL);
				SetScrollPos(hwnd,SB_VERT,lptw->ScrollPos.y,TRUE);
				UpdateWindow(hwnd);
			}
			return(0);
		case WM_HSCROLL:
			switch(LOWORD(wParam)) {
				case SB_LINEUP:
					nXinc = -lptw->CharSize.x;
					break;
				case SB_LINEDOWN:
					nXinc = lptw->CharSize.x;
					break;
				case SB_PAGEUP:
					nXinc = min(-1,-lptw->ClientSize.x);
					break;
				case SB_PAGEDOWN:
					nXinc = max(1,lptw->ClientSize.x);
					break;
				case SB_THUMBPOSITION:
#ifdef __WIN32__
					nXinc = HIWORD(wParam) - lptw->ScrollPos.x;
#else
					nXinc = LOWORD(lParam) - lptw->ScrollPos.x;
#endif
					break;
				default:
					nXinc = 0;
				}
			if ( (nXinc = max(-lptw->ScrollPos.x, 
				min(nXinc, lptw->ScrollMax.x - lptw->ScrollPos.x)))
				!= 0 ) {
				lptw->ScrollPos.x += nXinc;
				ScrollWindow(hwnd,-nXinc,0,NULL,NULL);
				SetScrollPos(hwnd,SB_HORZ,lptw->ScrollPos.x,TRUE);
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
				long count;
					count = lptw->KeyBufIn - lptw->KeyBufOut;
					if (count < 0) count += lptw->KeyBufSize;
					if (count < lptw->KeyBufSize-2) {
						*lptw->KeyBufIn++ = 0;
						if (lptw->KeyBufIn - lptw->KeyBuf >= lptw->KeyBufSize)
							lptw->KeyBufIn = lptw->KeyBuf;	/* wrap around */
						*lptw->KeyBufIn++ = HIWORD(lParam) & 0xff;
						if (lptw->KeyBufIn - lptw->KeyBuf >= lptw->KeyBufSize)
							lptw->KeyBufIn = lptw->KeyBuf;	/* wrap around */
					}
				}
			  }
			}
			break;
		case WM_CHAR:
			{ /* store key in circular buffer */
			long count;
				count = lptw->KeyBufIn - lptw->KeyBufOut;
				if (count < 0) count += lptw->KeyBufSize;
				if (count < lptw->KeyBufSize-1) {
					*lptw->KeyBufIn++ = wParam;
					if (lptw->KeyBufIn - lptw->KeyBuf >= lptw->KeyBufSize)
						lptw->KeyBufIn = lptw->KeyBuf;	/* wrap around */
				}
			}
			return(0);
		case WM_PAINT:
			{
			POINT source, width, dest;
			hdc = BeginPaint(hwnd, &ps);
			SelectFont(hdc, lptw->hfont);
			SetMapMode(hdc, MM_TEXT);
			SetBkMode(hdc,OPAQUE);
			GetClientRect(hwnd, &rect);
			source.x = (rect.left + lptw->ScrollPos.x) / lptw->CharSize.x;		/* source */
			source.y = (rect.top + lptw->ScrollPos.y) / lptw->CharSize.y;
			dest.x = source.x * lptw->CharSize.x - lptw->ScrollPos.x; 				/* destination */
			dest.y = source.y * lptw->CharSize.y - lptw->ScrollPos.y;
			width.x = ((rect.right  + lptw->ScrollPos.x + lptw->CharSize.x - 1) / lptw->CharSize.x) - source.x; /* width */
			width.y = ((rect.bottom + lptw->ScrollPos.y + lptw->CharSize.y - 1) / lptw->CharSize.y) - source.y;
			if (source.x < 0)
				source.x = 0;
			if (source.y < 0)
				source.y = 0;
			if (source.x+width.x > lptw->ScreenSize.x)
				width.x = lptw->ScreenSize.x - source.x;
			if (source.y+width.y > lptw->ScreenSize.y)
				width.y = lptw->ScreenSize.y - source.y;
			/* for each line */
			while (width.y>0) {
				TextOut(hdc,dest.x,dest.y,
				    (LPSTR)(lptw->ScreenBuffer + source.y*lptw->ScreenSize.x + source.x),
				    width.x);
				dest.y += lptw->CharSize.y;
				source.y++;
				width.y--;
			}
			EndPaint(hwnd, &ps);
			return 0;
			}
#if WINVER >= 0x030a
		case WM_DROPFILES:
			{
			WORD version = LOWORD(GetVersion());
			if ((LOBYTE(version)*100 + HIBYTE(version)) >= 310)
				DragFunc(lptw, (HDROP)wParam);
			}
			break;
#endif
		case WM_CREATE:
			{
			RECT crect, wrect;
			TEXTMETRIC tm;
			lptw = ((CREATESTRUCT FAR *)lParam)->lpCreateParams;
			SetWindowLong(hwnd, 0, (LONG)lptw);
			lptw->hWndText = hwnd;
			/* get character size */
			TextMakeFont(lptw);
			hdc = GetDC(hwnd);
			SelectFont(hdc, lptw->hfont);
			GetTextMetrics(hdc,(LPTEXTMETRIC)&tm);
			lptw->CharSize.y = tm.tmHeight;
			lptw->CharSize.x = tm.tmAveCharWidth;
			lptw->CharAscent = tm.tmAscent;
			ReleaseDC(hwnd,hdc);
			GetClientRect(hwnd, &crect);
			if ( (lptw->CharSize.y*lptw->ScreenSize.y < crect.bottom)
			  || (lptw->CharSize.x*lptw->ScreenSize.x < crect.right) ) {
				/* shrink size */
			    GetWindowRect(lptw->hWndText,&wrect);
			    MoveWindow(lptw->hWndText, wrect.left, wrect.top,
				 wrect.right-wrect.left + (lptw->CharSize.x*lptw->ScreenSize.x - crect.right),
				 wrect.bottom-wrect.top + (lptw->CharSize.y*lptw->ScreenSize.y - crect.bottom),
				 TRUE);
			}
#if WINVER >= 0x030a
			{
			WORD version = LOWORD(GetVersion());
			if ((LOBYTE(version)*100 + HIBYTE(version)) >= 310)
				if ( (lptw->DragPre!=(LPSTR)NULL) && (lptw->DragPost!=(LPSTR)NULL) )
					DragAcceptFiles(hwnd, TRUE);
			}
#endif
			}
			break;
		case WM_CLOSE:
			if (lptw->shutdown)
				(*(lptw->shutdown))();
			break;
		case WM_DESTROY:
#if WINVER >= 0x030a
			{
			WORD version = LOWORD(GetVersion());
			if ((LOBYTE(version)*100 + HIBYTE(version)) >= 310)
				DragAcceptFiles(hwnd, FALSE);
			}
#endif
			if (lptw->hfont != 0)
				DeleteFont(lptw->hfont);
			break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
