.TL
Plan 9 From Bell Labs
.br
Release Notes
.LP
.sp -.4i
.nf
.ce 1000
AT&T \(em Proprietary
This software contains proprietary information of
AT&T and is not to be disclosed or used except in
accordance with applicable agreements.

Copyright \(co 1993 AT&T

Unpublished and not for publication
All Rights Reserved
.sp .2i
.fi
.LP
Included in this package are a paper copy of the Programmer's Manual
and associated documents, an ISO 9660 CD-ROM,
installation notes, and
these release notes.
.LP
You should immediately send electronic mail to
.CW plan.9@research.att.com
to let us know you received the package correctly.
That is also the address to mail questions, bug reports,
complaints, or money.
Accolades should be posted to the net.
.LP
This is a snapshot of a working system, not a product
prepared in isolation, so it is not as ironclad and
consistent as we might like.
Still, we feel it's ready
to let others try it, and it's certainly time others
had a chance to see what we've been doing.
.LP
The system coexists well with other systems, such as Unix machines
on the Ethernet, but really expects to run a self-contained environment.
Unlike MACH, for instance, it is not another implementation of Unix.
It has its own set of users, its own authentication methods,
its own mail system,
stand-alone file servers, and so on.
.LP
Perhaps the most important consequence is that it has its own
method of backing up the file system: it does a daily incremental
dump to Write-Once-Read-Many (WORM) optical disks in a jukebox.
(See the manual pages
.I yesterday
in Section 1 and
.I fs
in Section 4 of the Programmer's Manual for details.)
See the document
.I
The Various Ports
.R
for more information.
.LP
A number of things in the system could be better:
.IP
The authentication mechanisms are serviceable but clumsy.
We wish we had a better idea.
.IP
There should be a way for Plan 9 to serve its files to Unix systems
using NFS, but we don't have graduate students to assign distasteful jobs.
.IP
The laser printer software works fine for converting text,
.CW troff
output, and so on to PostScript, but all our PostScript printers
are on Unix machines, so the
.CW lp
software on this distribution isn't self-contained.
Also, it's code we imported from the local Computer Center;
we don't stand by its structure.
.LP
If you have problems, mail
.CW plan.9@research.att.com
or, if it's critical, send a FAX to
.CW 1-908-5825857 .
Please don't mail us individually.
.LP
Good Luck!
