.TH INTRO 5 
.SH NAME
intro \- introduction New File Protocol, NP
.SH SYNOPSIS
.B #include <fcall.h>
.SH DESCRIPTION
A Plan 9
.I server
is an agent that provides one or more hierarchical file systems (file trees)
that may be accessed by Plan 9 processes.
A server responds to requests by
.I clients
to navigate the hierarchy,
and to create, remove, read, and write files.
The prototypical server is a separate machine that stores
large numbers of user files on permanent media;
such a machine is called, somewhat confusingly, a
.I file
.IR server .
Another possibility for a server is to synthesize
files on demand, perhaps based on information on data structures
inside the kernel; the
.IR proc (3)
.I kernel device
is a part of the Plan 9 kernel that does this.
User programs can also act as servers.
.PP
A
.I connection
to a server is a bidirectional communication path from the client to the server.
There may be a single client or
multiple clients sharing the same connection.
A server's file tree is attached to a process
group's name space by
.IR bind (2)
and
.I mount
calls;
see
.IR intro (2).
Processes in the group are then clients
of the server:
system calls operating on files are translated into requests
and responses transmitted on the connection to the appropriate service.
.PP
The
.IR "New Plan 9 File Protocol" ,
NP, is used for messages between
.I clients
and
.IR servers .
N9P is stateless, unlike 9P, statefulness having proved to be a problem for performance
and resilience. All objects, including files and transactions, are identified by a
UUID, also known as a GUID.
.PP
A client transmits
.I requests
.RI ( T-messages )
to a server, which
subsequently returns
.I replies
.RI ( R-messages )
to the client.
The combined acts of transmitting (receiving) a request of a particular type,
and receiving (transmitting)
its reply is called a
.I transaction
of that type.
.PP
Each message consists of a sequence of bytes.
Two-, four-, eight, and 16-byte fields hold unsigned
integers represented in little-endian order
(least significant byte first).
Data items of larger or variable lengths are represented
by a four-byte field specifying a count,
.IR n ,
followed by
.I n
bytes of data.
Text strings are represented this way,
with the text itself stored as a UTF-8
encoded sequence of Unicode characters (see
.IR utf (6)).
Text strings in NP messages are not
.SM NUL\c
-terminated:
.I n
counts the bytes of UTF-8 data, which include no final zero byte.
The
.SM NUL
character is illegal in all text strings in NP, and is therefore
excluded from file names, user names, and so on.
.PP
Each NP message begins with a four-byte size field
specifying the length in bytes of the complete message including
the four bytes of the size field itself.
The next byte is the message type, one of the constants
in the enumeration in the include file
.BR <fcall.h> .
The next 16 bytes are an identifying
.IR tag ,
described below.
The remaining bytes are parameters of different sizes.
In the message descriptions, the number of bytes in a field
is given in brackets after the field name.
The notation
.IR parameter [ n ]
where
.I n
is not a constant represents a variable-length parameter:
.IR n [2]
followed by
.I n
bytes of data forming the
.IR parameter .
The notation
.IR string [ s ]
(using a literal
.I s
character)
is shorthand for
.IR s [2]
followed by
.I s
bytes of UTF-8 text.
(Systems may choose to reduce the set of legal characters
to reduce syntactic problems,
for example to remove slashes from name components,
but the protocol has no such restriction.
Plan 9 names may contain any printable character (that is, any character
outside hexadecimal 00-1F and 80-9F)
except slash.)
Messages are transported in byte form to allow for machine independence;
.PP
UUIDs represent fid and tags.  They are represented by 
.IR [ u ].
.PP
.IR nfcall (2)
describes routines that convert to and from this form into a machine-dependent
C structure.
.SH MESSAGES
.B Tsession
.IP
.ne 2v
.IR size [4]
.B Tversion
.IR tag [u]
.IR msize [4]
.IR version [ s ]
.br
.IR size [4]
.B Rversion
.IR tag [u]
.IR msize [4]
.IR version [ s ]
.IP
.ne 2v
.IR size [4]
.B Tauth
.IR tag [u]
.IR afid [u]
.IR uname [ s ]
.IR aname [ s ]
.br
.br
.IR size [4]
.B Rauth
.IR tag [u]
.IR afid [ u ]
.IP
.ne 2v
.IR size [4]
.B Rerror
.IR tag [u]
.IR ename [ s ]
.IP
.ne 2v
.IR size [4]
.B Tflush
.IR tag [u]
.IR oldtag [u]
.br
.IR size [4]
.B Rflush
.IR tag [u]
.IP
.ne 2v
.IR size [4]
.B Tattach
.IR tag [u]
.IR fid [u]
.IR afid [u]
.IR uname [ s ]
.IR aname [ s ]
.br
.IR size [4]
.B Rattach
.IR tag [u]
.IR fid [u]
.IP
.ne 2v
.IR size [4]
.B Twalk
.IR tag [u]
.IR fid [u]
.IR newfid [u]
.IR nwname [2]
.IR nwname *( wname [ s ])
.br
.IR size [4]
.B Rwalk
.IR tag [u]
.IR nwguid [2]
.IR nwguid *( wfid [u])
.IP
.ne 2v
.IR size [4]
.B Tcreate
.IR tag [u]
.IR name [ s ]
.IR perm [4]
.IR mode [1]
.br
.IR size [4]
.B Rcreate
.IR tag [u]
.IR fid [u]
.IR iounit [4]
.IP
.ne 2v
.IR size [4]
.B Tread
.IR tag [u]
.IR fid [u]
.IR offset [8]
.IR count [4]
.br
.IR size [4]
.B Rread
.IR tag [u]
.IR count [4]
.IR data [ count ]
.IP
.ne 2v
.IR size [4]
.B Twrite
.IR tag [u]
.IR fid [u]
.IR offset [8]
.IR count [4]
.IR data [ count ]
.br
.IR size [4]
.B Rwrite
.IR tag [u]
.IR count [4]
.IP
.ne 2v
.IR size [4]
.B Tremove
.IR tag [u]
.IR fid [u]
.br
.IR size [4]
.B Rremove
.IR tag [u]
.IP
.ne 2v
.IR size [4]
.B Tstat
.IR tag [u]
.IR fid [u]
.br
.IR size [4]
.B Rstat
.IR tag [u]
.IR stat [ n ]
.IP
.ne 2v
.IR size [4]
.B Twstat
.IR tag [u]
.IR fid [u]
.IR stat [ n ]
.br
.IR size [4]
.B Rwstat
.IR tag [u]
.PP
Each T-message has a
.I tag
field, chosen and used by the client to identify the message.
The reply to the message will have the same tag.
.PP
The type of an R-message will either be one greater than the type
of the corresponding T-message or
.BR Rerror ,
indicating that the request failed.
In the latter case, the
.I ename
field contains a string describing the reason for failure.
.PP
The
.B version
message identifies the version of the protocol and indicates
the maximum message size the system is prepared to handle.
It also initializes the connection and
aborts all outstanding I/O on the connection.
The set of messages between
.B version
requests is called a
.IR session .
.PP
Most T-messages contain a
.IR fid ,
a UUID (GUID), which is a 16 byte unsigned integer that the client uses to identify
a ``current file'' on the server.
Fids are somewhat like file descriptors in a user process,
but they are not restricted to files open for I/O:
directories being examined, files being accessed by
.IR stat (2)
calls, and so on \(em all files being manipulated by the operating
system \(em are identified by fids.
Fids are chosen by the server.
.PP
The fid supplied in an
.B attach
message
will be taken by the server to refer to the root of the served file tree.
The
.B attach
identifies the user
to the server and may specify a particular file tree served
by the server (for those that supply more than one).
.PP
Permission to attach to the service is proven by providing a special fid, called
.BR afid ,
in the
.B attach
message.  This
.B afid
is established by exchanging
.B auth
messages and subsequently manipulated using
.B read
and
.B write
messages to exchange authentication information not defined explicitly by NP.
Once the authentication protocol is complete, the
.B afid
is presented in the
.B attach
to permit the user to access the service.
.PP
A
.B walk
message causes the server to change the current file associated
with a fid to be a file in the directory that is the old current file, or one of
its subdirectories.
.B Walk
returns a new fid that refers to the resulting file.
Usually, a client maintains a fid for the root,
and navigates by
.B walks
from the root fid.
.PP
A client can send multiple T-messages without waiting for the corresponding
R-messages, but all outstanding T-messages must specify different tags.
The server may delay the response to a request
and respond to later ones;
this is sometimes necessary, for example when the client reads
from a file that the server synthesizes from external events
such as keyboard characters.
.PP
Replies (R-messages) to
.BR auth ,
.BR attach ,
.BR walk ,
and
.B create
requests convey a
.I guid
field back to the client.
The guid is a UUID that represents the server's unique identification for the
file being accessed:
two files on the same server hierarchy are the same if and only if their guids
are the same.
(The client may have multiple fids pointing to a single file on a server
and hence having a single guid.)
A new file may be
.B created
in the current (directory) file.
I/O of a given number of bytes
at a given offset
on an open file is done by
.B read
and
.BR write .
.PP
The
.B remove
transaction deletes files.
.PP
The
.B stat
transaction retrieves information about the file.
The
.I stat
field in the reply includes the file's
name,
access permissions (read, write and execute for owner, group and public),
access and modification times, and
owner and group identifications.
The owner and group identifications are textual names.
The
.B wstat
transaction allows some of a file's properties
to be changed.
.PP
A request can be aborted with a
flush
request.
When a server receives a
.BR Tflush ,
it should not reply to the message with tag
.I oldtag
(unless it has already replied),
and it should immediately send an
.BR Rflush .
.PP
Because the message size is negotiable and some elements of the
protocol are variable length, it is possible (although unlikely) to have
a situation where a valid message is too large to fit within the negotiated size.
For example, a very long file name may cause a
.B Rstat
of the file or
.B Rread
of its directory entry to be too large to send.
In most such cases, the server should generate an error rather than
modify the data to fit, such as by truncating the file name.
The exception is that a long error string in an
.B Rerror
message should be truncated if necessary, since the string is only
advisory and in some sense arbitrary.
.PP
Most programs do not see the NP protocol directly; instead calls to library
routines that access files are
translated by the mount driver,
.IR mnt (3),
into NP messages.
.SH DIRECTORIES
Directories are created by
.B create
with
.B DMDIR
set in the permissions argument (see
.IR stat (5)).
The members of a directory can be found with
.IR read (5).
All directories must support
.B walks
to the directory
.B ..
(dot-dot)
meaning parent directory, although by convention directories
contain no explicit entry for
.B ..
or
.B .
(dot).
The parent of the root directory of a server's tree is itself.
.SH "ACCESS PERMISSIONS"
Each file server maintains a set of user and group names.
Each user can be a member of any number of groups.
Each group has a
.I group leader
who has special privileges (see
.IR stat (5)
and
.IR users (6)).
Every file request has an implicit user id (copied from the original
.BR attach )
and an implicit set of groups (every group of which the user is a member).
.PP
Each file has an associated
.I owner
and
.I group
id and
three sets of permissions:
those of the owner, those of the group, and those of ``other'' users.
When the owner attempts to do something to a file, the owner, group,
and other permissions are consulted, and if any of them grant
the requested permission, the operation is allowed.
For someone who is not the owner, but is a member of the file's group,
the group and other permissions are consulted.
For everyone else, the other permissions are used.
Each set of permissions says whether reading is allowed,
whether writing is allowed, and whether executing is allowed.
A
.B walk
in a directory is regarded as executing the directory,
not reading it.
Permissions are kept in the low-order bits of the file
.IR mode :
owner read/write/execute permission represented as 1 in bits 8, 7, and 6
respectively (using 0 to number the low order).
The group permissions are in bits 5, 4, and 3,
and the other permissions are in bits 2, 1, and 0.
.PP
The file
.I mode
contains some additional attributes besides the permissions.
If bit 31
.RB ( DMDIR )
is set, the file is a directory;
if bit 30
.RB ( DMAPPEND )
is set, the file is append-only (offset is ignored in writes);
if bit 29
.RB ( DMEXCL )
is set, the file is exclusive-use (only one client may have it
open at a time);
if bit 27
.RB ( DMAUTH )
is set, the file is an authentication file established by
.B auth
messages;
if bit 26
.RB ( DMTMP )
is set, the contents of the file (or directory) are not included in nightly archives.
(Bit 28 is skipped for historical reasons.)


