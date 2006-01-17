/* Copyright (C) 2003-2004 artofcode LLC.  All rights reserved.
  
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

/* $Id: dmmain.c,v 1.5 2004/12/09 08:24:28 giles Exp $ */

/* Ghostscript shlib example wrapper for Macintosh (Classic/Carbon) contributed
   by Nigel Hathaway. Uses the Metrowerks CodeWarrior SIOUX command-line library.
 */

#if __ide_target("Ghostscript PPC (Debug)") || __ide_target("Ghostscript PPC (Release)")
#define TARGET_API_MAC_CARBON 0
#define TARGET_API_MAC_OS8 1
#define ACCESSOR_CALLS_ARE_FUNCTIONS 1
#endif

#include <Carbon.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <console.h>
#include <SIOUX.h>
#include <SIOUXGlobals.h>
#include <SIOUXMenus.h>

#include "gscdefs.h"
#define GSREVISION gs_revision
#include "ierrors.h"
#include "iapi.h"

#if DEBUG
#include "vdtrace.h"
#endif

#include "gdevdsp.h"

#define kScrollBarWidth   15
#define MAX_ARGS 25

Boolean   gRunningOnX = false;
Boolean   gDone;
ControlActionUPP gActionFunctionScrollUPP;

const char start_string[] = "systemdict /start get exec\n";
void *instance;

const unsigned int display_format = DISPLAY_COLORS_RGB | DISPLAY_UNUSED_FIRST |
                                    DISPLAY_DEPTH_8 | DISPLAY_BIGENDIAN |
                                    DISPLAY_TOPFIRST;
typedef struct IMAGE_S IMAGE;
struct IMAGE_S {
    void *handle;
    void *device;
    WindowRef  windowRef;
    ControlRef scrollbarVertRef;
    ControlRef scrollbarHorizRef;
    PixMapHandle pixmapHdl;
    UInt64 update_time;
    int update_interval;
    IMAGE *next;
};

IMAGE *first_image;

static IMAGE *image_find(void *handle, void *device);

static int GSDLLCALL gsdll_stdin(void *instance, char *buf, int len);
static int GSDLLCALL gsdll_stdout(void *instance, const char *str, int len);
static int GSDLLCALL gsdll_stderr(void *instance, const char *str, int len);
static int GSDLLCALL gsdll_poll(void *handle);

static int display_open(void *handle, void *device);
static int display_preclose(void *handle, void *device);
static int display_close(void *handle, void *device);
static int display_presize(void *handle, void *device, int width, int height, 
    int raster, unsigned int format);
static int display_size(void *handle, void *device, int width, int height, 
    int raster, unsigned int format, unsigned char *pimage);
static int display_sync(void *handle, void *device);
static int display_page(void *handle, void *device, int copies, int flush);
static int display_update(void *handle, void *device, 
    int x, int y, int w, int h);

static size_t get_input(void *ptr, size_t size);

static void window_create (IMAGE *img);
static void window_invalidate (WindowRef windowRef);
static void window_adjust_scrollbars (WindowRef windowRef);

void    main                      (void);
OSErr   quitAppEventHandler       (AppleEvent *,AppleEvent *,SInt32);
void    doEvents                  (EventRecord *);
void    doMouseDown               (EventRecord *);
void    doUpdate                  (EventRecord *);
void    doUpdateWindow            (EventRecord *);
void    doOSEvent                 (EventRecord *);
void    doInContent               (EventRecord *,WindowRef);
pascal void    actionFunctionScroll      (ControlRef,ControlPartCode);

/*********************************************************************/
/* stdio functions */
static int GSDLLCALL
gsdll_stdin(void *instance, char *buf, int len)
{
    if (isatty(fileno(stdin)))
       return get_input(buf, len);
    else
       return fread(buf, 1, len, stdin);
}

static int GSDLLCALL
gsdll_stdout(void *instance, const char *str, int len)
{
    int n = fwrite(str, 1, len, stdout);
    fflush(stdout);
    return n;
}

static int GSDLLCALL
gsdll_stderr(void *instance, const char *str, int len)
{
    return gsdll_stdout(instance, str, len);
}

/* Poll the caller for cooperative multitasking. */
/* If this function is NULL, polling is not needed */
static int GSDLLCALL gsdll_poll(void *handle)
{
    EventRecord eventStructure;

    while (WaitNextEvent(everyEvent, &eventStructure, 0, NULL))
        doEvents(&eventStructure);

    return (gDone ? e_Fatal : 0);
}
/*********************************************************************/

