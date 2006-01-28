.HTML "Plan 9 — Third Edition Release Notes
.TL
Plan 9 From Bell Labs
.br
Third Release Notes
.br
June 7, 2000
.LP
.sp -.4i
.nf
.ce 1000
Copyright © 2000 Lucent Technologies Inc.
All Rights Reserved
.sp .2i
.fi
.LP
The third release of the Plan 9 operating system from Bell Labs
is something of a snapshot of the current system.
This differs from the previous, 1995 release,
which was a more coordinated, well-defined release of an already-out-of-date
system.
Also, the previous releases were distributed on fixed media, while this release
is being done over the web.
The other major difference is that the third release is licensed under
an open source agreement, which we hope will encourage people
to experiment with it.
.LP
Beyond that, there are innumerable little changes throughout the code.
Although superficially it is the same environment, there is hardly an aspect
of the system that has not been redesigned, rewritten, or replaced.
The following is an incomplete list of changes.
.de Xx
.LP
\(bu
..
.Xx
The list of architectures has changed; more compilers are included
and the list of kernels has changed.
There is solid support for Intel x86 multiprocessors.
Also, although the sources are available for
other architectures, the binaries and libraries are built only for the
Intel x86 architectures.
Kernel source is available for x86, Mips, DEC Alpha, and Power PC architectures.
Compilers also exist for AMD 29000, Motorola MC68000 and MC68020,
Intel i960, and SPARC.
(Unlike the the last release, no SPARC kernel exists for the current system.)
The compilers and related tools
have been made easier to port to Unix and Windows.
.Xx
The kernel now has a file cache to improve I/O performance.
Other kernel changes include the replacement of the streams interface
with a simpler, faster, but less flexible I/O queue structure.
The x86 kernels support PCI and PCMCIA devices.
.Xx
Network management has been simplified and generalized.
DNS supports a resolver mode and the DNS server is now solid.
DHCP is supported both at the client and server ends.
The system can handle multiple IP stacks, which are also
no longer Ethernet-specific.
.Xx
The organization of disks in the kernel has been unified, providing
a consistent interface to all disks and controllers: SCSI or ATAPI,
magnetic or CD-ROM.
.Xx
File offsets, such as in the
.CW seek
system call, are now 64-bit values.
The 1995 release defined the type
.CW Length
for the x86 as
.P1
typedef union
{
	char	clength[8];
	vlong	vlength;
	struct
	{
		long	hlength;
		long	length;
	};
} Length;
.P2
which is the wrong byte order.
Now, for all architectures,
.CW Length
is well handled by a
.CW vlong
.CW long "" (
.CW long )
type, although for compatibility it's still held in a union:
.P1
typedef union
{
	vlong	length;
} Length;
.P2
.Xx
The kernel now maintains a file name associated with each open file or
directory, which can be cheaply recovered by the
.CW fd2path
system call.
Plan 9 now does a much better job with
.CW ..
(dot-dot).
On a related note, a description of a process's name space may be
read with the
.CW ns
file in
.CW /proc ,
or by the
.CW ns
command.
.Xx
The security model is the same, although
the key format has changed.
If you have an old key file, use
.CW auth/convkeys2
(see
.I auth (8))
to update it.
There are new libraries for mulitprecision arithmetic and security.
.Xx
The graphics model is very different.
It is based on the Porter-Duff compositing algebra rather than
.CW bitblt ,
and the system supports everything from bitmaps to true-color displays.
Some of the graphics drivers exploit hardware acceleration.
.Xx
Coupled to the graphics changes, the image and font file formats have
changed.
They can represent a wider range of pixel formats and compress the data.
Also the white/black sense of value is reversed (zero is now black; pixels
represent light, not ink).
Most of the tools can handle the old format, but they all write the new format only.
.Xx
The user interface now incorporates plumbing, a language-driven
way for applications to communicate.  See
.I plumb (6)
for information.
.Xx
Building on plumbing and a program that presents the mail box as a file
system, Plan 9 now has convenient support for MIME mail messages.
.Xx
.CW 8½
has been replaced by
.CW rio ,
which has a similar appearance but a different architecture.
Although still a file server, it is much more efficient: the kernel driver
multiplexes graphics output so
.CW rio
is not in the display path.
.CW Rio
handles input and window control only.
.Xx
PC booting is more sophisticated.  PCs can now boot Plan 9 directly from
the disk without running DOS.
.Xx
Alef is gone.
It was deemed too difficult to maintain two sets of compilers and libraries
for all architectures.
Alef programs were translated into C, with the help of a new thread library
that preserves much of Alef's functionality, but none of its syntax.
.Xx
Mothra is gone.  There is no web browser included in this release,
but something may well appear before long.
.Xx
The
.CW fb
(frame buffer) suite is gone. Most of its tools are
supplanted by new ones, such as
.CW page ,
.CW jpg ,
and
.CW togif .
.Xx
Also gone from this release are the games and support for
international input
.CW ktrans "" (
etc.).
Both may return.
.Xx
New things include an implementation of
.CW ssh ,
an IMAP4 server,
and some spam-filtering software (see
.I scanmail (8)).
.LP
There's lots more.
If you have problems, mail
.CW 9trouble@plan9.bell-labs.com .
Please don't mail us individually.
.LP
Good Luck!
