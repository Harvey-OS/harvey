.de TP	\" An indented paragraph describing a function, tagged with the function name
.IP "\\f(CW\\$1\\fR" 5
.if \\w'\\f(CW\\$1\\fR'-4n .br
..
.de CI
.nr Sf \\n(.f
\%\&\\$3\f(CW\\$1\fI\&\\$2\f\\n(Sf
..
.TL
A Quick Introduction to the Panel Library
.AU
Tom Duff
td@plan9.att.com
.NH
Introduction
.PP
The panel library hides much of the dirty work involved in making simple user
interfaces for interactive applications on Plan 9.
The design is modeled strongly on Ousterhout's Tcl/Tk, except that the programming language is C
and most of Tcl/Tk's automatic behind-the-scenes recalculation and redrawing
is missing.
.PP
The library provides facilities to create user-interface panel elements
such as push-buttons, text entry windows, sliders, etc.,
to re-initialize existing elements,
to calculate layout automatically,
to display panels in a bitmap,
and to process mouse and keyboard events.
.PP
The library defines a
.CW Panel
data type that describes the structural, geometric, and operational
behavior of user-interface elements.
When displayed in a bitmap, a panel
corresponds to a rectangular area, usually with some
identifying graphics like a frame or a label drawn on it.
.CW Panel s
are organized into trees, with each panel's
children drawn on top of it and inside its rectangle.  None of the
children's rectangles overlap, so
the whole structure is like a nest of boxes.
.NH
A Simple Example
.PP
Here is a simple example that displays a panel containing a label and a
.I done
button and processes mouse events until the user hits
.CW done .
.P1
.so hello1.c
.P2
.P1
.so hello2.c
.P2
.P1
.so hello3.c
.P2
The
.CW main
routine first calls initialization functions for the
graphics library, the event library, and the panel library.
The panel library needs to know the screen's
.CW ldepth ,
because it draws things differently on one-bit and multi-bit displays.
(The examples here show the multi-bit version.)
.PP
The next three lines create a tree of panels.
The root panel draws a frame around its children.
Calling
.CW plgroup
here instead of
.CW plframe
would omit the frame.
The children are a
.CW Hello,
.CW world!
label and a button that calls the
.CW done
routine when hit.
The difference between
.CW pllabel
and
.CW plbutton
is that labels are not mouse-sensitive.  The display reflects this
by drawing the button in relief and changing its appearance when a mouse button
is depressed over it.
.PP
Next, a call to
.CW ereshaped
initializes the screen, and a simple loop
sends mouse events to the root panel
for processing.
.PP
The call to
.CW plpack
in
.CW ereshaped
calculates the geometry of the tree of
panels, and
.CW pldraw
displays the tree.
Here's what the display looks like:
.BP hello.ps 0.5 5.0 c
.NH
Display Layout
.PP
When
.CW plpack
lays out a panel tree, it calculates a rectangle for each
panel node, inside which the node's graphics and children
are drawn.  The layout of a node's children is controlled
by flags given when the node is created.
The first two arguments of every panel-creating
function are a pointer to the parent panel
and the flags used by
.CW plpack .
.PP
.CW Plpack
is declared thus:
.P1
int plpack(Panel *root, Rectangle space);
.P2
It packs
.CW root
into
.CW space ,
and recursively packs each node's
children into whatever space it
has left over after drawing their parent's
own graphics.
.CW Plpack
stores each panel's placement in its field
.CW r ,
a
.CW Rectangle .
.PP
.CW Plpack
starts off with an empty rectangular frame,
and fills it with children in eldest-to-youngest order.
Placing each child uses up part of the frame and leaves
a subrectangle of available space.
For each child, it slices enough space to hold it
from one of the four sides of the available space.
Flag values
.CW PACKN ,
.CW PACKE ,
.CW PACKS ,
and
.CW PACKW
determine which side the space is taken from.
This figure illustrates the process:
.PS 5
copy "pack.pic"
.PE
Figure (a) shows the available space in a frame before packing
a child whose flag is
.CW PACKE .
Figure (b) shows the child in place and indicates the space available
for packing the next child.
.PP
For example, 4 buttons packed below one another, like this,
.P1
.so pack1.eg
.P2
produce this:
.BP pack1.ps 0.8 5.0 c
.PP
Since the buttons are different sizes, the
result looks ragged.  The frame is wide
enough to accommodate the widest button,
and the parts sliced off for the four buttons
are all the same size, but the buttons don't use
all the space that
.CW plpack
makes available to them.  The
.CW FILLX
flag expands a node in the
.I x
direction to fill any extra space sliced off for it.
Here's the previous example, cleaned up:
.P1
.so pack2.eg
.P2
and here's the result:
.BP pack2.ps 0.8 5.0 c
Similarly,
.CW FILLY
expands in the
.I y
direction.
.PP
There may be some freedom in the placement of panels that do not use
.CW FILLX
and
.CW FILLY .
There are flags to place a panel at any of eight compass points of the space reserved
for it, or at the center, called
.CW PLACEN ,
.CW PLACENE ,
.CW PLACEE ,
.CW PLACESE ,
.CW PLACES ,
.CW PLACESW ,
.CW PLACEW ,
.CW PLACENW ,
and
.CW PLACECEN .
Here's an example:
.P1
.so pack3.eg
.P2
and the resulting display:
.BP pack3.ps 0.8 5.0 c
Of course, flagging the last button
.CW PLACES
or
.CW PLACECEN
produces the same result,
since it has no surplus space in the north-south direction.
.PP
Normally,
.CW plpack
fits everything into the smallest possible space.
There are four ways to make things larger.
First, the
.CW FIXED
flag tells
.CW plpack
that it should look in the panel's
.CW fixedsize
field (a
.CW Point )
to find the panel's
dimensions.  This is useful, for example,
to make space for an image of known size.
.CW FIXED "" (
is actually the OR of two flags,
.CW FIXEDX
and
.CW FIXEDY ,
that individually fix the width and height of the panel.)
.PP
Second, a margin around a panel can be specified
by setting its
.CW pad
field (again, a
.CW Point )
to a non-zero value.
Similarly, a panel can
be given extra space by setting its
.CW ipad
field.  The difference between the two is that
.CW pad
asks for extra space outside the panel, where
.CW ipad
asks for extra space inside.  Both
.CW pad
and
.CW ipad
are added to the space requested for the
.CW Panel
from its parent,
but only
.CW ipad
is added to the space actually given to the
.CW Panel \(emso
they both represent increments on the total size of the requested
rectangle, not margins around all four borders.
Here's an example:
.P1
.so pack4.eg
.P2
and its display:
.BP pack4.ps 0.6 5.0 c
.PP
Third, setting the
.CW EXPAND
flag causes
.CW plpack
to give the panel
whatever unused space the parent may have.
If multiple children of the same parent all set
.CW EXPAND ,
the unused space is distributed equitably among them.
Here is a text editor panel, with a scroll bar to the
left and a label on top.
.CW Pledit ,
described later, creates a window of editable text that
uses up all the space left over after placing the label
and scroll bar.
.P1
.so pack5.eg
.P2
Here's what it looks like:
.BP pack5.ps 2.0 5.0 c
Note that the
.CW EXPAND
flag must be supplied to both the root panel
and the text window.  The root expands to
fill the space passed in as
.CW plpack 's
second argument.  Without it,
the text window would expand to fill the
root, but the root would remain as small
as possible.
.PP
Fourth, the
.CW MAXX
and
.CW MAXY
flags set the panel's width or height equal to the largest of
its siblings.
.CW MAXX
evens up the widths of a horizontal array of buttons, like this:
.P1
.so pack6.eg
.P2
.PP
This is the resulting display:
.BP pack6.ps 0.2 5.0 c
Specifying 0 for the flags is equivalent to using
.CW PACKN|PLACECEN ,
which seems like a reasonable default.
.NH
The Public Parts of a
.CW Panel
.PP
The
.CW Panel
structure has a lot of fields that are just for the use of the library.
Here are the fields that applications can use with impunity.
.P1
typedef struct Panel Panel;
struct Panel{
	Point ipad, pad; /* internal and external padding */
	Point fixedsize; /* size of FIXED panels */
	int user;        /* available for application use */
	void *userp;     /* available for application use */
	Rectangle r;     /* filled in by pack */
	...              /* more private fields follow */
};
.P2
.CW Ipad ,
.CW pad ,
and
.CW fixedsize
have already been discussed.
The
.CW user
and
.CW userp
fields are not used by the library.  Applications can use them for
anything at all.  It is often handy to have a common hit function
for a group of panels and distinguish amongst them by an index number
or private data stored in
.CW user
or
.CW userp .
.PP
.CW Plpack
stores a panel's coordinates in
.CW "Rectangle r" .
.PP
.CW Plpack
should be called whenever changes occur that can be expected
to affect the panel's layout.  In particular, it should normally be called when a
panel tree is initially created and in
.CW ereshaped .
Usually
.CW plpack
should be called on the root of a panel tree.  It is possible to call
.CW plpack
on a subtree of a panel tree that has already been packed, for example
when the subtree has been updated and the program wishes for those changes
not to affect the rest of the display.  In that case,
.CW plpack 's
.CW Rectangle
argument should be the current size of the subtree to be repacked.
.NH
Panel-wrangling Functions
.PP
.P1
int plinit(int ldepth);
.P2
.CW Plinit
initializes the panel library's private data.  It must be called before
any other panel library routine, but after calling
.CW binit
to initialize the graphics library.
.CW Plinit 's
argument is normally
.CW screen.ldepth .
The panel library draws its display in four shades
of grey when they are available, but will use only
black and white if told otherwise.
.CW Plinit
returns 1 if it succeeds and 0 if it fails.
.P1
void pldraw(Panel *p, Bitmap *b);
.P2
.CW Pldraw
displays the panel tree pointed to by
.CW p
on the bitmap
.CW b .
The latter will usually be
.CW &screen
but it could be something from the layer library (see
.I layer (2)).
.PP
Because a panel's
children are drawn nested inside it, programs can update
parts of the display by calling
.CW pldraw
on subtrees of a panel
tree.  For example, after calling
.CW plinitlabel
(described below) to change the text of a label, it can be redisplayed by calling
.CW pldraw
on the label, leaving the rest of the display alone.
.P1
void plmove(Panel *p, Point min);
.P2
.CW Plmove
relocates the panel tree pointed to by
.CW p ,
which must already have been processed by
.CW plpack ,
so that its upper left-hand corner is at
.CW min .
.P1
void plfree(Panel *p);
.P2
.CW Plfree
reclaims the storage used by the panel pointed to by
.CW p ,
and all its descendants.
.NH
Event Handling
.PP
.P1
void plmouse(Panel *recipient, Mouse m);
void plgrabkb(Panel *p);
void plkeyboard(Rune c);
.P2
The panel library handles mouse and keyboard input in a straightforward way.
The application program acquires event information, usually
by calling graphics library functions
.CW emouse ,
.CW ekbd ,
or
.CW event ,
and passes it to the panel library's event-processing routines
.CW plmouse
and
.CW plkeyboard .
The arguments to
.CW plmouse
are a pointer to a
.CW Panel
tree to which the event will be sent and
the data received from the mouse.
.CW Plmouse
walks the panel tree looking for a mouse-sensitive panel whose rectangle
contains the mouse event.
Code specific to the panel type is called to process
the event.  For example, the
.CW plbutton -specific
code highlights itself when a mouse button is pressed over it, calling
the user-specified hit function if the mouse button is released before the
mouse moves out of the button's rectangle.
.PP
Keyboard events are sent to a designated panel.  The keyboard
focus is changed by calling
.CW plgrabkb .
Keyboard events are sent to the appropriate panel by calling
.CW plkeyboard ,
whose argument is the
.CW Rune
received from the keyboard.
.PP
Here's a generic skeleton of a simple panel library application that uses both mouse
and keyboard input:
.P1
.so skel1.c
.P2
.P1
.so skel2.c
.P2
.P1
.so skel3.c
.P2
.P1
.so skel4.c
.P2
.PP
The missing
.CW mkpanels
function is the user's code to create the application's panel tree.
The
.CW main
function does all the necessary library initialization, calls
.CW mkpanel
and
.CW ereshaped
to create and display the panel tree, and
calls
.CW eventloop ,
which gets keyboard and mouse events and passes them to
.CW plkeyboard
and
.CW plmouse .
Packing and displaying the panel tree is handled by
.CW ereshaped .
.NH
Panel Types
.PP
The examples so far have used a half-dozen panel types:
.CW plframe ,
.CW plgroup ,
.CW plbutton ,
.CW pllabel ,
.CW plscrollbar ,
and
.CW pledit ,
but those don't nearly exhaust the possibilities.
A reminder: in the following descriptions, the first two arguments of every
panel-creation function are a pointer to the new
panel's parent (0 if there is none) and the flags that guide
.CW plpack .
The new panel's parent must have been created by calling one of
.CW plgroup ,
.CW plframe ,
or
.CW plpopup ;
otherwise the creation function will fail and return zero.
.NH
Buttons
.PP
.P1
Panel *plbutton(Panel *parent, int flags,
	Icon *label, void (*hit)(Panel *, int));
.P2
.P1
Panel *plcheckbutton(Panel *parent, int flags,
	Icon *label, void (*hit)(Panel *, int, int));
.P2
.P1
Panel *plradiobutton(Panel *parent, int flags,
	Icon *label, void (*hit)(Panel *, int, int));
.P2
.P1
void plsetbutton(Panel *panel, int value);
.P2
.P1
Panel *plmenu(Panel *parent, int flags, Icon **items,
	int itemflags, void (*hit)(int, int));
.P2
.CW Plbutton
creates a push-button that displays the given label when drawn.
The function
.CW hit
is called when the button is pressed
(actually when it is released over it).
Its arguments are a pointer to the panel and the mouse button(s) that caused the hit.
.PP
Here,
.CW Icon*
is a synonym for
.CW void* .
The actual argument may be a
.CW char*
or a
.CW Bitmap* ,
in which case the
.CW BITMAP
bit must be set in the
.CW flags
word.
.PP
.CW Plcheckbutton
and
.CW plradiobutton
create push-buttons that, when drawn,
contain a check-box that may be marked or not.  Pushing the button
flips the check mark on or off.  In addition, when a radio button turns on,
it turns off all its radio button siblings, so at most one radio button
in a group can be on at any instant.
(They're called radio buttons because they imitate the
buttons on a car radio.)
The function
.CW plsetbutton
allows the state of the check-box to be initialized.  By default it is 0 (off).
.PP
The state of the
check mark is available to the hit routine in the check field of the
button.  The hit function's first argument points to
the button.  The second argument is the mouse buttons.
The third argument reflects the state of the check.
When a radio button turns off one of its siblings, the sibling's
hit function is not called.
.PP
The
.CW plmenu
function creates a group (see below) containing an array of buttons.
The
.CW items
argument is an array of
.CW Icon
pointers, ending with a zero pointer,
that gives the label of each button.
The
.CW itemflags
button indicates the flags to be used when creating each button.
Whenever a menu button is hit, the
.CW hit
function is called with the mouse buttons as first argument and the panel's index in the
.CW items
array as second argument.
Here's code that makes a radio button list, a check list, and a menu:
.P1
.so button.eg
.P2
and here's the result:
.BP button.ps 0.6 5.0 c
.NH
Labels and Message boxes
.PP
.P1
Panel *pllabel(Panel *parent, int flags, Icon *label);
.P2
.P1
Panel *plmessage(Panel *parent, int flags, int width,
	char *text);
.P2
A label, when drawn, displays its text at the appropriate place on the
screen.  Labels are intended for short pieces of text such as titles.
Message panels display longer pieces of text, such as error messages
or dialog box verbiage,
on multiple lines with wrapping at word boundaries.  The
.CW width
argument gives the desired width of the Panel, in pixels.  (It may
come out larger if the text contains words longer than the nominal width\(emmessage
panels don't hyphenate.)
Neither responds to mouse activity.  Here's an example:
.P1
.so message.eg
.P2
and the result:
.BP message.ps 1.4 5.0 c
.NH
Grouping
.P1
Panel *plgroup(Panel *parent, int flags)
.P2
.P1
Panel *plframe(Panel *parent, int flags)
.P2
Groups are useful to help arrange Panels, both logically and on the screen.
For example, a
Panel with two independent sets of radio buttons needs to place them
in groups so that each won't reset the other.  A frame behaves just
like a group, except that it also draws a rectangular frame around its
children.  Neither responds to mouse activity, although their children
may.
.PP
Most of the examples above use
.CW plgroup
or
.CW plframe ,
so we won't include another here.
.NH
Canvas
.P1
Panel *plcanvas(Panel *parent, int flags,
	void (*draw)(Panel *), void (*hit)(Panel *, Mouse *));
.P2
A canvas is just an empty piece of screen that passes mouse events
through to a user-supplied
.CW hit
function.
The argument
.CW draw ,
if non-null, points to a function that is called back by
.CW pldraw
to draw the canvas's display.
The
.CW hit
function, if non-zero, passes mouse events back to be handled by user code.
Mouse coordinates are guaranteed to be inside the canvas's rectangle,
except that whenever the mouse leaves the rectangle a single event is
passed back that has the
.CW OUT
bit set in its
.CW buttons
field.
Note that the canvas will be created with zero size, unless the
.CW EXPAND
or
.CW FIXED
flag is specified or
.CW ipad
is non-zero.
.NH
Sliders
.P1
Panel *plslider(Panel *parent, int flags, Point size,
	void (*hit)(Panel *p, int but, int val, int len))
void plsetslider(Panel *p, int val, int range)
.P2
A slider displays a thermometer-like indicator that can be adjusted
with the mouse.  The argument
.CW size
is its nominal size.
(It may be given a different size by
.CW plpack
due to flag settings.)
The
.CW hit
routine is called whenever the slider's value changes.
Its arguments are a pointer to the slider, the mouse buttons,
the value the slider now indicates, and the slider's length, both measured in pixels.
Sliders slide horizontally or vertically depending on whether the
.CW x
component of
.CW size
is larger than the
.CW y
component.
.PP
A slider's position can be set by calling
.CW plsetslider ,
whose arguments are a pointer to the slider, what to set its
position to, and the maximum value that the
.CW val
argument is assumed to take.  The value will be rescaled to the slider's
actual size.
.PP
Here's an example that makes a compound panel containing a slider and
a couple of labels, one of which is updated by the slider's hit function
to reflect the slider's value:
.P1
.so slider.eg
.P2
.BP slider.ps .5 5.0 c
.PP
The
.CW hit
function calls
.CW plinitlabel
and
.CW pldraw
to reinitialize and redisplay
.CW sliderlabel ,
making the label track the slider's value.
Panel reinitialization functions such as
.CW plinitlabel
are discussed in more detail below.
.PP
Note also that the slider's size is given as
.CW Pt(0,137) ;
the
.CW FILLY
flag makes it come out the same height as the label
next to it.
.NH
Text entry
.PP
.P1
Panel *plentry(Panel *parent, int flags, int width,
	char *init, void (*hit)(Panel *p, char *text))
char *plentryval(Panel *);
.P2
Entry panels display a line of text that can be edited by typing.
The
.CW width
and
.CW init
arguments specify the entry's width in pixels and the initial
contents of its string.
Clicking an entry with the mouse makes it the keyboard focus.
Characters typed in the entry are added to end of the
displayed string.  Backspace deletes the last character.  Ctrl-U
deletes the whole string.  Ctrl-W deletes a trailing word (sequence
of letters and digits) and any following punctuation.  Typing the
newline character causes the
.CW hit
routine to be called with pointers to the entry and the current text
(without a newline) as arguments.
The current text can be retrieved at any time by calling
.CW plentryval .
.PP
Here's code for an example:
.P1
.so entry.eg
.P2
and here's the resulting display:
.BP entry.ps 0.2 5.0 c
.NH
Pop-up panels
.PP
.P1
Panel *plpopup(Panel *parent, int flags,
	Panel *but1, Panel *but2, Panel *but3);
.P2
Just like a group, a pop-up panel has children that
it displays and that can be sensitive to mouse events.
The difference is that when it receives a mouse event
with a specified button pressed, it may temporarily display a new
panel and send events on to it.
The
.CW but1 ,
.CW but2 ,
and
.CW but3
arguments point at the roots of panel trees to be popped up when
the appropriate mouse button is pressed.  A zero pointer indicates
that mouse events with that button depressed are passed on to the
pop-up panel's children.
.PP
Here's an example, a text editor window with a couple of pop-up menus.
Pressing button 2 will pop up the cut/snarf/paste menu.
Pressing button 3 will pop up the read/write/exit menu.
Pressing button 1 doesn't pop up a menu \(em the mouse events
pass through the
.CW plpopup
to its child, the
.CW pledit
panel.
.P1
.so popup.neg
.P2
Here's what the screen looks like with the middle button depressed:
.BP popup.ps 2.0 5.0 c
.NH
Pull-down panels and menu bars
.PP
.P1
Panel *plpulldown(Panel *parent, int flags,
	Icon *label, Panel *pull, int side);
Panel *plmenubar(Panel *parent, int flags, int itemflags,
	Icon *label1, Panel *pull1, Icon *label2, ...);
.P2
A pull-down panel looks like an ordinary button when displayed.
When hit, it displays the panel pointed to by
.CW pull ,
and passes further mouse events off to it.
The
.CW side
argument must be one of
.CW PACKN ,
.CW PACKE ,
.CW PACKS
or
.CW PACKW .
It indicates on which side of the pull-down button the
.CW pull
panel should be displayed.  Pull-down panels can be used in
menus to provide cascading submenus.  An array of pull-down panels
arranged across the top of the screen behaves as a menu bar.  The
.CW plmenubar
routine creates a group containing just such an array.  Its
arguments (after the obligatory
.CW parent
and
.CW flags )
are the flags to be used for each menu bar item, followed by the
.CW label
and
.CW pull
arguments of the items.  The list is terminated by a zero
.CW label
pointer.
.PP
Here's an edit window with a menu bar at the top:
.P1
.so mbar.neg
.P2
.PP
Here's the display, with one of the menus pulled down:
.BP mbar.ps 2.0 5.0 c
.PP
Here's code for a similar arrangement, but with the menu bar
converted to a pop-up menu with the sub-menus cascaded to the
right:
.P1
.so cascade.neg
.P2
.PP
Here's the display, with the main menu popped up and
showing a sub-menu:
.BP cascade.ps 2.0 5.0 c
.NH
Scrollable lists and scroll bars
.PP
.P1
Panel *pllist(Panel *parent, int flags,
	char *(*generate)(int index), int nlist,
	void(*hit)(Panel *p, int buttons, int index));
.P2
.P1
Panel *plscrollbar(Panel *parent, int flags);
void plscroll(Panel *scrollee,
	Panel *xscroller, Panel *yscroller);
.P2
A list panel displays a list of character strings.
The
.CW generate
function must return a pointer to the
.I n th
string (using zero-origin indexing) when passed
.I n
as an argument.  If
.I n
is out of range, it must return a zero pointer.
When one of the displayed strings is selected with the mouse, the
.CW hit
function is called with a pointer to the panel, the mouse buttons, and the string's index
as arguments.  The minimum number of entries displayed is controlled by the
.CW nlist
parameter.  If
.CW flags
has
.CW FILLY
set, the list may be longer.
The list may be
scrolled by creating a scroll bar panel and calling
.CW plscroll
to associate the scroll bar with the list.
.CW Plscroll 's
arguments are the panel to be scrolled, a scroll bar that
will scroll it in the
.I x
direction, and a scroll bar for the
.I y
direction.  Either of these last two may be empty if the scrollee
cannot be scrolled in the corresponding direction.
(In fact, no currently-defined panel scrolls in the
.I x
direction, so the
.CW xscroller
argument is purely for future expansion.)
.PP
Here's code to create a list box with a scroll bar:
.P1
.so scroll.eg
.P2
and here's what the display looks like:
.BP scroll.ps 1.6 5.0 c
.NH
Editable text
.PP
.P1
Panel *pledit(Panel *parent, int flags, Point minsize,
	Rune *text, int ntext, void (*hit)(Panel *));
.P2
.PP
The
.CW pledit
function creates a scrollable window of single-font text.
Its arguments are:
.TP minsize
the minimum acceptable size (in pixels) of the window.
.TP text
an array of
.CW Runes
(not
.CW chars )
containing the window's initial contents.
.TP ntext
the number of runes in
.CW text .
.TP hit
Text in an edit window can be selected by sweeping
with the mouse.  After each selection,
.CW hit
is called with a pointer to the panel as argument.
.PP
When a
.CW pledit
window is the keyboard focus, typed-in
characters mostly replace the current selection.
The exceptions are backspace, which deletes the selection
and the character before it (if any), ctrl-U, which deletes
back to the beginning of the line on which the selection
starts, and ctrl-W, which deletes back to the beginning of
the word at which the selection starts.
.PP
There are a half-dozen functions that manipulate the
text in a
.CW pledit
window and its display.
.TP "Rune *pleget(Panel *p);
returns a pointer to the window's text.  Note: this is not the same as the
.CW text
pointer passed to
.CW pledit ,
which is only used to initialize an internal text buffer.  The pointer
returned by
.CW pleget
is only good as long as the text remains unchanged.  (That is, until
the next character is typed, or
.CW plpaste
is called.)
.TP "int plelen(Panel *p);
returns the length of the window's text.
.TP "void plescroll(Panel *p, int top);
The
.CW top
argument is an index into the window's text.  The window
is scrolled so that the line containing the indexed Rune
is at the top of the window.
.TP "void plegetsel(Panel *p, int *sel0, int *sel1);
stores the index of the first character of the selection in
.CW sel0
and the first beyond in
.CW sel1 .
.TP "void plesel(Panel *p, int sel0, int sel1);
picks a new selection, updating the display correspondingly
.TP "void plepaste(Panel *p, Rune *text, int ntext);
removes the runes currently selected from the buffer,
replacing them with the text pointed to by
.CW text ,
whose length is
.CW ntext .
.PP
The following example uses the panel library and the generic application
skeleton given above to implement most of a simple cut-and-paste text editor.
It lacks only facilities to read and write files.
.P1
.so edit1.c
.P2
.P1
.so edit2.c
.P2
.P1
.so edit3.c
.P2
.P1
.so edit4.c
.P2
.PP
The
.CW mkpanels
function creates a display with a scroll bar and a
.CW pledit
window, over which button 2 or button 3 pops up a four-entry
menu.
The
.CW hitmenu
function processes menu commands.
.CW Snarf
copies the current selection into
.CW snarfbuf ,
unless the selection is empty.
.NH
Formatted text
.PP
.P1
Panel *pltextview(Panel *parent, int flags, Point minsize,
	Rtext *text, void (*hit)(Panel *p, int buttons, Rtext *t));
.P2
A
.CW pltextview
panel displays scrollable multi-font formatted text interspersed with
illustrations and sub-panels.
Any word of text or bitmap illustration can be marked sensitive to
mouse events \(em in-line panels always are.
.CW Pltextview 's
arguments are
.TP minsize
the minimum acceptable size for the window.
.CW text
points to a linked list of
.CW rtext
structures (described below) describing text and illustrations, their
spacing and layout, and indicating which segments are mouse-sensitive.
.TP hit
the function called to register a mouse hit.  Its arguments
are the panel in which the hit occurred, which mouse buttons
were pressed, and a pointer to the piece of text that was hit.
.PP
Each member of a list of
.CW Rtext
structures describes either a
.CW Font
and a string that will be displayed in
a single call to
.CW string ,
or a
.CW Bitmap
to be displayed by calling
.CW bitblt
(see
.I bitblt (2)).
Here is (part of) the definition of
.CW Rtext :
.P1
typedef struct Rtext Rtext;
struct Rtext{
	int hot;     /* responds to hits? */
	void *user;  /* user data */
	int space;   /* how much space if no break */
	int indent;  /* how much space after a line break */
	Bitmap *b;   /* what to display, if b!=0 */
	Panel *p;    /* what do display, if p!=0 and b==0 */
	Font *font;  /* font in which to draw text */
	char *text;  /* what to display, if b==0 and p==0 */
	Rtext *next;
	...          /* more private fields follow */
};
.P2
Its fields are:
.IP \f(CWhot\fP 10
non-zero if this
.CW Rtext
is mouse-sensitive.
.IP \f(CWuser\fP 10
a pointer not used by the panel library.  It can be used in a
.CW hit
function to interpret the mouse action.
.IP \f(CWspace\fP 10
how many pixels of space to leave between this
.CW Rtext
and the previous one, if they're drawn on
the same line.
.IP \f(CWindent\fP 10
how many pixels to indent this
.CW Rtext
by, if it's the first one on a line.
.IP \f(CWb\fP 10
If
.CW b!=0
this
.CW Rtext
is an illustration and
.CW b
points to a bitmap containing it.
.IP \f(CWp\fP 10
If
.CW b==0
and
.CW p!=0
this
.CW Rtext
is an inline panel.
.IP \f(CWtext\fP 10
If
.CW b==0
and
.CW p==0
this
.CW Rtext
must point a string displayable by a single call to
.CW string .
.IP \f(CWfont\fP 10
the font in which
.CW text
should be displayed.
.IP \f(CWnext\fP 10
a pointer to the next
.CW Rtext
in the list.
.PP
The following three functions create new
.CW Rtexts ,
returning pointers to the newly allocated storage:
.P1
Rtext *plrtstr(Rtext **head, int space, int indent,
	Font *f, char *s, int hot, void *user);
.P2
.P1
Rtext *plrtbitmap(Rtext **head, int space, int indent,
	Bitmap *b, int hot, void *user);
.P2
.P1
Rtext *plrtpanel(Rtext **head, int space, int indent,
	Panel *p, void *user);
.P2
.PP
These functions create new
.CW Rtext
nodes of the three kinds.
.CW Plrtstr
creates a string node with font
.CW f
and text
.CW s ,
.CW plrtbitmap
creates a bitblt node with bitmap
.CW b ,
and
.CW plrtpanel
creates a subpanel node with
.CW Panel
.CW p .
Since
.CW plrtstr
doesn't make a copy of the string
.CW s ,
be careful that its storage is not
accidentally overwritten.
The
.CW space ,
.CW indent ,
.CW hot ,
and
.CW user
arguments are used to initialize the corresponding
fields of the new
.CW Rtext .
Since subpanel nodes are always mouse-sensitive,
.CW plrtpanel
does not take a
.CW hot
argument.
.CW Head
is a pointer to a pointer to the first node of an
.CW Rtext
list.  Initialize
.CW *head=0
before making a new list.
.CW Plrtstr ,
.CW plrtbitmap ,
and
.CW plrtpanel
will update it appropriately.
.PP
The function
.P1
void plrtfree(Rtext *t);
.P2
discards the list pointed to by
.CW t .
The caller is responsible for disposing of
any fonts, strings, bitmaps, and user data that the
.CW Rtext 's
fields may point to.
.PP
Each
.CW Rtext
node can be drawn in a single call to
.CW string ,
.CW bitblt ,
or
.CW pldraw .
Line breaks can occur between
.CW Rtext
nodes.
If an
.CW Rtext
node is drawn at the beginning of a line, its
.CW indent
field tells how far to indent it from the left margin.
When an
.CW Rtext
is drawn on the same line as its predecessor, its
.CW space
field indicates how many pixels of space to leave between the two.
If
.CW space
is zero, there will never be a line break between the node and its
predecessor, to allow for words with internal font changes.
.P1
Rtext *plgetpostextview(Panel *p);
.P2
.P1
void plsetpostextview(Panel *p, Rtext *top);
.P2
Calling
.CW plsetpostextview
scrolls a
.CW pltextview
panel so that the given
.CW Rtext
appears the upper left-hand corner.
.CW Plgetpostextview
returns a pointer to the
.CW Rtext
that the given panel currently displays in its upper left-hand corner.
.NH
Panel Reinitialization
.PP
For each panel-creation function, there is a corresponding reinitialization
function whose name is just the creation function with the initial letters
.CW pl
changed to
.CW plinit .
Its arguments are the same as the
creation function's, except that the initial
.CW Panel
pointer indicates the panel to be re-initialized, not its parent.
For example, there is an
.CW plinitmessage
corresponding to
.CW plmessage ,
that can be used to change the text displayed in a message panel.
.PP
The implementation has two shortcomings.
First, for no good reason,
.CW plinitmenubar
is missing.
Second, you can't change the kind of a panel by calling a different
.CW plinit
function on it.
.NH
Nothing is Automatic
.PP
When you use the panel library, nothing happens automatically.
There are no daemons watching for changes in the state of panels
to recompute their geometry or redisplay them.
If you call
.CW plinitmessage
to post a message to an error window, you must call
.CW pldraw
to see the display.
If you add a new panel to a tree, you must repack it
and redisplay it yourself.
If you call
.CW plmove
you should also call
.CW pldraw
to see the result.
.NH
Caveat Emptor
.PP
The Panel library is new software created to build the World-Wide Web browser
.I mothra (1).
It is too young to have been tried in many other programs, so it should probably
be regarded as experimental.
