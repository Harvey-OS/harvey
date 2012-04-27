// in this file, _Rect is os x Rect,
// _Point is os x Point
#undef Point
#define Point _Point
#undef Rect
#define Rect _Rect

#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h> // for full screen

#undef Rect
#undef Point

#undef nil


#include "u.h"
#include "lib.h"
#include "kern/dat.h"
#include "kern/fns.h"
#include "error.h"
#include "user.h"
#include <draw.h>
#include <memdraw.h>
#include "screen.h"
#include "keyboard.h"
#include "keycodes.h"

#define rWindowResource  128

#define topLeft(r)  (((Point *) &(r))[0])
#define botRight(r) (((Point *) &(r))[1])

extern int mousequeue;
static int depth;
Boolean   gDone;
RgnHandle gCursorRegionHdl;

Memimage	*gscreen;
Screeninfo	screen;

static int readybit;
static Rendez	rend;

///
// menu
//
static MenuRef windMenu;
static MenuRef viewMenu;

enum {
	kQuitCmd = 1,
	kFullScreenCmd = 2,
};

static WindowGroupRef winGroup = NULL;
static WindowRef theWindow = NULL;
static CGContextRef context;
static CGDataProviderRef dataProviderRef;
static CGImageRef fullScreenImage;
static CGRect devRect;
static CGRect bounds;
static PasteboardRef appleclip;
static _Rect winRect;

Boolean altPressed = false;
Boolean button2 = false;
Boolean button3 = false;


static int
isready(void*a)
{
	return readybit;
}

CGContextRef QuartzContext;

void winproc(void *a);

void screeninit(void)
{
	int fmt;
	int dx, dy;
	ProcessSerialNumber psn = { 0, kCurrentProcess };
	TransformProcessType(&psn, kProcessTransformToForegroundApplication);
	SetFrontProcess(&psn);

	memimageinit();
	depth = 32; // That's all this code deals with for now
	screen.depth = 32;
	fmt = XBGR32; //XRGB32;

	devRect = CGDisplayBounds(CGMainDisplayID());
//	devRect.origin.x = 0;
//	devRect.origin.y = 0;
//	devRect.size.width = 1024;
//	devRect.size.height = 768;
	dx = devRect.size.width;
	dy = devRect.size.height;

	gscreen = allocmemimage(Rect(0,0,dx,dy), fmt);
	dataProviderRef = CGDataProviderCreateWithData(0, gscreen->data->bdata,
						dx * dy * 4, 0);
	fullScreenImage = CGImageCreate(dx, dy, 8, 32, dx * 4,
				CGColorSpaceCreateDeviceRGB(),
				kCGImageAlphaNoneSkipLast,
				dataProviderRef, 0, 0, kCGRenderingIntentDefault);

	kproc("osxscreen", winproc, 0);
	ksleep(&rend, isready, 0);
}

// No wonder Apple sells so many wide displays!
static OSStatus ApplicationQuitEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData);
static OSStatus MainWindowEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData);
static OSStatus MainWindowCommandHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData);

void 
window_resized()
{
	GetWindowBounds(theWindow, kWindowContentRgn, &winRect );

	bounds = CGRectMake(0, 0, winRect.right-winRect.left, winRect.bottom - winRect.top);
}


