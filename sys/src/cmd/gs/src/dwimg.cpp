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

// $Id: dwimg.cpp,v 1.1 2000/03/09 08:40:40 lpd Exp $


#define STRICT
#include <windows.h>
#include <shellapi.h>
extern "C" {
#include "gsdll.h"
}
#include "dwmain.h"
#include "dwdll.h"
#include "dwimg.h"

extern gsdll_class gsdll;

char szImgName[] = "Ghostscript Image";
#define M_COPY_CLIP 1
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

// Forward references
LRESULT CALLBACK _export WndImgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HINSTANCE ImageWindow::hInstance;
HWND ImageWindow::hwndtext;
ImageWindow *ImageWindow::first;


// friend
ImageWindow *
FindImageWindow(char FAR *dev)
{
    ImageWindow *iw;
    for (iw = ImageWindow::first; iw!=0; iw=iw->next) {
	if (iw->device == dev)
	    return iw;
    }
    return NULL;
}


// open window for device and add to list
void 
ImageWindow::open(char FAR *dev)
{
    // register class if first window
    if (first == (ImageWindow *)NULL)
	register_class();
   
    // add to list
    if (first)
	next = first;
    first = this;

    // remember device handle
    device = dev;

    // open window
    create_window();
}

// close window and remove from list
void
ImageWindow::close(void)
{
    DestroyWindow(hwnd);

    // remove from list
    if (this == first) {
	first = this->next;
    }
    else {
	ImageWindow *tmp;
	for (tmp = first; tmp!=0; tmp=tmp->next) {
	    if (this == tmp->next)
		tmp->next = this->next;
	}
    }
    
}


