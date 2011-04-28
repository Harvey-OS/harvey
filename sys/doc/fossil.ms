.HTML "Fossil, an Archival File Server
... .FP times
... .fp 1 R R.nomath
... .fp 5 CW LucidaSansCW83
.TL
Fossil, an Archival File Server
.AU
Sean Quinlan
Jim McKie
Russ Cox
jmk,rsc@plan9.bell-labs.com
.AB
This paper describes the internals and 
operation of Fossil, an archival file server built for Plan 9.
Fossil has not yet replaced the current Plan 9 file server
and
.CW kfs ,
but that is our eventual intent.
Both fossil and this documentation are
works in progress.  Comments on either are most welcome.
.AE
.de HP
.LP
..
.NH 1
Introduction
.HP
Fossil is an archival file server built for Plan 9.
In a typical configuration, it maintains a traditional file system
in a local disk partition and periodically archives snapshots of the file system
to a Venti server.  These archives are made available through
a file system interface.
Fossil can also be run without a Venti server, in which case the
snapshots (if any) occupy local disk space.
.PP
The bulk of this paper explains the underlying data structures:
Venti trees, the Venti archival file system format, and finally Fossil's
file system format.
The end of the paper discusses the architecture of the Fossil server.
.PP
The presentation of the data structures is very detailed, perhaps
too detailed for most readers.
The intent is to record all the details necessary to make structural
changes to the file system format.
Feel free to jump ahead when boredom strikes.
.NH 1
Venti trees and directory hierarchies
.HP
Venti [3] is an archival block storage server.
Once a block is stored, it can be retrieved by presenting the 20-byte
SHA1 hash of its contents, called a
.I score .
Blocks on Venti have a maximum length of about 56 kilobytes,
though in practice smaller blocks are used.
To store a byte stream of arbitrary length, Venti uses a hash tree.
Conceptually, the data stream is broken into fixed-size (say,
.I dsize -byte)
chunks, which are stored on the Venti server.
The resulting scores are concatenated into a new pointer stream, which is
broken into fixed size (say,
.I psize -byte)
chunks, which are stored on the Venti server.
.I Psize "" (
is different from
.I dsize
so that we can ensure that each pointer block holds an
integral number of pointers.)
This yields a new pointer stream, and so on, until there is a single block
and finally a single score describing the entire tree.
The resulting structure looks like:
.PS
.ps 8
.vs 10
boxht=0.1
boxwid=0.1

B0: box invis wid 1 "\f(CWVtDataType\fP"
move right 0.1
L0a: box wid 0.2
move right 0.1
L0b: box wid 0.2
move right 0.1
L0c: box invis wid 0.2 "..."
move right 0.1

L0d: box wid 0.2
move right 0.1
L0e: box wid 0.2
move right 0.2
L0f: box invis wid 0.2 "..."
move right 0.2

L0g: box wid 0.2
move right 0.1
L0h: box wid 0.2
move right 0.1
L0i: box invis wid 0.2 "..."
move right 0.1

L0j: box wid 0.2
move right 0.1
L0k: box wid 0.2
move right 0.1
L0l: box invis wid 0.2 "..."
move right 0.1
L0m: box wid 0.2

define boxppddd {
	line from 0.2<$1.nw,$1.ne> to 0.2<$1.sw,$1.se>
	line from 0.4<$1.nw,$1.ne> to 0.4<$1.sw,$1.se>
	X: box invis at 0.1<$1.nw,$1.ne>
	Y: box invis at 0.1<$1.sw,$1.se>
	line -> from 0.5<X,Y> to $2.nw
	X: box invis at 0.3<$1.nw,$1.ne>
	Y: box invis at 0.3<$1.sw,$1.se>
	line -> from 0.5<X,Y> to $3.nw
	"..." at 0.7<$1.w,$1.e>
}

define boxppdddp {
	line from 0.2<$1.nw,$1.ne> to 0.2<$1.sw,$1.se>
	line from 0.4<$1.nw,$1.ne> to 0.4<$1.sw,$1.se>
	line from 0.8<$1.nw,$1.ne> to 0.8<$1.sw,$1.se>
	X: box invis at 0.1<$1.nw,$1.ne>
	Y: box invis at 0.1<$1.sw,$1.se>
	line -> from 0.5<X,Y> to $2.nw
	X: box invis at 0.3<$1.nw,$1.ne>
	Y: box invis at 0.3<$1.sw,$1.se>
	line -> from 0.5<X,Y> to $3.nw
	"..." at 0.6<$1.w,$1.e>
	X: box invis at 0.9<$1.nw,$1.ne>
	Y: box invis at 0.9<$1.sw,$1.se>
	line -> from 0.5<X,Y> to $4.nw
}