/* new dll display device */

/* New device has been opened */
/* This is the first event from this device. */
static int display_open(void *handle, void *device)
{
    IMAGE *img = (IMAGE *)malloc(sizeof(IMAGE));
    if (img == NULL)
       return -1;
    memset(img, 0, sizeof(IMAGE));

    /* add to list */
    if (first_image)
       img->next = first_image;
    first_image = img;

    /* remember device and handle */
    img->handle = handle;
    img->device = device;

    /* create window */
    window_create(img);

    gsdll_poll(handle);
    return 0;
}

/* Device is about to be closed. */
/* Device will not be closed until this function returns. */
static int display_preclose(void *handle, void *device)
{
    /* do nothing - no thread synchonisation needed */
    return 0;
}

/* Device has been closed. */
/* This is the last event from this device. */
static int display_close(void *handle, void *device)
{
    IMAGE *img = image_find(handle, device);
    if (img == NULL)
       return -1;

    gsdll_poll(handle);

    /* remove from list */
    if (img == first_image)
        first_image = img->next;
    else
    {
        IMAGE *tmp;
        for (tmp = first_image; tmp!=0; tmp=tmp->next)
        {
            if (img == tmp->next)
            tmp->next = img->next;
        }
    }

    DisposePixMap(img->pixmapHdl);   // need to go in doCloseWindow()
    DisposeWindow(img->windowRef);

    free(img);

    return 0;
}

/* Device is about to be resized. */
/* Resize will only occur if this function returns 0. */
static int display_presize(void *handle, void *device, int width, int height, 
    int raster, unsigned int format)
{
    /* Check for correct format (32-bit RGB), fatal error if not */
    if (format != display_format)
    {
        printf("DisplayFormat has been set to an incompatible value.\n");
        fflush(stdout);
        return e_rangecheck;
    }

    return 0;
}
   
/* Device has been resized. */
/* New pointer to raster returned in pimage */
static int display_size(void *handle, void *device, int width, int height, 
    int raster, unsigned int format, unsigned char *pimage)
{
    PixMapPtr pixmap;
    IMAGE *img = image_find(handle, device);
    if (img == NULL)
       return -1;

    /* Check that image is within allowable bounds */
    if (raster > 0x3fff)
    {
       printf("QuickDraw can't cope with an image this big.\n");
       fflush(stdout);
       if (img->pixmapHdl)
       {
           DisposePixMap(img->pixmapHdl);
           img->pixmapHdl = NULL;
       }
       return e_rangecheck;
    }

    /* Create the PixMap */
    if (!img->pixmapHdl)
        img->pixmapHdl = NewPixMap();

    pixmap = *(img->pixmapHdl);
    pixmap->baseAddr = (char*)pimage;
    pixmap->rowBytes = (((SInt16)raster) & 0x3fff) | 0x8000;
    pixmap->bounds.right = width;
    pixmap->bounds.bottom = height;
    pixmap->packType = 0;
    pixmap->packSize = 0;
    pixmap->pixelType = RGBDirect;
    pixmap->pixelSize = 32;
    pixmap->cmpCount = 3;
    pixmap->cmpSize = 8;

    /* Update the display window */
    window_adjust_scrollbars(img->windowRef);
    window_invalidate(img->windowRef);
    return gsdll_poll(handle);
}
   
/* flushpage */
static int display_sync(void *handle, void *device)
{
    IMAGE *img = image_find(handle, device);
    if (img == NULL)
       return -1;

    window_invalidate(img->windowRef);
    gsdll_poll(handle);

    return 0;
}

/* showpage */
/* If you want to pause on showpage, then don't return immediately */
static int display_page(void *handle, void *device, int copies, int flush)
{
    return display_sync(handle, device);
}

