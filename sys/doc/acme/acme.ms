.HTML "Acme: A User Interface for Programmers
.de EX
.nr x \\$1v
\\!h0c n \\nx 0
..
.de FG		\" start figure caption: .FG filename.ps verticalsize
.KF
.BP \\$1 \\$2
.sp .5v
.if !h .EX \\$2v
.ps -1
.vs -1
..
.de fg		\" end figure caption (yes, it is clumsy)
.ps
.vs
.br
\l'1i'
.KE
..
.if h \{\
.de FG
.B1 10 60
.html - <center><a href="\\$1.gif"><img src="\\$1.gif" /></a></center>
.\" .sp .5v
..
.de fg
.br
.B2
..
.\}
.TL
Acme: A User Interface for Programmers
.AU
.I "Rob Pike
.I rob@plan9.bell-labs.com
.SP .22i exactly
.AB
.FS
.if !h \l'1i'
.if !h .br
Originally appeared in
.I
Proc. of the Winter 1994 USENIX Conf.,
.R
pp. 223-234,
San Francisco, CA
.FE
A hybrid of window system, shell, and editor, Acme gives text-oriented
applications a clean, expressive, and consistent style of interaction.
Traditional window systems support interactive client programs and offer libraries of
pre-defined operations such as pop-up menus
and buttons to promote a consistent
user interface among the clients.
Acme instead provides its clients with a fixed user interface and
simple conventions to encourage its uniform use.
Clients access the facilities of Acme through a file system interface;
Acme is in part a file server that exports device-like files that may be
manipulated to access and control the contents of its windows.
Written in a concurrent programming language,
Acme is structured as a set of communicating processes that neatly subdivide
the various aspects of its tasks: display management, input, file server, and so on.
.PP
Acme attaches distinct functions to the three mouse buttons:
the left selects text;
the middle executes textual commands;
and the right combines context search and file opening
functions to integrate the various applications and files in
the system.
.PP
Acme works well enough to have developed
a community that uses it exclusively.
Although Acme discourages the traditional style of interaction
based on typescript windows\(emteletypes\(emits
users find Acme's other services render
typescripts obsolete. 
.AE
.SH
History and motivation
.PP
The usual typescript style of interaction with
Unix and its relatives is an old one.
The typescript\(eman intermingling of textual commands and their
output\(emoriginates with the scrolls of paper on teletypes.
The advent of windowed terminals has given each user what
amounts to an array of teletypes, a limited and unimaginative
use of the powers of bitmap displays and mice.
Systems like the Macintosh
that do involve the mouse as an integral part of the interaction
are geared towards general users, not experts, and certainly
not programmers.
Software developers, at least on time-sharing systems, have been left behind.
.FG acme.fig1 5i
Figure 1.  A small Acme screen\(emnormally it runs on a larger display\(emdemonstrating
some of the details discussed in the text.
The right column contains some guide files,
a mailbox presented by Acme's mail program,
the columnated display of files in Acme's own source directory,
a couple of windows from the OED browser,
a debugger window,
and an error window showing diagnostics from a compilation.
The left column holds a couple of source files
.CW dat.h "" (
and
.CW acme.l ),
another debugger window displaying a stack trace,
and a third source file
.CW time.l ). (
.CW Time.l
was opened from the debugger by clicking the right mouse button
on a line in the stack window;
the mouse cursor landed on the offending line of
.CW acme.l
after a click on the compiler message.
.fg
.PP
Some programs have mouse-based editing of
text files and typescripts;
ones I have built include
the window systems
.CW mux
[Pike88]
and
.CW 8½
[Pike91]
and the text editor
Sam [Pike87].
These have put the programmer's mouse to some productive work,
but not wholeheartedly.  Even experienced users of these programs
often retype text that could be grabbed with the mouse,
partly because the menu-driven interface is imperfect
and partly because the various pieces are not well enough integrated.
.PP
Other programs\(emEMACS [Stal93] is the prime example\(emoffer a high
degree of integration but with a user interface built around the
ideas of cursor-addressed terminals that date from the 1970's.
They are still keyboard-intensive and
dauntingly complex.
.PP
The most ambitious attempt to face these issues was the Cedar
system, developed at Xerox [Swei86].
It combined a new programming language, compilers,
window system, even microcode\(ema complete system\(emto
construct a productive, highly
integrated and interactive environment
for experienced users of compiled languages.
Although successful internally, the system was so large
and so tied to specific hardware that it never fledged.
.PP
Cedar was, however, the major inspiration for Oberon [Wirt89],
a system of similar scope but much smaller scale.
Through careful selection of Cedar's ideas, Oberon shows
that its lessons can be applied to a small, coherent system
that can run efficiently on modest hardware.
In fact, Oberon probably
errs too far towards simplicity: a single-process system
with weak networking, it seems an architectural throwback.
.PP
Acme is a new program,
a combined window system, editor, and shell,
that applies
some of the ideas distilled by Oberon.
Where Oberon uses objects and modules within a programming language (also called Oberon),
Acme uses files and commands within an existing operating system (Plan 9).
Unlike Oberon, Acme does not yet have support for graphical output, just text.
At least for now, the work on Acme has concentrated on
producing the smoothest user interface possible for a programmer
at work.
.PP
The rest of this paper describes Acme's interface,
explains how programs can access it,
compares it to existing systems,
and finally presents some unusual aspects of its implementation.
.SH
User interface
.PP
.FG acme.fig2 3i
Figure 2.  An Acme window showing a section of code.
The upper line of text is the tag containing the file name,
relevant commands, and a scratch area (right of the vertical bar);
the lower portion of the window is the
body, or contents, of the file.
Here the scratch area contains a command for the middle button
.CW mk ) (
and a word to search for with the right button
.CW cxfidalloc ). (
The user has just
clicked the right button on
.CW cxfidalloc
and Acme has searched for the word, highlighted it,
and moved the mouse cursor there.  The file has been modified:
the center of the layout box is black and the command
.CW Put
appears in the tag.
.fg
Acme windows are arrayed in columns (Figure 1) and are used more
dynamically than in an environment like X Windows or
.CW 8½
[Sche86, Pike91].
The system frequently creates them automatically and the user
can order a new one with a single mouse button click.
The initial placement of a new window is determined
automatically, but the user may move an existing window anywhere
by clicking or dragging a
.I "layout box
in the upper left corner of
the window.
.PP
Acme windows have two parts: a
.I tag
holding a single line of text,
above a
.I body
holding zero or more lines (Figure 2).
The body typically contains an image of a file being edited
or the editable output of a
program, analogous to an
EMACS shell
window.  The tag contains
the name of the window
(usually the name of the associated
file or directory), some built-in commands, and a scratch area to hold arbitrary text.
If a window represents a directory, the name in the tag ends with
a slash and the body contains a list of the names of the files
in the directory.
Finally, each non-empty body holds a scroll bar at the left of the text.
.PP
Each column of windows also has a layout box and a tag.
The tag has no special meaning, although Acme pre-loads it with a few
built-in commands.
There is also a tag across the whole display, also loaded with
helpful commands and a list of active processes started
by Acme.
.PP
Typing with the keyboard and selecting with the left button are as in
many other systems, including the Macintosh,
.CW 8½ ,
and Sam.
The middle and right buttons are used, somewhat like the left button,
to `sweep' text, but the indicated text is treated in a way
that depends on the text's location\(em\f2context\f1\(emas well as its content.
This context, based on the directory of the file containing the text,
is a central component of Acme's style of interaction.
.PP
Acme has no single notion of `current directory'.
Instead, every command, file name,
action, and so on is interpreted or executed in the directory named by the
tag of the window containing the command.  For example, the string
.CW mammals
in a window labeled
.CW /lib/
or
.CW /lib/insects
will be interpreted as the file name
.CW /lib/mammals
if such a file exists.
.PP
Throughout Acme, the middle mouse button is used to execute commands
and the right mouse button is used to locate and select files and text.
Even when there are no true files on which to operate\(emfor example
when editing mail messages\(emAcme and its applications use
consistent extensions of these basic functions.
This idea is as vital to Acme as icons are to the Macintosh.
.PP
The middle button executes commands: text swept with the button
pressed is underlined; when the button is released, the underline is
removed and the indicated text is executed.
A modest number of commands are recognized as built-ins: words like
.CW Cut ,
.CW Paste ,
and
.CW New
name
functions performed directly by Acme.
These words often appear in tags to make them always available,
but the tags are not menus: any text anywhere in Acme may be a command.
For example, in the tag or body of any window one may type
.CW Cut ,
select it with the left button, use the middle button to execute it,
and watch it disappear again.
.PP
If the middle button indicates a command that is not recognized as a built-in,
it is executed in the directory
named by the tag of the window holding the text.
Also, the file to be executed is searched for first in that directory.
Standard input is connected to
.CW /dev/null ,
but standard and error outputs are connected to an Acme window,
created if needed, called
\f2dir\f(CW/+Errors\f1 where
.I dir
is the directory of the window.
(Programs that need interactive input use a different interface, described below.)
A typical use of this is to type
.CW mk
(Plan 9's
.CW make )
in the scratch area in the tag of a C source window, say
.CW /sys/src/cmd/sam/regexp.c ,
and execute it.
Output, including compiler errors, appears in the window labeled
.CW /sys/src/cmd/sam/+Errors ,
so file names in the output are associated with the windows and directory
holding the source.
The
.CW mk
command remains in the tag, serving as a sort of menu item for the associated
window.
.PP
Like the middle button, the right button is used to indicate text by sweeping it out.
The indicated text is not a command, however, but the argument of a generalized
search operator.
If the text, perhaps after appending it to the directory of the window containing it,
is the name of an existing file, Acme creates a new window to hold the file
and reads it in.  It then moves the mouse cursor to that window.  If the file is
already loaded into Acme, the mouse motion happens but no new window is made.
For example, indicating the string
.CW sam.h
in
.P1
#include "sam.h"
.P2
in a window on the file
.CW /sys/src/cmd/sam/regexp.c
will open the file
.CW /sys/src/cmd/sam/sam.h .
.PP
If the file name is followed immediately by a colon and a legal address in
Sam notation (for example a line number or a regular expression delimited in
slashes or a comma-separated compound of such addresses), Acme highlights
the target of that address in the file and places the mouse there.  One may jump to
line 27 of
.CW dat.h
by indicating with the right button the text
.CW dat.h:27 .
If the file is not already open, Acme loads it.
If the file name is null, for example if the indicated string is
.CW :/^main/ ,
the file is assumed to be that of the window containing the string.
Such strings, when typed and evaluated in the tag of a window, amount to
context searches.
.PP
If the indicated text is not the name of an existing file, it is taken to be literal
text and is searched for in the body of the window containing the text, highlighting
the result as if it were the result of a context search.
.PP
For the rare occasion when a file name
.I is
just text to search for, it can be selected with the left button and used as the
argument to a built-in
.CW Look
command that always searches for literal text.
.SH
Nuances and heuristics
.PP
A user interface should not only provide the necessary functions, it should also
.I feel
right.
In fact, it should almost not be felt at all; when one notices a
user interface, one is distracted from the job at hand [Pike88].
To approach this invisibility, some of Acme's properties and features
are there just to make the others easy to use.
Many are based on a fundamental principle of good design:
let the machine do the work.
.PP
Acme tries to avoid needless clicking and typing.
There is no `click-to-type', eliminating a button click.
There are no pop-up or pull-down menus, eliminating the mouse action needed to
make a menu appear.
The overall design is intended to make text on the screen useful without
copying or retyping; the ways in which this happens involve
the combination of many aspects of the interface.
.PP
Acme tiles its windows and places them automatically
to avoid asking the user to place and arrange them.
For this policy to succeed, the automatic placement must behave well enough
that the user is usually content with the location of a new window.
The system will never get it right all the time, but in practice most
windows are used at least for a while where Acme first places them.
There have been several complete rewrites of the
heuristics for placing a new window,
and with each rewrite the system became
noticeably more comfortable.  The rules are as follows, although
they are still subject to improvement.
The window appears in the `active' column, that most recently used for typing or
selecting.
Executing and searching do not affect the choice of active column,
so windows of commands and such do not draw new windows towards them,
but rather let them form near the targets of their actions.
Output (error) windows always appear towards the right, away from
edited text, which is typically kept towards the left.
Within the column, several competing desires are balanced to decide where
and how large the window should be:
large blank spaces should be consumed;
existing text should remain visible;
existing large windows should be divided before small ones;
and the window should appear near the one containing the action that caused
its creation.
.PP
Acme binds some actions to chords of mouse buttons.
These include
.CW Cut
and
.CW Paste
so these common operations can be done without
moving the mouse.
Another is a way to apply a command in one window to text (often a file name)
in another, avoiding the actions needed to assemble the command textually.
.PP
Another way Acme avoids the need to move the mouse is instead to move the cursor
to where it is likely to be used next.  When a new window is made, Acme
moves the cursor to the new window; in fact, to the selected text in that window.
When the user deletes a newly made window, the cursor is
returned to the point it was before the window was made,
reducing the irritation of windows that pop up to report annoying errors.
.PP
When a window is moved, Acme moves the cursor to the layout box in
its new place, to permit further adjustment without moving the mouse.
For example, when a click of the left mouse button on the layout box grows
the window, the cursor moves to the new location of the box so repeated clicks,
without moving the mouse, continue to grow it.
.PP
Another form of assistance the system can offer is to supply precision in
pointing the mouse.  The best-known form of this is `double-clicking' to
select a word rather than carefully sweeping out the entire word.
Acme provides this feature, using context to decide whether to select
a word, line, quoted string, parenthesized expression, and so on.
But Acme takes the idea much further by applying it to execution
and searching.
A
.I single
click, that is, a null selection, with either the middle or right buttons,
is expanded automatically to indicate the appropriate text containing
the click.  What is appropriate depends on the context.
.PP
For example, to execute a single-word command
such as
.CW Cut ,
it is not necessary to sweep the entire word; just clicking the button once with
the mouse pointing at the word is sufficient.  `Word'
means the largest string of likely file name characters surrounding the location
of the click: click on a file name, run that program.
On the right button, the rules are more complicated because
the target of the click might be a file name, file name with address,
or just plain text.  Acme examines the text near the click to find
a likely file name;
if it finds one, it checks that it names an existing file (in the directory named in the tag, if the name is relative)
and if so, takes that as the result, after extending it with any address
that may be present.  If there is no file with that name, Acme
just takes the largest alphanumeric string under the click.
The effect is a natural overloading of the button to refer to plain text as
well as file names.
.PP
First, though, if the click occurs over the left-button-selected text in the window,
that text is taken to be what is selected.
This makes it easy to skip through the occurrences of a string in a file: just click
the right button
on some occurrence of the text in the window (perhaps after typing it in the tag)
and click once for each subsequent occurrence.  It isn't even necessary to move
the mouse between clicks; Acme does that.
To turn a complicated command into a sort of menu item, select it:
thereafter, clicking the middle button on it will execute the full command.
.PP
As an extra feature, Acme recognizes file names in angle brackets
.CW <>
as names of files in standard directories of include files,
making it possible for instance to look at
.CW <stdio.h>
with a single click.
.PP
Here's an example to demonstrate how the actions and defaults work together.
Assume
.CW /sys/src/cmd/sam/regexp.c
is
open and has been edited.  We write it (execute
.CW Put
in the tag; once the file is written, Acme removes the word from the tag)
and type
.CW mk
in the tag.  We execute
.CW mk
and get some errors, which appear in a new window labeled
.CW /sys/src/cmd/sam/+Errors .
The cursor moves automatically to that window.
Say the error is
.P1
main.c:112: incompatible types on assignment to `pattern'
.P2
We move the mouse slightly and click the right button
at the left of the error message; Acme
makes a new window, reads
.CW /sys/src/cmd/main.c
into it, selects line 112
and places the mouse there, right on the offending line.
.SH
Coupling to existing programs
.PP
Acme's syntax for file names and addresses makes it easy for other programs
to connect automatically to Acme's capabilities.  For example, the output of
.P1
grep -n variable *.[ch]
.P2
can be used to help Acme step through the occurrences of a variable in a program;
every line of output is potentially a command to open a file.
The file names need not be absolute, either: the output
appears in a window labeled with the directory in which
.CW grep
was run, from which Acme can derive the full path names.
.PP
When necessary, we have changed the output of some programs,
such as compiler error messages, to match
Acme's syntax.
Some might argue that it shouldn't be necessary to change old programs,
but sometimes programs need to be updated when systems change,
and consistent output benefits people as well as programs.
A historical example is the retrofitting of standard error output to the
early Unix programs when pipes were invented.
.PP
Another change was to record full path names in
the symbol table of executables, so line numbers reported by the debugger
are absolute names that may be used directly by Acme; it's not necessary
to run the debugger in the source directory.  (This aids debugging
even without Acme.)
.PP
A related change was to add lines of the form
.P1
#pragma src "/sys/src/libregexp"
.P2
to header files; coupled with Acme's ability to locate a header file,
this provides a fast, keyboardless way to get the source associated with a library.
.PP
Finally, Acme directs the standard output of programs it runs to
windows labeled by the directory in which the program is run.
Acme's splitting of the
output into directory-labeled windows is a small feature that has a major effect:
local file names printed by programs can be interpreted directly by Acme.
By indirectly coupling the output of programs to the input,
it also simplifies the management of software that occupies multiple
directories.
.SH
Coupling to new programs
.PP
Like many Plan 9 programs,
Acme offers a programmable interface to
other programs by acting as a file server.
The best example of such a file server is the window system
.CW 8½
[Pike91],
which exports files with names such as
.CW screen ,
.CW cons ,
and
.CW mouse
through which applications may access the I/O capabilities of the windows.
.CW 8½
provides a
.I distinct
set of files for each window and builds a private file name space
for the clients running `in' each window;
clients in separate windows see distinct files with the same names
(for example
.CW /dev/mouse ).
Acme, like the process file system [PPTTW93], instead associates each
window with a directory of files; the files of each window are visible
to any application.
This difference reflects a difference in how the systems are used:
.CW 8½
tells a client what keyboard and mouse activity has happened in its window;
Acme tells a client what changes that activity wrought on any window it asks about.
Putting it another way,
.CW 8½
enables the construction of interactive applications;
Acme provides the interaction for applications.
.PP
The root of
Acme's file system is mounted using Plan 9 operations on the directory
.CW /mnt/acme .
In
that root directory appears a directory for each window, numbered with the window's identifier,
analogous to a process identifier, for example
.CW /mnt/acme/27 .
The window's directory
contains 6 files:
.CW /mnt/acme/27/addr ,
.CW body ,
.CW ctl ,
.CW data ,
.CW event ,
and
.CW tag .
The
.CW body
and
.CW tag
files contain the text of the respective parts of the window; they may be
read to recover the contents.  Data written to these files is appended to the text;
.CW seeks
are ignored.
The
.CW addr
and
.CW data
files provide random access to the contents of the body.
The
.CW addr
file is written to set a character position within the body; the
.CW data
file may then be read to recover the contents at that position,
or written to change them.
(The tag is assumed
small and special-purpose enough not to need special treatment.
Also,
.CW addr
indexes by character position, which is not the same as byte offset
in Plan 9's multi-byte character set [Pike93]).
The format accepted by the
.CW addr
file is exactly the syntax of addresses within the user interface,
permitting regular expressions, line numbers, and compound addresses
to be specified.  For example, to replace the contents of lines 3 through 7,
write the text
.P1
3,7
.P2
to the
.CW addr
file, then write the replacement text to the
.CW data
file.  A zero-length write deletes the addressed text; further writes extend the replacement.
.PP
The control file,
.CW ctl ,
may be written with commands to effect actions on the window; for example
the command
.P1
name /adm/users
.P2
sets the name in the tag of the window to
.CW /adm/users .
Other commands allow deleting the window, writing it to a file, and so on.
Reading the
.CW ctl
file recovers a fixed-format string containing 5 textual numbers\(emthe window
identifier, the number of characters in the tag, the number in the body,
and some status information\(emfollowed by the text of the tag, up to a newline.
.PP
The last file,
.CW event ,
is the most unusual.
A program reading a window's
.CW event
file is notified of all changes to the text of the window, and
is asked to interpret all middle- and right-button actions.
The data passed to the program is fixed-format and reports
the source of the action (keyboard, mouse, external program, etc.),
its location (what was pointed at or modified), and its nature (change,
search, execution, etc.).
This message, for example,
.P1
MI15 19 0 4 time
.P2
reports that actions of the mouse
.CW M ) (
inserted in the body (capital
.CW I )
the 4 characters of
.CW time
at character positions 15 through 19; the zero is a flag word.
Programs may apply their own interpretations of searching and
execution, or may simply reflect the events back to Acme,
by writing them back to the
.CW event
file, to have the default interpretation applied.
Some examples of these ideas in action are presented below.
.PP
Notice that changes to the window are reported
after the fact; the program is told about them but is not required to act
on them.  Compare this to a more traditional interface in which a program
is told, for example, that a character has been typed on the keyboard and
must then display and interpret it.
Acme's style stems from the basic model of the system, in which any
number of agents\(emthe keyboard, mouse, external programs
writing to
.CW data
or
.CW body ,
and so on\(emmay
change the contents of a window.
The style is efficient: many programs are content
to have Acme do most of the work and act only when the editing is completed.
An example is the Acme mail program, which can ignore the changes
made to a message being composed
and just read its body when asked to send it.
A disadvantage is that some traditional ways of working are impossible.
For example, there is no way `to turn off echo': characters appear on the
screen and are read from there; no agent or buffer stands between
the keyboard and the display.
.PP
There are a couple of other files made available by Acme in its root directory
rather than in the directory of each window.
The text file
.CW /mnt/acme/index
holds a list of all window names and numerical identifiers,
somewhat analogous to the output of the
.CW ps
command for processes.
The most important, though, is
.CW /mnt/acme/new ,
a directory that makes new windows, similar to the
.CW clone
directory in the Plan 9 network devices [Pres93].
The act of opening any file in
.CW new
creates a new Acme window; thus the shell command
.P1
grep -n var *.c > /mnt/acme/new/body
.P2
places its output in the body of a fresh window.
More sophisticated applications may open
.CW new/ctl ,
read it to discover the new window's identifier, and then
open the window's other files in the numbered directory.
.SH
Acme-specific programs
.PP
Although Acme is in part an attempt to move beyond typescripts,
they will probably always have utility.
The first program written for Acme was therefore one
to run a shell or other traditional interactive application
in a window, the Acme analog of
.CW xterm .
This program,
.CW win ,
has a simple structure:
it acts as a two-way intermediary between Acme and the shell,
cross-connecting the standard input and output of the shell to the
text of the window.
The style of interaction is modeled after
.CW mux
[Pike88]: standard output is added to the window at the
.I "output point;
text typed after the output point
is made available on standard input when a newline is typed.
After either of these actions, the output point is advanced.
This is different from the working of a regular terminal,
permitting cut-and-paste editing of an input line until the newline is typed.
Arbitrary editing may be done to any text in the window.
The implementation of
.CW win ,
using the
.CW event ,
.CW addr ,
and
.CW data
files, is straightforward.
.CW Win
needs no code for handling the keyboard and mouse; it just monitors the
contents of the window.  Nonetheless, it allows Acme's full editing to be
applied to shell commands.
The division of labor between
.CW win
and
.CW Acme
contrasted with
.CW xterm
and the X server demonstrates how much work Acme handles automatically.
.CW Win
is implemented by a single source file 560 lines long and has no graphics code.
.PP
.CW Win
uses the middle and right buttons to connect itself in a consistent way
with the rest of Acme.
The middle button still executes commands, but in a style more suited
to typescripts.  Text selected with the middle button is treated as if
it had been typed after the output point, much as a similar feature in
.CW xterm
or
.CW 8½ ,
and therefore causes it to be `executed' by the application running in the window.
Right button actions are reflected back to Acme but refer to the appropriate
files because
.CW win
places the name of the current directory in the tag of the window.
If the shell is running, a simple shell function replacing the
.CW cd
command can maintain the tag as the shell navigates the file system.
This means, for example, that a right button click on a file mentioned in an
.CW ls
listing opens the file within Acme.
.PP
Another Acme-specific program is a mail reader that begins by presenting,
in a window, a listing of the messages in the user's mailbox, one per line.
Here the middle and right button actions are modified to refer to
mail commands
and messages, but the change feels natural.
Clicking the right button on a line creates a new window and displays the
message there, or, if it's already displayed, moves the mouse to that window.
The metaphor is that the mailbox is a directory whose constituent files are messages.
The mail program also places some relevant commands in the tag lines of
the windows; for example, executing the word
.CW Reply
in a message's tag creates a new window
in which to compose a message to the sender of the original;
.CW Post
then dispatches it.
In such windows, the addressee is just a list of names
on the first line of the body, which may be edited to add or change recipients.
The program also monitors the mailbox, updating the `directory' as new messages
arrive.
.PP
The mail program is as simple as it sounds; all the work of interaction,
editing, and management of the display is done by Acme.
The only
difficult sections of the 1200
lines of code concern honoring the external protocols for managing
the mailbox and connecting to
.CW sendmail .
.PP
One of the things Acme does not provide directly is a facility like
Sam's command language to enable actions such as global substitution;
within Acme, all editing is done manually.
It is easy, though, to write external programs for such tasks.
In this, Acme comes closer to the original intent of Oberon:
a directory,
.CW /acme/edit ,
contains a set of tools for repetitive editing and a template
or `guide' file that gives examples
of its use.  
Acme's editing guide,
.CW /acme/edit/guide ,
looks like this:
.P1
e file | x '/regexp/' | c 'replacement'
e file:'0,$' | x '/.*word.*\en/' | p -n
e file | pipe command args ...
.P2
The syntax is reminiscent of Sam's command language, but here the individual
one-letter commands are all stand-alone programs connected by pipes.
Passed along the pipes are addresses, analogous to structural expressions
in Sam terminology.
The
.CW e
command, unlike that of Sam, starts the process by generating the address
(default dot, the highlighted selection) in the named files.
The other commands are as in Sam:
.CW p
prints the addressed text on standard output (the
.CW -n
option is analogous to that of
.CW grep ,
useful in combination with the right mouse button);
.CW x
matches a regular expression to the addressed (incoming) text,
subdividing the text;
.CW c
replaces the text; and so on.  Thus, global substitution throughout a file,
which would be expressed in Sam as
.P1
0,$ x/regexp/ c/replacement/
.P2
in Acme's editor becomes
.P1
e 'file:0,$' | x '/regexp/' | c 'replacement'
.P2
.PP
To use the Acme editing commands, open
.CW /acme/edit/guide ,
use the mouse and keyboard to edit one of the commands to the right form,
and execute it with the middle button.
Acme's context rules find the appropriate binaries in
.CW /acme/edit
rather than
.CW /bin ;
the effect is to turn
.CW /acme/edit
into a toolbox containing tools and instructions (the guide file) for their use.
In fact, the source for these tools is also there, in the directory
.CW /acme/edit/src .
This setup allows some control of the file name space for binary programs;
not only does it group related programs, it permits the use of common
names for uncommon jobs.  For example, the single-letter names would
be unwise in a directory in everyone's search path; here they are only
visible when running editing commands.
.PP
In Oberon,
such a collection would be called a
.I tool
and would consist
of a set of entry points in a module and a menu-like piece of text containing
representative commands that may be edited to suit and executed.
There is, in fact, a tool called
.CW Edit
in Oberon.
To provide related functionality,
Acme exploits the directory and file structure of the underlying
system, rather than the module structure of the language;
this fits well with Plan 9's
file-oriented philosophy.
Such tools are central to the working of Oberon but they are
less used in Acme, at least so far.
The main reason is probably that Acme's program interface permits
an external program to remain executing in the background, providing
its own commands as needed (for example, the
.CW Reply
command in the mail program); Oberon uses tools to
implement such services because its must invoke
a fresh program for each command.
Also,
Acme's better integration allows more
basic functions to be handled internally; the right mouse button
covers a lot of the basic utility of the editing tools in Oberon.
Nonetheless, as more applications are written for Acme,
many are sure to take this Oberon tool-like form.
.SH
Comparison with other systems
.PP
Acme's immediate ancestor is Help [Pike92], an experimental system written
a few years ago as a first try at exploring some of Oberon's ideas
in an existing operating system.
Besides much better engineering, Acme's advances over Help
include the actions of the right button (Help had nothing comparable),
the ability to connect long-running programs to the user interface
(Help had no analog of the
.CW event
file),
and the small but important change to split command output into
windows labeled with the directory in which the commands run.
.PP
Most of Acme's style, however, derives from the user interface and window
system of Oberon [Wirt89, Reis91].
Oberon includes a programming language and operating system,
which Acme instead borrows from an existing system, Plan 9.
When I first saw Oberon, in 1988, I was struck by the
simplicity of its user interface, particularly its lack of menus
and its elegant use of multiple mouse buttons.
The system seemed restrictive, though\(emsingle process,
single language, no networking, event-driven programming\(emand
failed to follow through on some of its own ideas.
For example, the middle mouse button had to be pointed accurately and
the right button was essentially unused.
Acme does follow through:
to the basic idea planted by Oberon, it adds
the ability to run on different operating systems and hardware,
connection to existing applications including
interactive ones such as shells and debuggers,
support for multiple processes,
the right mouse button's features,
the default actions and context-dependent properties
of execution and searching,
and a host of little touches such as moving the mouse cursor that make the system 
more pleasant.
At the moment, though, Oberon does have one distinct advantage: it incorporates
graphical programs well into its model, an issue Acme has not yet faced.
.PP
Acme shares with the Macintosh a desire to use the mouse well and it is
worth comparing the results.
The mouse on the Macintosh has a single button, so menus are essential
and the mouse must frequently move a long way
to reach the appropriate function.
An indication that this style has trouble is that applications provide
keyboard sequences to invoke menu selections and users often prefer them.
A deeper comparison is that the Macintosh uses pictures where Acme uses text.
In contrast to pictures, text can be edited quickly, created on demand,
and fine-tuned to the job at hand; consider adding an option to a command.
It is also self-referential; Acme doesn't need menus because any text can be
in effect a menu item.
The result is that, although a Macintosh screen is certainly prettier and probably
more attractive, especially to beginners, an Acme screen is more dynamic
and expressive, at least for programmers and experienced users.
.PP
For its role in the overall system,
Acme most resembles EMACS [Stal93].
It is tricky to compare Acme to EMACS, though, because there are
many versions of EMACS and, since it is fully programmable, EMACS
can in principle do anything Acme does.
Also, Acme is much younger and therefore has not
had the time to acquire as many features.
The issue therefore is less what the systems can be programmed to do than
how they are used.
The EMACS versions that come closest to Acme's style are those that
have been extended to provide a programming environment, usually
for a language such as LISP [Alle92, Lucid92].
For richness of the existing interface, these EMACS versions are certainly superior to Acme.
On the other hand, Acme's interface works equally well already for a variety
of languages; for example, one of its most enthusiastic users works almost
exclusively in Standard ML, a language nothing like C.
.PP
Where Acme excels is in the smoothness of its interface.
Until recently, EMACS did not support the mouse especially well,
and even with the latest version providing features such as `extents'
that can be programmed to behave much like Acme commands,
many users don't bother to upgrade.
Moreover, in the versions that provide extents, 
most EMACS packages don't take advantage of them.
.PP
The most important distinction is just that
EMACS is fundamentally keyboard-based, while
Acme is mouse-based.
.PP
People who try Acme find it hard to go back to their previous environment.
Acme automates so much that to return to a traditional interface
is to draw attention to the extra work it requires.
.SH
Concurrency in the implementation
.PP
Acme is about 8,000 lines of code in Alef, a concurrent object-oriented language syntactically similar to C [Alef].
Acme's structure is a set of communicating
processes in a single address space.
One subset of the processes drives the display and user interface,
maintaining the windows; other processes forward mouse and keyboard
activity and implement the file server interface for external programs.
The language and design worked out well;
as explained elsewhere [Pike89, Gans93, Reppy93],
user interfaces built with concurrent systems
can avoid the clumsy
top-level event loop typical of traditional interactive systems.
.PP
An example of the benefits of the multi-process style
is the management of the state of open
files held by clients of the file system interface.
The problem is that some I/O requests,
such as reading the
.CW event
file, may block if no data is available, and the server must
maintain the state of (possibly many) requests until data appears.
For example,
in
.CW 8½ ,
a single-process window system written in C, pending requests were queued in
a data structure associated with each window.
After activity in the window that might complete pending I/O,
the data structure was scanned for requests that could now finish.
This structure did not fit well with the rest of the program and, worse,
required meticulous effort
to guarantee correct behavior under all conditions
(consider raw mode, reads of partial lines, deleting a window,
multibyte characters, etc.).
.PP
Acme instead creates a new dedicated process
for each I/O request.
This process coordinates with the rest of the system
using Alef's synchronous communication;
its state implicitly encodes the state of
the I/O request and obviates the need for queuing.
The passage of the request through Acme proceeds as follows.
.PP
Acme contains a file server process, F, that executes a
.CW read
system call to receive a Plan 9 file protocol (9P) message from the client [AT&T92].
The client blocks until Acme answers the request.
F communicates with an allocation process, M,
to acquire an object of type
.CW Xfid
(`executing fid'; fid is a 9P term)
to hold the request.
M sits in a loop (reproduced in Figure 2) waiting for either a request for
a new
.CW Xfid
or notification that an existing one has finished its task.
When an
.CW Xfid
is created, an associated process, X,
is also made.
M queues idle
.CW Xfids ,
allocating new ones only when the list is empty.
Thus, there is always a pool of
.CW Xfids ,
some executing, some idle.
.PP
The
.CW Xfid
object contains a channel,
.CW Xfid.c ,
for communication with its process;
the unpacked message; and some associated functions,
mostly corresponding to 9P messages such as
.CW Xfid.write
to handle a 9P write request.
.PP
The file server process F parses the message to see its nature\(emopen,
close, read, write, etc.  Many messages, such as directory
lookups, can be handled immediately; these are responded to directly
and efficiently
by F without invoking the
.CW Xfid ,
which is therefore maintained until the next message.
When a message, such as a write to the display, requires the attention
of the main display process and interlocked access to its data structures,
F enables X
by sending a function pointer on
.CW Xfid.c .
For example, if the message is a write, F executes
.P1
x->c <-= Xfid.write;
.P2
which sends
the address of
.CW Xfid.write
on
.CW Xfid.c ,
waking up X.
.PP
The
.CW Xfid
process, X, executes a simple loop:
.P1
void
Xfid.ctl(Xfid *x)
{
    for(;;){
        (*<-x->c)(x);      /* receive and execute message */
        bflush();          /* synchronize bitmap display */
        cxfidfree <-= x;   /* return to free list */
    }
}
.P2 
Thus X
will wake up with the address of a function to call (here
.CW Xfid.write )
and execute it; once that completes, it returns itself to the pool of
free processes by sending its address back to the allocator.
.PP
Although this sequence may seem complicated, it is just a few lines
of code and is in fact far simpler
than the management of the I/O queues in
.CW 8½ .
The hard work of synchronization is done by the Alef run time system.
Moreover, the code worked the first time, which cannot be said for the code in
.CW 8½ .
.SH
Undo
.PP
Acme provides a general undo facility like that of Sam, permitting
textual changes to be unwound arbitrarily.
The implementation is superior to Sam's, though,
with much higher performance and the ability to `redo' changes.
.PP
Sam uses
a multi-pass algorithm that builds
a transcript of changes to be made simultaneously
and then executes them atomically.
This was thought necessary because the elements of a repetitive
command such as a global substitution should all be applied to the same
initial file and implemented simultaneously; forming the complete
transcript before executing any of the changes avoids the
cumbersome management of addresses in a changing file.
Acme, however, doesn't have this problem; global substitution
is controlled externally and may be made incrementally by exploiting
an observation: if the changes are sorted in address order and
executed in reverse, changes will not invalidate the addresses of
pending changes.
.PP
Acme therefore avoids the initial transcript.  Instead, changes are applied
directly to the file, with an undo transcript recorded in a separate list.
For example, when text is added to a window, it is added directly and a record
of what to delete to restore the state is appended to the undo list.
Each undo action and the file are marked with a sequence number;
actions with the same sequence number are considered a unit
to be undone together.
The invariant state of the structure
is that the last action in the undo list applies to the current state of the file,
even if that action is one of a related set from, for example, a global substitute.
(In Sam, a related set of actions needed to be undone simultaneously.)
To undo an action, pop the last item on the undo list, apply it to the file,
revert it, and append it to a second, redo list.
To redo an action, do the identical operation with the lists interchanged.
The expensive operations occur
only when actually undoing; in normal editing the overhead is minor.
For example, Acme reads files about seven times faster than Sam, partly
because of this improvement and partly because of a cleaner implementation.
.PP
Acme uses a temporary file to hold the text, keeping in memory only the
visible portion, and therefore can edit large files comfortably
even on small-memory machines such as laptops.
.SH
Future
.PP
Acme is still under development.
Some things are simply missing.
For example, Acme should support non-textual graphics, but this is being
deferred until it can be done using a new graphics model being developed
for Plan 9.  Also, it is undecided how Acme's style of interaction should best be
extended to graphical applications.
On a smaller scale, although the system feels smooth and comfortable,
work continues to tune the heuristics and
try new ideas for the user interface.
.PP
There need to be more programs that use Acme.  Browsers for
Usenet and AP News articles, the Oxford English Dictionary, and other
such text sources exist, but more imaginative applications will
be necessary to prove that Acme's approach is viable.
One that has recently been started is an interface to the debugger Acid [Wint94],
although it is still
unclear what form it will ultimately take.
.PP
Acme shows that it is possible to make a user interface a stand-alone component
of an interactive environment.  By absorbing more of the interactive
functionality than a simple window system, Acme off-loads much of the
computation from its applications, which helps keep them small and
consistent in their interface.  Acme can afford to dedicate
considerable effort to making that interface as good as possible; the result
will benefit the entire system.
.PP
Acme is complete and useful enough to attract users.
Its comfortable user interface,
the ease with which it handles multiple tasks and
programs in multiple directories,
and its high level of integration
make it addictive.
Perhaps most telling,
Acme shows that typescripts may not be the most
productive interface to a time-sharing system.
.SH
Acknowledgements
.PP
Howard Trickey, Acme's first user, suffered buggy versions gracefully and made
many helpful suggestions.  Chris Fraser provided the necessary insight for the Acme editing
commands.
.SH
References
.LP
[Alef] P. Winterbottom,
``Alef Language Reference Manual'',
.I
Plan 9 Programmer's Manual,
.R
AT&T Bell Laboratories,
Murray Hill, NJ,
1992;
revised in this volume.
.br
[Alle92]
.I
Allegro Common Lisp user Guide, Vol 2, 
.R
Chapter 14, "The Emacs-Lisp Interface". 
March 1992.
.br
[AT&T92] Plan 9 Programmer's manual, Murray Hill, New Jersey, 1992.
.br
[Far89] Far too many people, XTERM(1), Massachusetts Institute of Technology, 1989.
.br
[Gans93] Emden R. Gansner and John H. Reppy,  ``A Multi-threaded Higher-order User Interface Toolkit'', in
.I
Software Trends, Volume 1,
User Interface Software,
.R
Bass and Dewan (Eds.),
John Wiley & Sons 1993,
pp. 61-80.
.br
[Lucid92] Richard Stallman and Lucid, Inc.,
.I
Lucid GNU EMACS Manual,
.R
March 1992.
.br
[Pike87] Rob Pike, ``The Text Editor \f(CWsam\fP'', Softw. - Pract. and Exp., Nov 1987, Vol 17 #11, pp. 813-845; reprinted in this volume.
.br
[Pike88] Rob Pike, ``Window Systems Should Be Transparent'', Comp. Sys., Summer 1988, Vol 1 #3, pp. 279-296.
.br
[Pike89] Rob Pike, ``A Concurrent Window System'', Comp. Sys., Spring 1989, Vol 2 #2, pp. 133-153.
.br
[PPTTW93] Rob Pike, Dave Presotto, Ken Thompson, Howard Trickey, and Phil Winterbottom, ``The Use of Name Spaces in Plan 9'',
Op. Sys. Rev.,  Vol. 27, No. 2, April 1993, pp. 72-76,
reprinted in this volume.
.br
[Pike91] Rob Pike, ``8½, the Plan 9 Window System'', USENIX Summer Conf. Proc., Nashville, June, 1991, pp. 257-265,
reprinted in this volume.
.br
[Pike92] Rob Pike, ``A Minimalist Global User Interface'', Graphics Interface '92 Proc., Vancouver, 1992, pp. 282-293.  An earlier version appeared under the same title in USENIX Summer Conf. Proc., Nashville, June, 1991, pp. 267-279.
.br
[Pike93] Rob Pike and Ken Thompson, ``Hello World or Καλημέρα κόσμε or
\f(Jpこんにちは 世界\fP'', USENIX Winter Conf. Proc., San Diego, 1993, pp. 43-50,
reprinted in this volume.
.br
[Pres93] Dave Presotto and Phil Winterbottom, ``The Organization of Networks in Plan 9'', Proc. Usenix Winter 1993, pp. 271-287, San Diego, CA,
reprinted in this volume.
.br
[Reis91] Martin Reiser, \fIThe Oberon System,\fP Addison Wesley, New York, 1991.
.br
[Reppy93] John H. Reppy,
``CML: A higher-order concurrent language'', Proc. SIGPLAN'91 Conf. on Programming, Lang. Design and Impl., June, 1991, pp. 293-305.
.br
[Sche86] Robert W. Scheifler and Jim Gettys,
``The X Window System'',
ACM Trans. on Graph., Vol 5 #2, pp. 79-109.
.br
[Stal93] Richard Stallman,
.I
Gnu Emacs Manual, 9th edition, Emacs version 19.19,
.R
MIT.
.br
[Swei86] Daniel Sweinhart, Polle Zellweger, Richard Beach, and Robert Hagmann,
``A Structural View of the Cedar Programming Environment'',
ACM Trans. Prog. Lang. and Sys., Vol. 8, No. 4, pp. 419-490, Oct. 1986.
.br
[Wint94], Philip Winterbottom, ``Acid: A Debugger based on a Language'', USENIX Winter Conf. Proc., San Francisco, CA, 1993,
reprinted in this volume.
.br
[Wirt89] N. Wirth and J. Gutknecht, ``The Oberon System'', Softw. - Prac. and Exp., Sep 1989, Vol 19 #9, pp 857-894.
