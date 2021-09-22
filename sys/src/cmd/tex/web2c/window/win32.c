/*

  Win32  interface for Metafont.
  Author: Fabrice Popineau <Fabrice.Popineau@supelec.fr>

 */

#define	EXTERN	extern
#include "../mfd.h"

#ifdef WIN32WIN
#include <windows.h>

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) < (b) ? (b) : (a))
/* 
   The following constant enables some hack that should allow the
   window to process its messages. Basically, the principle is to 
   create a thread which will do the message loop. Unfortunately,
   it does not seem to work very well :-)
   */
#undef  LOOPMSG
#define LOOPMSG 1

#undef  DEBUG
/* #define DEBUG 1 */

static char szAppName[] = "MF";
static char szTitle[] = " MetaFont V2.718 Online Display";
static HWND my_window;
static HDC my_dc;
static HDC drawing_dc;
static HBITMAP hbm;
static RGBQUAD black = {0,0,0,0};
static RGBQUAD white = {255,255,255,0};
static MSG msg;
static HANDLE hAccelTable;
static HANDLE hMutex;
/* Scrollbars' variables */
static SCROLLINFO si;
static int xMinScroll;
static int xMaxScroll;
static int xCurrentScroll;
static int yMinScroll;
static int yMaxScroll;
static int yCurrentScroll;
static BOOL fScroll;
static BOOL fSize;

void Win32Error(char *);
#ifdef LOOPMSG
void __cdecl InitGui(void*);
#endif
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

int
mf_win32_initscreen()
{
  int ret;
  if ((ret = _beginthread(InitGui, 0, NULL)) == -1) {
    fprintf(stderr, "_beginthread returned with %d\n", ret);
    return 0;
  }
  else
    return 1;
}

  /* This window should be registered into a class. And maybe the
     message loop could run into a separate thread. */