/* Poll the caller for cooperative multitasking. */
/* If this function is NULL, polling is not needed */
static int display_update(void *handle, void *device, 
    int x, int y, int w, int h)
{
    UInt64 t1;
    UInt64 t2;
    int delta;
    IMAGE *img = image_find(handle, device);
    if (img == NULL)
       return -1;

    Microseconds((UnsignedWide*)&t1);
    delta = (t1 - img->update_time) / 1000000L;
    if (img->update_interval < 1)
    img->update_interval = 1;    /* seconds */
    if (delta < 0)
        img->update_time = t1;
    else if (delta > img->update_interval)
    {
        /* redraw window */
        window_invalidate(img->windowRef);

        /* Make sure the update interval is at least 10 times
         * what it takes to paint the window
         */
        Microseconds((UnsignedWide*)&t2);
        delta = (t2 - t1) / 1000;
        if (delta < 0)
            delta += 60000;    /* delta = time to redraw */
        if (delta > img->update_interval * 100)
            img->update_interval = delta/100;
        img->update_time = t2;
    }

    return gsdll_poll(handle);
}

display_callback display = { 
    sizeof(display_callback),
    DISPLAY_VERSION_MAJOR,
    DISPLAY_VERSION_MINOR,
    display_open,
    display_preclose,
    display_close,
    display_presize,
    display_size,
    display_sync,
    display_page,
    display_update,
    NULL,    /* memalloc */
    NULL,    /* memfree */
    NULL	 /* display_separation */
};

static IMAGE * image_find(void *handle, void *device)
{
    IMAGE *img;
    for (img = first_image; img!=0; img=img->next) {
    if ((img->handle == handle) && (img->device == device))
        return img;
    }
    return NULL;
}

/*********************************************************************/

static char *stdin_buf = NULL;
static size_t stdin_bufpos = 0;
static size_t stdin_bufsize = 0;

/* This function is a fudge which allows the SIOUX window to be waiting for
   input and not be modal at the same time. (Why didn't MetroWerks think of that?)
   It is based on the SIOUX function ReadCharsFromConsole(), and contains an
   event loop which allows other windows to be active.
   It collects characters up to when the user presses ENTER, stores the complete
   buffer and gives as much to the calling function as it wants until it runs
   out, at which point it gets another line (or set of lines if pasting from the
   clipboard) from the user.
*/
static size_t get_input(void *ptr, size_t size)
{
    EventRecord eventStructure;
    long charswaiting, old_charswaiting = 0;
    char *text;

#if SIOUX_USE_WASTE
    Handle textHandle;
#endif

    /* If needing more input, set edit start position */
    if (!stdin_buf)
#if SIOUX_USE_WASTE
        SIOUXselstart = WEGetTextLength(SIOUXTextWindow->edit);
#else
        SIOUXselstart = (*SIOUXTextWindow->edit)->teLength;
#endif

    /* Wait until user presses exit (or quits) */
    while(!gDone && !stdin_buf)
    {
#if SIOUX_USE_WASTE
        charswaiting = WEGetTextLength(SIOUXTextWindow->edit) - SIOUXselstart;
#else
        if ((*SIOUXTextWindow->edit)->teLength > 0)
            charswaiting = (*SIOUXTextWindow->edit)->teLength - SIOUXselstart;
        else
            charswaiting = ((unsigned short) (*SIOUXTextWindow->edit)->teLength) - SIOUXselstart;
#endif

        /* If something has happened, see if we need to do anything */
        if (charswaiting != old_charswaiting)
        {
#if SIOUX_USE_WASTE
            textHandle = WEGetText(SIOUXTextWindow->edit);
            HLock(textHandle);
            text = *textHandle + SIOUXselstart;
#else
            text = (*(*SIOUXTextWindow->edit)->hText) + SIOUXselstart;
#endif
            /* If user has pressed enter, gather up the buffer ready for returning */
            if (text[charswaiting-1] == '\r')
            {
                stdin_buf = malloc(charswaiting);
                if (!stdin_buf)
                    return -1;
                stdin_bufsize = charswaiting;
                memcpy(stdin_buf, text, stdin_bufsize);
                SIOUXselstart += charswaiting;
                
                text = stdin_buf;
                while (text = memchr(text, '\r', charswaiting - (text - stdin_buf)))
                    *text = '\n';
            }
#if SIOUX_USE_WASTE
            HUnlock(textHandle);
#endif
            old_charswaiting = charswaiting;

            if (stdin_buf)
                break;
        }

        /* Wait for next event and process it */
        SIOUXState = SCANFING;

        if(WaitNextEvent(everyEvent, &eventStructure, SIOUXSettings.sleep ,NULL))
            doEvents(&eventStructure);
        else
            SIOUXHandleOneEvent(&eventStructure);

        SIOUXState = IDLE;
    }

    /* If data has been entered, return as much as has been requested */
    if (stdin_buf && !gDone)
    {
        if (size >= stdin_bufsize - stdin_bufpos)
        {
            size = stdin_bufsize - stdin_bufpos;
            memcpy (ptr, stdin_buf + stdin_bufpos, size);
            free(stdin_buf);
            stdin_buf = NULL;
            stdin_bufpos = 0;
            stdin_bufsize = 0;
        }
        else
        {
            memcpy (ptr, stdin_buf + stdin_bufpos, size);
            stdin_bufpos += size;
        }
        return size;
    }
    else if (stdin_buf)
    {
        free(stdin_buf);
        stdin_buf = NULL;
        stdin_bufpos = 0;
        stdin_bufsize = 0;
    }

    return 0;
}

