.TH B.COM 8
.SH NAME
b.com \- PC bootstrap program
.SH SYNOPSIS
.I "(Under MS-DOS)
.br
[
.I drive
:][
.I path
.RB ] b
[
.I bootfile
]
.SH DESCRIPTION
.B B.com
is an MS-DOS program that loads and starts a program
in Plan 9 boot format
.RB ( -H3
option to
.I 8l
(see
.IR 2l (1))).
.B B.com
loads the
bootfile
at the entry address specified by the header,
usually 0x80100020.
After loading, control is passed to the entry location.
.PP
.I B.com
can be loaded two ways: either by booting MS-DOS and typing
.B b
in the appropriate directory, or by preparing a Plan 9 boot floppy using
.B format
(see
.IR prep (8));
such floppies include and run
.I b.com
automatically.
With a boot floppy, the
.I bootfile
must be named explicitly to
.IR b.com ;
from DOS it may be named as an argument to the command.
.PP
The
.I bootfile
can be specified in one of 3 ways,
in order of precedence:
.EX
.ft 1
	command-line argument
	configuration file option
	default, based on available devices
.EE
The format of the
.I bootfile
name is
.IB device ! unit ! file\f1.
If
.BI ! file
is omitted, the default for the particular
.I device
is used.
Supported
.I devices
are
.TF hd
.TP
.B fd
An MS-DOS floppy disk.
The
.I bootfile
is the contents of the MS-DOS
.IR file .
The default
.I file
is
.BR 9dos .
.TP
.B e
Ethernet.
.I File
is
.RI [ host :] pathname .
The default
.I file
is determined by the
.B /lib/ndb
(see
.IR ndb (6))
entry for this PC.
.TP
.B h
Hard (IDE)
disk partition.
The
.I bootfile
is the contents of the partition given by
.IR file .
The default partition
is
.BR boot .
.TP
.B hd
Hard (IDE)
disk MS-DOS partition.
As for
.BR fd .
.TP
.B s
SCSI
disk boot partition.
As for
.BR h .
.TP
.B sd
SCSI
disk MS-DOS partition.
As for
.BR fd .
.PD
.PP
.I Unit
specifies which disk or Ethernet interface to use.
The default is to use the
lowest unit number found.
.PP
When
.B b.com
starts, it relocates itself to address 0x80000 in
the standard PC memory and switches to 32-bit mode.
It then double maps the first 16Mb of physical memory to
virtual addresses 0 and 0x80000000.
Physical memory between 0x80000 and 0xA0000
and from 0x200000 upwards is used as program and data
space.
Next, in order to find configuration information,
.B b.com
searches all units on devices
.BR fd ,
.BR hd ,
and
.BR sd ,
in that order, for an MS-DOS file system
containing a file called
.B plan9\eplan9.ini
or
.B plan9.ini
(see
.IR plan9.ini (8)).
If one is found, searching stops and the file is read into memory
at physical address 0x400
where it can be found later by any loaded
.IR bootfile .
Some options in
.B plan9.ini
are used by
.BR b.com :
.TF bootfile=manual
.TP
.B console
.TP
.B baud
Specifies the console device and baud rate if not a display.
.TP
.BI ether X
(where
.I X
is a number)
Ethernet interfaces. These can be used to load the
.I bootfile
over a network.
Probing for Ethernet interfaces is too prone to error.
.TP
.B bootfile=device!unit!file
Specifies the
.IR bootfile .
This option is overridden by a command-line argument.
.TP
.B bootfile=auto
Default.
.TP
.B bootfile=local
Like
.IR auto ,
but do not attempt to load over the network.
.TP
.B bootfile=manual
After determining which devices are available for loading from,
enter prompt mode.
.PD
.PP
When the search for
.B plan9.ini
is done,
.B b.com
proceeds to determine which bootfile to load.
If there was no command-line argument or
.I bootfile
option,
.B b.com
chooses a default
from the following prioritized device list:
.EX
	fd e h hd s sd
.EE
.B B.com
then attempts to load the
.I bootfile
unless
the
.B bootfile=manual
option was given, in which case prompt mode is entered immediately.
If the default device is
.BR fd ,
.B b.com
will prompt the user for input before proceeding with the
default bootfile load after 5 seconds;
this prompt is omitted if
a command-line argument or
.I bootfile
option
was given.
.PP
.B B.com
prints the list of available
.IB device ! unit
pairs and
enters prompt mode on encountering any error
or if directed to do so by a
.B bootfile=manual
option.
In prompt mode, the user is required to type
a
.IB device ! unit ! file
in response to the
.L "Boot from:
prompt.
.PP
A
control-P
character typed at any time on the console causes
.B b.com
to perform a hardware reset.
.SH FILES
.RI [ drive :]
[
.I path
.RB ] b.com
.br
.IB "MS-DOS filesystem" :\eplan9.ini
.SH SOURCE
.B /sys/src/boot/pc
.SH "SEE ALSO"
.IR plan9.ini (8)
.SH BUGS
When looking for a
.B plan9.ini
file, SCSI disk controllers are assumed to be
at I/O port 0x330.
Only one SCSI controller is recognized.
.PP
Much of the work done by
.B b.com
is duplicated by the loaded kernel.
.PP
The BIOS data area at physical address 0x400 should
not be overwritten, and more use made of the information
therein.
.PP
If
.I b.com
detects an installed MS-DOS Extended Memory Manager,
it attempts to de-install it, but the technique
used may not always work.
It is safer not to install the Extended Memory Manager before running
.IR b.com .