define boxpdddp {
	line from 0.2<$1.nw,$1.ne> to 0.2<$1.sw,$1.se>
	line from 0.8<$1.nw,$1.ne> to 0.8<$1.sw,$1.se>
	X: box invis at 0.1<$1.nw,$1.ne>
	Y: box invis at 0.1<$1.sw,$1.se>
	line -> from 0.5<X,Y> to $2.nw
	"..." at 0.5<$1.w,$1.e>
	X: box invis at 0.9<$1.nw,$1.ne>
	Y: box invis at 0.9<$1.sw,$1.se>
	line -> from 0.5<X,Y> to $3.nw
}

bhd=0.4
L1abc: box wid 0.5 at 0.5<L0a, L0b>+(0,bhd)
boxppddd(L1abc, L0a, L0b)
L1def: box wid 0.5 at 0.5<L0d, L0e>+(0,bhd)
boxppddd(L1def, L0d, L0e)
L1ghi: box wid 0.5 at 0.5<L0g, L0h>+(0,bhd)
boxppddd(L1ghi, L0g, L0h)
L1jklm: box wid 0.5 at 0.5<L0j, L0k>+(0,bhd)
boxppdddp(L1jklm, L0j, L0k, L0m)
B1: box invis wid 1 "\f(CWVtPointerType0\fP" at B0+(0,bhd)

L2abcdef: box wid 0.5 at 0.5<L1abc,L1def>+(0,bhd)
boxppddd(L2abcdef, L1abc, L1def)
L2ghijklm: box wid 0.5 at 0.5<L1ghi,L1jklm>+(0,bhd)
boxpdddp(L2ghijklm, L1ghi, L1jklm)
B2: box invis wid 1 "\f(CWVtPointerType1\fP" at B1+(0,bhd)

L3atom: box wid 0.5 at 0.5<L2abcdef, L2ghijklm>+(0,bhd)
boxpdddp(L3atom, L2abcdef, L2ghijklm)
B3: box invis wid 1 "\f(CWVtPointerType2\fP" at B2+(0,bhd)
.PE
.LP
The leaves are the original data stream.  Those blocks have type
.CW VtDataType .
The first pointer stream has type
.CW VtPointerType0 ,
the next has type
.CW VtPointerType1 ,
and so on.
The figure ends with a single block of type
.CW VtPointerType2 ,
but in general trees can have height up to
.CW VtPointerType6 .
For a
.I dsize
of 8192 bytes
and
.I psize
of 8180 bytes (409 pointers),
this gives a maximum stream size of approximately 10 zettabytes
(2\s-2\u73\d\s+2 or 10\s-2\u22\d\s+2 bytes).
.PP
Data blocks are truncated to remove trailing runs of zeros before
storage to Venti; they are zero-filled back to
.I dsize
bytes after retrieval from Venti.
Similarly, trailing runs of pointers to zero-length blocks are
removed from and added back to pointer blocks.
These simple rules happen to make it particularly efficient to store
large runs of zeros, as might occur in a data stream with ``holes:''
the zero-length block itself can be interpreted as a tree of any
depth encoding an all-zero data stream.
.PP
Reconstructing the data stream requires the score and type of the
topmost block in the tree, the data chunk size, the pointer chunk size,
and the data stream size.
(From the data stream size and the chunk sizes we could derive the
depth of the tree and thus the type of the topmost block, but it is convenient
to allow trees that are deeper than necessary.)
This information is kept in a 40-byte structure called a
.CW VtEntry :
.P1
VtEntry:
.ta +\w'    'u +\w'            'u
	gen[4]	\fRgeneration number\fP
	psize[2]	\fRsize of pointer blocks\fP
	dsize[2]	\fRsize of data blocks\fP
	flags[1]
	zero[5]
	size[6]	\fRlength of file\fP
	score[20]	\fRscore of root block in tree\fP