/*********************************************************************/

static void window_create(IMAGE *img)
{
    WindowRef windowRef;
    Str255    windowTitle = "\pGhostscript Image";
    Rect      windowRect = {20,4,580,420};//, portRect;
    Rect      scrollbarRect = {0,0,0,0};

#if TARGET_API_MAC_CARBON
    GetAvailableWindowPositioningBounds(GetMainDevice(),&windowRect);
#endif

    /* Create a new suitablty positioned window */
    windowRect.top = windowRect.top * 2 + 2;
    windowRect.bottom -= 10;
    windowRect.left += 4;
    windowRect.right = ((windowRect.bottom - windowRect.top) * 3) / 4 + windowRect.left;

    if(!(windowRef = NewCWindow(NULL, &windowRect, windowTitle, true,
                                zoomDocProc, (WindowRef) -1, false, 0)))
        ExitToShell();

    img->windowRef = windowRef;

    SetWRefCon(img->windowRef, (SInt32)img);

    /* Create the window's scrollbars */
#if TARGET_API_MAC_CARBON
    if(gRunningOnX)
        ChangeWindowAttributes(windowRef,kWindowLiveResizeAttribute,0);

    CreateScrollBarControl(windowRef, &scrollbarRect, 0, 0, 0, 0,
                           true, gActionFunctionScrollUPP, &(img->scrollbarVertRef));

    CreateScrollBarControl(windowRef, &scrollbarRect, 0, 0, 0, 0,
                           true, gActionFunctionScrollUPP, &(img->scrollbarHorizRef));
#else
    img->scrollbarVertRef = NewControl(windowRef,&scrollbarRect,"\p",false,0,0,0,scrollBarProc,0);
    img->scrollbarHorizRef = NewControl(windowRef,&scrollbarRect,"\p",false,0,0,0,scrollBarProc,0);
#endif

    window_adjust_scrollbars(windowRef);
}

static void window_invalidate(WindowRef windowRef)
{
    Rect portRect;

    GetWindowPortBounds(windowRef, &portRect);
    InvalWindowRect(windowRef, &portRect);
}

static void window_adjust_scrollbars(WindowRef windowRef)
{
    IMAGE *img;
    Rect   portRect;

    img = (IMAGE*)GetWRefCon(windowRef);
    GetWindowPortBounds(windowRef,&portRect);

    /* Move the crollbars to the edges of the window */
    HideControl(img->scrollbarVertRef);
    HideControl(img->scrollbarHorizRef);

    MoveControl(img->scrollbarVertRef,portRect.right - kScrollBarWidth,
                portRect.top - 1);
    MoveControl(img->scrollbarHorizRef,portRect.left - 1,
                portRect.bottom - kScrollBarWidth);

    SizeControl(img->scrollbarVertRef,kScrollBarWidth + 1,
                portRect.bottom - portRect.top - kScrollBarWidth + 1);
    SizeControl(img->scrollbarHorizRef, portRect.right - portRect.left - kScrollBarWidth + 1,
                kScrollBarWidth + 1);

    /* Adjust the scroll position showing */
    if (img->pixmapHdl)
    {
        PixMap *pixmap = *(img->pixmapHdl);
        int visibleHeight = portRect.bottom - portRect.top - kScrollBarWidth;
        int visibleWidth = portRect.right - portRect.left - kScrollBarWidth;

        if (pixmap->bounds.bottom > visibleHeight)
        {
            SetControl32BitMaximum(img->scrollbarVertRef,
                                   pixmap->bounds.bottom - visibleHeight);
            SetControlViewSize(img->scrollbarVertRef,visibleHeight);
        }
        else
            SetControlMaximum(img->scrollbarVertRef, 0);

        if (pixmap->bounds.right > visibleWidth)
        {
            SetControl32BitMaximum(img->scrollbarHorizRef,
                                   pixmap->bounds.right - visibleWidth);
            SetControlViewSize(img->scrollbarHorizRef, visibleWidth);
        }
        else
            SetControlMaximum(img->scrollbarHorizRef, 0);
    }

    ShowControl(img->scrollbarVertRef);
    ShowControl(img->scrollbarHorizRef);
}

