.HTML "8½, the Plan 9 Window System
.TL
8½, the Plan 9 Window System
.AU
Rob Pike
rob@plan9.bell-labs.com
.AB
.FS
Originally appeared, in a slightly different form, in
.I
Proc. of the Summer 1991 USENIX Conf.,
.R
pp. 257-265,
Nashville.
Note that
.CW 8½
has been replaced by
.CW rio
(see
.I rio (1)).
.FE
The Plan 9 window system, 8½, is a modest-sized program of novel design.
It provides textual I/O and bitmap graphic services to both
local and remote client programs by offering a multiplexed file service to those clients.
It serves traditional UNIX files like
.CW /dev/tty
as well as more unusual ones that provide access to the mouse
and the raw screen.
Bitmap graphics operations are provided by serving a file called
.CW /dev/bitblt
that interprets client messages to perform raster operations.
The file service that 8½ offers its clients is identical to that it uses for
its own implementation, so it is fundamentally no more than
a multiplexer.
This architecture has some rewarding symmetries and can be implemented
compactly.
.AE
.SH
Introduction
.PP
In 1989 I constructed a toy window system from only a few hundred
lines of source code using a custom language and an unusual architecture
involving concurrent processes [Pike89].
Although that system was rudimentary at best, it demonstrated that
window systems are not inherently complicated.
The following year, for the new Plan 9 distributed system [Pike92], I applied some of
the lessons from that toy project to write, in C,
a production-quality window system
called 8½.
8½ provides, on black-and-white, grey-scale, or color displays,
the services required of a modern window system, including
programmability and support for remote graphics.
The entire system, including the default program that runs in the 
window \(em the equivalent of
.CW xterm
[Far89] with `cutting and pasting' between windows \(em
is well under 90 kilobytes of text on a Motorola 68020 processor, about
half the size of the
operating system
kernel that supports it and a tenth the size of the X server
[Sche86]
.I without
.CW xterm .
.PP
What makes 8½ so compact?  Much of the saving comes from overall simplicity:
8½ has little graphical fanciness, a concise programming interface, and
a simple, fixed user interface.
8½ also makes some decisions by fiat
\(em three-button mouse, overlapping windows, built-in terminal program and
window manager, etc. \(em
rather than trying to appeal to all tastes.
Although compact, 8½ is not ascetic.
It provides the fundamentals and
enough extras to make them comfortable to use.
The most important contributor to its small size, though, is its
overall design as a file server.
This structure may be applicable to window systems
on traditional UNIX-like operating systems.
.PP
The small size of 8½ does not reflect reduced functionality:
8½ provides service roughly equivalent to the X window system.
8½'s clients may of course be as complex as they choose,
although the tendency to mimic 8½'s design
and the clean programming interface means they
are not nearly as bloated as X applications.
.SH
User's Model
.PP
8½ turns the single screen, mouse, and keyboard of the terminal
(in Plan 9 terminology) or workstation (in commercial terminology) into an array
of independent virtual terminals that may be textual terminals supporting a shell and
the usual suite of tools
or graphical applications using the full power of the bitmap screen and mouse.
Text is represented in UTF, an encoding of the Unicode Standard [Pike93].
The entire programming interface is provided through
reading and writing files in
.CW /dev .
.PP
Primarily for reasons of history and familiarity,
the general model and appearance of 8½ are similar to those of
.CW mux
[Pike88].
The right button has a short menu for controlling window creation, destruction,
and placement.
When a window is created, it runs the default shell,
.CW rc
[Duff90], with standard input
and output directed to the window and accessible through the file
.CW /dev/cons
(`console'),
analogous to the
.CW /dev/tty
of UNIX.
The name change represents a break with the past: Plan 9 does not provide a
Teletype-style model of terminals.  8½ provides the only way
most users ever access Plan 9.
.PP
Graphical applications,
like ordinary programs,
may be run by typing their names
to the shell running in a window.
This runs the application in the same window;
to run the application in a new window one may use an external program,
.CW window ,
described below.
For graphical applications, the virtual terminal model
is extended somewhat to allow programs to perform graphical operations,
access the
mouse, and perform related functions by reading and writing files with
suggestive names such as
.CW /dev/mouse
and
.CW /dev/window
multiplexed per-window
much like
.CW /dev/cons .
The implementation and semantics of these files,
described below, is central to the structure of 8½.
.PP
The default program that runs in a window is familiar to users of Blit terminals [Pike83].
It is very similar to that of
.CW mux
[Pike88], providing mouse-based editing of input and output text,
the ability to scroll back to see earlier output, and so on.
It also has a new feature, toggled by typing ESC,
that enables the user to control when
typed characters may be read by the shell or application,
instead of (for example) after each newline.
This feature makes the window program directly useful for many text-editing
tasks such as composing mail messages before sending them.
.SH
Plan 9 and 8½
.PP
Plan 9 is a distributed system that provides support for UNIX-like applications
in an environment built from distinct CPU servers, file servers, and terminals
connected by a variety of networks [Pike90].
The terminals are comparable to modest workstations that, once connected to a file
server over a medium-bandwidth network such as Ethernet, are self-sufficient computers
running a full operating system.
Unlike workstations, however, their role is just to
provide an affordable multiplexed user interface to the rest of the system:
they run the window system and support simple interactive
tasks such as text editing.
Thus they lie somewhere between workstations and X terminals in design,
cost, performance, and function.
(The terminals can be used
for general computing, but in practice Plan 9 users do their
computing on the CPU servers.)
The Plan 9 terminal software, including 8½,
was developed on a 68020-based
machine called a Gnot
and has been ported to
the NeXTstation,
the MIPS Magnum 3000,
SGI Indigos,
and Sun SPARCstations\(emall small workstations that we use as terminals\(emas
well as PCs.
.PP
Heavy computations such as compilation, text processing,
or scientific calculation are done on the CPU servers, which are connected
to the file servers by high-bandwidth networks.
For interactive work,
these computations can access the terminal that instantiated them.
The terminal and CPU server being used by a particular user are connected to the
same file server, although over different networks; Plan 9 provides a view of the
file server that is independent of location in the network.
.PP
The components of Plan 9 are connected by a common protocol based on the sharing of files.
All resources in the network are implemented as file servers; programs that wish to
access them connect to them over the network and communicate using ordinary file
operations.
An unusual aspect of Plan 9 is that the
.I
name space
.R
of a process, the set of files that can be accessed by name
(for example by an
.CW open
system call) is not global to all processes on a machine; distinct processes
may have distinct name spaces.  The system provides methods by which processes
may change their name spaces, such as the ability to
.I mount
a service upon an existing directory, making the files of the service
visible in the directory.
(This is a different operation from its
UNIX
namesake.)
Multiple services may be mounted upon the same directory,
allowing the files from multiple services to be accessed in the same directory.
Options to the
.CW mount
system call control the order of searching for files in such a
.I
union directory.
.R
.PP
The most obvious example of a network resource is a file server, where permanent
files reside.  There are a number of unusual services, however, whose design in
a different environment would likely not be file-based.  Many are described
elsewhere [Pike92]; some examples are the representation
of processes for debugging,
much like Killian's process files for the 8th edition [Kill84],
and the implementation of the name/value pairs of the
UNIX
.CW exec
environment as files.
User processes may also implement a file service and make it available to clients
in the network, much like the `mounted streams' in the 9th Edition
[Pres90].
A typical example is a program that interprets an externally-defined file system
such as that on a CD-ROM or a standard
UNIX
system and makes the contents available to Plan 9 programs.
This design is used by all distributed applications in Plan 9, including 8½.
.PP
8½ serves a set of files in the conventional directory
.CW /dev
with names like
.CW cons ,
.CW mouse ,
and
.CW screen .
Clients of 8½ communicate with the window system by reading and writing
these files.
For example, a client program, such as a shell,
can print text by writing its standard output, which is automatically
connected to
.CW /dev/cons ,
or it may open and write that file explicitly.
Unlike files served by a traditional file server, however, the instance of
.CW /dev/cons
served in each window by 8½ is a distinct file;
the per-process name spaces of Plan 9 allow 8½ to provide a unique
.CW /dev/cons
to each client.
This mechanism is best illustrated by the creation of a new 8½ client.
.PP
When 8½ starts, it creates a full-duplex pipe to be the communication
medium for the messages that implement the file service it will provide.
One end will be shared by all the clients; the other end is held by
8½ to accept requests for I/O.
When a user makes a new window using the mouse,
8½ allocates the window data structures and forks a child process.
The child's name space,
initially shared with the parent,
is then duplicated
so that changes the child makes to its name space will not affect the parent.
The child then attaches its end of the communication pipe,
.CW cfd ,
to the directory
.CW /dev
by doing a
.CW mount
system call:
.P1
mount(cfd, "/dev", MBEFORE, buf)
.P2
This call attaches the service associated with the file descriptor
.CW cfd
\(em the client end of the pipe \(em to the beginning of
.CW /dev
so that the files in the new service take priority over existing files
in the directory.
This makes the new files
.CW cons ,
.CW mouse ,
and so on,
available in
.CW /dev
in a way that hides any files with the same names already in place.
The argument
.CW buf
is a character string (null in this case),
described below.
.PP
The client process then closes file descriptors 0, 1, and 2 and opens
.CW /dev/cons
repeatedly to connect the standard
input, output, and error files to the window's
.CW /dev/cons .
It then does an
.CW exec
system call to begin executing the shell in the window.
This entire sequence, complete with error handling, is 33 lines of C.
.PP
The view of these events from 8½'s end of the pipe is a sequence
of file protocol messages from the new client generated by the
intervening operating
system in response to the
.CW mount
and
.CW open
system calls executed by the client.
The message generated by the
.CW mount
informs 8½ that a new client has attached to the file service it provides;
8½'s response is a unique identifier kept by the operating system and
passed in all messages generated by I/O on the files derived from that
.CW mount .
This identifier is used by 8½ to distinguish the various clients so
each sees a unique
.CW /dev/cons ;
most servers do not need to make this distinction.
.PP
A process unrelated to 8½ may create windows by a variant of this mechanism.
When 8½ begins, it uses a Plan 9 service to `post' the client end of the
communication pipe in a public place.
A process may open that pipe and
.CW mount
it to attach to the window system,
much in the way an X client may connect to a
UNIX
domain socket to the server bound to the file system.
The final argument to
.CW mount
is passed through uninterpreted by the operating
system.
It provides a way for the client and server to
exchange information at the time of the
.CW mount .
8½ interprets it as the dimensions of the window to be
created for the new client.  (In the case above, the window has been
created by the time the mount occurs, and
.CW buf
carries no information.)
When the
.CW mount
returns, the process can open the files of the new window and begin I/O to
use it.
.PP
Because 8½'s interface is based on files,
standard system utilities can be used to control its services.
For example,
its method of creating windows externally is packaged in a
16-line shell script, called
.CW window ,
the core of which is just a
.CW mount
operation that prefixes 8½'s directory to
.CW /dev
and runs a command passed on the argument line:
.P1
mount -b $'8½serv' /dev
$* < /dev/cons > /dev/cons >[2] /dev/cons &
.P2
The
.CW window
program is typically employed by users to create their
initial working environment when they boot the system, although
it has more general possibilities.
.PP
Other basic features of the system fall out naturally from the
file-based model.
When the user deletes a window, 8½ sends the equivalent of a
UNIX
signal to the process group \(em the clients \(em in the window,
removes the window from the screen, and poisons the incoming connections
to the files that drive it.  If a client ignores the signal and
continues to write to the window, it will get I/O errors.
If, on the other hand, all the processes in a window exit spontaneously,
they will automatically close all connections to the window.
8½ counts references to the window's files; when none are left,
it shuts down the window and removes it from the screen.
As a different example, when the user hits the DEL key to generate an
interrupt,
8½ writes a message to a special file, provided by Plan 9's
process control interface, that interrupts all the processes
in the window.
In all these examples, the implementation works seamlessly
across a network.
.PP
There are two valuable side effects of implementing
a window system by multiplexing
.CW /dev/cons
and other such files.
First, the problem of giving a meaningful
interpretation to the file
.CW /dev/cons
.CW /dev/tty ) (
in each window is solved automatically.
To provide
.CW /dev/cons
is the fundamental job of the window system, rather than just an awkward burden;
other systems must often make special and otherwise irrelevant arrangements for
.CW /dev/tty
to behave as expected in a window.
Second, any program that can access the server, including a
process on a remote machine, can access the files using standard
read and write system calls to communicate with the window system,
and standard open and close calls to connect to it.
Again, no special arrangements need to be made for remote processes to
use all the graphics facilities of 8½.
.SH
Graphical input
.PP
Of course 8½ offers more than ASCII I/O to its clients.
The state of the mouse may be discovered by reading the file
.CW /dev/mouse ,
which returns a ten-byte message encoding the state
of the buttons and the position of the cursor.
If the mouse has not moved since the last read of
.CW /dev/mouse ,
or if the window associated with the instance of
.CW /dev/mouse
is not the `input focus', the read blocks.
.PP
The format of the message is:
.DS
.CW 'm'
1 byte of button state
4 bytes of x, low byte first
4 bytes of y, low byte first
.DE
As in all shared data structures in Plan 9,
the order of every byte in the message is defined 
so all clients can execute the same code to unpack the message
into a local data structure.
.PP
For keyboard input, clients can read
.CW /dev/cons
or, if they need character-at-a-time input,
.CW /dev/rcons
(`raw console').
There is no explicit event mechanism to help clients that need to read
from multiple sources.
Instead, a small (365 line) external
support library can be used.
It attaches a process
to the various blocking input sources \(em mouse, keyboard, and perhaps
a third user-provided file descriptor \(em
and funnels their input into a single pipe from which may be read
the various types of
events in the traditional style.
This package is a compromise.  As discussed in a previous paper
[Pike89] I prefer
to free applications from event-based programming.  Unfortunately, though, I see
no easy way to achieve this in single-threaded C programs, and am unwilling
to require all programmers to master concurrent programming.
It should be noted, though, that even this compromise results in a small
and easily understood interface.  An example program that uses it is
given near the end of the paper.
.SH
Graphical output
.PP
The file
.CW /dev/screen
may be read by any client to recover the contents of the entire screen,
such as for printing (see Figure 1).
Similarly,
.CW /dev/window
holds the contents of the current window.
These are read-only files.
.PP
To perform graphics operations in their windows, client programs access
.CW /dev/bitblt .
It implements a protocol that encodes bitmap graphics operations.
Most of the messages in the protocol (there are 23 messages in all, about
half to manage the multi-level fonts necessary for efficient handling
of Unicode characters)
are transmissions (via a write)
from the client to the window system to perform a graphical
operation such as a
.CW bitblt
[PLR85] or character-drawing operation; a few include return information
(recovered via a read) to the client.
As with
.CW /dev/mouse ,
the
.CW /dev/bitblt
protocol is in a defined byte order.
Here, for example, is the layout of the
.CW bitblt
message:
.DS
.CW 'b'
2 bytes of destination id
2x4 bytes of destination point
2 bytes of source id
4x4 bytes of source rectangle
2 bytes of boolean function code
.DE
.KF
.ie h .html - <center><a href="8½.fig1.png"><img src="8½.fig1s.png"></a></center>
.el .BP fig1.ps 4.16 5.6 r 0 0
.EP
.IP
Figure 1.
A representative 8½ screen, running on a NeXTstation under Plan 9
(with no NeXT software).  In the upper right, a program announces the
arrival of mail.  In the top and left are a broswer for astronomical
databases and an image of a galaxy produced by the browser.
In the lower left there is a screen editor,
.CW sam
[Pike87],
editing Japanese text encoded in UTF,
and in the lower right an 8½ running recursively and, inside that instantiation,
a previewer for
.CW troff
output.
Underneath the faces is a small window running the command that
prints the screen by passing
.CW /dev/screen
to the bitmap printing utility.
.sp
.KE
.PP
The message is trivially constructed from the
.CW bitblt
subroutine in the library, defined as
.P1
void bitblt(Bitmap *dst, Point dp,
            Bitmap *src, Rectangle sr, Fcode c).
.P2
.PP
The `id'
fields in the message indicate another property of 8½:
the clients do not store the actual data for any of their bitmaps locally.
Instead, the protocol provides a message to allocate a bitmap, to be
stored in the server, and returns to the client an integer identifier,
much like a
UNIX
file descriptor, to be used in operations on that bitmap.
Bitmap number 0 is conventionally the client's window,
analogous to standard input for file I/O.
In fact, no bitmap graphics operations are executed in the client at all;
they are all performed on its behalf by the server.
Again, using the standard remote file operations in Plan 9,
this permits remote machines having no graphics capability, such
as the CPU server,
to run graphics applications.
Analogous features of the original Andrew window system [Gos86]
and of X [Sche86] require more complex mechanisms.
.PP
Nor does 8½ itself operate directly on bitmaps.
Instead, it calls another server to do its graphics operations for it,
using an identical protocol.
The operating system for the Plan 9 terminals contains an internal
server that implements that protocol, exactly as does 8½, but for a single
client.  That server stores the actual bytes for the bitmaps
and implements the fundamental bitmap graphics operations.
Thus the environment in which 8½ runs
has exactly the structure it provides for its clients;
8½ reproduces the environment for its clients,
multiplexing the interface to keep the clients separate.
.PP
This idea of multiplexing by simulation is applicable to more
than window systems, of course, and has some side effects.
Since 8½ simulates its own environment for its clients, it may run
in one of its own windows (see Figure 1).
A useful and common application of this
technique is to connect a window to a remote machine, such as a CPU
server, and run the window system there so that each subwindow is automatically
on the remote machine.
It is also a handy way to debug a new version of the window system
or to create an environment with, for example, a different default font.
.SH
Implementation
.PP
To provide graphics to its clients, 8½ mostly just multiplexes and passes
through to its own server the clients' requests, occasionally rearranging
the messages to maintain the fiction that the clients have unique screens
(windows).
To manage the overlapping windows it uses the layers model,
which is handled by a separate library [Pike83a].
Thus it has little work to do and is a fairly simple program;
it is dominated by a couple of switch statements to interpret
the bitmap and file server protocols.
The built-in window program and its associated menus and text-management
support are responsible for most of the code.
.PP
The operating system's server is also compact:
the version for the 68020 processor, excluding the implementation
of a half dozen bitmap graphics operations, is 2295 lines of C
(again, about half dealing with fonts);
the graphics operations are another 2214 lines.
.PP
8½ is structured as a set of communicating coroutines,
much as discussed in a 1989 paper [Pike89].
One coroutine manages the mouse, another the keyboard, and another
is instantiated to manage the state of each window and associated client.
When no coroutine wishes to run, 8½ reads the next file I/O request from
its clients, which arrive serially on the full-duplex communication pipe.
Thus 8½ is entirely synchronous.
.PP
The program source is small and compiles in about 10 seconds
in our Plan 9 environment.  There are ten source files and
one
.CW makefile
totaling 5100 lines.
This includes the source for the window management process,
the cut-and-paste terminal program,
the window/file server itself,
and a small coroutine library
.CW proc.c ). (
It does not include the layer library
(another 1031 lines)
or the library to handle the cutting and pasting of text
displayed in a window (960 lines),
or the general graphics support library that manages all the
non-drawing aspects of graphics \(em arithmetic on points and rectangles,
memory management, error handling, clipping, \(em plus fonts,
events, and non-primitive drawing operations such as circles and ellipses
(a final 3051 lines).
Not all the pieces of these libraries are used by 8½ itself;
a large part of the graphics library in particular is used only by clients.
Thus it is somewhat unfair to 8½ just to sum these numbers, including
the 4509 lines of support in the kernel, and arrive
at a total implementation size of 14651 lines of source to implement
all of 8½ from the lowest levels to the highest.
But that number gives a fair measure of the complexity of the overall system.
.PP
The implementation is also efficient.
8½'s performance is competitive to X windows'.
Compared using Dunwoody's and Linton's
.CW gbench
benchmarks on the 68020,
distributed with the ``X Test Suite'',
circles and arcs are drawn about half as fast in 8½ as in
X11 release 4 compiled with
.CW gcc
for equivalent hardware,
probably because they are currently implemented in a user library
by calls to the
.CW point
primitive.
Line drawing speed is about equal between the two systems.
Unicode text is drawn about the same speed by 8½ as ASCII text by
X, and
the
.CW bitblt
test is runs four times faster for 8½.
These numbers vary enough to caution against drawing sweeping
conclusions, but they
suggest that 8½'s architecture does not penalize its performance.
Finally, 8½ boots in under a second and creates a new window
apparently instantaneously.
.SH
An example
.PP
Here is a complete program that runs under 8½.
It prints the string
.CW \&"hello
.CW world"
wherever the left mouse button is depressed, and exits when the
right mouse button is depressed.
It also prints the string in the center of its window, and maintains
that string when the window is resized.
.P1
#include <u.h>
#include <libc.h>
#include <libg.h>

void
ereshaped(Rectangle r)
{
    Point p;

    screen.r = r;
    bitblt(&screen, screen.r.min, &screen, r, Zero); /* clear */
    p.x = screen.r.min.x + Dx(screen.r)/2;
    p.y = screen.r.min.y + Dy(screen.r)/2;
    p = sub(p, div(strsize(font, "hello world"), 2));
    string(&screen, p, font, "hello world", S);
}

main(void)
{
    Mouse m;

    binit(0, 0, 0);	/* initialize graphics library */
    einit(Emouse);	/* initialize event library */
    ereshaped(screen.r);
    for(;;){
        m = emouse();
        if(m.buttons & RIGHTB)
            break;
        if(m.buttons & LEFTB){
            string(&screen, m.xy, font, "hello world", S);
            /* wait for release of button */
            do; while(emouse().buttons & LEFTB);
        }
    }
}
.P2
The complete loaded binary is a little over 26K bytes on a 68020.
This program should be compared to the similar ones in the excellent paper
by Rosenthal [Rose88].
(The current program does more: it also employs the mouse.)
The clumsiest part is
.CW ereshaped ,
a function with a known name that is called from the event library
whenever the window is
reshaped or moved, as is discovered inelegantly but adequately
by a special case of a mouse message.
(Simple so-called expose events are not events
at all in 8½; the layer library takes care of them transparently.)
The lesson of this program, with deference to Rosenthal, is that if
the window system is cleanly designed a toolkit should be unnecessary
for simple tasks.
.SH
Status
.PP
As of 1992, 8½ is in regular daily use by almost all the 60 people in our
research center.  Some of those people use it to access Plan 9 itself; others
use it as a front end to remote
UNIX
systems, much as one would use an X terminal.
.PP
Some things about 8½ may change.
It would be nice if its capabilities were more easily accessible
from the shell.
A companion to this paper [Pike91] proposes one way to do this,
but that does not include any graphics functionality.
Perhaps a textual version of the
.CW /dev/bitblt
file is a way to proceed; that would allow, for example,
.CW awk
programs to draw graphs directly.
.PP
Can this style of window system be built on other operating systems?
A major part of the design of 8½ depends on its structure as a file server.
In principle this could be done for any system that supports user processes
that serve files, such as any system running NFS or AFS [Sun89, Kaza87].
One requirement, however, is 8½'s need
to respond to its clients' requests out of order:
if one client reads
.CW /dev/cons
in a window with no characters to be read,
other clients should be able to perform I/O in their windows, or even
the same window.
Another constraint is that the 8½ files are like devices,
and must not be cached by the client.
NFS cannot honor these requirements; AFS may be able to.
Of course, other interprocess communication mechanisms such as sockets
could be used as a basis for a window system.  One may even argue that
X's model fits into this overall scheme.  It may prove easy and worthwhile
to write a small 8½-like system for commercial
UNIX
systems to demonstrate that its merits can be won in systems other than
Plan 9.
.SH
Conclusion
.PP
In conclusion, 8½ uses an unusual architecture in
concert with the file-oriented interprocess communication of Plan 9
to provide network-based interactive graphics to client programs.
It demonstrates that even production-quality window systems are not
inherently large or complicated
and may be simple to use and to program.
.SH
Acknowledgements
.PP
Helpful comments on early drafts of this paper were made by
Doug Blewett,
Stu Feldman,
Chris Fraser,
Brian Kernighan,
Dennis Ritchie,
and Phil Winterbottom.
8½'s support for color was added by Howard Trickey.
Many of the ideas leading to 8½ were tried out in earlier, sometimes less
successful, programs.  I would like to thank those users who suffered
through some of my previous 7½ window systems.
.SH
References
.LP
[Duff90] Tom Duff, ``Rc - A Shell for Plan 9 and UNIX systems'', Proc. of the Summer 1990 UKUUG Conf., London, July, 1990, pp. 21-33, reprinted, in a different form, in this volume.
.LP
[Far89] Far too many people, XTERM(1), Massachusetts Institute of Technology, 1989.
.LP
[Gos86] James Gosling and David Rosenthal,
``A window manager for bitmapped displays and UNIX'', in Methodology of Window Management, edited by F.R.A. Hopgood et al., Springer, 1986.
.LP
[Kaza87] Mike Kazar, ``Synchronization and Caching issues in the Andrew File System'', Tech. Rept. CMU-ITC-058, Information Technology Center, Carnegie Mellon University, June, 1987.
.LP
[Kill84] Tom Killian, ``Processes as Files'', USENIX Summer Conf. Proc., Salt Lake City June, 1984.
.LP
[Pike83] Rob Pike, ``The Blit: A Multiplexed Graphics Terminal'', Bell Labs Tech. J., V63, #8, part 2, pp. 1607-1631.
.LP
[Pike83a] Rob Pike, ``Graphics in Overlapping Bitmap Layers'', Trans. on Graph., Vol 2, #2, 135-160, reprinted in Proc. SIGGRAPH '83, pp. 331-356.
.LP
[Pike87] Rob Pike, ``The Text Editor \f(CWsam\fP'', Softw. - Prac. and Exp., Nov 1987, Vol 17 #11, pp. 813-845, reprinted in this volume.
.LP
[Pike88] Rob Pike, ``Window Systems Should Be Transparent'', Comp. Sys., Summer 1988, Vol 1 #3, pp. 279-296.
.LP
[Pike89] Rob Pike, ``A Concurrent Window System'', Comp. Sys., Spring 1989, Vol 2 #2, pp. 133-153.
.LP
[Pike91] Rob Pike, ``A Minimalist Global User Interface'', USENIX Summer Conf. Proc., Nashville, June, 1991.
.LP
[Pike92]  Rob Pike, Dave Presotto, Ken Thompson, Howard Trickey, and Phil Winterbottom,
Operating Systems Review
Vol 27, #2, Apr 1993, pp. 72-76
(reprinted from Proceedings of the 5th ACM SIGOPS European Workshop, Mont Saint-Michel, 1992, Paper nº 34, and reprinted in this volume).
.LP
[Pike94] Rob Pike and Ken Thompson, ``Hello World or Καλημέρα κόσμε or \f(Jpこんにちは 世界\fP'', USENIX Winter Conf. Proc., San Diego, Jan, 1993, reprinted in this volume.
.LP
[PLR85] Rob Pike, Bart Locanthi and John Reiser, ``Hardware/Software Tradeoffs for Bitmap Graphics on the Blit'', Softw. - Prac. and Exp., Feb 1985, Vol 15 #2, pp. 131-152.
.LP
[Pres90] David L. Presotto and Dennis M. Ritchie, ``Interprocess Communication in the Ninth Edition Unix System'', Softw. - Prac. and Exp., June 1990, Vol 20 #S1, pp. S1/3-S1/17.
.LP
[Rose88] David Rosenthal, ``A Simple X11 Client Program -or- How hard can it really be to write ``Hello, World''?'', USENIX Winter Conf. Proc., Dallas, Jan, 1988, pp. 229-242.
.LP
[Sche86] Robert W. Scheifler and Jim Gettys,
``The X Window System'',
ACM Trans. on Graph., Vol 5 #2, pp. 79-109.
.LP
[Sun89] Sun Microsystems, NFS: Network file system protocol specification,
RFC 1094, Network Information Center, SRI International, March, 1989.
.br