void winproc(void *a)
{
	winRect.left = 30;
	winRect.top = 60;
	winRect.bottom = (devRect.size.height * 0.75) + winRect.top;
	winRect.right = (devRect.size.width * 0.75) + winRect.left;

	ClearMenuBar();
	InitCursor();

	CreateStandardWindowMenu(0, &windMenu);
	InsertMenu(windMenu, 0);

    MenuItemIndex index;
	CreateNewMenu(1004, 0, &viewMenu);
	SetMenuTitleWithCFString(viewMenu, CFSTR("View"));
	AppendMenuItemTextWithCFString(viewMenu, CFSTR("Full Screen"), 0,
			kFullScreenCmd, &index);
	SetMenuItemCommandKey(viewMenu, index, 0, 'F');
	InsertMenu(viewMenu, GetMenuID(windMenu));

	DrawMenuBar();
	uint32_t windowAttrs = 0
				| kWindowCloseBoxAttribute
				| kWindowCollapseBoxAttribute
				| kWindowResizableAttribute
				| kWindowStandardHandlerAttribute
				| kWindowFullZoomAttribute
		;

	CreateNewWindow(kDocumentWindowClass, windowAttrs, &winRect, &theWindow);
	CreateWindowGroup(0, &winGroup);
	SetWindowGroup(theWindow, winGroup);

	SetWindowTitleWithCFString(theWindow, CFSTR("Drawterm"));

	if(PasteboardCreate(kPasteboardClipboard, &appleclip) != noErr)
		sysfatal("pasteboard create failed");

	const EventTypeSpec quit_events[] = {
		{ kEventClassApplication, kEventAppQuit }
	};
	const EventTypeSpec commands[] = {
		{ kEventClassWindow, kEventWindowClosed },
		{ kEventClassWindow, kEventWindowBoundsChanged },
		{ kEventClassCommand, kEventCommandProcess }
	};
	const EventTypeSpec events[] = {
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassMouse, kEventMouseDown },
		{ kEventClassMouse, kEventMouseUp },
		{ kEventClassMouse, kEventMouseMoved },
		{ kEventClassMouse, kEventMouseDragged },
		{ kEventClassMouse, kEventMouseWheelMoved },
	};

	InstallApplicationEventHandler (
								NewEventHandlerUPP (ApplicationQuitEventHandler),
								GetEventTypeCount(quit_events),
								quit_events,
								NULL,
								NULL);

 	InstallApplicationEventHandler (
 								NewEventHandlerUPP (MainWindowEventHandler),
								GetEventTypeCount(events),
								events,
								NULL,
								NULL);
	InstallWindowEventHandler (
								theWindow,
								NewEventHandlerUPP (MainWindowCommandHandler),
								GetEventTypeCount(commands),
								commands,
								theWindow,
								NULL);

	ShowWindow(theWindow);
	ShowMenuBar();
	window_resized();
	SelectWindow(theWindow);
	terminit();
	// Run the event loop
	readybit = 1;
	wakeup(&rend);
	RunApplicationEventLoop();

}

static inline int convert_key(UInt32 key, UInt32 charcode)
{
	switch(key) {
		case QZ_IBOOK_ENTER:
		case QZ_RETURN: return '\n';
		case QZ_ESCAPE: return 27;
		case QZ_BACKSPACE: return '\b';
		case QZ_LALT: return Kalt;
		case QZ_LCTRL: return Kctl;
		case QZ_LSHIFT: return Kshift;
		case QZ_F1: return KF+1;
		case QZ_F2: return KF+2;
		case QZ_F3: return KF+3;
		case QZ_F4: return KF+4;
		case QZ_F5: return KF+5;
		case QZ_F6: return KF+6;
		case QZ_F7: return KF+7;
		case QZ_F8: return KF+8;
		case QZ_F9: return KF+9;
		case QZ_F10: return KF+10;
		case QZ_F11: return KF+11;
		case QZ_F12: return KF+12;
		case QZ_INSERT: return Kins;
		case QZ_DELETE: return 0x7F;
		case QZ_HOME: return Khome;
		case QZ_END: return Kend;
		case QZ_KP_PLUS: return '+';
		case QZ_KP_MINUS: return '-';
		case QZ_TAB: return '\t';
		case QZ_PAGEUP: return Kpgup;
		case QZ_PAGEDOWN: return Kpgdown;
		case QZ_UP: return Kup;
		case QZ_DOWN: return Kdown;
		case QZ_LEFT: return Kleft;
		case QZ_RIGHT: return Kright;
		case QZ_KP_MULTIPLY: return '*';
		case QZ_KP_DIVIDE: return '/';
		case QZ_KP_ENTER: return '\n';
		case QZ_KP_PERIOD: return '.';
		case QZ_KP0: return '0';
		case QZ_KP1: return '1';
		case QZ_KP2: return '2';
		case QZ_KP3: return '3';
		case QZ_KP4: return '4';
		case QZ_KP5: return '5';
		case QZ_KP6: return '6';
		case QZ_KP7: return '7';
		case QZ_KP8: return '8';
		case QZ_KP9: return '9';
		default: return charcode;
	}
}