/*********************************************************************/
void main(void)
{
    int code;
    int exit_code;
    int argc;
    char **argv;
    char dformat[64], ddevice[32];
    SInt32        response;

    /* Initialize operating environment */
#if TARGET_API_MAC_CARBON
    MoreMasterPointers(224);
#else
    MoreMasters();
#endif
    InitCursor();
    FlushEvents(everyEvent,0);

    if (AEInstallEventHandler(kCoreEventClass,kAEQuitApplication,
                              NewAEEventHandlerUPP((AEEventHandlerProcPtr) quitAppEventHandler),
                              0L,false) != noErr)
        ExitToShell();

	gActionFunctionScrollUPP = NewControlActionUPP(&actionFunctionScroll);

    Gestalt(gestaltMenuMgrAttr,&response);
    if(response & gestaltMenuMgrAquaLayoutMask)
                gRunningOnX = true;

    /* Initialize SIOUX */
    SIOUXSettings.initializeTB = false;
    SIOUXSettings.standalone = false;
    SIOUXSettings.asktosaveonclose = false;
    SIOUXSettings.sleep = GetCaretTime();
    SIOUXSettings.userwindowtitle = "\pGhostscript";

    /* Get arguments from user */
    argc = ccommand(&argv);

    /* Show command line window */
    if (InstallConsole(0))
        ExitToShell();

    /* Part of fudge to make SIOUX accept characters without becoming modal */
    SelectWindow(SIOUXTextWindow->window);
    PostEvent(keyDown, 0x4c00);  // Enter
    ReadCharsFromConsole(dformat, 0x7FFF);
    clrscr();

    /* Add in the display format as the first command line argument */
    if (argc >= MAX_ARGS - 1)
    {
       printf("Too many command line arguments\n");
       return;
    }

    memmove(&argv[3], &argv[1], (argc-1) * sizeof(char**));
    argc += 2;
    argv[1] = ddevice;
    argv[2] = dformat;

	sprintf(ddevice, "-sDEVICE=display");
    sprintf(dformat, "-dDisplayFormat=%d", display_format);

    /* Run Ghostscript */
    if (gsapi_new_instance(&instance, NULL) < 0)
    {
       printf("Can't create Ghostscript instance\n");
       return;
    }

#ifdef DEBUG
    visual_tracer_init();
    set_visual_tracer(&visual_tracer);
#endif

    gsapi_set_stdio(instance, gsdll_stdin, gsdll_stdout, gsdll_stderr);
    gsapi_set_poll(instance, gsdll_poll);
    gsapi_set_display_callback(instance, &display);

    code = gsapi_init_with_args(instance, argc, argv);
    if (code == 0)
       code = gsapi_run_string(instance, start_string, 0, &exit_code);
    else 
    {
       printf("Failed to initialize. Error %d.\n", code);
       fflush(stdout);
    }
    code = gsapi_exit(instance);
    if (code != 0) 
    {
       printf("Failed to terminate. Error %d.\n", code);
       fflush(stdout);
    }

    gsapi_delete_instance(instance);

#ifdef DEBUG
    visual_tracer_close();
#endif

    /* Ghostscript has finished - let user see output before quitting */
    WriteCharsToConsole("\r[Finished - hit any key to quit]", 33);
    fflush(stdout);

    /* Process events until a key is hit or user quits from menu */
    while(!gDone)
    {
        EventRecord eventStructure;

        if(WaitNextEvent(everyEvent,&eventStructure,SIOUXSettings.sleep,NULL))
        {
            if (eventStructure.what == keyDown)
            gDone = true;

            doEvents(&eventStructure);
        }
        else
            SIOUXHandleOneEvent(&eventStructure);
    }
}

/*********************************************************************/

