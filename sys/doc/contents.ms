.HTML "Plan 9 — The Documents (Volume 2)
.FP lucidasans
.na
.ds CH
.
.de He
.sp
.ft B
.in 0
\\$1
.sp .5
.ft
.br
..
.
.\" BUG: \! transparency always emits the rest of the line, even in
.\" an `unexecuted' if-else branch, so this doesn't work as desired.
.\" .de Ti
.\" .ft R
.\" .ie h \{\
.\" \\$3 
.\" (\c
.\" .if !'\\$2'troff' \! \<a href="\\$2.html"\>html\</a\> |
.\" \! \<a href="\\$2.ps"\>ps\</a\>
.\" |
.\" \! \<a href="\\$2.pdf"\>pdf\</a\>)
.\" ...\}
.\" .el \{\
.\" .ta \\n(LLuR
.\" \\$3 \a\|\|\\$1
.\" ...\}
.\" .br
.\" ..
.
.de Au
.ft I
.in 1i
.ll -.5i
\\$1
.br
.ll +.5i
..
.de De
.ft R
.in .5i
.ll -.5i
\\$1
.br
.ll +.5i
.in 0
.sp .5
..
.de Co
..
.
.nr PS 9
.nr VS 11
.nf
.
.LP
.ps 12
.ce
.ie h .B "Plan 9 \(em The Documents (Volume 2)
.el .B "Table of Contents
.ps
.sp .5i
.He "Introduction
.
.Ti 1 9 "Plan 9 From Bell Labs
.Au "Rob Pike, Dave Presotto, Sean Dorward, Bob Flandrena, Ken Thompson, Howard Trickey, and Phil \%Winterbottom
.De "An overview of the system; read at least this paper before you install.
.Co seems ok; a little dated; unchanged except to mention appearance in Computing Systems
.Ti 23 names "The Use of Name Spaces in Plan 9
.Au "Rob Pike, Dave Presotto, Ken Thompson, Howard Trickey, and Phil \%Winterbottom
.Co updated only to mention appearance in SIGOPS
.De "What's in a name?
.Ti 29 net/net "The Organization of Networks in Plan 9
.Au "Dave Presotto and Phil Winterbottom
.De "Connecting the pieces.  The details in the paper are outdated but the ideas still apply.
.Ti 43 auth "Security in Plan 9
.Au "Russ Cox, Eric Grosse, Rob Pike, Dave Presotto, and Sean Quinlan
.De "An overview of the security architecture.
.He "Programming
.Ti 63 comp "How to Use the Plan 9 C Compiler
.Au "Rob Pike
.Co updated
.De "The starting point for C programming under Plan 9.
.Ti 77 prog4 "Changes to the Programming Environment in the Fourth Release of Plan 9
.Au "Rob Pike
.De "An overview of the changes to the C library since the Third Release.
.Ti 83 ape "APE \(em The ANSI/POSIX Environment
.Au "Howard Trickey
.Co updated
.De "Moving C code between UNIX and Plan 9.
.Ti 87 acidpaper "Acid: A Debugger Built From A Language
.Au "Phil Winterbottom
.Co fine
.De "An overview paper about the Acid debugger.
.Ti 101 acid "Acid Manual
.Au "Phil Winterbottom
.Co updated
.De "The reference manual for the language and its libraries.
.Ti 127 mk "Maintaining Files on Plan 9 with Mk
.Au "Andrew G. Hume and Bob Flandrena
.Co updated
.De "An introduction to Plan 9's replacement for \f(CWmake\fP.
.Ti 141 mkfiles "Plan 9 Mkfiles
.Au "Bob Flandrena
.Co updated
.De "The conventions for using \f(CWmk\fP in Plan 9.
.Ti 149 asm "A Manual for the Plan 9 assembler
.Au "Rob Pike
.Co updated
.De "Things you'd rather not know.
.bp
.He "User Interfaces
.Ti 159 8½/8½ "8½, the Plan 9 Window System
.Au "Rob Pike
.Co fine - introductory footnote tweaked
.De "An introduction to the (previous) window system and its unusual implementation.
.Ti 169 rc "Rc \(em The Plan 9 Shell
.Au "Tom Duff
.Co updated
.De "An introduction to the new shell, complete with examples.
.Ti 185 sam/sam "The Text Editor \&\f(CWsam\fP
.Au "Rob Pike
.Co fine
.De "\f(CWSam\fP is the standard editor on Plan 9.
.Ti 215 acme/acme "Acme: A User Interface for Programmers
.Au "Rob Pike
.Co fine
.De "A system with a more radical approach to programming and editing.
.Ti 231 plumb "Plumbing and Other Utilities
.Au "Rob Pike
.Co new
.De "Inter-process communication that enlivens the interactive user interface.
.He "Implementation
.Ti 247 utf "Hello World, or Καλημέρα κόσμε, or \f(Jpこんにちは 世界\fP
.Au "Rob Pike and Ken Thompson
.Co fine
.De "The details about Plan 9's character set: the Unicode Standard plus an ASCII-compatible encoding.
.Ti 259 compiler "Plan 9 C Compilers
.Au "Ken Thompson
.Co updated
.De "The design and some internals of the compiler suite.
.Ti 271 libmach "Adding Application Support for a New Architecture in Plan 9
.Au "Bob Flandrena
.Co updated
.De "The procedures necessary to add a new instruction set to Plan 9's programming environment.
.Ti 279 fs/fs "The Plan 9 File Server
.Au "Ken Thompson
.Co updated
.De "The design of the central file server and its novel backup system.
.Ti 287 venti/venti "Venti: A new approach to archival storage
.Au "Sean Quinlan and Sean Dorward
.De "Archival block-level storage using secure hashes as block identifiers.
.Ti 301 il/il "The IL protocol
.Au "Dave Presotto and Phil Winterbottom
.De "A description of the Internet protocol Plan 9 uses for internal communication.
.Ti 307 lexnames "Lexical File Names in Plan 9, or, Getting Dot-Dot Right
.Au "Rob Pike
.Co new
.De "A vexing old problem solved: how to make \f(CWpwd\fP get the right answer in the face of multiply-bound directories.
.Ti 319 sleep "Process Sleep and Wakeup on a Shared-memory Multiprocessor
.Au "Rob Pike, Dave Presotto, Ken Thompson, and Gerard Holzmann
.Co fine - updated with citation footnote
.De "A detailed study of a central issue in the Plan 9 kernel.
.He "Miscellany
.Ti 325 lp "A Guide to the Lp Printer Spooler
.Au "Paul Glick
.De "Adminstering the suite of tools to drive PostScript printers.
.Ti 333 troff "Troff User's Manual
.Au "Joseph F. Ossanna and Brian W. Kernighan
.Co updated
.De "The old warhorse, updated for Unicode characters.
.bp
.Ti 373 spin "Using SPIN
.Au "Gerard Holzmann
.Co updated
.De "An introduction to a tool for analyzing parallel and distributed programs.
.He "Installation
.Ti 400 port "The Various Ports
.De "The hardware requirements for the Plan 9 compilers and kernels.
.
.Hp
