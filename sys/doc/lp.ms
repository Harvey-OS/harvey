.HTML "A Guide to the Lp Printer Spooler
.TL
A Guide to the Lp
Printer Spooler
.AU
Paul Glick
pg@plan9.bell-labs.com
.AB
.PP
.I Lp
is a collection of programs used to provide an easy-to-use
interface for printing a variety of document types on a variety
of printers.
.I Lp
is the glue that connects various document language
translators and printer communication programs together so that
the users may have a consistent view of printers.
Most of the glue
is shell script, which can be easily modified.
The user need not
specify options to get sensible output in most cases.
.I Lp
is described here
so that others may make additions and changes.
.AE
\" .2C
.NH
Introduction
.PP
.I Lp
is used to format and print data on a variety of output devices.
The need for
.I lp
was rooted in the inability of other printer spoolers to do simple
tasks without a great deal of user specification of options.
At the time
.I lp
was written, there were several printer
languages, such as ImPress and PostScript, and
an internally developed printer that would accept
.I troff
output.
Now, all our printers take PostScript,
but printers that use HPCL and HPGL abound and
support for those printers may be added easily.
A great deal of what underlies
.I lp
is taken from BSD's
.I lpr
and System V's
.I lp .
The important features of this system are that most of the programs
are easily modified shell scripts and the user need not
learn to use the large amount of underlying software developed by others.
.I Lp
runs under Plan 9 and several flavors of
UNIX.
This document deals with
.I lp
as it relates to Plan 9.
.I Lp
was developed using both Datakit and Ethernet to transport data between machines.
Now only the Ethernet transport mechanism remains.
.PP
Text, graphics, and formatted text files are appropriately processed and
placed into a spool directory from which they are taken to be printed by a daemon process.
Additional functions include checking the status of a printer queue
and removing jobs from the printer queue.
.PP
All the shell scripts (see
.I rc (1))
associated with
.I lp
reside in the spool directory
.CW /sys/lib/lp
except for the
.I lp
command itself, which resides in
.CW /rc/bin .
Commands related to
.I lp
that are not shell scripts can most often be found
in
.CW /$cputype/bin/aux .
The directory where all the
.I lp
scripts  reside is defined within
.I lp
by the shell variable
.CW LPLIB .
In the remainder of this document, file names will be specified
with this shell variable as their root.
.NH
Usage
.PP
.I Lp
requires an output device to be specified
before it will process input.
This can be done in any of three ways described here.
.IP 1)
The file
.CW $LPLIB/defdevice
may contain the name of a default output device.
This may not be practical for environments where
there are many printers.
.IP 2)
The user's environment variable
.CW LPDEST
may be set to the name of the device to be used.
This is often a more practical solution when there are several printers
available.
This overrides a
.CW defdevice
specification.
.IP 3)
The
.CW -d
.I printer
option to the
.I lp
command specifies
.I printer
as the device to which output should be directed, overriding the
previous two specifications.
.PP
.ti 0
If
.I printer
is
.CW ? ,
a list of printers and other information in the
.CW devices
file is printed, as shown in Figure 1.
Quote the question mark to prevent it from being
interpreted by the shell language as a metacharacter.
\" .1C
.KF
.P1
% lp -d'?'
device   location  host             class
fn       2C-501    helix            post/2+600dpi+duplex
pcclone  -         -                post+nohead
peacock  2C-501    cetus            post/2+300dpi+nohead+color
ps83     st8_fl3   rice             post+300dpi+reverse
psu      2C-501    cetus            post/2+1200dpi
     .
     .
     .