void
sendbuttons(int b, int x, int y)
{
	int i;
	lock(&mouse.lk);
	i = mouse.wi;
	if(mousequeue) {
		if(i == mouse.ri || mouse.lastb != b || mouse.trans) {
			mouse.wi = (i+1)%Mousequeue;
			if(mouse.wi == mouse.ri)
				mouse.ri = (mouse.ri+1)%Mousequeue;
			mouse.trans = mouse.lastb != b;
		} else {
			i = (i-1+Mousequeue)%Mousequeue;
		}
	} else {
		mouse.wi = (i+1)%Mousequeue;
		mouse.ri = i;
	}
	mouse.queue[i].xy.x = x;
	mouse.queue[i].xy.y = y;
	mouse.queue[i].buttons = b;
	mouse.queue[i].msec = ticks();
	mouse.lastb = b;
	unlock(&mouse.lk);
	wakeup(&mouse.r);
}

static Ptr fullScreenRestore;
static int amFullScreen = 0;
static WindowRef oldWindow = NULL;

static void
leave_full_screen()
{
	if (amFullScreen) {
		EndFullScreen(fullScreenRestore, 0);
		theWindow = oldWindow;
		ShowWindow(theWindow);
		amFullScreen = 0;
		window_resized();
		Rectangle rect =  { { 0, 0 },
 							{ bounds.size.width,
 							  bounds.size.height} };
		drawqlock();
 		flushmemscreen(rect);
 		drawqunlock();
	}
}

static void
full_screen()
{
	if (!amFullScreen) {
		oldWindow = theWindow;
		HideWindow(theWindow);
		BeginFullScreen(&fullScreenRestore, 0, 0, 0, &theWindow, 0, 0);
		amFullScreen = 1;
		window_resized();
		Rectangle rect =  { { 0, 0 },
 							{ bounds.size.width,
 							  bounds.size.height} };
		drawqlock();
 		flushmemscreen(rect);
 		drawqunlock();
	}
}

// catch quit events to handle quits from menu, Cmd+Q, applescript, and task switcher
static OSStatus ApplicationQuitEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
	exit(0);
//	QuitApplicationEventLoop();
	return noErr;
}
 
