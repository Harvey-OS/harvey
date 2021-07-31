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


// $Id: dwimg.h,v 1.1 2000/03/09 08:40:40 lpd Exp $

// Image Window class

class ImageWindow {
    static ImageWindow *first;
    ImageWindow *next;

    HWND hwnd;
    char FAR *device;		// handle to Ghostscript device

    int width, height;

    // Window scrolling stuff
    int cxClient, cyClient;
    int cxAdjust, cyAdjust;
    int nVscrollPos, nVscrollMax;
    int nHscrollPos, nHscrollMax;

    void register_class(void);

         public:
    static HINSTANCE hInstance;	// instance of EXE

    static HWND hwndtext;	// handle to text window

    friend ImageWindow *FindImageWindow(char FAR * dev);
    void open(char FAR * dev);
    void close(void);
    void sync(void);
    void page(void);
    void size(int x, int y);
    void create_window(void);
    LRESULT WndProc(HWND, UINT, WPARAM, LPARAM);
};
