.TL
README
.DA
.AU
Brian W. Kernighan
.AI
.MH
.PP
This brief document is intended to help you get started using Plan 9.
It is written by a casual Plan 9 user
who is not in any way part of the Plan 9 group,
and is aimed at ordinary users with a Unix background.
.NH
Getting Started
.PP
I'm assuming that you or someone you trust has read,
understood, and performed the instructions in the Plan 9 installation
procedure.
I'm also assuming that you have at least looked at the Plan 9 overview
paper in Computing Science Technical Report 158,
which explains what Plan 9 is and how it goes about it.
There are also some helpful explanations and examples in
the paper ``The Use of Name Spaces in Plan 9,''
which is also part of the distribution.
I am further assuming that you have the Plan 9 manual near to
hand, and are willing to read manual pages for commands
as their names appear here.
.PP
How you get started after you turn
your terminal on depends on how your Plan 9 system is set up;
the details are different for a standalone system or a terminal
connected to a file server and a CPU server.
The latter is mostly what I'll talk about.
.PP
When Plan 9 boots, it runs a very dumb terminal program,
so dumb that it doesn't even know how to scroll a screen.
You won't want to use that for much of anything.
In much the same way that
.CW .profile
is executed by the shell on Unix systems,
the file
.CW lib/profile
is executed by the shell on Plan 9
when you log in.
The
.CW newuser
command normally creates a few directories
.CW bin , (
.CW bin/rc ,
.CW lib ,
and
.CW tmp ),
then sets up a profile 
.CW lib/profile
that looks like this, though with 
some more frills:
.P1
bind -a $home/bin/rc /bin
font = /lib/font/bit/pelm/euro.9.font
switch($service){
case terminal
	prompt=('term% ' '	')
	exec 8½
case cpu
	bind -b /mnt/term/mnt/8½ /dev
	prompt=('cpu% ' '	')
}
.P2
Many of the interesting bits of Plan 9 are implicit
in this file.
.NH
Commands
.PP
Most of the standard Unix commands exist in almost the same form
on Plan 9; this includes standbys like 
.CW cat ,
.CW ls ,
.CW cd ,
.CW pwd ,
.CW cp ,
.CW mv ,
.CW diff ,
.CW grep ,
.CW awk ,
and so on.
You'll notice minor differences in behavior but for the most part
you don't have to think about this.
.NH
The rc Shell
.PP
One big difference:
Plan 9 uses a different shell, called
.CW rc .
For running commands interactively, it's almost the same as
the Bourne shell, so filename metacharacters like
.CW *
and
.CW ?
behave the same, and simple redirections with
.CW >
and
.CW >>
are the same (but not redirection of streams by number).
Quoting is simpler than in the Bourne shell:
double quotes and backslash have no special meaning,
and single quotes quote anything.
To get a single quote into a quoted string, double the quote:
.P1
echo ''''
.P2
prints a single quote.
.PP
For programming, 
.CW rc
is almost unrelated, which is a nuisance.
All shell scripts have to begin with
.P1
#!/bin/rc
.P2
.PP
Environment variables are set by
.P1
var = '\f2anything\fP'
.P2
where the quotes can be omitted if 
.I anything 
contains no spaces.
Environment variables are accessed as
.CW $var ;
certain variables are initialized when your process begins,
including
.CW user 
(your name),
.CW home
(your home directory), and
.CW service ,
which is
.CW terminal
when you are running on your terminal.
.NH
Directories and Search Paths
.PP
One of the unifying ideas in Plan 9 
is that all resources are accessed as file systems.
Central to this is management of the name space so that
you can select and arrange the resources you want to use.
.PP
The
.CW bind
and
.CW mount
commands manipulate the name space.
In particular, the command
.P1
bind -a $home/bin/rc /bin
.P2
in the profile binds the directory
.CW $home/bin/rc
after
the directory 
.CW /bin ,
forming a
.I "union directory.
(More precisely, it makes
.CW /bin
an alias for the union).
The shell searches only
.CW /bin
for commands to run, but it searches all the directories
that have been unioned together.
By convention, your personal shell scripts are placed in 
.CW $home/bin/rc .
.PP
When you first log in, several directories are bound to
.CW /bin ,
including
.CW /rc/bin ,
which contains shell scripts, and
.CW /$cputype/bin ,
which contains binaries for your current cpu type.
.CW cputype 
is the type of processor you are running on,
typically one of
.CW 68020 ,
.CW sparc ,
.CW mips ,
or
.CW 386 .
When you run a program like
.CW ls ,
the version for your current cpu type will be found and executed.
If you subsequently execute the
.CW cpu
command to access a cpu server, in that process and those
started by it,
.CW cputype
will be the type of the cpu server, and the
.CW ls
command you run there will be the right binary for that cpu.
.PP
If you think about it, you can see that this mechanism of
union directories replaces the search path of conventional
Unix shells.
As far as you are concerned,
.I all
executable programs are in
.CW /bin .
Try
.P1
lc /bin
.P2
to see the names of all executable programs.
.NH
Interesting File Systems
.PP
Unix users are familiar with the idea that devices like
disks and tapes are part of the file system.
Plan 9 carries that idea a lot further.
If you look at the directory
.CW /dev ,
you will see some familiar names.
Try
.P1
cat /dev/time
.P2
a couple of times, for example.
Or, after you have snarfed some text using the button 2 menu item,
try
.P1
cat /dev/snarf
.P2
Note that some files like
.CW cons
and
.CW mouse
occur more than once.
.CW /dev
is a union directory, and these are multiple occurrences of the same file.
The first
.CW /dev/mouse
refers to the current window, and the next one to the
enclosing window (which is probably the whole screen at this point).
Try
.P1
cat /dev/mouse
.P2
then move the mouse around inside and outside of the current window.
.PP
The shell environment is kept in a directory called
.CW /env ;
each shell environment variable is stored in a file.
Try
.P1
cat /env/font
.P2
for example.
.PP
Running processes are found in
.CW /proc ;
each process is a directory, and each file in that directory
accesses some aspect of the process.
For example, the
.CW status
file contains (all ASCII) status information about the process.
Try
.P1
awk '{print $1}' /proc/*/status
.P2
to get a list of the names of the running processes.
.PP
You might also find it interesting to poke around
in 
.CW /net ;
all network connections are managed as file systems as well.
In all of these cases, the service presents a file-system
interface to its clients, although the implementation behind
is not generally a traditional file system.
.NH
The 8½ Window System
.PP
The window system for Plan 9 is called 8½;
the
.CW terminal
case in the profile starts 8½ with the line
.P1
exec 8½
.P2
8½ provides much less of the ``flexibility'' and certainly
far fewer features than
.CW xterm
on X terminals, but I much prefer it.
.PP
The most important difference is that 8½ treats all text
on the screen alike; with the mouse you can edit anything you
can see, so you can fix up and resubmit commands,
fiddle the output of a program or its input and resubmit it,
and so on.
This ability to edit the past is liberating
to such a degree that once you use it, you'll never
want to go back to something like
.CW xterm .
(There is an implementation of an X server so you can
run it if you want, but I never have.)
.PP
8½ does not provide any of the terminal-handling mechanisms like
termcap or curses.
That means that there is no support for (nor indeed implementation of)
old favorites like
.CW vi ,
.CW emacs ,
and
.CW ksh .
In my experience, the ability to cut, paste, and edit what's
on all the windows on the screen obviates the need for many
of the mechanisms in these programs,
so it's not as big a loss as you might think.
.PP
8½ can be called recursively: you can make a new window, 
run 8½ in it, and
everything you do there is insulated from the surroundings.
8½ does not provide any analog of the virtual window management of,
for example, VTWM, nor does it provide zillions of
(or even a few) icons, but you can move a window almost off the screen,
and you can hide it and then recall it from a popup menu.
.PP
You can also give 8½ a file of commands to run when it starts,
which most people put into their profile:
.P1
exec 8½ -i lib/windows
.P2
Normally this is used to set up windows that you always use:
.P1
#!/bin/rc
window '\f2x0 y0 x1 y1\fP' \f2command line\fP
\&\f2...\fP
.P2
where
.I x0,y0
and
.I x1,y1
are the coordinates of the window in question (and
.I x
increases down the screen).
The
.CW window
command opens a window at the specified place,
then runs the command in it.
The command
.CW wloc
will tell you the names and locations of all windows in the right format
to be inserted directly in a file.
Set up the windows and programs the way you want them,
then run
.CW wloc
and snarf its output.
.NH
Fonts
.PP
One aspect of 8½ that you can change is the font it uses
for displaying text.
There is a default font, but normally the variable
.CW font
is set explicitly in the profile:
.P1
font = /lib/font/bit/pelm/euro.9.font
8½ -f $font
.P2
The font
.CW euro.9.font
is a collection of almost any character you might find
in European languages, including Cyrillic, Greek,
and a bunch of special characters.
There are other fonts that include oriental languages as well,
and a variety of sizes.
.PP
Plan 9 uses the Unicode character set throughout,
which means that the system and the various programs all
deal pretty comfortably with a very large character set.
(Think 16 bits, or 65K characters.)
So if you want to edit files in languages that
use more than ASCII characters, or run
.CW grep
or
.CW awk
over them, it just works.
(You may have trouble printing such characters on standard printers,
but they will appear fine on the screen.)
.NH
Editing
.PP
The standard Plan 9 editor is called
.CW sam ;
it's a particularly good multi-file editor,
it provides regular expression syntax
the same as in the venerable
.CW ed
(which also exists), and you can snarf text from one of its
windows and paste it into other 8½ windows or vice versa.
The mouse idioms for
.CW sam
and
.CW 8½
are the same.
It will also edit files on other systems if there is
a network connection.
.PP
By the way, regular expressions have been cleaned up --
all programs support the same regular expressions,
which are pretty close to those found in
.CW egrep
on Unix systems.
.NH
The CPU Server
.PP
In the Plan 9 world view one is meant to run interactive programs
like editors on the terminal and compute-intensive programs
like compilers on a cpu server,
which runs faster and has a higher bandwidth to the file server.
The
.CW cpu
command connects you to a cpu server
so your computation runs faster (in theory), but everything else
stays the same.
The mechanism is quite different from either
remote login (which does not preserve the name space you are
currently working in) or network file system access
(which does not change the processor).
The line
.P1
	bind -b /mnt/term/mnt/8½ /dev
.P2
in your profile arranges that all the devices (including mouse,
keyboard and screen) associated with your terminal
are inherited by the cpu server so they continue to work
in a cpu window.
.NH
Connecting to Unix Systems
.PP
It is highly likely that your Plan 9 system will be
connected by some network to a Unix system.
The command
.CW con
connects to another system (typically Unix);
the command
.CW rx
is rather like the
.CW rsh
command
on Unix systems, for executing a single command on another machine.
.PP
If the Unix system cooperates, it is also possible to mount
a Unix file system in the Plan 9 name space so that files
on the Unix side are accessible from Plan 9.
The command
.P1
9fs \f2machine\fP
.P2
establishes the connection and mounts the files;
thereafter the root of the target file system is
in the Plan 9 directory at
.CW /n/\f2machine\fP .
.PP
There is no connection from Unix to Plan 9;
security is taken seriously (no superuser, for example) and it's
hard to provide a satisfactory mechanism.
.NH
Backup and Recovery
.PP
Normally the state of the Plan 9 file system is recorded every day
or so; on our system, it's stored on an optical disk.
If your Plan 9 system is suitably equipped, you should
be able to run another service that makes the past
state of the file system accessible (read only).
The command
.P1
9fs dump
.P2
mounts this file system on
.CW /n/dump .
At that point, I can
.CW cd
into the past:
.P1
cd /n/dump/1991/0401/usr/bwk
ls -l
.P2
puts me in my directory as it was on April 1, 1991.
This really is a file system, so all the normal commands
work fine; I can
.CW diff
a file from then with one on some other date,
or copy an old version to the present.
Plan 9 has no backup or recovery programs; this mechanism subsumes
them all.
.NH
Programming in Plan 9
.PP
Most programming in Plan 9 is done in ANSI C,
with the usual supporting tools like YACC available.
One difference of note:
.CW make
has been largely supplanted by
.CW mk ,
which is cleaner but different.  As with the shell, it takes
time to internalize the differences.
.PP
There are separate C compilers for each supported cpu type
(badly named with a single letter mnemonic),
but each compiler can produce code for any object type.
The
.CW mkfile
normally encapsulates this.
.PP
Although ANSI C is supported, the Plan 9 libraries are not
ANSI and the standard ANSI header files normally are not found.
Compiling C programs is different enough that you should
read the paper called ``How to Use the Plan 9 C Compiler''
before starting.
.PP
If you are importing or exporting a C program, you will want
to use the ANSI/POSIX environment (``ape''), which really does provide
for portability, including a complete set of POSIX-compatible 
libraries and some POSIX tools.
The compiler driver is called
.CW pcc .
The commands
.P1
bind -a /bin/ape /bin
bind -a /rc/bin/ape /bin
.P2
will bind the right files.
.NH
Envoi
.PP
Plan 9 is not Unix.
If you think of it as Unix, you'll often be frustrated
because something doesn't exist or works differently.
If you think of it as Plan 9, however, you'll find that
most of it works very smoothly, and that there are
some really neat ideas that make things much cleaner
than you have seen before.