static OSStatus MainWindowEventHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
	OSStatus result = noErr;
	result = CallNextEventHandler(nextHandler, event);
	UInt32 class = GetEventClass (event);
	UInt32 kind = GetEventKind (event);
	static uint32_t mousebuttons = 0; // bitmask of buttons currently down
	static uint32_t mouseX = 0; 
	static uint32_t mouseY = 0; 

	if(class == kEventClassKeyboard) {
		char macCharCodes;
		UInt32 macKeyCode;
		UInt32 macKeyModifiers;

		GetEventParameter(event, kEventParamKeyMacCharCodes, typeChar,
							NULL, sizeof(macCharCodes), NULL, &macCharCodes);
		GetEventParameter(event, kEventParamKeyCode, typeUInt32, NULL,
							sizeof(macKeyCode), NULL, &macKeyCode);
		GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, NULL,
							sizeof(macKeyModifiers), NULL, &macKeyModifiers);
        switch(kind) {
		case kEventRawKeyModifiersChanged:
			if (macKeyModifiers == (controlKey | optionKey)) leave_full_screen();

			switch(macKeyModifiers & (optionKey | cmdKey)) {
			case (optionKey | cmdKey):
				/* due to chording we need to handle the case when both
				 * modifier keys are pressed at the same time. 
				 * currently it's only 2-3 snarf and the 3-2 noop
				 */
				altPressed = true;
				if(mousebuttons & 1 || mousebuttons & 2 || mousebuttons & 4) {
					mousebuttons |= 2;	/* set button 2 */
					mousebuttons |= 4;	/* set button 3 */
					button2 = true;
					button3 = true;
					sendbuttons(mousebuttons, mouseX, mouseY);
				} 
				break;
			case optionKey:
				altPressed = true;
				if(mousebuttons & 1 || mousebuttons & 4) {
					mousebuttons |= 2;	/* set button 2 */
					button2 = true;
					sendbuttons(mousebuttons, mouseX, mouseY);
				} 
				break;
			case cmdKey:
				if(mousebuttons & 1 || mousebuttons & 2) {
					mousebuttons |= 4;	/* set button 3 */
					button3 = true;
					sendbuttons(mousebuttons, mouseX, mouseY);
				}
				break;
			case 0:
			default:
				if(button2 || button3) {
					if(button2) {
						mousebuttons &= ~2;	/* clear button 2 */
						button2 = false;
						altPressed = false;
					} 
					if(button3) {
						mousebuttons &= ~4;	/* clear button 3 */
						button3 = false;
					}
					sendbuttons(mousebuttons, mouseX, mouseY);
				}
				if(altPressed) {
					kbdputc(kbdq, Kalt);
					altPressed = false;
				} 
				break;
			}
			break;
		case kEventRawKeyDown:
		case kEventRawKeyRepeat:
			if(macKeyModifiers != cmdKey) {
				int key = convert_key(macKeyCode, macCharCodes);
				if (key != -1) kbdputc(kbdq, key);
			} else
				result = eventNotHandledErr;
			break;
		default:
			break;
		}
	}
	else if(class == kEventClassMouse) {
		_Point mousePos;

		GetEventParameter(event, kEventParamMouseLocation, typeQDPoint,
							0, sizeof mousePos, 0, &mousePos);
		
		switch (kind) {
			case kEventMouseWheelMoved:
			{
			    int32_t wheeldelta;
				GetEventParameter(event,kEventParamMouseWheelDelta,typeSInt32,
									0,sizeof(wheeldelta), 0, &wheeldelta);
				mouseX = mousePos.h - winRect.left;
				mouseY = mousePos.v - winRect.top;
				sendbuttons(wheeldelta>0 ? 8 : 16, mouseX, mouseY);
				break;
			}
			case kEventMouseUp:
			case kEventMouseDown:
			{
				uint32_t buttons;
				uint32_t modifiers;
				GetEventParameter(event, kEventParamKeyModifiers, typeUInt32, 
									0, sizeof(modifiers), 0, &modifiers);
				GetEventParameter(event, kEventParamMouseChord, typeUInt32, 
									0, sizeof buttons, 0, &buttons);
				/* simulate other buttons via alt/apple key. like x11 */
				if(modifiers & optionKey) {
					mousebuttons = ((buttons & 1) ? 2 : 0);
					altPressed = false;
				} else if(modifiers & cmdKey)
					mousebuttons = ((buttons & 1) ? 4 : 0);
				else
					mousebuttons = (buttons & 1);

				mousebuttons |= ((buttons & 2)<<1);
				mousebuttons |= ((buttons & 4)>>1);

			} /* Fallthrough */
			case kEventMouseMoved:
			case kEventMouseDragged:
				mouseX = mousePos.h - winRect.left;
				mouseY = mousePos.v - winRect.top;
				sendbuttons(mousebuttons, mouseX, mouseY);
				break;
			default:
				result = eventNotHandledErr;
				break;
		}
	}
	return result;
}