void __cdecl InitGui(void *param)
{
  HWND hparentwnd;
  HANDLE hinst;
  WNDCLASSEX wndclass;
  char szFile[80];

  hinst = GetModuleHandle(NULL);
  GetModuleFileName (hinst, szFile, sizeof(szFile));
#ifdef DEBUG
  printf ("hinst = %x\n", hinst);
  printf ("File = %s\n", szFile);
#endif
  /* Fill in window class structure with parameters that describe
     the main window. */
  /* CS_OWNDC : un DC pour chaque fenêtre de la classe */
  wndclass.cbSize        = sizeof(wndclass);
  wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wndclass.lpfnWndProc   = (WNDPROC)WndProc;
  wndclass.cbClsExtra    = 0;
  wndclass.cbWndExtra    = 0;
  wndclass.hInstance     = hinst;
  wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
  wndclass.hIconSm       = LoadIcon (NULL, IDI_APPLICATION);
  wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
  wndclass.hbrBackground = GetStockObject(WHITE_BRUSH);
  wndclass.lpszClassName = szFile;
  wndclass.lpszMenuName  = NULL;

  if (!RegisterClassEx(&wndclass))
    Win32Error("Register class");

  hparentwnd = GetFocus();
  my_window = CreateWindow(szFile, szTitle, 
			   WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
			   CW_USEDEFAULT, 0, 
			   CW_USEDEFAULT, 0,
			   /* screenwidth, screendepth, */
			   hparentwnd, NULL, hinst, NULL);
  if (!my_window) {
    Win32Error("Create window");
  }
#ifdef DEBUG
  fprintf(stderr, "my_window = %x\n", my_window);
#endif
  
#ifdef LOOPMSG
  /* Acquiting for UpdateWindow() (WM_PAINT message generated) */
  hMutex = CreateMutex(NULL, FALSE, "DrawingMutex");
  my_dc = GetDC(my_window);
  /* Device context for drawing and the associated bitmap. */
  drawing_dc = CreateCompatibleDC(my_dc);
  hbm = CreateCompatibleBitmap(my_dc, screenwidth, screendepth);
  SelectObject(drawing_dc, hbm);
  /* Blank the bitmap */
  SelectObject(drawing_dc, GetStockObject(WHITE_BRUSH));
  PatBlt(drawing_dc, 0, 0, screenwidth, screendepth, PATCOPY);
  hAccelTable = LoadAccelerators (hinst, szTitle);

  ShowWindow(my_window, SW_SHOWNORMAL);
  UpdateWindow(my_window);

  /* Running the message loop */
  while (GetMessage(&msg, my_window, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
  }

#else
  drawing_dc = my_dc = GetDC(my_window);
#endif
}

/* Make sure the screen is up to date. (section 8.6 Handling the output
buffer)  */

void
mf_win32_updatescreen()
{
  RECT r;
  r.left   = 0;
  r.top    = 0;
  r.right  = screenwidth;
  r.bottom = screendepth;
  
  /* Send a WM_PAINT message to the window.
     The rectangle should be invalidated for the message being sent. */
  InvalidateRect(my_window, &r, FALSE);
  UpdateWindow(my_window);
}


/* Blank the rectangular inside the given coordinates. We don't need to
reset the foreground to black because we always set it at the beginning
of paintrow (below).  */

void mf_win32_blankrectangle P4C(screencol, left,
                                 screencol, right,
                                 screenrow, top,
                                 screenrow, bottom)
{
  RECT r;
  r.left   = left;
  r.top    = top;
  r.right  = right+1;		/* FIXME: must we fill the last row/column ? */
  r.bottom = bottom+1;
  /* Fixme : should synchronize with message processing. */
  WaitForSingleObject(hMutex, INFINITE);
  FillRect(drawing_dc, &r, GetStockObject(WHITE_BRUSH));
  ReleaseMutex(hMutex);
}


/* Paint a row with the given ``transition specifications''. We might be
able to do something here with drawing many lines.  */

void
mf_win32_paintrow P4C(screenrow, row,
                      pixelcolor, init_color,
                      transspec, tvect,
                      register screencol, vector_size)
{
    register int col;
    HGDIOBJ CurrentPen;
    HGDIOBJ WhitePen = GetStockObject(WHITE_PEN);
    HGDIOBJ BlackPen = GetStockObject(BLACK_PEN);
    /* FIXME: should sync with msg processing */
    WaitForSingleObject(hMutex, INFINITE);
    CurrentPen = (init_color == 0) ? WhitePen : BlackPen;
    do {
      col = *tvect++;
      MoveToEx(drawing_dc, col, (int) row, NULL);
      SelectObject(drawing_dc, CurrentPen);
      
      /* (section 6.3.2 Drawing single and multiple lines)
       */
      LineTo(drawing_dc, (int) *tvect, (int) row);
      CurrentPen = (CurrentPen == WhitePen) ? BlackPen : WhitePen;
    } while (--vector_size > 0);
    SelectObject(drawing_dc, GetStockObject(NULL_PEN));
    ReleaseMutex(hMutex);
}

void Win32Error(char *caller)
{
  LPVOID lpMsgBuf;

  FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
  /* Display the string. */
  MessageBox( NULL, lpMsgBuf, caller, MB_OK|MB_ICONINFORMATION );
  /* Free the buffer. */
  LocalFree( lpMsgBuf );
}

#if 0
void __cdecl LoopMsg(void* p)
{
  fprintf(stderr, "Entering thread ...\n");
  while (GetMessage(&msg, my_window, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
  }
}
#endif

LRESULT APIENTRY WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
  PAINTSTRUCT ps;
  int retval;

#ifdef DEBUG
  fprintf(stderr, "Message %x\n", iMsg);
#endif
#ifdef LOOPMSG
  WaitForSingleObject(hMutex, INFINITE);
  switch (iMsg) {
  case WM_CREATE:
    xMinScroll = 0;
    xMaxScroll = screenwidth;
    xCurrentScroll = 0;

    yMinScroll = 0;
    yMaxScroll = screendepth;
    yCurrentScroll = 0;

    fScroll = FALSE;
    fSize = FALSE;

    break;
  case WM_SIZE:
    {
      int xNewSize, yNewSize;
      xNewSize = LOWORD(lParam);
      yNewSize = HIWORD(lParam);

      fSize = TRUE;

      xMaxScroll = max(screenwidth, xNewSize);
      xCurrentScroll = min(xCurrentScroll, xMaxScroll);

      si.cbSize = sizeof(si);
      si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
      si.nMin = xMinScroll;
      si.nMax = xMaxScroll;
      si.nPage = xNewSize;
      si.nPos = xCurrentScroll;
      SetScrollInfo(my_window, SB_HORZ, &si, TRUE);

      yMaxScroll = max(screendepth, yNewSize);
      yCurrentScroll = min(yCurrentScroll, yMaxScroll);

      si.cbSize = sizeof(si);
      si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
      si.nMin = yMinScroll;
      si.nMax = yMaxScroll;
      si.nPage = yNewSize;
      si.nPos = yCurrentScroll;
      SetScrollInfo(my_window, SB_VERT, &si, TRUE);
      }
  case WM_PAINT:
    {
#if 0
      PRECT prect;
#endif
#ifdef DEBUG
      fprintf(stderr, "WM_PAINT message hbm = %x\n", hbm);
#endif
      BeginPaint(my_window, &ps);
      if (!BitBlt(my_dc,
		    0, 0, 
		    screenwidth, screendepth,
		    drawing_dc, 
		    xCurrentScroll, 
		    yCurrentScroll, SRCCOPY))
	  Win32Error("Bitblt");
	
#if 0
      /* Shouldn't repaint all the screen, but not so trivial! */
      if (fSize) {
	if (!BitBlt(my_dc,
		    0, 0, 
		    screenwidth, screendepth,
		    drawing_dc, 
		    xCurrentScroll, 
		    yCurrentScroll, SRCCOPY))
	  Win32Error("Bitblt");
	fSize = FALSE;
      }
    
      if (fScroll) {
	if (!BitBlt(my_dc,
		    prect->left, prect->top, 
		    prect->right - prect->left,
		    prect->bottom - prect->top, 
		    drawing_dc, 
		    prect->left+xCurrentScroll, 
		    prect->top+ yCurrentScroll, SRCCOPY))
	  Win32Error("Bitblt");
	fScroll = FALSE;
      }
#endif
      EndPaint(my_window, &ps);
      retval = 0;
      break;
    }
  case WM_HSCROLL:
    {
      int xDelta;
      int xNewPos;
      int yDelta = 0;
      switch (LOWORD(wParam)) {
      case SB_PAGEUP:
	xNewPos = xCurrentScroll - 50;
	break;
      case SB_PAGEDOWN:
	xNewPos = xCurrentScroll + 50;
	break;
      case SB_LINEUP:
	xNewPos = xCurrentScroll - 5;
	break;
      case SB_LINEDOWN:
	xNewPos = xCurrentScroll + 5;
	break;
      case SB_THUMBPOSITION:
	xNewPos = HIWORD(wParam);
	break;
      default:
	xNewPos = xCurrentScroll;
      }
      xNewPos = max(0, xNewPos);
      xNewPos = min(xMaxScroll, xNewPos);
      
      if (xNewPos == xCurrentScroll)
	break;
      
      fScroll = TRUE;
      
      xDelta = xNewPos - xCurrentScroll;
      xCurrentScroll = xNewPos;
      
      ScrollWindowEx(my_window, -xDelta, -yDelta, (CONST RECT *)NULL,
		     (CONST RECT *)NULL, (HRGN)NULL, (LPRECT)NULL,
		     SW_INVALIDATE);
      UpdateWindow(my_window);
      
      si.cbSize = sizeof(si);
      si.fMask = SIF_POS;
      si.nPos = xCurrentScroll;
      SetScrollInfo(my_window, SB_HORZ, &si, TRUE);
    }
  break;

  case WM_VSCROLL:
    {
      int xDelta = 0;
      int yNewPos;
      int yDelta;
      switch (LOWORD(wParam)) {
      case SB_PAGEUP:
	yNewPos = yCurrentScroll - 50;
	break;
      case SB_PAGEDOWN:
	yNewPos = yCurrentScroll + 50;
	break;
      case SB_LINEUP:
	yNewPos = yCurrentScroll - 5;
	break;
      case SB_LINEDOWN:
	yNewPos = yCurrentScroll + 5;
	break;
      case SB_THUMBPOSITION:
	yNewPos = HIWORD(wParam);
	break;
      default:
	yNewPos = yCurrentScroll;
      }
      yNewPos = max(0, yNewPos);
      yNewPos = min(yMaxScroll, yNewPos);
      
      if (yNewPos == yCurrentScroll)
	break;
      
      fScroll = TRUE;
      
      yDelta = yNewPos - yCurrentScroll;
      yCurrentScroll = yNewPos;
      
      ScrollWindowEx(my_window, -xDelta, -yDelta, (CONST RECT *)NULL,
		     (CONST RECT *)NULL, (HRGN)NULL, (LPRECT)NULL,
		     SW_INVALIDATE);
      UpdateWindow(my_window);
      
      si.cbSize = sizeof(si);
      si.fMask = SIF_POS;
      si.nPos = yCurrentScroll;
      SetScrollInfo(my_window, SB_VERT, &si, TRUE);
    }
  break;
  
  case WM_DESTROY:
    PostQuitMessage(0);
    retval = 0;
    break;
  default:
    retval = DefWindowProc(hwnd, iMsg, wParam, lParam);
    break;
  }
  ReleaseMutex(hMutex);
  return retval;
#else
  return DefWindowProc(hwnd, iMsg, wParam, lParam);
#endif
}

#else
int win32_dummy;
#endif /* WIN32WIN */
