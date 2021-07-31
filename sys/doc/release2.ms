.TL
Plan 9 From Bell Labs
.br
Second Release Notes
.LP
.sp -.4i
.nf
.ce 1000
AT&T \(em Proprietary
This software contains proprietary information of
AT&T and is not to be disclosed or used except in
accordance with applicable agreements.

Copyright © 1993 AT&T

Unpublished and not for publication
All Rights Reserved
.sp .2i
.fi
.LP
Included in this package are an ISO 9660 CD-ROM
and paper copy of
these release notes.
The paper documentation shipped with the first
CD-ROM is still accurate enough to stand, although
you may want to print
.CW /sys/doc/print.out
to get a current copy of the Programmer's Manual.
.LP
As before, you should immediately send electronic mail to
.CW plan.9@research.att.com
to let us know you received the package correctly.
That is also the address to mail questions, bug reports,
complaints, or money.
Accolades should be posted to the net.
.LP
This release is formatted exactly like the first:
a complete file system on CD-ROM.  We recommend installing
it as a complete replacement for the previous distribution.
The changes are too widely scattered to isolate as a
set of patches.  The operating system and file system,
in particular, have had major surgery.  Bug fixes occur
throughout.  The system is much more solid and stable.
There should be no major incompatibilities with the previous
release; for example, 9P protocol is unchanged.
Here is a very partial list of major changes; you should
assume changes and fixes even when we don't mention them.
.IP
In the operating system
.CW /sys/src/9 ), (
the PC support has been heavily reworked, providing support
for several popular Ethernet interfaces, improved VGA software,
new booting procedures, etc.
Some serious bugs have been fixed in the paging code.
The internal SGI SCSI is now supported.
TCP has flow control.
A variety of mice are now supported; the command
.CW aux/mouse
queries the mouse at boot time and reports to the mouse driver.
There is a driver for the Nextstation's audio hardware.
In
.CW devbit.c ,
there is code to support DMA to an attached LCD screen,
although it has been run only on locally produced hardware.
.IP
The file system also supports the internal SGI SCSI, and
has support for multiple Cyclone cards.
The SCSI support on the Suns is much improved.
.IP
There is a new debugger,
.CW acid .
(The venerable
.CW db
is still there.)
.CW Zl
has profiling support.
.CW Sam
has had a lot of work, mostly to improve its portability to Unix.
The
.CW tapefs
suite allows access to various archives old and new using file
server methods.
.IP
The libraries have had many fixes but retain mostly the same interface.
.CW Print 's
formatting routines have a different interface that permits
multiprocessed printing.
.IP
The Unicode fonts are considerably better.  Several versions
of Chinese are now available.
.LP
If you have problems, mail
.CW plan.9@research.att.com
or, if it's critical, send a FAX to
.CW 1-908-5825857 .
Please don't mail us individually.
.LP
Good Luck!