//default window command handler (from menus)
static OSStatus MainWindowCommandHandler(EventHandlerCallRef nextHandler,
					EventRef event, void *userData)
{
	OSStatus result = noErr;
	UInt32 class = GetEventClass (event);
	UInt32 kind = GetEventKind (event);

	result = CallNextEventHandler(nextHandler, event);

	if(class == kEventClassCommand)
	{
		HICommand theHICommand;
		GetEventParameter( event, kEventParamDirectObject, typeHICommand,
							NULL, sizeof( HICommand ), NULL, &theHICommand );

		switch ( theHICommand.commandID )
		{
			case kHICommandQuit:
				exit(0);
				break;

			case kFullScreenCmd:
				full_screen();
				break;

			default:
				result = eventNotHandledErr;
				break;
		}
	}
	else if(class == kEventClassWindow)
	{
		WindowRef     window;
		_Rect          rectPort = {0,0,0,0};

		GetEventParameter(event, kEventParamDirectObject, typeWindowRef,
							NULL, sizeof(WindowRef), NULL, &window);

		if(window)
		{
			GetPortBounds(GetWindowPort(window), &rectPort);
		}

		switch (kind)
		{
			case kEventWindowClosed:
				// send a quit carbon event instead of directly calling cleanexit 
				// so that all quits are done in ApplicationQuitEventHandler
				{
				EventRef quitEvent;
				CreateEvent(NULL,
							kEventClassApplication,
							kEventAppQuit,
							0,
							kEventAttributeNone,
							&quitEvent);
				EventTargetRef target;
				target = GetApplicationEventTarget();
				SendEventToEventTarget(quitEvent, target);
				}
 				break;

			//resize window
			case kEventWindowBoundsChanged:
				window_resized();
				Rectangle rect =  { { 0, 0 },
 									{ bounds.size.width,
 									  bounds.size.height} };
				drawqlock();
 				flushmemscreen(rect);
 				drawqunlock();
				break;

			default:
				result = eventNotHandledErr;
				break;
		}
	}

	return result;
}

void
flushmemscreen(Rectangle r)
{
	// sanity check.  Trips from the initial "terminal"
    if (r.max.x < r.min.x || r.max.y < r.min.y) return;
    
	screenload(r, gscreen->depth, byteaddr(gscreen, ZP), ZP,
		gscreen->width*sizeof(ulong));
}

uchar*
attachscreen(Rectangle *r, ulong *chan, int *depth, int *width, int *softscreen, void **X)
{
	*r = gscreen->r;
	*chan = gscreen->chan;
	*depth = gscreen->depth;
	*width = gscreen->width;
	*softscreen = 1;

	return gscreen->data->bdata;
}

// PAL - no palette handling.  Don't intend to either.
void
getcolor(ulong i, ulong *r, ulong *g, ulong *b)
{

// PAL: Certainly wrong to return a grayscale.
	 *r = i;
	 *g = i;
	 *b = i;
}

void
setcolor(ulong index, ulong red, ulong green, ulong blue)
{
	assert(0);
}


static char snarf[3*SnarfSize+1];
static Rune rsnarf[SnarfSize+1];

char*
clipread(void)
{
	CFDataRef cfdata;
	OSStatus err = noErr;
	ItemCount nItems;

	// Wow.  This is ridiculously complicated.
	PasteboardSynchronize(appleclip);
	if((err = PasteboardGetItemCount(appleclip, &nItems)) != noErr) {
		fprint(2, "apple pasteboard GetItemCount failed - Error %d\n", err);
		return 0;
	}

	uint32_t i;
	// Yes, based at 1.  Silly API.
	for(i = 1; i <= nItems; ++i) {
		PasteboardItemID itemID;
		CFArrayRef flavorTypeArray;
		CFIndex flavorCount;

		if((err = PasteboardGetItemIdentifier(appleclip, i, &itemID)) != noErr){
			fprint(2, "Can't get pasteboard item identifier: %d\n", err);
			return 0;
		}

		if((err = PasteboardCopyItemFlavors(appleclip, itemID, &flavorTypeArray))!=noErr){
			fprint(2, "Can't copy pasteboard item flavors: %d\n", err);
			return 0;
		}

		flavorCount = CFArrayGetCount(flavorTypeArray);
		CFIndex flavorIndex;
		for(flavorIndex = 0; flavorIndex < flavorCount; ++flavorIndex){
			CFStringRef flavorType;
			flavorType = (CFStringRef)CFArrayGetValueAtIndex(flavorTypeArray, flavorIndex);
			if (UTTypeConformsTo(flavorType, CFSTR("public.utf16-plain-text"))){
				if((err = PasteboardCopyItemFlavorData(appleclip, itemID,
					CFSTR("public.utf16-plain-text"), &cfdata)) != noErr){
					fprint(2, "apple pasteboard CopyItem failed - Error %d\n", err);
					return 0;
				}
				CFIndex length = CFDataGetLength(cfdata);
				if (length > sizeof rsnarf) length = sizeof rsnarf;
				CFDataGetBytes(cfdata, CFRangeMake(0, length), (uint8_t *)rsnarf);
				snprint(snarf, sizeof snarf, "%.*S", length/sizeof(Rune), rsnarf);
				char *s = snarf;
				while (*s) {
					if (*s == '\r') *s = '\n';
					s++;
				}
				CFRelease(cfdata);
				return strdup(snarf);
			}
		}
	}
	return 0;
}