%
.P2
.ce
.I "Figure 1.  Sample listing of installed printers"
.KE
.PP
Normally,
.I lp
uses the
.CW file
command to figure out what type of input it is receiving.
This is done within the
.CW generic
process which is discussed later in this paper in the
.B "Process directory"
section.
To select a specific input processor the
\f(CW-p\fP\fIprocess\fP
option is used where
.I process
is one of the shell scripts in the
.CW process
directory.
.LP
Troff
output can be printed, in this case, on printer
.I fn
with
.P1
% troff -ms lp.ms | lp -dfn
.P2
.LP
A file can be converted to PostScript using the pseudo-printer
.CW stdout :
.P1
% troff -ms lp.ms | lp -dstdout > lp.ps
.P2
LaTeX (and analogously TeX)
documents are printed in two steps:
.P1
% latex lp.tex
     .
     .
% lp lp.dvi
     .
     .
%
.P2
LaTeX
produces a `.dvi' file and
does not permit the use of a pipe
connection to the standard input of
.I lp .
To look at the status and queue of a device, use
.CW -q :
.P1
% lp -dpsu -q
daemon status:
:  67.17% sent
printer status:
%%[ status: busy; source: lpd ]%%

queue on cetus:
job		user	try	size
rice29436.1	pg	0	17454
slocum17565.1	ches	1	49995
%
.P2
This command can print the status and queue of the local
and remote hosts.
Administrators should be advised that working in an environment where the
.I lp
spool directory is shared among the local and remote hosts,
no spooling should be done on the local hosts.
The format of the status and queue printout is up to the administrator.
The job started above can be killed with
.CW -k :
.P1
$ lp -dpsu -k rice29436.1
rice29436.1 removed from psu queue on cetus
.P2
.NH
Options
.PP
There are options available to modify the way in which a job is handled.
It is the job of the
.I lp
programs to convert the option settings so they may be used by each of the
different translation and interface programs.
Not all options are applicable to all printer environments.
Table 1 lists the standard
.I lp
options, the shell variable settings, and description of the options.
\" .1C
.KF
.sp
.in 0
.TS
center;
c | c s s | c
c | c c c | c
lfCWp-2 | lfCWp-2 cfCWp-2 cfCWp-2 | lp-2w(3i).
=
option	shell variable	action
\^	name	default	set	\^
_
-D	DEBUG	N	1	turn on debugging mode.
_
-H	NOHEADER	N	1	suppress header page.
_
-L	LAND	N	1	make long page dimension horizontal.
_
-M \fImach\fP	LPMACHID	N	\fImach\fP	set the source machine name.
_
-Q	QONLY	N	1	do not execute daemon; for debugging.
_
-c \fIn\fP	COPIES	N	\fIn\fP	number of copies to be printed.
_
-d \fIprinter\fP	LPDEST	U	\fIprinter\fP	set job destination; override other settings.
_
-f \fIfont.pt\fP	FONT	N	\fIfont\fP	set font style and point size for printing.
	POINT	N	\fIpt\fP
_
-i \fIn\fP	IBIN	N	\fIn\fP	T{
select input paper tray options.
The argument given is dependent on the printer type.
A number can be given to select a particular tray and/or
.CW simplex
or
.CW duplex
may be used to get single or double sided output, where
applicable.
Multiple options should be separated by commas.
T}
_
-k	KILLFLAG	0	1	T{
take non-option arguments as job numbers to be removed from queue.
T}
_
-l \fIn\fP	LINES	N	\fIn\fP	T{
for printed data, the number of lines per logical page.
T}
_
-m \fIf\fP	MAG	N	\fIf\fP	T{
magnify the image by a factor \fIf\fP.
The factor should be a positive real number.
T}
_
-n \fIn\fP	NPAG	N	\fIn\fP	T{
put \fIn\fP logical pages on a single physical page.
A simple algorithm is used to pack the pages.
T}
_
-o \fIlist\fP	OLIST	N	\fIlist\fP	T{
print only those pages specified in the list.
The list may be a sequence of numbers or ranges separated by commas.
A range is a pair of numbers separated by a hyphen.
T}
_
-p \fIproc\fP	LPPROC	L	\fIproc\fP	T{
use the preprocessor \fIproc\fP instead of the preprocessor given
in the
.CW devices
file for this printer.
T}
_
-q	LPQ	N	1	T{
print the status and queue.
T}
_
-r	REVERSE	L	1	T{
this toggles the
.CW REVERSE
flag, changing whether or not page reversal should occur in preprocessing.
Page reversal is needed if a printer delivers pages face up.
The keyword
.CW reverse
can be placed in the
.I lpclass
field of the
.CW devices
file.
If a document has already been processed this flag has no effect.
T}
_
-u \fIuser\fP	LPUSERID	U	\fIuser\fP	T{
change the user id that appears on the cover page.
T}
_
-x \fIoffset\fP	XOFF	N	\fIoffset\fP	T{
move the image \fIoffset\fP inches to the right.
A negative \fIoffset\fP will move the image to the left.
The \fIoffset\fP may be any reasonable real number.
T}
_
-y \fIoffset\fP	YOFF	N	\fIoffset\fP	T{
same as for
.CW -x
except a positive offset will move the image down.
T}
_
.T&
l l cp-2 lp-2 s
l l cfCWp-2 lp-2 s.
.vs -2p

		default	setting definition
		N	set to the null string (`') initially in \fIlp\fP.
		L	set from printer entry in \f(CW\\s-\\n(XPdevices\\s+\\n(XP\fP file.
		U	set from the user's environment.
.vs +2p
.TE
.sp
.ce
.I "Table 1. Lp Option List"
.sp
.ll \\n(LLu
.KE
\" .2C	
.NH
Devices file
.PP
The
.CW devices
file is found in the spool directory.
Each line in the file is composed of 12 fields, separated
by tabs or spaces, that describe the attributes
of the printer and how it should be serviced.
Within the
.CW lp
command, a shell variable is set for each attribute;
the following list describes them:
.IP "\f(CW\s-\n(XPLPDEST\s+\n(XP\fP " 12
is the name of the device as given to
.I lp
with the
.CW -d
option
or as specified by the shell environment variable
.CW LPDEST
or as specified by
the file
.CW $LPLIB/defdevice .
This name is used in creating directories and log files that are associated with
the printers operation.
.IP "\f(CW\s-\n(XPLOC\s+\n(XP\fP "
just describes where the printer is physically located.
.IP "\f(CW\s-\n(XPDEST_HOST\s+\n(XP\fP "
is the host from which the files are printed.
Files may be spooled on other machines before being transferred to the
destination host.
.IP "\f(CW\s-\n(XPOUT_DEV\s+\n(XP\fP "
is the physical device name or network address needed by the printer daemon
to connect to the printer.
This field depends on the requirements of the daemon and may contain a `\(en'
if not required.
.IP "\f(CW\s-\n(XPSPEED\s+\n(XP\fP "
is the baud rate setting for the port.
This field depends on the requirements of the daemon and may contain a `\(en'
if not required.
.IP "\f(CW\s-\n(XPLPCLASS\s+\n(XP\fP "
is used to encode minor printer differences.
The keyword
.CW reverse
is used by some of the preprocessors
to reverse the order the pages are printed to accommodate different output
trays (either face up or face down).
The keyword
.CW nohead
is used to suppress the header page.
This is used for special and color printers.
The keyword
.CW duplex
is used to coax double sided output from duplex printers.
.IP "\f(CW\s-\n(XPLPPROC\s+\n(XP\fP "
is the command from the
.CW LPLIB/process
directory to be used to convert input to a format
that will be accepted by the device.
The preprocessor is invoked by the spooler.
.IP "\f(CW\s-\n(XPSPOOLER\s+\n(XP\fP "
is the command from the
.CW LPLIB/spooler
directory which will select files using the
.CW SCHED
command and invoke the
.CW LPPROC
command, putting its output
into the remote spool directory.
The output is sent directly to the spool directory on the
destination machine to avoid conflicts when client and
server machines share spool directories.
.IP "\f(CW\s-\n(XPSTAT\s+\n(XP\fP "
is the command from the
.CW LPLIB/stat
directory that prints the status of the device and the list of jobs
waiting on the queue for the device.
The status information depends on what is available from the printer
and interface software.
The queue information should be changed to show information
useful in tracking down problems.
The
.CW SCHED
command is used to show the jobs in the order
in which they will be printed.
.IP "\f(CW\s-\n(XPKILL\s+\n(XP\fP "
is the command from the
.CW LPLIB/kill
that removes jobs from the queue.
The jobs to be removed are given as arguments to the
.I lp
command.
When possible, it should also abort the currently running job
if it has to be killed.
.IP "\f(CW\s-\n(XPDAEMON\s+\n(XP\fP "
is the command from the
.CW LPLIB/daemon
that is meant to run asynchronously to remove
jobs from the queue.
Jobs may either be passed on to another host or sent to the
printing device.
.I Lp
always tries to start a daemon process when one is specified.
.IP "\f(CW\s-\n(XPSCHED\s+\n(XP\fP "
is the command from the
.CW LPLIB/sched
that is used to present the job names to the
daemon and stat programs
in some order, e.g., first-in-first-out, smallest first.
.NH
Support programs
.PP
The following sections describe the basic functions of the programs
that are found in the subdirectories of
.CW $LPLIB .
The programs in a specific directory vary with the
type of output device or networks that have to be used.
.NH 2
Process directory
.PP
The
.CW generic
preprocessor
is the default preprocessor for most printers.
It uses the
.I file (1)
command to determine the format of the input file.
The appropriate preprocessor is then selected to transform the
file to a format suitable for the printer.
.PP
Here is a list of some of the preprocessors and
a description of their function.
A complete list of preprocessors and their descriptions can be found in the manual page
.I lp (8).
.sp
.IP \f(CWdvipost\fP 14
Converts TeX or LaTeX output (\f(CW.dvi\fP files) to PostScript
.IP \f(CWppost\fP
Converts UTF text to PostScript.
The default font is Courier with Lucida fonts filling in
the remainder of the (available) Unicode character space.
.IP \f(CWtr2post\fP
Converts (device independent) troff output for the device type
.CW utf .
See
.CW /sys/lib/troff/font/devutf
directory for troff font width table descriptions.
See also the
.CW /sys/lib/postscript/troff
directory for mappings of
troff
.CW UTF
character space to PostScript font space.
.IP \f(CWp9bitpost\fP
Converts Plan 9 bitmaps (see
.I bitfile (9.6))
to PostScript.
.IP \f(CWg3post\fP
Converts fax (CCITT-G31 format) to PostScript.
.IP \f(CWhpost\fP
Does header page processing and page reversal processing, if
necessary.
Page reversal is done here so the header page always comes
out at the beginning of the job.
Header page processing is very location-dependent.
.NH 2
Spool directory
.PP
The
.CW generic
spooler is responsible for executing the preprocessor
and directing its output to a file in the printer's queue.
An additional file is created containing information such as the system name,
user id, job number, and number of times this job was attempted.
.PP
Certain printer handling programs do not require separate
preprocessing and spooling.
For such circumstances a
.CW nospool
spooler is available that just executes the preprocessing program.
The processing and spooling functions are assumed by this program and the output is sent to
.CW OUT_DEV
or standard output if
.CW OUT_DEV
is '-'.
.PP
The
.CW pcclone
spooler is used to send print jobs directly to a printer connected
to a 386 compatible printer port (See
.I lpt (3)).
.NH 2
Stat directory
.PP
The function of the shell scripts in the
.CW stat
directory is to present status information about the
printer and its queue.
When necessary, the
.CW stat
scripts may be designed
to return information about the local queue as well as the remote queue.
This is not done on Plan 9 because many systems share the same queue directory.
The scheduler is used to print the queue in the order in which the jobs
will be executed.
.NH 2
Kill directory
.PP
The
.CW kill
scripts receive command line arguments passed to them by
.I lp
and remove the job and id files which match the arguments
for the particular queue.
When a job is killed, the generic kill procedure:
.IP 1)
kills the daemon for this queue if the job being killed
is first in the queue,
.IP 2)
removes the files associated with the job from the queue,
.IP 3)
attempts to restart the daemon.
.NH 2
Daemon directory
.PP
The
.CW daemon
shell scripts are the last to be invoked by
.I lp
if the
.CW -Q
option has not been given.
The daemon process is executed asynchronously
with its standard output and standard error appended to
the printer log file.
The log file is described in a subsequent section.
Because the daemon runs asynchronously, it must
catch signals that could cause it to terminate abnormally.
The daemon first checks to see that it is the only one running
by using the
.CW LOCK
program found in the
.CW /$cputype/bin/aux
directory.
The
.CW LOCK
command creates a
.CW LOCK
file in the printer's queue directory.
The daemon then executes the scheduler to obtain the name of the
next job on the queue.
.PP
The processing of jobs may entail transfer to another host
or transmission to a printer.
The details of this are specific to the individual daemons.
If a job is processed without error, it is removed from the queue.
If a job does not succeed, the associated files may be
moved to a printer specific directory in
.CW $LPLIB/prob .
In either case, the daemon can make an entry in the printer's
log file.
Before exiting, the daemon should clean up lock files by calling
.CW UNLOCK .
.PP
Several non-standard daemon programs have been designed
to suit various requirements and whims.
One such program announces job completion and empty paper trays
by causing icons to appear in peoples'
.CW seemail
window.
Another, using a voice synthesizer, makes verbal announcements.
Other daemons may be designed to taste.
.NH 2
Sched directory
.PP
The scheduler must decide which job files should be executed and
in what order.
The most commonly used scheduler program is
.CW FIFO ,
which looks like this:
.P1
ls -tr $* | sed -n -e 's/.*  *//' \e
  -e '/^[0-9][0-9]*\.[1-9][0-9]*$/p'
.P2
This lists all the job files in this printer's queue in modification
time order.
Jobs entering the queue have a dot (.) prefixed to their name
to keep the scheduler from selecting them before they are complete.
.NH
Where Things Go Wrong
.PP
There are four directories where
.I lp
writes files.
On the Plan 9 release these directories may be found
in a directory on a scratch filesystem that is not
backed-up.
This directory is
.CW /n/emelieother/lp .
It is built on top of a file system
.CW other
that is mounted on the file server
.CW emelie .
The four directories in
this scratch directory
are
.CW log ,
.CW prob ,
.CW queue ,
and
.CW tmp .
.I Lp
binds (see
.I bind (1))
the first three into the directory
.CW /sys/lib/lp
for its processes and their children.
The
.CW tmp
directory is bound to the
.CW /tmp
directory so that the lp daemons, which run as user `none',
may write into this directory.
.PP
On any new installation, it is important that these directories
be set up and that the
.I /rc/bin/lp
command be editted to reflect the change.
If you do not have a scratch filesystem for these directories,
create the four directories
.CW log ,
.CW prob ,
.CW queue ,
and
.CW tmp
in
.CW $LPLIB
.CW (/sys/lib/lp)
so that they are writable by anyone.
.NH 2
Log directory
.PP
The log files for a particular
.I printer
appear in a subdirectory of the spool directory
\f(CWlog\fP/\fIprinter\fP.
There are currently two types of log files.
One is for the daemon to log errors and successful completions
of jobs.
These are named
.I printer.day
where
.I day
is the three letter abbreviation for the day of the week.
These are overwritten once a week to avoid the need for regular
cleanup.
The other type of log file contains the status of the printer and
is written by the program that communicates with the printer itself.
These are named
\fIprinter\fP.\f(CWst\fP.
These are overwritten with each new job and are saved in the
.CW $LPLIB/prob
directory along with the job under circumstances described below.
When a printer does not appear to be functioning these files are the
place to look first.
.NH 2
Prob directory
.PP
When a job fails to produce output,
the log files should be checked for any obvious problems.
If none can be found, a directory with full read and write permissions
should be created with the name of the printer in the
.CW $LPLIB/prob
directory.
Subsequent failure of a job will cause the daemon to leave a
copy of the job and the printer communication log in
\f(CW$LPLIB/prob/\fP\fIprinter\fP
directory.
It is common for a printer to enter states from which
it cannot be rescued except by manually cycling the power on the printer.
After this is done the print daemon should recover by itself
(give it a minute).
If it does not recover, remove the
.CW LOCK
file from the printer's spool directory to kill the daemon.
The daemon will have to be restarted by sending another job
to the printer.
For PostScript printers just use:
.P1
echo '%!PS' | lp
.P2
.NH 2
Repairing Stuck Daemons
.PP
There are conditions that occur which are not handled
by the daemons.
One such problem can only be described as the printer entering a
comatose state.
The printer does not respond to any messages sent to it.
The daemon should recover from the reset and an error message
will appear in the log files.
If all else fails, one can kill the first job in the queue
or remove the
.CW LOCK
file from the queue directory.
This will kill the daemon, which will have to be restarted.
.NH
Interprocessor Communication
.PP
A Plan 9 CPU server can be set up as a printer's spooling host.
That is, the machine where jobs are spooled and from which those jobs
are sent directly to the printer.
To do this, the CPU must listen on TCP port 515 which is the well known
port for the BSD line printer daemon.
The file
.CW /rc/bin/service/tcp515
is executed when a call comes in on that port.
The Plan 9
.CW lpdaemon
will accept jobs sent from BSD LPR/LPD systems.
The
.CW /$cputype/bin/aux/lpdaemon
command is executed from the service call and it accepts print jobs, requests for status,
and requests to kill jobs.
The command
.CW /$cputype/bin/aux/lpsend
is used to send jobs
to other Plan 9 machines and is usually called from
within a spooler or daemon script.
The command
.CW /$cputype/bin/aux/lpdsend
is used to send jobs
to machines and printers that use the BSD LPR/LPD protocol and is also usually called from
within a spooler or daemon script.
.NH
Acknowledgements
.PP
Special thanks to Rich Drechsler for supplying and maintaining most of
the PostScript translation and interface programs,
without which
.I lp
would be an empty shell.
Tomas Rokicki provided the
TeX
to PostScript
translation program.
.NH
References
.LP
[Camp86] Ralph Campbell,
``4.3BSD Line Printer Spooler Manual'', UNIX System Manager's Manual,
May, 1986, Berkeley, CA
.br
[RFC1179] Request for Comments: 1179, Line Printer Daemon Protocol, Aug 1990
.br
[Sys5] System V manual, date unknown