void doEvents(EventRecord *eventStrucPtr)
{
    WindowRef      windowRef;
  
    if (eventStrucPtr->what == mouseDown &&
        FindWindow(eventStrucPtr->where,&windowRef) == inMenuBar)
        SelectWindow(SIOUXTextWindow->window);

    SIOUXSettings.standalone = true;
    if (SIOUXHandleOneEvent(eventStrucPtr))
    {
        if (SIOUXQuitting)
            gDone = true;
        SIOUXSettings.standalone = false;
        return;
    }
    SIOUXSettings.standalone = false;

    switch(eventStrucPtr->what)
    {
    case kHighLevelEvent:
        AEProcessAppleEvent(eventStrucPtr);
        break;

    case mouseDown:
        doMouseDown(eventStrucPtr);
        break;

    case keyDown:
    case autoKey:
        break;

    case updateEvt:
        doUpdate(eventStrucPtr);
        break;

    case activateEvt:
        DrawGrowIcon(windowRef);
        break;

    case osEvt:
        doOSEvent(eventStrucPtr);
        break;
    }
}

void doMouseDown(EventRecord *eventStrucPtr)
{
    WindowRef      windowRef;
    WindowPartCode partCode, zoomPart;
    BitMap         screenBits;
    Rect           constraintRect, mainScreenRect;
    Point          standardStateHeightAndWidth;
    long           newSize;

    partCode = FindWindow(eventStrucPtr->where,&windowRef);

    switch(partCode)
    {
    case inMenuBar:
        break;

    case inContent:
        if(windowRef != FrontWindow())
            SelectWindow(windowRef);
        else
            doInContent(eventStrucPtr,windowRef);
        break;

    case inDrag:
        DragWindow(windowRef,eventStrucPtr->where,NULL);
        break;

    case inGoAway:
        break;

    case inGrow:
        constraintRect.top   = 75;
        constraintRect.left = 250;
        constraintRect.bottom = constraintRect.right = 32767;
        newSize = GrowWindow(windowRef,eventStrucPtr->where,&constraintRect);
        if (newSize != 0)
            SizeWindow(windowRef,LoWord(newSize),HiWord(newSize),true);
        window_adjust_scrollbars(windowRef);
        window_invalidate(windowRef);
        break;

    case inZoomIn:
    case inZoomOut:
        mainScreenRect = GetQDGlobalsScreenBits(&screenBits)->bounds;
        standardStateHeightAndWidth.v = mainScreenRect.bottom;
        standardStateHeightAndWidth.h = mainScreenRect.right;

        if(IsWindowInStandardState(windowRef,&standardStateHeightAndWidth,NULL))
            zoomPart = inZoomIn;
        else
            zoomPart = inZoomOut;

        if(TrackBox(windowRef,eventStrucPtr->where,partCode))
        {
            ZoomWindowIdeal(windowRef,zoomPart,&standardStateHeightAndWidth);
            window_adjust_scrollbars(windowRef);
        }
        break;
    }
}

void doUpdate(EventRecord *eventStrucPtr)
{
    WindowRef windowRef;

    windowRef = (WindowRef) eventStrucPtr->message;
  
    window_adjust_scrollbars(windowRef);

    BeginUpdate(windowRef);

    SetPortWindowPort(windowRef);
    doUpdateWindow(eventStrucPtr);

    EndUpdate(windowRef);
}

