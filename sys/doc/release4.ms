.HTML "Plan 9 — Fourth Edition Release Notes
.TL
Plan 9 From Bell Labs
.br
Fourth Release Notes
.br
April, 2002
.br
updated June, 2003
.LP
.sp -.4i
.nf
.ce 1000
Copyright © 2002-2003 Lucent Technologies Inc.
All Rights Reserved
.sp .2i
.fi
.LP
The fourth release of the Plan 9 operating system from Bell Labs
packages a major overhaul of the system at every level.
From the underlying file system protocol, 9P, through the kernel,
libraries, and applications, almost everything has been modified
and, in many cases, redesigned or rewritten.
.LP
The most significant change is that 9P has been redesigned to address
a number of shortcomings, most important, its previous inability to handle long
file names.
Unfortunately, squeezing long names onto the disks of existing
file servers is a messy business that we're still grappling with,
so at the moment
.I fs (4)
and
.I kfs (4)
can't yet handle long names,
although they do talk the new protocol.
(In fact, they
talk both old and new, as required, to ease transition.)
In the meantime, there is a workaround \(em
.I lnfs (4)
\(em
and many of the other file servers such as
.I ramfs (4)
and
.I u9fs (4)
work just fine with long names.
It's only the old disk-resident file servers
that don't.
The new file server
.I fossil (4)
handles supports long names and many other features.
The older servers are now deprecated.
.LP
The following is a partial list of the major changes throughout the system.
.de Xx
.LP
\(bu
..
.Xx
The file system protocol, 9P, has been reworked.
It now has variable-length names, so it can handle long names
but also is more compact when handling short ones.
It uses a different format that is easily parsed, eliminating the need for the old
.CW aux/fcall
utility,
and delegates its authentication duties to an external agent,
.CW factotum .
.Xx
Security has been a focus of attention.
A new security agent,
.I factotum (4),
manages passwords and other secrets and, coupled with a new secure file store
.I secstore (8),
enables secure single sign-on.
.Xx
.CW Cpu ,
.CW import ,
and
.CW exportfs
all encrypt their connections now, and since they use the new 9P they
also use new network port numbers.
A new service
.I aan (1)
is used by
.CW import
to make its network connections more reliable in the face of network outages.
The old ports still work, through the agency of a protocol conversion filter
.I srvold9p (4).
.Xx
We are phasing out the IL protocol since it doesn't handle long-distance connections
well (and long-distance networks don't handle it well, either).
IL is still used by
.I fs (4)
but TCP has become the standard protocol for all other services.
.Xx
The software for the new network-resident secure block store,
.I venti (8),
is included with this distribution.
The new
file server
.I fossil (4)
uses Venti rather than a WORM as its permanent block repository/backup medium.
It is still being developed, but is mature enough that a handful of users
throughout the world are using it as their primary file server.
.Xx
The need to handle longer file names triggered a rethinking of the way the
system handles strings in general.
The kernel is now more explanatory when it gives an error message and
more consistent in how it handles strings such as commands to devices.
The interfaces to many of the system calls, such as
.I errstr (2)
and
.I wait (2)
all had to change as a result, as did the library interface to read directories,
.I stat (2)
and its relatives.
.Xx
The formatted I/O package described in
.I print (2)
and
.I fmtinstall (2)
has been redesigned.
Although the basic interface is unchanged, it now runs without locks and
has an internal buffer management mechanism that means
.CW print
no longer needs a large on-stack buffer.
The interface for writing custom print verbs and custom formatted I/O routines
has also been greatly improved.
.Xx
The thread library
.I thread (2)
has been completely rewritten.
The main visible change is that, coupled with the changes to printing,
.CW threadprint
is gone; you can just use
.CW print
or
.CW fprint
at will.
.Xx
Support for electronic mail has been extended in many ways and now includes
some new spam filtering tools,
much better (and more standard) handling of MIME messages,
the ability to render incoming HTML mail,
and much more.
.LP
There are so many changes to the programming interfaces of the system
that they are described in a separate document, entitled
.I
Changes to the Programming Environment in the Fourth Release of Plan 9.
.R
Please read it before you start updating your own software to run under the new system.
.LP
The installation method has also changed and we're moving towards a new
method for maintaining updates.
The Plan 9 Wiki
.CW http://plan9.bell-labs.com/wiki/plan9 ) (
and Usenet group
.CW comp.os.plan9 ) (
are the places to visit to learn more and stay current.
In particular, the installation notes are now maintained in the Wiki;
the traditional papers on installation and start-up are gone.
.LP
There's lots more new stuff.
If you have problems, mail
.CW 9trouble@plan9.bell-labs.com
or, better, check the wiki
.CW http://plan9.bell-labs.com/wiki/plan9
or ask the Usenet newsgroup
.CW comp.os.plan9 .
.LP
Good Luck!