void
ImageWindow::register_class(void)
{
	WNDCLASS wndclass;

	/* register the window class for graphics */
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndImgProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = sizeof(LONG);
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(hInstance,(LPSTR)MAKEINTRESOURCE(GSIMAGE_ICON));
	wndclass.hCursor = LoadCursor((HINSTANCE)NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szImgName;
	RegisterClass(&wndclass);
}

void
ImageWindow::create_window(void)
{
	HMENU sysmenu;

	cxClient = cyClient = 0;
	nVscrollPos = nVscrollMax = 0;
	nHscrollPos = nHscrollMax = 0;

	// create window
	hwnd = CreateWindow(szImgName, (LPSTR)szImgName,
		  WS_OVERLAPPEDWINDOW,
		  CW_USEDEFAULT, CW_USEDEFAULT, 
		  CW_USEDEFAULT, CW_USEDEFAULT, 
		  NULL, NULL, hInstance, (void FAR *)this);
	ShowWindow(hwnd, SW_SHOWMINNOACTIVE);

	// modify the menu to have the new items we want
	sysmenu = GetSystemMenu(hwnd, 0);	// get the sysmenu
	AppendMenu(sysmenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(sysmenu, MF_STRING, M_COPY_CLIP, "Copy to Clip&board");
}

	
void
ImageWindow::sync(void)
{
    if ( !IsWindow(hwnd) )	// some clod closed the window
	create_window();

    if ( !IsIconic(hwnd) ) {  /* redraw window */
	InvalidateRect(hwnd, NULL, 1);
	UpdateWindow(hwnd);
    }
}


void
ImageWindow::page(void)
{
    if (IsIconic(hwnd))    // useless as an Icon so fix it
	ShowWindow(hwnd, SW_SHOWNORMAL);
    BringWindowToTop(hwnd);

    sync();
}


void
ImageWindow::size(int x, int y)
{
    width = x;
    height = y;
}


/* image window */
LRESULT CALLBACK _export
WndImgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ImageWindow *iw;
    if (message == WM_CREATE) {
	// Object is stored in window extra data.
	// Nothing must try to use the object before WM_CREATE
	// initializes it here.
	iw = (ImageWindow *)(((CREATESTRUCT FAR *)lParam)->lpCreateParams);
	SetWindowLong(hwnd, 0, (LONG)iw);
    }

    // call the object window procedure
    iw = (ImageWindow *)GetWindowLong(hwnd, 0);
    return iw->WndProc(hwnd, message, wParam, lParam);
}

/* graphics window */
LRESULT 
ImageWindow::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{	
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;
    int nVscrollInc, nHscrollInc;

    switch(message) {
	case WM_SYSCOMMAND:
	    // copy to clipboard
	    if (LOWORD(wParam) == M_COPY_CLIP) {
		HGLOBAL hglobal;
		HPALETTE hpalette;
		gsdll.lock_device(device, 1);
		hglobal = gsdll.copy_dib(device);
		if (hglobal == (HGLOBAL)NULL) {
		    MessageBox(hwnd, "Not enough memory to Copy to Clipboard", 
			szImgName, MB_OK | MB_ICONEXCLAMATION);
		    gsdll.lock_device(device, 0);
		    return 0;
		}
		OpenClipboard(hwnd);
		EmptyClipboard();
		SetClipboardData(CF_DIB, hglobal);
		hpalette = gsdll.copy_palette(device);
		if (hpalette)
		    SetClipboardData(CF_PALETTE, hpalette);
		CloseClipboard();
		gsdll.lock_device(device, 0);
		return 0;
	    }
	    break;
	case WM_CREATE:
	    // enable drag-drop
	    DragAcceptFiles(hwnd, TRUE);
	    break;
	case WM_SIZE:
	    if (wParam == SIZE_MINIMIZED)
		    return(0);
	    cyClient = HIWORD(lParam);
	    cxClient = LOWORD(lParam);

	    cyAdjust = min(height, cyClient) - cyClient;
	    cyClient += cyAdjust;

	    nVscrollMax = max(0, height - cyClient);
	    nVscrollPos = min(nVscrollPos, nVscrollMax);

	    SetScrollRange(hwnd, SB_VERT, 0, nVscrollMax, FALSE);
	    SetScrollPos(hwnd, SB_VERT, nVscrollPos, TRUE);

	    cxAdjust = min(width,  cxClient) - cxClient;
	    cxClient += cxAdjust;

	    nHscrollMax = max(0, width - cxClient);
	    nHscrollPos = min(nHscrollPos, nHscrollMax);

	    SetScrollRange(hwnd, SB_HORZ, 0, nHscrollMax, FALSE);
	    SetScrollPos(hwnd, SB_HORZ, nHscrollPos, TRUE);

	    if ((wParam==SIZENORMAL) && (cxAdjust!=0 || cyAdjust!=0)) {
		GetWindowRect(GetParent(hwnd),&rect);
		MoveWindow(GetParent(hwnd),rect.left,rect.top,
		    rect.right-rect.left+cxAdjust,
		    rect.bottom-rect.top+cyAdjust, TRUE);
		cxAdjust = cyAdjust = 0;
	    }
	    return(0);
	case WM_VSCROLL:
	    switch(LOWORD(wParam)) {
		case SB_TOP:
		    nVscrollInc = -nVscrollPos;
		    break;
		case SB_BOTTOM:
		    nVscrollInc = nVscrollMax - nVscrollPos;
		    break;
		case SB_LINEUP:
		    nVscrollInc = -cyClient/16;
		    break;
		case SB_LINEDOWN:
		    nVscrollInc = cyClient/16;
		    break;
		case SB_PAGEUP:
		    nVscrollInc = min(-1,-cyClient);
		    break;
		case SB_PAGEDOWN:
		    nVscrollInc = max(1,cyClient);
		    break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
#ifdef __WIN32__
		    nVscrollInc = HIWORD(wParam) - nVscrollPos;
#else
		    nVscrollInc = LOWORD(lParam) - nVscrollPos;
#endif
		    break;
		default:
		    nVscrollInc = 0;
	    }
	    if ((nVscrollInc = max(-nVscrollPos, 
		min(nVscrollInc, nVscrollMax - nVscrollPos)))!=0) {
		nVscrollPos += nVscrollInc;
		ScrollWindow(hwnd,0,-nVscrollInc,NULL,NULL);
		SetScrollPos(hwnd,SB_VERT,nVscrollPos,TRUE);
		UpdateWindow(hwnd);
	    }
	    return(0);
	case WM_HSCROLL:
	    switch(LOWORD(wParam)) {
		case SB_LINEUP:
		    nHscrollInc = -cxClient/16;
		    break;
		case SB_LINEDOWN:
		    nHscrollInc = cyClient/16;
		    break;
		case SB_PAGEUP:
		    nHscrollInc = min(-1,-cxClient);
		    break;
		case SB_PAGEDOWN:
		    nHscrollInc = max(1,cxClient);
		    break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
#ifdef __WIN32__
		    nHscrollInc = HIWORD(wParam) - nHscrollPos;
#else
		    nHscrollInc = LOWORD(lParam) - nHscrollPos;
#endif
		    break;
		default:
		    nHscrollInc = 0;
	    }
	    if ((nHscrollInc = max(-nHscrollPos, 
		min(nHscrollInc, nHscrollMax - nHscrollPos)))!=0) {
		nHscrollPos += nHscrollInc;
		ScrollWindow(hwnd,-nHscrollInc,0,NULL,NULL);
		SetScrollPos(hwnd,SB_HORZ,nHscrollPos,TRUE);
		UpdateWindow(hwnd);
	    }
	    return(0);
	case WM_KEYDOWN:
	    switch(LOWORD(wParam)) {
		case VK_HOME:
		    SendMessage(hwnd,WM_VSCROLL,SB_TOP,0L);
		    break;
		case VK_END:
		    SendMessage(hwnd,WM_VSCROLL,SB_BOTTOM,0L);
		    break;
		case VK_PRIOR:
		    SendMessage(hwnd,WM_VSCROLL,SB_PAGEUP,0L);
		    break;
		case VK_NEXT:
		    SendMessage(hwnd,WM_VSCROLL,SB_PAGEDOWN,0L);
		    break;
		case VK_UP:
		    SendMessage(hwnd,WM_VSCROLL,SB_LINEUP,0L);
		    break;
		case VK_DOWN:
		    SendMessage(hwnd,WM_VSCROLL,SB_LINEDOWN,0L);
		    break;
		case VK_LEFT:
		    SendMessage(hwnd,WM_HSCROLL,SB_PAGEUP,0L);
		    break;
		case VK_RIGHT:
		    SendMessage(hwnd,WM_HSCROLL,SB_PAGEDOWN,0L);
		    break;
		case VK_RETURN:
		    BringWindowToTop(hwndtext);
		    break;
	    }
	    return(0);
	case WM_CHAR:
	    // send on all characters to text window
	    if (hwndtext)
		SendMessage(hwndtext, message, wParam, lParam);
	    return 0;
	case WM_PAINT:
	    {
	    int sx,sy,wx,wy,dx,dy;
	    hdc = BeginPaint(hwnd, &ps);
	    SetMapMode(hdc, MM_TEXT);
	    SetBkMode(hdc,OPAQUE);
	    gsdll.lock_device(device, 1);
	    rect = ps.rcPaint;
	    dx = rect.left;	/* destination */
	    dy = rect.top;
	    wx = rect.right-rect.left; /* width */
	    wy = rect.bottom-rect.top;
	    sx = rect.left;	/* source */
	    sy = rect.top;
	    sx += nHscrollPos; /* scrollbars */
	    sy += nVscrollPos;	
	    if (sx+wx > width)
		    wx = width - sx;
	    if (sy+wy > height)
		    wy = height - sy;

	    gsdll.draw(device, hdc, dx, dy, wx, wy, sx, sy);
	    gsdll.lock_device(device, 0);

	    EndPaint(hwnd, &ps);
	    return 0;
	    }
	case WM_DROPFILES:
	    SendMessage(hwndtext, message, wParam, lParam);
	    break;
	case WM_DESTROY:
	    DragAcceptFiles(hwnd, FALSE);
	    break;

    }

not_ours:
	return DefWindowProc(hwnd, message, wParam, lParam);
}