void doUpdateWindow(EventRecord *eventStrucPtr)
{
    IMAGE *img;
    WindowRef    windowRef;
    Rect         srcRect, destRect, fillRect;
    PixMapHandle srcPixmapHdl, destPixmapHdl;
    RGBColor     grayColour = { 0xC000,0xC000,0xC000 };
    SInt32  hScroll, vScroll;
  
    windowRef = (WindowRef) eventStrucPtr->message;
    img = (IMAGE*)GetWRefCon(windowRef);
    srcPixmapHdl = img->pixmapHdl;
    destPixmapHdl = GetPortPixMap(GetWindowPort(windowRef));
    hScroll = GetControl32BitValue(img->scrollbarHorizRef);
    vScroll = GetControl32BitValue(img->scrollbarVertRef);

    if (srcPixmapHdl)
    {
        PixMap *pixmap = *srcPixmapHdl;
        PixPatHandle hdlPixPat = NewPixPat();
        MakeRGBPat(hdlPixPat, &grayColour);

        GetWindowPortBounds(windowRef,&destRect);
        destRect.right  -= kScrollBarWidth;
        destRect.bottom -= kScrollBarWidth;

        if (destRect.right > pixmap->bounds.right)
        {
            fillRect.top = destRect.top;
            fillRect.bottom = destRect.bottom;
            fillRect.left = pixmap->bounds.right;
            fillRect.right = destRect.right;
            FillCRect(&fillRect, hdlPixPat);
            destRect.right = pixmap->bounds.right;
        }
        if (destRect.bottom > pixmap->bounds.bottom)
        {
            fillRect.top = pixmap->bounds.bottom;
            fillRect.bottom = destRect.bottom;
            fillRect.left = destRect.left;
            fillRect.right = destRect.right;
            FillCRect(&fillRect, hdlPixPat);
            destRect.bottom = pixmap->bounds.bottom;
        }
        DisposePixPat(hdlPixPat);
    
        srcRect = destRect;
        srcRect.left += hScroll;
        srcRect.right += hScroll;
        srcRect.top += vScroll;
        srcRect.bottom += vScroll;
    
        CopyBits((BitMap*)*srcPixmapHdl, (BitMap*)*destPixmapHdl,
                 &srcRect, &destRect, srcCopy, NULL);
    }

    DrawGrowIcon(windowRef);
}

void doOSEvent(EventRecord *eventStrucPtr)
{
    switch((eventStrucPtr->message >> 24) & 0x000000FF)
    {
    case suspendResumeMessage:
        if((eventStrucPtr->message & resumeFlag) == 1)
          SetThemeCursor(kThemeArrowCursor);
        break;
    }
}

void doInContent(EventRecord *eventStrucPtr,WindowRef windowRef)
{
    ControlPartCode controlPartCode;
    ControlRef      controlRef;

    SetPortWindowPort(windowRef);
    GlobalToLocal(&eventStrucPtr->where);

    if(controlRef = FindControlUnderMouse(eventStrucPtr->where,windowRef,&controlPartCode))
    {
#if TARGET_API_MAC_CARBON
        TrackControl(controlRef,eventStrucPtr->where,(ControlActionUPP) -1);
#else
        if (controlPartCode == kControlIndicatorPart)
            TrackControl(controlRef,eventStrucPtr->where,NULL);
        else
            TrackControl(controlRef,eventStrucPtr->where,gActionFunctionScrollUPP);
#endif

        window_invalidate(windowRef);
    }
}

pascal void actionFunctionScroll(ControlRef controlRef,ControlPartCode controlPartCode)
{
    SInt32 scrollDistance, controlValue, oldControlValue, controlMax;

    if(controlPartCode != kControlNoPart)
    {
        if(controlPartCode != kControlIndicatorPart)
        {
            switch(controlPartCode)
            {
            case kControlUpButtonPart:
            case kControlDownButtonPart:
                scrollDistance = 10;
                break;

            case kControlPageUpPart:
            case kControlPageDownPart:
                scrollDistance = 100;
                break;

            default:
                scrollDistance = 0;
                break;
            }

            if (scrollDistance)
            {
                if((controlPartCode == kControlDownButtonPart) ||
                   (controlPartCode == kControlPageDownPart))
                    scrollDistance = -scrollDistance;

                controlValue = GetControl32BitValue(controlRef);

                if(((controlValue == GetControl32BitMaximum(controlRef)) && scrollDistance < 0) ||
                   ((controlValue == GetControl32BitMinimum(controlRef)) && scrollDistance > 0))
                    return;

                oldControlValue = controlValue;
                controlMax = GetControl32BitMaximum(controlRef);
                controlValue = oldControlValue - scrollDistance;
  
                if(controlValue < 0)
                    controlValue = 0;
                else if(controlValue > controlMax)
                    controlValue = controlMax;

                SetControl32BitValue(controlRef,controlValue);
            }
        }
    }
}

OSErr quitAppEventHandler(AppleEvent *appEvent,AppleEvent *reply,SInt32 handlerRefcon)
{
    OSErr    osError;
    DescType returnedType;
    Size     actualSize;

    osError = AEGetAttributePtr(appEvent,keyMissedKeywordAttr,typeWildCard,&returnedType,NULL,0,
                                &actualSize);

    if(osError == errAEDescNotFound)
    {
        gDone = true;
        osError = noErr;
    }
    else if(osError == noErr)
        osError = errAEParamMissed;

    return osError;
}

/*********************************************************************/