.P2
(In this notation,
.CW name[sz]
indicates a
.CW sz -byte
field called
.CW name .
Integers are stored in big-endian order.
.CW Size
really is a 48-bit field.)
.CW Flags
is made up of the following bit fields.
.P1
.ta +\w'      'u +\w'                      'u
0x01	VtEntryActive	\fRentry is allocated\fP
0x02	VtEntryDir	\fRentry describes a Venti directory (q.v.)\fP
0x1C	VtEntryDepthMask	\fRmask for tree depth\fP
0x20	VtEntryLocal	\fRreserved (q.v.)\fP
.P2
.LP
The depth of the described tree is stored in the 3 bits indicated:
a tree with a topmost node of type
.CW VtPointerType3
has depth 4.
.PP
With
.CW VtEntry
we can build more complicated data structures,
ones with multiple or nested data streams.
A data stream consisting of
.CW VtEntry
structures is called a Venti directory.
It is identical in structure to the Venti data stream
we described earlier except that the bottom-level type is
.CW VtDirType ,
and
the
.CW VtEntry
describing a Venti directory has the
.CW VtEntryDir
flag bit set.
The
.I dsize
for a Venti directory
is a multiple of 40 so that each data chunk holds
an integer number of
.CW VtEntry
structures.
By analogy with Venti directories,
we call the original data stream a
Venti file.
Note that Venti files are assumed
.I not
to contain pointers to other Venti blocks.
The only pointers to Venti blocks occur in 
.CW VtEntry
structures in
Venti directories
(and in the internal hash tree structure of the
individual files and directories).
Note also that these directories are nothing more than pointer lists.
In particular, there are no names or metadata like in a file system.
.PP
To make it easier to pass hierarchies between applications,
the root of a hierarchy is described in a 300-byte structure
called a
.CW VtRoot :
.P1
VtRoot:
.ta +\w'    'u +\w'                'u
	version[2]	\f(CW2\fP
	name[128]	\fRname of structure (just a comment)\fP
	type[128]	\fRstring describing structure (\f(CWvac\fR)\f(CW
	score[20]	\fRpointer to \f(CWVtDirType\fP block\f(CW
	blockSize[2]	\fRmaximum block size in structure\fP
	prev[20]	\fRprevious \f(CWVtRoot\fP in chain, if any\fP
.P2
.LP
This structure is stored to Venti and its score is passed
between applications, typically in the form
``\fItype\f(CW:\fIrootscore\fR,''
where
.I type
is the type field from the
.CW VtRoot
structure, and
.I rootscore
is the score of the
.CW VtRoot
block.
.CW VtRoot
structures can be chained together using the
.I prev
field to encode an archival history
of the data structure.
.PP
For example, a small Venti hierarchy might look like:
.PS
.ps 8
.vs 10
boxwid=0.1
boxht=0.1
f=0.9
mb=0.16

VtRoot: [
	right
	B1: box
	move right 0.1
	"\f(CWVtRoot\fP" ljust
]

Root: [
	right
	B1: box fill f
	B2: box fill f
	B3: box fill f
	move right 0.1
] with .nw at VtRoot.sw+(0.2,-.1)
Level1: [
	RootMeta: [
		box wid mb
	]
	MetaSource: [
		right
		B1: box wid 5*mb
	] with .nw at RootMeta.sw+(0,-.1)

	Source: [
		right
		B1: box fill f
		B2: box fill f
		B3: box fill f
		B4: box fill f
		B5: box fill f
		B6: box fill f
		B7: box fill f
		B8: box fill f
	] with .nw at MetaSource.sw+(0,-.1)
	SB1: box invis at Source.B1
	SB2: box invis at Source.B2
	SB3: box invis at Source.B3
] with .nw at Root.sw+(0.4,-.1)
Level2: [
	MetaSource: [
		right
		B1: box wid 5*mb
	] 
	Source: [
		right
		B1: box fill f
		B2: box fill f
		B3: box fill f
		B4: box fill f
		B5: box fill f
		B6: box fill f
		B7: box fill f
		B8: box fill f
	] with .nw at MetaSource.sw+(0,-.1)
	File: box wid 0.8 with .nw at Source.sw+(0,-.1)
] with .nw at Level1.sw+(0.6,-.1)

line -> from VtRoot.B1 down boxwid/2+0.1+boxwid/2 then to Root.w
line -> from Root.B3 down boxwid/2+0.1+boxwid/2 then to Level1.RootMeta.w
line -> from Root.B2 down boxwid/2+0.1+boxwid+0.1+boxwid/2 then to Level1.MetaSource.w
line -> from Root.B1 down boxwid/2+0.1+boxwid+0.1+boxwid+0.1+boxwid/2 then to Level1.Source.w

line -> from Level1.SB3 down boxwid/2+0.1+boxwid/2 then to Level2.MetaSource.w
line -> from Level1.SB2 down boxwid/2+0.1+boxwid+0.1+boxwid/2 then to Level2.Source.w
line -> from Level1.SB1 down boxwid/2+0.1+boxwid+0.1+boxwid+0.1+boxwid/2 then to Level2.File.w

[
	KEY: box wid 1.5 invis "Key"
	line from KEY.sw to KEY.se
	k = -.1
	kk=0.5
	A: [
		box wid 4*boxwid
		"Venti file" ljust with .w at last box .w+(kk,0)
	] with .nw at KEY.sw+(0,2*k)
	B: [
		box fill f
		"Venti entry (\f(CWVtEntry\fP)" ljust with .w at last box .w+(kk,0)
	] with .nw at A.sw+(0,k)
	C: [
		right
		CC: box fill f
		box fill f
		box fill f
		box fill f
		"Venti directory" ljust with .w at CC.w+(kk,0)
	] with .nw at B.sw+(0,k)
	D: [
		line -> right 3*boxwid
		"Venti pointer (score)" ljust with .w at last line .w+(kk, 0)
	] with .nw at C.sw+(0,k)
] with .nw at VtRoot.nw+(3,0)
.PE
.LP
Venti files are shown as white boxes, while directories are shown
as shaded boxes.  Each shaded square represents a
.CW VtEntry .
Arrows represent pointers from
.CW VtEntry
structures to other
Venti files or directories.
.PP
The hierarchical structure provided by Venti files and directories
can be used as the base for more complicated data structures.
Because this structure captures all the information
about pointers to other blocks, tools written to traverse
Venti hierarchies can traverse the more complicated
data structures as well.
For example,
.I venti/copy
(see
.I venti (1))
copies a Venti hierarchy from one Venti server to another,
given the root
.CW VtEntry .
Because the traditional file system described in later sections is
layered on a Venti hierarchy, 
.I venti/copy
can copy it without fully understanding its structure.
.NH 1
Vac file system format
.HP
The Venti archive format
.I vac
builds a traditional file system using a Venti hierarchy.
Each vac file is implemented as a Venti file;
each vac directory is implemented as a Venti
directory and a Venti file to provide traditional file system metadata.
The metadata is stored in a structure called a
.CW DirEntry :
.P1
DirEntry:
.ta +\w'    'u +\w'            'u
	magic[4]	\f(CW0x1c4d9072\fP (DirMagic)\fP
	version[2]	\f(CW9\fP
	elem[s]	\fRname (final path element only)\fP
	entry[4]	\fRentry number for Venti file or directory\fP
	gen[4]	\fRgeneration number\fP
	mentry[4]	\fRentry number for Venti file holding metadata\fP
	mgen[4]	\fRgeneration number\fP
	qid[8]	\fRunique file serial number\fP
	uid[s]	\fRowner\fP
	gid[s]	\fRgroup\fP
	mid[s]	\fRlast modified by\fP
	mtime[4]	\fRlast modification time\fP
	ctime[4]	\fRcreation time\fP
	atime[4]	\fRlast access time\fP
	mode[4]	\fRmode bits\fP
.P2
The notation
.CW name[s]
denotes a string stored as a two-byte length
and then that many bytes.
The above describes Version 9 of the 
.CW DirEntry
format.  Versions 7 and 8 are very similar; they can be
read by the current
.I vac
source code but are not written.
Earlier versions were not widespread.
A
.CW DirEntry
may be followed by optional extension sections, though none
are currently used.
The
.CW mode
bits include bits commonly used by
Unix and Windows, in addition to those used by Plan 9.
.PP
The
.CW entry
field is an index into the parallel Venti directory.
The
.CW gen
field must match the
.CW gen 
field in the corresponding
.CW VtEntry
in the directory;
it is used to detect
stale indices.
Similarly,
.CW mentry
and
.CW mgen
are the index and generation number
for the metadata Venti file,
if the
.CW DirEntry
describes a vac directory.
.PP
The relation between Venti files and directories and
vac files and directories can be seen in this figure:
.PS
.ps 8
.vs 10
boxwid=0.1
boxht=0.1
f=0.9
mb=0.16

VtRoot: [
	right
	B1: box
	move right 0.1
	"\f(CWVtRoot\fP" ljust
]

SuperRoot: [
	right
	B1: box fill f
	move right 0.1
	"fs root block" ljust
] with .nw at VtRoot.sw + (0.2, -.2)
Root: [
	right
	B1: box fill f
	B2: box fill f
	B3: box fill f
	move right 0.1
	"root directory info block" ljust
] with .nw at SuperRoot.sw+(0.2, -.2)
Level1: [
	RootMeta: [
		box wid mb
		move right 0.1
		"root metadata" ljust
	]
	MetaSource: [
		right
		B1: box wid mb
		B2: box wid mb
		B3: box wid mb
		B4: box wid mb
		B5: box wid mb
	] with .nw at RootMeta.sw+(0,-.2)
	MB1: box wid mb invis at MetaSource.B1
	MB2: box wid mb invis at MetaSource.B2
	MB3: box wid mb invis at MetaSource.B3
	MB4: box wid mb invis at MetaSource.B4
	MB5: box wid mb invis at MetaSource.B5

	Source: [
		right
		B1: box fill f
		B2: box fill f
		B3: box fill f
		B4: box fill f
		B5: box fill f
		B6: box fill f
		B7: box fill f
		B8: box fill f
	] with .nw at MetaSource.sw+(0,-.1)
	SB1: box invis at Source.B1
	SB2: box invis at Source.B2
	SB3: box invis at Source.B3
	SB4: box invis at Source.B4
	SB5: box invis at Source.B5
	SB6: box invis at Source.B6
	SB7: box invis at Source.B7
	SB8: box invis at Source.B8
] with .nw at Root.sw+(0.4,-.2)
Level2: [
	MetaSource: [
		right
		B1: box wid mb
		B2: box wid mb
		B3: box wid mb
		B4: box wid mb
		B5: box wid mb
	] 
	Source: [
		right
		B1: box fill f
		B2: box fill f
		B3: box fill f
		B4: box fill f
		B5: box fill f
		B6: box fill f
		B7: box fill f
		B8: box fill f
	] with .nw at MetaSource.sw+(0,-.1)
	File: box wid 0.8 with .nw at Source.sw+(0,-.2)
] with .nw at Level1.sw+(0.6,-.2)

line -> from VtRoot.B1 down boxwid/2+0.2+boxwid/2 then to SuperRoot.w
line -> from SuperRoot.B1 down boxwid/2+0.2+boxwid/2 then to Root.w
line -> from Root.B3 down boxwid/2+0.2+boxwid/2 then to Level1.RootMeta.w
line -> from Root.B2 down boxwid/2+0.2+boxwid+0.2+boxwid/2 then to Level1.MetaSource.w
line -> from Root.B1 down boxwid/2+0.2+boxwid+0.1+boxwid+0.2+boxwid/2 then to Level1.Source.w

line -> from Level1.SB3 down boxwid/2+0.2+boxwid/2 then to Level2.MetaSource.w
line -> from Level1.SB2 down boxwid/2+0.2+boxwid+0.1+boxwid/2 then to Level2.Source.w
line -> from Level1.SB1 down boxwid/2+0.2+boxwid+0.1+boxwid+0.2+boxwid/2 then to Level2.File.w

arrowwid = arrowwid/2
arrowht = arrowht/2
line -> from Level1.MB1 to Level1.SB1.n
line -> from Level1.MB2 to Level1.SB2.n
line -> from Level1.MB2 to Level1.SB3.n
line -> from Level1.MB4 to Level1.SB7.n
line -> from Level1.MB5 to Level1.SB5.n
arrowwid = arrowwid * 2
arrowht = arrowht * 2

box dashed with .nw at Level1.MetaSource.nw+(-.05,.05) wid 0.8+.05*2 ht .3+.05*2
box dashed with .nw at Level2.MetaSource.nw+(-.05,.05) wid 0.8+.05*2 ht .3+.05*2
box dotted with .nw at Level2.File.nw+(-.05,.05) wid 0.8+0.05*2 ht .1+.05*2

[
	KEY: box wid 1.5 invis "Key"
	line from KEY.sw to KEY.se
	k = -.1
	kk=0.5
	A: [
		box wid 4*boxwid
		"Venti file" ljust with .w at last box .w+(kk,0)
	] with .nw at KEY.sw+(0,2*k)
	B: [
		box fill f
		"Venti entry (\f(CWEntry\fP)" ljust with .w at last box .w+(kk,0)
	] with .nw at A.sw+(0,k)
	C: [
		right
		CC: box fill f
		box fill f
		box fill f
		box fill f
		"Venti directory" ljust with .w at CC.w+(kk,0)
	] with .nw at B.sw+(0,k)
	D: [
		line -> right 3*boxwid
		"Venti pointer (score)" ljust with .w at last line .w+(kk, 0)
	] with .nw at C.sw+(0,k)
	DD: [
		box dotted wid 4*boxwid
		"Vac file" ljust with .w at last box .w+(kk,0)
	] with .nw at D.sw+(0,k)
	E: [
		box wid mb
		"Vac entry (\f(CWDirEntry\fP)" ljust with .w at last box .w+(kk,0)
	] with .nw at DD.sw+(0,k)
	G: [
		box dashed wid 4*boxwid
		"Vac directory" ljust with .w at last box .w+(kk,0)
	] with .nw at E.sw+(0,k)
	H: [
		arrowwid = arrowwid/2
		arrowht = arrowht/2
		line -> right 1.5*boxwid
		"Vac pointer (integer index)" ljust with .w at last line .w+(kk, 0)
		arrowwid = arrowwid * 2
		arrowht = arrowht * 2
	] with .nw at G.sw+(0,k)
] with .nw at VtRoot.nw+(3,0)
.PE
.LP
In reality, the story is slightly more complicated.
The metadata file in a Vac directory
is not just the concatenation of
.CW DirEntry
structures.
Instead, it is the concatenation of
.CW MetaBlocks .
A
.CW MetaBlock
contains some number of
.CW DirEntry
structures along with a sorted index to make it easy
to look for a particular
.CW DirEntry
by its
.CW elem 
field.
The details are in the source code.
.PP
As shown in the diagram,
the root directory of the file system is summarized by
three
.CW VtEntry
structures describing
the Venti directory for the children of the root,
the Venti file for the metadata describing the children of the root,
and a Venti file holding metadata for the root directory itself.
These
.CW VtEntry
structures are placed in a Venti directory of their own,
described by the single 
.CW VtEntry
in the
root block.
.NH 1
Fossil file system format
.HP
Fossil uses the vac format, with some small changes.
The changes only affect the data on the local disk; the data
archived to Venti is exactly in vac format.
.PP
Blocks stored on local disk may contain scores pointing at local disk
blocks or at Venti blocks. 
Local block addresses are stored as 20-byte scores in which the first 16 bytes
are all zero and the last 4 bytes specify a block number in the disk.
Before a block is archived, all the
blocks it points to must be archived, and the local scores in the block
must be changed to Venti scores.
Using block addresses rather than content hashes for local data
makes the local file system easier to manage: if a local block's contents
change, the pointer to the block does not need to change.
.NH 2
Snapshots
.HP
Fossil is an archival file server.
It takes periodic snapshots of the file system,
which are made accessible through the file system.
Specifically, the active file system is presented in
.CW /active .
Ephemeral snapshots (those that are kept on local disk and eventually deleted)
are presented in
\f(CW/snapshot/\fIyyyy\f(CW/\fImmdd\f(CW/\fIhhmm\fR,
where
.I yyyy
is the full year,
.I mm
is the month number,
.I dd
is the day number,
.I hh
is the hour,
and
.I mm
is the minute.
Archival snapshots (those that are archived to Venti and persist forever)
are presented in
\f(CW/archive/\fIyyyy\f(CW/\fImmdds\fR,
where
.I yyyy ,
.I mm ,
and
.I dd
are year, month, and day as before,
and
.I s
is a sequence number if more than one
archival snapshot is done in a day.
For the first snapshot,
.I s
is null.
For the subsequent snapshots,
.I s
is
.CW .1 ,
.CW .2 ,
.CW .3 ,
etc.
.PP
To implement the snapshots, the file server maintains a
current
.I epoch
for the active file system.
Each local block has a label that records, among other things,
the epoch in which the block was allocated.
If a block was allocated in an epoch earlier than the current one,
it is immutable and treated as copy-on-write.
Taking a snapshot can be accomplished by
recording the address of the current root block and then 
incrementing the epoch number.
Notice that the copy-on-write method makes
snapshots both time efficient and space efficient.
The only time cost is waiting for all current file system
requests to finish and then incrementing a counter.
After a snapshot, blocks only get copied when they are
next modified, so the per-snapshot
space requirement is proportional
to the amount of new data rather than the total
size of the file system.
.PP
The blocks in the archival snapshots are moved to Venti,
but the blocks in the ephemeral snapshots take up space
in the local disk file.
To allow reclamation of this disk space, the file system
maintains a 
.I low
.I epoch ,
which is the epoch of the earliest ephemeral snapshot
still available.
Fossil only allows access to snapshots with epoch numbers
between the 
low epoch and the current epoch
(also called the high epoch).
Incrementing the low epoch thus makes old
snapshots inaccessible.
The space required to store those snapshots can then
be reclaimed, as described below.
.NH 2
Local blocks
.HP
The bulk of the local disk file is the local blocks.
Each block has a 14-byte label associated with it, of the format:
.P1
Label:
.ta +\w'    'u +\w'                'u
	state[1]	\fRblock state\fP
	type[1]	\fRblock type\fP
	epoch[4]	\fRallocation epoch\fP
	epochClose[4]	\fRclose epoch\fP
	tag[4]	\fRrandom tag\fP
.P2
.LP
The
.CW type
is an analogue of the block types described earlier,
though different names are used, to distinguish between
pointers blocks in a hash tree for a data stream
and pointer blocks for a directory stream.
The
.CW epoch
was mentioned in the last section.
The other fields are explained below.
.PP
There are two distinguished blocks states
.CW BsFree
.CW 0x00 ) (
and
.CW BsBad
.CW 0xFF ), (
which mark blocks that are available for allocation
and blocks that are bad and should be avoided.
If
.CW state
is not one of these values, it is a bitwise
.I or ' `
of the following flags:
.P1
.ta +\w'      'u +\w'                'u
0x01	BsAlloc	\fRblock is in use\fP
0x02	BsCopied	\fRblock has been copied\fP
0x04	BsVenti	\fRblock has been stored on Venti\fP
0x08	BsClosed	\fRblock has been unlinked from active file system\fP
.P2
.LP
The flags are explained as they arise in the discussions below.
.PP
It is convenient to store some extra fields in the
.CW VtEntry
structure when it describes a Venti file or directory
stored on local disk.
Specifically, we set the
.CW VtEntryLocal
flag bit
and then use the bytes 7-16 of the score (which would
otherwise be zero, since it is a local score) to hold these fields:
.P1
.ta +\w'    'u +\w'                'u
	archive[1]	\fRboolean: this is an archival snapshot\fP
	snap[4]	\fRepoch number if root of snapshot\fP
	tag[4]	\fRrandom tag\fP
.P2
.LP
The extended
.CW VtEntry
structure is called an
.CW Entry .
The
.CW tag
field
in the
.CW Label
and the
.CW Entry
is used to identify dangling pointers or other file system corruption:
all the local blocks in a hash tree must
have tags matching the tag in the
.CW Entry .
If this
.CW Entry
points at the root of a snapshot,
the
.CW snap
field is the epoch of the snapshot.
If the snapshot is intended to be archived to Venti,
the
.CW archive
field is non-zero.
.NH 2
Block reclamation
.HP
The blocks in the active file system form a tree: each
block has only one parent.
Once a copy-on-write block 
.I b
is replaced by its copy, it is no longer
needed by the active file system.
At this point,
.I b
is unlinked from the active file system.
We say that
.I b
is now
.I closed :
it is needed only for snapshots.
When a block is closed, the
.CW BsClosed
bit is set in its state, and the current epoch (called the block's closing epoch)
is stored in the
.CW epochClose
label field.
(Open blocks have an
.CW epochClose
of
.CW ~0 ).
.PP
A block is referenced by snapshots with epochs
between the block's allocation epoch and its closing epoch.
Once the file system's low epoch grows to be greater than or equal to the block's
closing epoch, the block is no longer needed for any snapshots
and can be reused.
.PP
In a typical configuration, where nightly archival snapshots
are taken and written to Venti, it is desirable to reclaim
the space occupied by now-archived blocks if possible.
To do this, Fossil keeps track of whether the pointers
in each block are unique to that block.
When a block
.I bb
is allocated, a pointer to
.I bb
is written into exactly one active block (say,
.I b ).
In the absence of snapshots, the pointer to
.I bb
will remain unique to
.I b ,
so that if the pointer is zeroed,
.I bb
can be immediately reused.
Snapshots complicate this invariant:
when
.I b
is copied-on-write, all its pointers
are no longer unique to it.
At time of the copy, the
.CW BsCopied
state bit in the block's label
is set to note the duplication of the pointers contained within.
.NH 2
Disk layout
.HP
The file system header describes the file system layout and has this format:
.P1
.ta +\w'    'u +\w'                'u
Header:
	magic[4]	\fR0x3776AE89 (HeaderMagic)\fP
	version[2]	\fR1 (HeaderVersion)\fP
	blockSize[2]	\fIfile system block size\fP
	super[4]	\fRblock offset of super block\fP
	label[4]	\fRblock offset of labels\fP
	data[4]	\fRdata blocks\fP
	end[4]	\fRend of file system\fP
.P2
.LP
The corresponding file system layout is:
.PS
.ps 8
.vs 9
boxwid=0.75
boxht=0.15
Empty: box "empty" ht 0.25
Header: box "header" with .n at Empty.s
Empty2: box "empty" with .n at Header.s
Super: box "super block" with .n at Empty2.s
Label: box "label" "blocks" with .n at Super.s ht 0.25
Data: box "data" "blocks" with .n at Label.s ht 0.3
"  0" ljust at Empty.ne
"  128kB" ljust at Header.ne
"  \f5super\fP \(mu \f(CWblockSize\fP" ljust at Super.ne
"  \f5label\fP \(mu \f(CWblockSize\fP" ljust at Label.ne
"  \f5data\fP \(mu \f(CWblockSize\fP" ljust at Data.ne
"  \f5end\fP \(mu \f(CWblockSize\fP" ljust at Data.se
"" at (-1,0)
"" at (6,0)
.PE
.LP
The numbers to the right of the blocks are byte offsets
of the boundaries.
.LP
The super block describes the file system itself and looks like:
.P1
.ta +\w'    'u +\w'                'u
Super:
	magic[4]	\fR0x2340A3B1 (SuperMagic)\fP
	version[2]	\fR1 (SuperVersion)\fP
	epochLow[4]	\fRfile system low epoch\fP
	epochHigh[4]	\fRfile system high (active) epoch\fP
	qid[8]	\fRnext qid to allocate\fP
	active[4]	\fRdata block number: root of active file system\fP
	next[4]	\fRdata block number: root of next file system to archive\fP
	current[4]	\fRdata block number: root of file system currently being archived\fP
	last[20]	\fRVenti score of last successful archive\fP
	name[128]	\fRname of file system (just a comment)\fP
.P2
.LP
.NH 1
Fossil server
.HP
The Fossil server is a user-space program that runs on a standard Plan 9 kernel.
.NH 2
Process structure
.PP
The file server is structured as a set of processes synchronizing
mostly through message passing along queues.
The processes are given names, which can be seen in the output of
.CW ps 
.CW -a .
.PP
.CW Listen
processes announce on various network addresses.
A
.CW con
process handles each incoming connection, reading 9P requests
and adding them to a central message queue.
.CW Msg
processes remove 9P requests from the queue,
handle them, and write the responses to the appropriate
file descriptors.
.PP
The
.CW disk
process handles disk I/O requests made by the other processes.
The
.CW flush
process writes dirty blocks from the in-memory block cache to disk.
The
.CW unlink
process frees previously linked blocks once the blocks that point at them
have been written to disk.
.PP
A
.CW consI
reads from each console file (typically a pipe posted in
.CW /srv ),
adding the typed characters to the input queue.
The
.CW cons
process echoes input and runs the commands, saving
output in a ring buffer.
Because there is only one
.CW cons
process, only one console command may be executing at a time.
A
.CW consO
process copies this ring buffer to each console file.
.PP
The
.CW periodic
process runs periodic events, like
flushing the root metadata to disk or
taking snapshots of the file system.
.NH 2
Block cache
.HP
Fossil maintains an in-memory block cache which 
holds both local disk blocks and Venti blocks.
Cache eviction follows a least recently used policy.
Dirty blocks are restricted to at most half the cache.
This can be changed by editing
.CW DirtyPercentage
in 
.CW dat.h .
.PP
The block cache uses soft updates [1] to ensure that the on-disk
file system is always self-consistent.
Thus there is no
.I halt
console command
and no need to check a file system 
that was shut down without halting.
.NH 2
Archiving
.HP
A background process writes blocks in archival snapshots to Venti.
Although
.CW /archive/\fIyyyy\fP/\fImmdds\fR
is a copy of only
.CW /active
at the time of the snapshot,
the archival process archives the
entire file tree rather than just
the subtree rooted at
.CW /active .
The snapshots
.CW /snapshot/\fIyyyy\fP/\fImmdd\fP/\fIhhmm
are stored as empty directories.
Once all the blocks have been archived,
a 
.CW VtRoot
header for the file system is archived.
The score of that header is recorded in
.CW super.score
and also printed on the file server console.
The score can used by
.I flfmt
to restore a file system (see
.I fossil (4)).
.NH 2
Contrast with the old file server
.HP
The most obvious difference between Fossil and the 
old Plan 9 file server [2] is that Fossil uses a Venti server as 
its archival storage in place of a WORM juke box.
There are a few other architectural differences to be 
aware of.
.PP
Fossil is a user-level program run on a standard kernel.
.PP
Fossil does not have any way to concatenate, stripe, or
mirror disk files.  For functionality similar to the old file server's
configuration strings, use the experimental file stack device 
(see
.I fs (3)).
.PP
Fossil speaks only 9P2000.  Old 9P (aka 9P1) is not supported.
.PP
... XXX words about converting an old file system to fossil?
.NH 1
References
.LP
[1] Gregory R. Ganger, Marshall Kirk McKusick, Craig A. N. Soules,
and Yale N. Patt.
``Soft Updates: A Solution to the Metadata Update Problem
in File Systems,''
.I "ACM Transactions on Computer Systems" ,
Vol 18., No. 2, May 2000, pp. 127\-153.
.LP
[2] Sean Quinlan, ``A Cached WORM File System,''
.I "Software\(emPractice and Experience" ,
Vol 21., No 12., December 1991, pp. 1289\-1299.
.LP
[3] Sean Quinlan and Sean Dorward, ``Venti: A New Approach to Archival Storage,''
.I "Usenix Conference on File and Storage Technologies" ,
2002.