int
clipwrite(char *snarf)
{
	CFDataRef cfdata;
	PasteboardSyncFlags flags;

	runesnprint(rsnarf, nelem(rsnarf), "%s", snarf);
	if(PasteboardClear(appleclip) != noErr){
		fprint(2, "apple pasteboard clear failed\n");
		return 0;
	}
	flags = PasteboardSynchronize(appleclip);
	if((flags&kPasteboardModified) || !(flags&kPasteboardClientIsOwner)){
		fprint(2, "apple pasteboard cannot assert ownership\n");
		return 0;
	}
	cfdata = CFDataCreate(kCFAllocatorDefault, 
		(uchar*)rsnarf, runestrlen(rsnarf)*2);
	if(cfdata == nil){
		fprint(2, "apple pasteboard cfdatacreate failed\n");
		return 0;
	}
	if(PasteboardPutItemFlavor(appleclip, (PasteboardItemID)1,
		CFSTR("public.utf16-plain-text"), cfdata, 0) != noErr){
		fprint(2, "apple pasteboard putitem failed\n");
		CFRelease(cfdata);
		return 0;
	}
	CFRelease(cfdata);
	return 1;
}


void
mouseset(Point xy)
{
	CGPoint pnt;
	pnt.x = xy.x + winRect.left;
	pnt.y = xy.y + winRect.top;
	CGWarpMouseCursorPosition(pnt);
}

void
screenload(Rectangle r, int depth, uchar *p, Point pt, int step)
{
	CGRect rbounds;
	rbounds.size.width = r.max.x - r.min.x;
	rbounds.size.height = r.max.y - r.min.y;
	rbounds.origin.x = r.min.x;
	rbounds.origin.y = r.min.y;
		
	if(depth != gscreen->depth)
		panic("screenload: bad ldepth");
		
	QDBeginCGContext( GetWindowPort(theWindow), &context);
	
	// The sub-image is relative to our whole screen image.
	CGImageRef subimg = CGImageCreateWithImageInRect(fullScreenImage, rbounds);
	
	// Drawing the sub-image is relative to the window.
	rbounds.origin.y = winRect.bottom - winRect.top - r.min.y - rbounds.size.height;
	CGContextDrawImage(context, rbounds, subimg);
	CGContextFlush(context);
	CGImageRelease(subimg);
	QDEndCGContext( GetWindowPort(theWindow), &context);

}

// PAL: these don't work.
// SetCursor and InitCursor are marked as deprecated in 10.4, and I can't for the
// life of me find out what has replaced them.
void
setcursor(void)
{
    Cursor crsr;
    int i;
    
	for(i=0; i<16; i++){
		crsr.data[i] = ((ushort*)cursor.set)[i];
		crsr.mask[i] = crsr.data[i] | ((ushort*)cursor.clr)[i];
	}
	crsr.hotSpot.h = -cursor.offset.x;
	crsr.hotSpot.v = -cursor.offset.y;
	SetCursor(&crsr);
}

void
cursorarrow(void)
{
	InitCursor();
}
