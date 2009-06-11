.HTML "The Text Editor sam
.Vx 17 11 November 87 1 32 "ROB PIKE" "THE TEXT EDITOR SAM"
.ds DY "31 May 1987
.ds DR "Revised 1 July 1987
.de CW		\" puts first arg in CW font, same as UL; maintains font
\%\&\\$3\f(CW\\$1\fP\&\\$2
..
.de Cs
.br
.fi
.ft 2
.ps -2
.vs -2
..
.de Ce
.br
.nf
.ft 1
.ps
.vs
.sp
..
.de XP
.ie h .html - <center><img src="\\$1.gif" /></center>
.el .BP \\$1.ps \\$2
..
.TL
The Text Editor \&\f(CWsam\fP
.AU
Rob Pike
rob@plan9.bell-labs.com
.AB
.LP
.CW Sam
is an interactive multi-file text editor intended for
bitmap displays.
A textual command language
supplements the mouse-driven, cut-and-paste interface
to make complex or
repetitive editing tasks easy to specify.
The language is characterized by the composition of regular expressions
to describe the structure of the text being modified.
The treatment of files as a database, with changes logged
as atomic transactions, guides the implementation and
makes a general `undo' mechanism straightforward.
.PP
.CW Sam
is implemented as two processes connected by a low-bandwidth stream,
one process handling the display and the other the editing
algorithms.  Therefore it can run with the display process
in a bitmap terminal and the editor on a local host,
with both processes on a bitmap-equipped host, or with
the display process in the terminal and the editor in a
remote host.
By suppressing the display process,
it can even run without a bitmap terminal.
.PP
This paper is reprinted from Software\(emPractice and Experience,
Vol 17, number 11, pp. 813-845, November 1987.
The paper has not been updated for the Plan 9 manuals.  Although
.CW Sam
has not changed much since the paper was written, the system around it certainly has.
Nonetheless, the description here still stands as the best introduction to the editor.
.AE
.SH
Introduction
.LP
.CW Sam
is an interactive text editor that combines cut-and-paste interactive editing with
an unusual command language based on the composition of regular expressions.
It is written as two programs: one, the `host part,' runs on a UNIX system
and implements the command language and provides file access; the other, the
`terminal part,' runs asynchronously
on a machine with a mouse and bitmap display
and supports the display and interactive editing.
The host part may be even run in isolation on an ordinary terminal
to edit text using the command
language, much like a traditional line editor,
without assistance from a mouse or display.
Most often,
the terminal part runs on a Blit\u\s-4\&1\s+4\d terminal
(actually on a Teletype DMD 5620, the production version of the Blit), whose
host connection is an ordinary 9600 bps RS232 link;
on the SUN computer the host and display processes run on a single machine,
connected by a pipe.
.PP
.CW Sam
edits uninterpreted
ASCII text.
It has no facilities for multiple fonts, graphics or tables,
unlike MacWrite,\u\s-4\&2\s+4\d Bravo,\u\s-4\&3\s+4\d Tioga\u\s-4\&4\s+4\d
or Lara.\u\s-4\&5\s+4\d
Also unlike them, it has a rich command language.
(Throughout this paper, the phrase
.I
command language
.R
refers to
textual commands; commands activated from the mouse form the
.I mouse
.I language. )
.CW Sam
developed as an editor for use by programmers, and tries to join
the styles of the UNIX text editor
.CW ed \u\s-4\&6,7\s+4\d
with that of interactive cut-and-paste editors by
providing a comfortable mouse-driven interface
to a program with a solid command language driven by regular expressions.
The command language developed more than the mouse language, and
acquired a notation for describing the structure of files
more richly than as a sequence of lines,
using a dataflow-like syntax for specifying changes.
.PP
The interactive style was influenced by
.CW jim ,\u\s-4\&1\s+4\d
an early cut-and-paste editor for the Blit, and by
.CW mux ,\u\s-4\&8\s+4\d
the Blit window system.
.CW Mux
merges the original Blit window system,
.CW mpx ,\u\s-4\&1\s+4\d
with cut-and-paste editing, forming something like a
multiplexed version of
.CW jim
that edits the output of (and input to) command sessions rather than files.
.PP
The first part of this paper describes the command language, then the mouse
language, and explains how they interact.
That is followed by a description of the implementation,
first of the host part, then of the terminal part.
A principle that influenced the design of
.CW sam
is that it should have no explicit limits, such as upper limits on
file size or line length.
A secondary consideration is that it be efficient.
To honor these two goals together requires a method for efficiently
manipulating
huge strings (files) without breaking them into lines,
perhaps while making thousands of changes
under control of the command language.
.CW Sam 's
method is to
treat the file as a transaction database, implementing changes as atomic
updates.  These updates may be unwound easily to `undo' changes.
Efficiency is achieved through a collection of caches that minimizes
disc traffic and data motion, both within the two parts of the program
and between them.
.PP
The terminal part of
.CW sam
is fairly straightforward.
More interesting is how the two halves of the editor stay
synchronized when either half may initiate a change.
This is achieved through a data structure that organizes the
communications and is maintained in parallel by both halves.
.PP
The last part of the paper chronicles the writing of
.CW sam
and discusses the lessons that were learned through its development and use.
.PP
The paper is long, but is composed largely of two papers of reasonable length:
a description of the user interface of
.CW sam
and a discussion of its implementation.
They are combined because the implementation is strongly influenced by
the user interface, and vice versa.
.SH
The Interface
.LP
.CW Sam
is a text editor for multiple files.
File names may be provided when it is invoked:
.P1
sam file1 file2 ...
.P2
and there are commands
to add new files and discard unneeded ones.
Files are not read until necessary
to complete some command.
Editing operations apply to an internal copy
made when the file is read; the UNIX file associated with the copy
is changed only by an explicit command.
To simplify the discussion, the internal copy is here called a
.I file ,
while the disc-resident original is called a
.I
disc file.
.R
.PP
.CW Sam
is usually connected to a bitmap display that presents a cut-and-paste
editor driven by the mouse.
In this mode, the command language is still available:
text typed in a special window, called the
.CW sam
.I window,
is interpreted
as commands to be executed in the current file.
Cut-and-paste editing may be used in any window \(em even in the
.CW sam
window to construct commands.
The other mode of operation, invoked by starting
.CW sam
with the option
.CW -d
(for `no download'),
does not use the mouse or bitmap display, but still permits
editing using the textual command language, even on an ordinary terminal,
interactively or from a script.
.PP
The following sections describe first the command language (under
.CW sam\ -d
and in the
.CW sam
window), and then the mouse interface.
These two languages are nearly independent, but connect through the
.I current
.I text,
described below.
.SH 2
The Command Language
.LP
A file consists of its contents, which are an array of characters
(that is, a string); the
.I name
of the associated disc file; the
.I
modified bit
.R
that states whether the contents match those of
the disc file;
and a substring of the contents, called the
.I
current text
.R
or
.I dot
(see Figures 1 and 2).
If the current text is a null string, dot falls between characters.
The
.I value
of dot is the location of the current text; the
.I contents
of dot are the characters it contains.
.CW Sam
imparts to the text no two-dimensional interpretation such as columns
or fields; text is always one-dimensional.
Even the idea of a `line' of text as understood by most UNIX programs
\(em a sequence of characters terminated by a newline character \(em
is only weakly supported.
.PP
The
.I
current file
.R
is the file to which editing commands refer.
The current text is therefore dot in the current file.
If a command doesn't explicitly name a particular file or piece of text,
the command is assumed to apply to the current text.
For the moment, ignore the presence of multiple files and consider
editing a single file.
.KF L
.XP fig1 3.5i
.Cs
Figure 1. A typical
.CW sam
screen, with the editing menu presented.
The
.CW sam
(command language) window is in the middle, with file windows above and below.
(The user interface makes it easy to create these abutting windows.)
The partially obscured window is a third file window.
The uppermost window is that to which typing and mouse operations apply,
as indicated by its heavy border.
Each window has its current text highlighted in reverse video.
The
.CW sam
window's current text is the null string on the last visible line,
indicated by a vertical bar.
See also Figure 2.
.Ce
.KE
.PP
Commands have one-letter names.
Except for non-editing commands such as writing
the file to disc, most commands make some change
to the text in dot and leave dot set to the text resulting from the change.
For example, the delete command,
.CW d ,
deletes the text in dot, replacing it by the null string and setting dot
to the result.
The change command,
.CW c ,
replaces dot by text delimited by an arbitrary punctuation character,
conventionally
a slash.  Thus,
.P1
c/Peter/
.P2
replaces the text in dot by the string
.CW Peter .
Similarly,
.P1
a/Peter/
.P2
(append) adds the string after dot, and
.P1
i/Peter/
.P2
(insert) inserts before dot.
All three leave dot set to the new text,
.CW Peter .
.PP
Newlines are part of the syntax of commands:
the newline character lexically terminates a command.
Within the inserted text, however, newlines are never implicit.
But since it is often convenient to insert multiple lines of text,
.CW sam
has a special
syntax for that case:
.P1
a
some lines of text
to be inserted in the file,
terminated by a period
on a line by itself
\&.
.P2
In the one-line syntax, a newline character may be specified by a C-like
escape, so
.P1
c/\en/
.P2
replaces dot by a single newline character.
.PP
.CW Sam
also has a substitute command,
.CW s :
.P1
s/\f2expression\fP/\f2replacement\fP/
.P2
substitutes the replacement text for the first match, in dot,
of the regular expression.
Thus, if dot is the string
.CW Peter ,
the command
.P1
s/t/st/
.P2
changes it to
.CW Pester .
In general,
.CW s
is unnecessary, but it was inherited from
.CW ed
and it has some convenient variations.
For instance, the replacement text may include the matched text,
specified by
.CW & :
.P1
s/Peter/Oh, &, &, &, &!/
.P2
.PP
There are also three commands that apply programs
to text:
.P1
< \f2UNIX program\fP
.P2
replaces dot by the output of the UNIX program.
Similarly, the
.CW >
command
runs the program with dot as its standard input, and
.CW |
does both.  For example,
.P1
| sort
.P2
replaces dot by the result of applying the standard sorting utility to it.
Again, newlines have no special significance for these
.CW sam
commands.
The text acted upon and resulting from these commands is not necessarily
bounded by newlines, although for connection with UNIX programs,
newlines may be necessary to obey conventions.
.PP
One more command:
.CW p
prints the contents of dot.
Table I summarizes
.CW sam 's
commands.
.KF
.TS
center;
c s
lfCW l.
Table I. \f(CWSam\fP commands
.sp .4
.ft CW
_
.ft
.sp .4
\f1Text commands\fP	
.sp .4
_
.sp .4
a/\f2text\fP/	Append text after dot
c/\f2text\fP/	Change text in dot
i/\f2text\fP/	Insert text before dot
d	Delete text in dot
s/\f2regexp\fP/\f2text\fP/	Substitute text for match of regular expression in dot
m \f2address\fP	Move text in dot after address
t \f2address\fP	Copy text in dot after address
.sp .4
_
.sp .4
\f1Display commands\fP	
.sp .4
_
.sp .2
p	Print contents of dot
\&=	Print value (line numbers and character numbers) of dot
.sp .4
_
.sp .4
\f1File commands\fP
.sp .4
_
.sp .2
b \f2file-list\fP	Set current file to first file in list that \f(CWsam\fP has in menu
B \f2file-list\fP	Same as \f(CWb\fP, but load new files
n	Print menu lines of all files
D \f2file-list\fP	Delete named files from \f(CWsam\fP
.sp .4
_
.sp .4
\f1I/O commands\fP	
.sp .4
_
.sp .2
e \f2filename\fP	Replace file with named disc file
r \f2filename\fP	Replace dot by contents of named disc file
w \f2filename\fP	Write file to named disc file
f \f2filename\fP	Set file name and print new menu line
< \f2UNIX-command\fP	Replace dot by standard output of command
> \f2UNIX-command\fP	Send dot to standard input of command
| \f2UNIX-command\fP	Replace dot by result of command applied to dot
! \f2UNIX-command\fP	Run the command
.sp .4
_
.sp .4
\f1Loops and conditionals\fP	
.sp .4
_
.sp .2
x/\f2regexp\fP/ \f2command\fP	For each match of regexp, set dot and run command
y/\f2regexp\fP/ \f2command\fP	Between adjacent matches of regexp, set dot and run command
X/\f2regexp\fP/ \f2command\fP	Run command in each file whose menu line matches regexp
Y/\f2regexp\fP/ \f2command\fP	Run command in each file whose menu line does not match
g/\f2regexp\fP/ \f2command\fP	If dot contains a match of regexp, run command
v/\f2regexp\fP/ \f2command\fP	If dot does not contain a match of regexp, run command
.sp .4
_
.sp .4
\f1Miscellany\fP	
.sp .4
_
.sp .2
k	Set address mark to value of dot
q	Quit
u \f2n\fP	Undo last \f2n\fP (default 1) changes
{ }	Braces group commands
.sp .3
.ft CW
_
.ft
.TE
.sp
.KE
.PP
The value of dot may be changed by
specifying an
.I address
for the command.
The simplest address is a line number:
.P1
3
.P2
refers to the third line of the file, so
.P1
3d
.P2
deletes the third line of the file, and implicitly renumbers
the lines so the old line 4 is now numbered 3.
(This is one of the few places where
.CW sam
deals with lines directly.)
Line
.CW 0
is the null string at the beginning of the file.
If a command consists of only an address, a
.CW p
command is assumed, so typing an unadorned
.CW 3
prints line 3 on the terminal.
There are a couple of other basic addresses:
a period addresses dot itself; and
a dollar sign
.CW $ ) (
addresses the null string at the end of the file.
.PP
An address is always a single substring of the file.
Thus, the address
.CW 3
addresses the characters
after the second newline of
the file through the third newline of the file.
A
.I
compound address
.R
is constructed by the comma operator
.P1
\f2address1\fP,\f2address2\fP
.P2
and addresses the substring of the file from the beginning of
.I address1
to the end of
.I address2 .
For example, the command
.CW 3,5p
prints the third through fifth lines of the file and
.CW .,$d
deletes the text from the beginning of dot to the end of the file.
.PP
These addresses are all absolute positions in the file, but
.CW sam
also has relative addresses, indicated by
.CW +
or
.CW - .
For example,
.P1
$-3
.P2
is the third line before the end of the file and
.P1
\&.+1
.P2
is the line after dot.
If no address appears to the left of the
.CW +
or
.CW - ,
dot is assumed;
if nothing appears to the right,
.CW 1
is assumed.
Therefore,
.CW .+1
may be abbreviated to just a plus sign.
.PP
The
.CW +
operator acts relative to the end of its first argument, while the
.CW -
operator acts relative to the beginning.  Thus
.CW .+1
addresses the first line after dot,
.CW .-
addresses the first line before dot, and
.CW +-
refers to the line containing the end of dot.  (Dot may span multiple lines, and
.CW +
selects the line after the end of dot, then
.CW -
backs up one line.)
.PP
The final type of address is a regular expression, which addresses the
text matched by the expression.  The expression is enclosed in slashes, as in
.P1
/\f2expression\fP/
.P2
The expressions are the same as those in the UNIX program
.CW egrep ,\u\s-4\&6,7\s+4\d
and include closures, alternations, and so on.
They find the
.I
leftmost longest
.R
string that matches the expression, that is,
the first match after the point where the search is started,
and if more than one match begins at the same spot, the longest such match.
(I assume familiarity with the syntax for regular expressions in UNIX programs.\u\s-4\&9\s+4\d)
For example,
.P1
/x/
.P2
matches the next
.CW x
character in the file,
.P1
/xx*/
.P2
matches the next run of one or more
.CW x 's,
and
.P1
/x|Peter/
.P2
matches the next
.CW x
or
.CW Peter .
For compatibility with other UNIX programs, the `any character' operator,
a period,
does not match a newline, so
.P1
/.*/
.P2
matches the text from dot to the end of the line, but excludes the newline
and so will not match across
the line boundary.
.PP
Regular expressions are always relative addresses.
The direction is forwards by default,
so
.CW /Peter/
is really an abbreviation for
.CW +/Peter/ .
The search can be reversed with a minus sign, so
.P1
.CW -/Peter/
.P2
finds the first
.CW Peter
before dot.
Regular expressions may be used with other address forms, so
.CW 0+/Peter/
finds the first
.CW Peter
in the file and
.CW $-/Peter/
finds the last.
Table II summarizes
.CW sam 's
addresses.
.KF
.TS
center;
c s
lfCW l.
Table II. \f(CWSam\fP addresses
.sp .4
.ft CW
_
.ft
.sp .4
\f1Simple addresses\fP	
.sp .4
_
.sp .2
#\f2n\fP	The empty string after character \f2n\fP
\f2n\fP	Line \f2n\fP.
/\f2regexp\fP/	The first following match of the regular expression
-/\f2regexp\fP/	The first previous match of the regular expression
$	The null string at the end of the file
\&.	Dot
\&'	The address mark, set by \f(CWk\fP command
"\f2regexp\fP"	Dot in the file whose menu line matches regexp
.sp .4
_
.sp .4
\f1Compound addresses\fP	
.sp .4
_
.sp .2
\f2a1\fP+\f2a2\fP	The address \f2a2\fP evaluated starting at right of \f2a1\fP
\f2a1\fP-\f2a2\fP	\f2a2\fP evaluated in the reverse direction starting at left of \f2a1\fP
\f2a1\fP,\f2a2\fP	From the left of \f2a1\fP to the right of \f2a2\fP (default \f(CW0,$\fP)
\f2a1\fP;\f2a2\fP	Like \f(CW,\fP but sets dot after evaluating \f2a1\fP
.sp .4
_
.sp .4
.T&
c s.
T{
The operators
.CW +
and
.CW -
are high precedence, while
.CW ,
and
.CW ;
are low precedence.
In both
.CW +
and
.CW -
forms,
.I a2
defaults to 1 and
.I a1
defaults to dot.
If both
.I a1
and
.I a2
are present,
.CW +
may be elided.
T}
.sp .5
.ft CW
_
.ft
.TE
.sp
.KE
.PP
The language discussed so far will not seem novel
to people who use UNIX text editors
such as
.CW ed
or
.CW vi .\u\s-4\&9\s+4\d
Moreover, the kinds of editing operations these commands allow, with the exception
of regular expressions and line numbers,
are clearly more conveniently handled by a mouse-based interface.
Indeed,
.CW sam 's
mouse language (discussed at length below) is the means by which
simple changes are usually made.
For large or repetitive changes, however, a textual language
outperforms a manual interface.
.PP
Imagine that, instead of deleting just one occurrence of the string
.CW Peter ,
we wanted to eliminate every
.CW Peter .
What's needed is an iterator that runs a command for each occurrence of some
text.
.CW Sam 's
iterator is called
.CW x ,
for extract:
.P1
x/\f2expression\fP/ \f2command\fP
.P2
finds all matches in dot of the specified expression, and for each
such match, sets dot to the text matched and runs the command.
So to delete all the
.CW Peters:
.P1
0,$ x/Peter/ d
.P2
(Blanks in these examples are to improve readability;
.CW sam
neither requires nor interprets them.)
This searches the entire file
.CW 0,$ ) (
for occurrences of the string
.CW Peter ,
and runs the
.CW d
command with dot set to each such occurrence.
(By contrast, the comparable
.CW ed
command would delete all
.I lines
containing
.CW Peter ;
.CW sam
deletes only the
.CW Peters .)
The address
.CW 0,$
is commonly used, and may be abbreviated to just a comma.
As another example,
.P1
, x/Peter/ p
.P2
prints a list of
.CW Peters,
one for each appearance in the file, with no intervening text (not even newlines
to separate the instances).
.PP
Of course, the text extracted by
.CW x
may be selected by a regular expression,
which complicates deciding what set of matches is chosen \(em
matches may overlap.  This is resolved by generating the matches
starting from the beginning of dot using the leftmost-longest rule,
and searching for each match starting from the end of the previous one.
Regular expressions may also match null strings, but a null match
adjacent to a non-null match is never selected; at least one character
must intervene.
For example,
.P1
, c/AAA/
x/B*/ c/-/
, p
.P2
produces as output
.P1
-A-A-A-
.P2
because the pattern
.CW B*
matches the null strings separating the
.CW A 's.
.PP
The
.CW x
command has a complement,
.CW y ,
with similar syntax, that executes the command with dot set to the text
.I between
the matches of the expression.
For example,
.P1
, c/AAA/
y/A/ c/-/
, p
.P2
produces the same result as the example above.
.PP
The
.CW x
and
.CW y
commands are looping constructs, and
.CW sam
has a pair of conditional commands to go with them.
They have similar syntax:
.P1
g/\f2expression\fP/ \f2command\fP
.P2
(guard)
runs the command exactly once if dot contains a match of the expression.
This is different from
.CW x ,
which runs the command for
.I each
match:
.CW x
loops;
.CW g
merely tests, without changing the value of dot.
Thus,
.P1
, x/Peter/ d
.P2
deletes all occurrences of
.CW Peter ,
but
.P1
, g/Peter/ d
.P2
deletes the whole file (reduces it to a null string) if
.CW Peter
occurs anywhere in the text.
The complementary conditional is
.CW v ,
which runs the command if there is
.I no
match of the expression.
.PP
These control-structure-like commands may be composed to construct more
involved operations.  For example, to print those lines of text that
contain the string
.CW Peter :
.P1
, x/.*\en/ g/Peter/ p
.P2
The
.CW x
breaks the file into lines, the
.CW g
selects those lines containing
.CW Peter ,
and the
.CW p
prints them.
This command gives an address for the
.CW x
command (the whole file), but because
.CW g
does not have an explicit address, it applies to the value of
dot produced by the
.CW x
command, that is, to each line.
All commands in
.CW sam
except for the command to write a file to disc use dot for the
default address.
.PP
Composition may be continued indefinitely.
.P1
, x/.*\en/ g/Peter/ v/SaltPeter/ p
.P2
prints those lines containing
.CW Peter
but
.I not
those containing
.CW SaltPeter .
.SH 2
Structural Regular Expressions
.LP
Unlike other UNIX text editors,
including the non-interactive ones such as
.CW sed
and
.CW awk ,\u\s-4\&7\s+4\d
.CW sam
is good for manipulating files with multi-line `records.'
An example is an on-line phone book composed of records,
separated by blank lines, of the form
.P1
Herbert Tic
44 Turnip Ave., Endive, NJ
201-5555642

Norbert Twinge
16 Potato St., Cabbagetown, NJ
201-5553145

\&...
.P2
The format may be encoded as a regular expression:
.P1
(.+\en)+
.P2
that is, a sequence of one or more non-blank lines.
The command to print Mr. Tic's entire record is then
.P1
, x/(.+\en)+/ g/^Herbert Tic$/ p
.P2
and that to extract just the phone number is
.P1
, x/(.+\en)+/ g/^Herbert Tic$/ x/^[0-9]*-[0-9]*\en/ p
.P2
The latter command breaks the file into records,
chooses Mr. Tic's record,
extracts the phone number from the record,
and finally prints the number.
.PP
A more involved problem is that of
renaming a particular variable, say
.CW n ,
to
.CW num
in a C program.
The obvious first attempt,
.P1
, x/n/ c/num/
.P2
is badly flawed: it changes not only the variable
.CW n
but any letter
.CW n
that appears.
We need to extract all the variables, and select those that match
.CW n
and only
.CW n :
.P1
, x/[A-Za-z_][A-Za-z_0-9]*/ g/n/ v/../ c/num/
.P2
The pattern
.CW [A-Za-z_][A-Za-z_0-9]*
matches C identifiers.
Next
.CW g/n/
selects those containing an
.CW n .
Then
.CW v/../
rejects those containing two (or more) characters, and finally
.CW c/num/
changes the remainder (identifiers
.CW n )
to
.CW num .
This version clearly works much better, but there may still be problems.
For example, in C character and string constants, the sequence
.CW \en
is interpreted as a newline character, and we don't want to change it to
.CW \enum.
This problem can be forestalled with a
.CW y
command:
.P1
, y/\e\en/ x/[A-Za-z_][A-Za-z_0-9]*/ g/n/ v/../ c/num/
.P2
(the second
.CW \e
is necessary because of lexical conventions in regular expressions),
or we could even reject character constants and strings outright:
.P1 0
,y/'[^']*'/ y/"[^"]*"/ x/[A-Za-z_][A-Za-z_0-9]*/ g/n/ v/../ c/num/
.P2
The
.CW y
commands in this version exclude from consideration all character constants
and strings.
The only remaining problem is to deal with the possible occurrence of
.CW \e'
or
.CW \e"
within these sequences, but it's easy to see how to resolve this difficulty.
.PP
The point of these composed commands is successive refinement.
A simple version of the command is tried, and if it's not good enough,
it can be honed by adding a clause or two.
(Mistakes can be undone; see below.
Also, the mouse language makes it unnecessary to retype the command each time.)
The resulting chains of commands are somewhat reminiscent of
shell pipelines.\u\s-4\&7\s+4\d
Unlike pipelines, though, which pass along modified
.I data ,
.CW sam
commands pass a
.I view
of the data.
The text at each step of the command is the same, but which pieces
are selected is refined step by step until the correct piece is
available to the final step of the command line, which ultimately makes the change.
.PP
In other UNIX programs, regular expressions are used only for selection,
as in the
.CW sam
.CW g
command, never for extraction as in the
.CW x
or
.CW y
command.
For example, patterns in
.CW awk \u\s-4\&7\s+4\d
are used to select lines to be operated on, but cannot be used
to describe the format of the input text, or to handle newline-free text.
The use of regular expressions to describe the structure of a piece
of text rather than its contents, as in the
.CW x
command, 
has been given a name:
.I
structural regular expressions.
.R
When they are composed, as in the above example,
they are pleasantly expressive.
Their use is discussed at greater length elsewhere.\u\s-4\&10\s+4\d
.PP
.SH 2
Multiple files
.LP
.CW Sam
has a few other commands, mostly relating to input and output.
.P1
e discfilename
.P2
replaces the contents and name of the current file with those of the named
disc file;
.P1
w discfilename
.P2
writes the contents to the named disc file; and
.P1
r discfilename
.P2
replaces dot with the contents of the named disc file.
All these commands use the current file's name if none is specified.
Finally,
.P1
f discfilename
.P2
changes the name associated with the file and displays the result:
.P1
\&'-. discfilename
.P2
This output is called the file's
.I
menu line,
.R
because it is the contents of the file's line in the button 3 menu (described
in the
next section).
The first three characters are a concise notation for the state of the file.
The apostrophe signifies that the file is modified.
The minus sign indicates the number of windows
open on the file (see the next section):
.CW -
means none,
.CW +
means one, and
.CW *
means more than one.
Finally, the period indicates that this is the current file.
These characters are useful for controlling the
.CW X
command, described shortly.
.PP
.CW Sam
may be started with a set of disc files (such as all the source for
a program) by invoking it with a list of file names as arguments, and
more may be added or deleted on demand.
.P1
B discfile1 discfile2 ...
.P2
adds the named files to
.CW sam 's
list, and
.P1
D discfile1 discfile2 ...
.P2
removes them from
.CW sam 's
memory (without effect on associated disc files).
Both these commands have a syntax for using the shell\u\s-4\&7\s+4\d
(the UNIX command interpreter) to generate the lists:
.P1
B <echo *.c
.P2
will add all C source files, and
.P1
B <grep -l variable *.c
.P2
will add all C source files referencing a particular variable
(the UNIX command
.CW grep\ -l
lists all files in its arguments that contain matches of
the specified regular expression).
Finally,
.CW D
without arguments deletes the current file.
.PP
There are two ways to change which file is current:
.P1
b filename
.P2
makes the named file current.
The
.CW B
command
does the same, but also adds any new files to
.CW sam 's
list.
(In practice, of course, the current file
is usually chosen by mouse actions, not by textual commands.)
The other way is to use a form of address that refers to files:
.P1
"\f2expression\fP" \f2address\fP
.P2
refers to the address evaluated in the file whose menu line
matches the expression (there must be exactly one match).
For example,
.P1
"peter.c" 3
.P2
refers to the third line of the file whose name matches
.CW peter.c .
This is most useful in the move
.CW m ) (
and copy
.CW t ) (
commands:
.P1
0,$ t "peter.c" 0
.P2
makes a copy of the current file at the beginning of
.CW peter.c .
.PP
The
.CW X
command
is a looping construct, like
.CW x ,
that refers to files instead of strings:
.P1
X/\f2expression\fP/ \f2command\fP
.P2
runs the command in all
files whose menu lines match the expression.  The best example is
.P1
X/'/ w
.P2
which writes to disc all modified files.
.CW Y
is the complement of
.CW X :
it runs the command on all files whose menu lines don't match the expression:
.P1
Y/\e.c/ D
.P2
deletes all files that don't have
.CW \&.c
in their names, that is, it keeps all C source files and deletes the rest.
.PP
Braces allow commands to be grouped, so
.P1
{
	\f2command1\fP
	\f2command2\fP
}
.P2
is syntactically a single command that runs two commands.
Thus,
.P1
X/\e.c/ ,g/variable/ {
	f
	, x/.*\en/ g/variable/ p
}
.P2
finds all occurrences of
.CW variable
in C source files, and prints
out the file names and lines of each match.
The precise semantics of compound operations is discussed in the implementation
sections below.
.PP
Finally,
the undo command,
.CW u ,
undoes the last command,
no matter how many files were affected.
Multiple undo operations move further back in time, so
.P1
u
u
.P2
(which may be abbreviated
.CW u2 )
undoes the last two commands.  An undo may not be undone, however, nor
may any command that adds or deletes files.
Everything else is undoable, though, including for example
.CW e
commands:
.P1
e filename
u
.P2
restores the state of the file completely, including its name, dot,
and modified bit.  Because of the undo, potentially dangerous commands
are not guarded by confirmations.  Only
.CW D ,
which destroys the information necessary to restore itself, is protected.
It will not delete a modified file, but a second
.CW D
of the same file will succeed regardless.
The
.CW q
command, which exits
.CW sam ,
is similarly guarded.
.SH 2
Mouse Interface
.LP
.CW Sam
is most commonly run
connected to a bitmap display and mouse for interactive editing.
The only difference in the command language
between regular, mouse-driven
.CW sam
and
.CW sam\ -d
is that if an address
is provided without a command,
.CW sam\ -d
will print the text referenced by the address, but
regular
.CW sam
will highlight it on the screen \(em in fact,
dot is always highlighted (see Figure 2).
.WS 1
.KF
.XP fig3 2.04i
.Cs
Figure 2. A
.CW sam
window.  The scroll bar down the left
represents the file, with the bubble showing the fraction
visible in the window.
The scroll bar may be manipulated by the mouse for convenient browsing.
The current text,
which is highlighted, need not fit on a line.  Here it consists of one partial
line, one complete line, and final partial line.
.Ce
.KE
.PP
Each file may have zero or more windows open on the display.
At any time, only one window in all of
.CW sam
is the
.I
current window,
.R
that is, the window to which typing and mouse actions refer;
this may be the
.CW sam
window (that in which commands may be typed)
or one of the file windows.
When a file has multiple windows, the image of the file in each window
is always kept up to date.
The current file is the last file affected by a command,
so if the
.CW sam
window is current,
the current window is not a window on the current file.
However, each window on a file has its own value of dot,
and when switching between windows on a single file,
the file's value of dot is changed to that of the window.
Thus, flipping between windows behaves in the obvious, convenient way.
.PP
The mouse on the Blit has three buttons, numbered left to right.
Button 3 has a list of commands to manipulate windows,
followed by a list of `menu lines' exactly as printed by the
.CW f
command, one per file (not one per window).
These menu lines are sorted by file name.
If the list is long, the Blit menu software will make it more manageable
by generating a scrolling menu instead of an unwieldy long list.
Using the menu to select a file from the list makes that file the current
file, and the most recently current window in that file the current window.
But if that file is already current, selecting it in the menu cycles through
the windows on the file; this simple trick avoids a special menu to
choose windows on a file.
If there is no window open on the file,
.CW sam
changes the mouse cursor to prompt the user to create one.
.PP
The commands on the button 3 menu are straightforward (see Figure 3), and
are like the commands to manipulate windows in
.CW mux ,\u\s-4\&8\s+4\d
the Blit's window system.
.CW New
makes a new file, and gives it one empty window, whose size is determined
by a rectangle swept by the mouse.
.CW Zerox
prompts for a window to be selected, and
makes a clone of that window; this is how multiple windows are created on one file.
.CW Reshape
changes the size of the indicated window, and
.CW close
deletes it.  If that is the last window open on the file,
.CW close
first does a
.CW D
command on the file.
.CW Write
is identical to a
.CW w
command on the file; it is in the menu purely for convenience.
Finally,
.CW ~~sam~~
is a menu item that appears between the commands and the file names.
Selecting it makes the
.CW sam
window the current window,
causing subsequent typing to be interpreted as commands.
.KF
.XP fig2 2.74i
.Cs
Figure 3. The menu on button 3.
The black rectangle on the left is a scroll bar; the menu is limited to
the length shown to prevent its becoming unwieldy.
Above the
.CW ~~sam~~
line is a list of commands;
beneath it is a list of files, presented exactly as with the
.CW f
command.
.Ce
.KE
.PP
When
.CW sam
requests that a window be swept, in response to
.CW new ,
.CW zerox
or
.CW reshape ,
it changes the mouse cursor from the usual arrow to a box with
a small arrow.
In this state, the mouse may be used to indicate an arbitrary rectangle by
pressing button 3 at one corner and releasing it at the opposite corner.
More conveniently,
button 3 may simply be clicked,
whereupon
.CW sam
creates the maximal rectangle that contains the cursor
and abuts the
.CW sam
window.
By placing the
.CW sam
window in the middle of the screen, the user can define two regions (one above,
one below) in which stacked fully-overlapping
windows can be created with minimal fuss (see Figure 1).
This simple user interface trick makes window creation noticeably easier.
.PP
The cut-and-paste editor is essentially the same as that in Smalltalk-80.\u\s-4\&11\s+4\d
The text in dot is always highlighted on the screen.
When a character is typed it replaces dot, and sets dot to the null
string after the character.  Thus, ordinary typing inserts text.
Button 1 is used for selection:
pressing the button, moving the mouse, and lifting the button
selects (sets dot to) the text between the points where the
button was pressed and released.
Pressing and releasing at the same point selects a null string; this
is called clicking.  Clicking twice quickly, or
.I
double clicking,
.R
selects larger objects;
for example, double clicking in a word selects the word,
double clicking just inside an opening bracket selects the text
contained in the brackets (handling nested brackets correctly),
and similarly for
parentheses, quotes, and so on.
The double-clicking rules reflect a bias toward
programmers.
If
.CW sam
were intended more for word processing, double-clicks would probably
select linguistic structures such as sentences.
.PP
If button 1 is pressed outside the current window, it makes the indicated
window current.
This is the easiest way to switch between windows and files.
.PP
Pressing button 2 brings up a menu of editing functions (see Figure 4).
These mostly apply to the selected text:
.CW cut
deletes the selected text, and remembers it in a hidden buffer called the
.I
snarf buffer,
.R
.CW paste
replaces the selected text by the contents of the snarf buffer,
.CW snarf
just copies the selected text to the snarf buffer,
.CW look
searches forward for the next literal occurrence of the selected text, and
.CW <mux>
exchanges snarf buffers with the window system in which
.CW sam
is running.
Finally, the last regular expression used appears as a menu entry
to search
forward for the next occurrence of a match for the expression.
.WS 1
.KF
.XP fig4 1.20i
.Cs
Figure 4. The menu on button 2.
The bottom entry tracks the most recently used regular expression, which may
be literal text.
.Ce
.KE
.PP
The relationship between the command language and the mouse language is
entirely due to the equality of dot and the selected text chosen
with button 1 on the mouse.
For example, to make a set of changes in a C subroutine, dot can be
set by double clicking on the left brace that begins the subroutine,
which sets dot for the command language.
An address-free command then typed in the
.CW sam
window will apply only to the text between the opening and closing
braces of the function.
The idea is to select what you want, and then say what you want
to do with it, whether invoked by a menu selection or by a typed command.
And of course, the value of dot is highlighted on
the display after the command completes.
This relationship between mouse interface and command language
is clumsy to explain, but comfortable, even natural, in practice.
.SH
The Implementation
.LP
The next few sections describe how
.CW sam
is put together, first the host part,
then the inter-component communication,
then the terminal part.
After explaining how the command language is implemented,
the discussion follows (roughly) the path of a character
from the temporary file on disc to the screen.
The presentation centers on the data structures,
because that is how the program was designed and because
the algorithms are easy to provide, given the right data
structures.
.SH 2
Parsing and execution
.LP
The command language is interpreted by parsing each command with a
table-driven recursive
descent parser, and when a complete command is assembled, invoking a top-down
executor.
Most editors instead employ a simple character-at-a-time
lexical scanner.
Use of a parser makes it
easy and unambiguous to detect when a command is complete,
which has two advantages.
First, escape conventions such as backslashes to quote
multiple-line commands are unnecessary;  if the command isn't finished,
the parser keeps reading.  For example, a multiple-line append driven by an
.CW x
command is straightforward:
.P1
x/.*\en/ g/Peter/ a
one line about Peter
another line about Peter
\&.
.P2
Other UNIX editors would require a backslash after all but the last line.
.PP
The other advantage is specific to the two-process structure of
.CW sam .
The host process must decide when a command is completed so the
command interpreter can be called.  This problem is easily resolved
by having the lexical analyzer read the single stream of events from the
terminal, directly executing all typing and mouse commands,
but passing to the parser characters typed to the
.CW sam
command window.
This scheme is slightly complicated by the availability of cut-and-paste
editing in the
.CW sam
window, but that difficulty is resolved by applying the rules
used in
.CW mux :
when a newline is typed to the
.CW sam
window, all text between the newline and the previously typed newline
is made available to the parser.
This permits arbitrary editing to be done to a command before
typing newline and thereby requesting execution.
.PP
The parser is driven by a table because the syntax of addresses
and commands is regular enough
to be encoded compactly.  There are few special cases, such as the
replacement text in a substitution, so the syntax of almost all commands
can be encoded with a few flags.
These include whether the command allows an address (for example,
.CW e
does not), whether it takes a regular expression (as in
.CW x
and
.CW s ),
whether it takes replacement text (as in
.CW c
or
.CW i ),
which may be multi-line, and so on.
The internal syntax of regular expressions is handled by a separate
parser; a regular expression is a leaf of the command parse tree.
Regular expressions are discussed fully in the next section.
.PP
The parser table also has information about defaults, so the interpreter
is always called with a complete tree.  For example, the parser fills in
the implicit
.CW 0
and
.CW $
in the abbreviated address
.CW ,
(comma),
inserts a
.CW +
to the left of an unadorned regular expression in an address,
and provides the usual default address
.CW .
(dot) for commands that expect an address but are not given one.
.PP
Once a complete command is parsed, the evaluation is easy.
The address is evaluated left-to-right starting from the value of dot,
with a mostly ordinary expression evaluator.
Addresses, like many of the data structures in
.CW sam ,
are held in a C structure and passed around by value:
.P1
typedef long Posn;    /* Position in a file */
typedef struct Range{
        Posn    p1, p2;
}Range;
typedef struct Address{
        Range   r;
        File    *f;
}Address;
.P2
An address is encoded as a substring (character positions
.CW p1
to
.CW p2 )
in a file
.CW f .
(The data type
.CW File
is described in detail below.)
.PP
The address interpreter is an
.CW Address -valued
function that traverses the parse tree describing an address (the
parse tree for the address has type
.CW Addrtree ):
.P1
Address
address(ap, a, sign)
	Addrtree *ap;
	Address a;
	int sign;
{
	Address a2;
	do
		switch(ap->type){
		case '.':
			a=a.f->dot;
			break;
		case '$':
			a.r.p1=a.r.p2=a.f->nbytes;
			break;
		case '"':	
			a=matchfile(a, ap->aregexp)->dot; 
			break;
		case ',':
			a2=address(ap->right, a, 0);
			a=address(ap->left, a, 0);
			if(a.f!=a2.f || a2.r.p2<a.r.p1)
				error(Eorder);
			a.r.p2=a2.r.p2;
			return a;
		/* and so on */
		}
	while((ap=ap->right)!=0);
	return a;
}
.P2
.PP
Throughout, errors are handled by a non-local
.CW goto
(a
.CW setjmp/longjmp
in C terminology)
hidden in a routine called
.CW error
that immediately aborts the execution, retracts any
partially made changes (see the section below on `undoing'), and
returns to the top level of the parser.
The argument to
.CW error
is an enumeration type that
is translated to a terse but possibly helpful
message such as `?addresses out of order.'
Very common messages are kept short; for example the message for
a failed regular expression search is `?search.'
.PP
Character addresses such as
.CW #3
are trivial to implement, as the
.CW File
data structure is accessible by character number.
However,
.CW sam
keeps no information about the position of newlines \(em it is too
expensive to track dynamically \(em so line addresses are computed by reading
the file, counting newlines.  Except in very large files, this has proven
acceptable: file access is fast enough to make the technique practical,
and lines are not central to the structure of the command language.
.PP
The command interpreter, called
.CW cmdexec ,
is also straightforward.  The parse table includes a
function to call to interpret a particular command.  That function
receives as arguments
the calculated address
for the command
and the command tree (of type
.CW Cmdtree ),
which may contain information such as the subtree for compound commands.
Here, for example, is the function for the
.CW g
and
.CW v
commands:
.P1
int
g_cmd(a, cp)
	Address a;
	Cmdtree *cp;
{
	compile(cp->regexp);
	if(execute(a.f, a.r.p1, a.r.p2)!=(cp->cmdchar=='v')){
		a.f->dot=a;
		return cmdexec(a, cp->subcmd);
	}
	return TRUE;	/* cause execution to continue */
}
.P2
.CW Compile "" (
and
.CW execute
are part of the regular expression code, described in the next section.)
Because the parser and the
.CW File
data structure do most of the work, most commands
are similarly brief.
.SH 2
Regular expressions
.LP
The regular expression code in
.CW sam
is an interpreted, rather than compiled on-the-fly, implementation of Thompson's
non-deterministic finite automaton algorithm.\u\s-4\&12\s+4\d
The syntax and semantics of the expressions are as in the UNIX program
.CW egrep ,
including alternation, closures, character classes, and so on.
The only changes in the notation are two additions:
.CW \en
is translated to, and matches, a newline character, and
.CW @
matches any character.  In
.CW egrep ,
the character
.CW \&.
matches any character except newline, and in
.CW sam
the same rule seemed safest, to prevent idioms like
.CW \&.*
from spanning newlines.
.CW Egrep
expressions are arguably too complicated for an interactive editor \(em
certainly it would make sense if all the special characters were two-character
sequences, so that most of the punctuation characters wouldn't have
peculiar meanings \(em but for an interesting command language, full
regular expressions are necessary, and
.CW egrep
defines the full regular expression syntax for UNIX programs.
Also, it seemed superfluous to define a new syntax, since various UNIX programs
.CW ed , (
.CW egrep
and
.CW vi )
define too many already.
.PP
The expressions are compiled by a routine,
.CW compile ,
that generates the description of the non-deterministic finite state machine.
A second routine,
.CW execute ,
interprets the machine to generate the leftmost-longest match of the
expression in a substring of the file.
The algorithm is described elsewhere.\u\s-4\&12,13\s+4\d
.CW Execute
reports
whether a match was found, and sets a global variable,
of type
.CW Range ,
to the substring matched.
.PP
A trick is required to evaluate the expression in reverse, such as when
searching backwards for an expression.
For example,
.P1
-/P.*r/
.P2
looks backwards through the file for a match of the expression.
The expression, however, is defined for a forward search.
The solution is to construct a machine identical to the machine
for a forward search except for a reversal of all the concatenation
operators (the other operators are symmetric under direction reversal),
to exchange the meaning of the operators
.CW ^
and
.CW $ ,
and then to read the file backwards, looking for the
usual earliest longest match.
.PP
.CW Execute
generates only one match each time it is called.
To interpret looping constructs such as the
.CW x
command,
.CW sam
must therefore synchronize between
calls of
.CW execute
to avoid
problems with null matches.
For example, even given the leftmost-longest rule,
the expression
.CW a*
matches three times in the string
.CW ab
(the character
.CW a ,
the null string between the
.CW a
and
.CW b ,
and the final null string).
After returning a match for the
.CW a ,
.CW sam
must not match the null string before the
.CW b .
The algorithm starts
.CW execute
at the end of its previous match, and
if the match it returns
is null and abuts the previous match, rejects the match and advances
the initial position one character.
.SH 2
Memory allocation
.LP
The C language has no memory allocation primitives, although a standard
library routine,
.CW malloc ,
provides adequate service for simple programs.
For specific uses, however,
it can be better to write a custom allocator.
The allocator (or rather, pair of allocators) described here
work in both the terminal and host parts of
.CW sam .
They are designed for efficient manipulation of strings,
which are allocated and freed frequently and vary in length from essentially
zero to 32 Kbytes (very large strings are written to disc).
More important, strings may be large and change size often,
so to minimize memory usage it is helpful to reclaim and to coalesce the
unused portions of strings when they are truncated.
.PP
Objects to be allocated in
.CW sam
are of two flavors:
the first is C
.CW structs ,
which are small and often addressed by pointer variables;
the second is variable-sized arrays of characters
or integers whose
base pointer is always used to access them.
The memory allocator in
.CW sam
is therefore in two parts:
first, a traditional first-fit allocator that provides fixed storage for
.CW structs ;
and second, a garbage-compacting allocator that reduces storage
overhead for variable-sized objects, at the cost of some bookkeeping.
The two types of objects are allocated from adjoining arenas, with
the garbage-compacting allocator controlling the arena with higher addresses.
Separating into two arenas simplifies compaction and prevents fragmentation due
to immovable objects.
The access rules for garbage-compactable objects
(discussed in the next paragraph) allow them to be relocated, so when
the first-fit arena needs space, it moves the garbage-compacted arena
to higher addresses to make room.  Storage is therefore created only
at successively higher addresses, either when more garbage-compacted
space is needed or when the first-fit arena pushes up the other arena.
.PP
Objects that may be compacted declare to the
allocator a cell that is guaranteed to be the sole repository of the
address of the object whenever a compaction can occur.
The compactor can then update the address when the object is moved.
For example, the implementation of type
.CW List
(really a variable-length array)
is:
.P1
typedef struct List{
        int     nused;
        long    *ptr;
}List;
.P2
The
.CW ptr
cell must always be used directly, and never copied.  When a
.CW List
is to be created the
.CW List
structure is allocated in the ordinary first-fit arena
and its
.CW ptr
is allocated in the garbage-compacted arena.
A similar data type for strings, called
.CW String ,
stores variable-length character arrays of up to 32767 elements.
.PP
A related matter of programming style:
.CW sam
frequently passes structures by value, which
simplifies the code.
Traditionally, C programs have
passed structures by reference, but implicit allocation on
the stack is easier to use.
Structure passing is a relatively new feature of C
(it is not in the 
standard reference manual for C\u\s-4\&14\s+4\d), and is poorly supported in most
commercial C compilers.
It's convenient and expressive, though,
and simplifies memory management by
avoiding the allocator altogether
and eliminating pointer aliases.
.SH 2
Data structures for manipulating files
.LP
Experience with
.CW jim
showed that the requirements
of the file data structure were few, but strict.
First, files need to be read and written quickly;
adding a fresh file must be painless.
Second, the implementation must place no arbitrary upper limit on
the number or sizes of files.  (It should be practical to edit many files,
and files up to megabytes in length should be handled gracefully.)
This implies that files be stored on disc, not in main memory.
(Aficionados of virtual memory may argue otherwise, but the
implementation of virtual
memory in our system is not something to depend on
for good performance.)
Third, changes to files need be made by only two primitives:
deletion and insertion.
These are inverses of each other,
which simplifies the implementation of the undo operation.
Finally,
it must be easy and efficient to access the file, either
forwards or backwards, a byte at a time.
.PP
The
.CW File
data type is constructed from three simpler data structures that hold arrays
of characters.
Each of these types has an insertion and deletion operator, and the
insertion and deletion operators of the
.CW File
type itself are constructed from them.
.PP
The simplest type is the
.CW String ,
which is used to hold strings in main memory.
The code that manages
.CW Strings
guarantees that they will never be longer
than some moderate size, and in practice they are rarely larger than 8 Kbytes.
.CW Strings
have two purposes: they hold short strings like file names with little overhead,
and because they are deliberately small, they are efficient to modify.
They are therefore used as the data structure for in-memory caches.
.PP
The disc copy of the file is managed by a data structure called a
.CW Disc ,
which corresponds to a temporary file.  A
.CW Disc
has no storage in main memory other than bookkeeping information;
the actual data being held is all on the disc.
To reduce the number of open files needed,
.CW sam
opens a dozen temporary UNIX files and multiplexes the
.CW Discs
upon them.
This permits many files to
be edited; the entire
.CW sam
source (48 files) may be edited comfortably with a single
instance of
.CW sam .
Allocating one temporary file per
.CW Disc
would strain the operating system's limit on the number of open files.
Also, spreading the traffic among temporary files keeps the files shorter,
and shorter files are more efficiently implemented by the UNIX
I/O subsystem.
.PP
A
.CW Disc
is an array of fixed-length blocks, each of which contains
between 1 and 4096 characters of active data.
(The block size of our UNIX file system is 4096 bytes.)
The block addresses within the temporary file and the length of each
block are stored in a
.CW List .
When changes are made the live part of blocks may change size.
Blocks are created and coalesced when necessary to try to keep the sizes
between 2048 and 4096 bytes.
An actively changing part of the
.CW Disc
therefore typically has about a kilobyte of slop that can be
inserted or deleted
without changing more than one block or affecting the block order.
When an insertion would overflow a block, the block is split, a new one
is allocated to receive the overflow, and the memory-resident list of blocks
is rearranged to reflect the insertion of the new block.
.PP
Obviously, going to the disc for every modification to the file is
prohibitively expensive.
The data type
.CW Buffer
consists of a
.CW Disc
to hold the data and a
.CW String
that acts as a cache.
This is the first of a series of caches throughout the data structures in
.CW sam.
The caches not only improve performance, they provide a way to organize
the flow of data, particularly in the communication between the host
and terminal.
This idea is developed below, in the section on communications.
.PP
To reduce disc traffic, changes to a
.CW Buffer
are mediated by a variable-length string, in memory, that acts as a cache.
When an insertion or deletion is made to a
.CW Buffer ,
if the change can be accommodated by the cache, it is done there.
If the cache becomes bigger than a block because of an insertion,
some of it is written to the
.CW Disc
and deleted from the cache.
If the change does not intersect the cache, the cache is flushed.
The cache is only loaded at the new position if the change is smaller than a block;
otherwise, it is sent directly to the
.CW Disc .
This is because
large changes are typically sequential,
whereupon the next change is unlikely to overlap the current one.
.PP
A
.CW File
comprises a
.CW String
to hold the file name and some ancillary data such as dot and the modified bit.
The most important components, though, are a pair of
.CW Buffers ,
one called the transcript and the other the contents.
Their use is described in the next section.
.PP
The overall structure is shown in Figure 5.
Although it may seem that the data is touched many times on its
way from the
.CW Disc ,
it is read (by one UNIX system call) directly into the cache of the
associated
.CW Buffer ;
no extra copy is done.
Similarly, when flushing the cache, the text is written
directly from the cache to disc.
Most operations act directly on the text in the cache.
A principle applied throughout
.CW sam
is that the fewer times the data is copied, the faster the program will run
(see also the paper by Waite\u\s-4\&15\s+4\d).
.KF
.PS
copy "fig5.pic"
.PE
.Cs
Figure 5. File data structures.
The temporary files are stored in the standard repository for such files
on the host system.
.Ce
.KE
.PP
The contents of a
.CW File
are accessed by a routine that
copies to a buffer a substring of a file starting at a specified offset.
To read a byte at a time, a
.CW File "" per-
array is loaded starting from a specified initial position,
and bytes may then be read from the array.
The implementation is done by a macro similar to the C standard I/O
.CW getc
macro.\u\s-4\&14\s+4\d
Because the reading may be done at any address, a minor change to the
macro allows the file to be read backwards.
This array is read-only; there is no
.CW putc .
.SH 2
Doing and undoing
.LP
.CW Sam
has an unusual method for managing changes to files.
The command language makes it easy to specify multiple variable-length changes
to a file millions of bytes long, and such changes
must be made efficiently if the editor is to be practical.
The usual techniques for inserting and deleting strings
are inadequate under these conditions.
The
.CW Buffer
and
.CW Disc
data structures are designed for efficient random access to long strings,
but care must be taken to avoid super-linear behavior when making
many changes simultaneously.
.PP
.CW Sam
uses a two-pass algorithm for making changes, and treats each file as a database
against which transactions are registered.
Changes are not made directly to the contents.
Instead, when a command is started, a `mark' containing
a sequence number is placed in the transcript
.CW Buffer ,
and each change made to the file, either an insertion or deletion
or a change to the file name,
is appended to the end of the transcript.
When the command is complete, the transcript is rewound to the
mark and applied to the contents.
.PP
One reason for separating evaluation from
application in this way is to simplify tracking the addresses of changes
made in the middle of a long sequence.
The two-pass algorithm also allows all changes to apply to the
.I original
data: no change can affect another change made in the same command.
This is particularly important when evaluating an
.CW x
command because it prevents regular expression matches
from stumbling over changes made earlier in the execution.
Also, the two-pass
algorithm is cleaner than the way other UNIX editors allow changes to
affect each other;
for example,
.CW ed 's
idioms to do things like delete every other line
depend critically on the implementation.
Instead,
.CW sam 's
simple model, in which all changes in a command occur effectively
simultaneously, is easy to explain and to understand.
.PP
The records in the transcript are of the form ``delete substring from
locations
123 to 456'' and ``insert 11 characters `hello there' at location 789.''
(It is an error if the changes are not at monotonically greater
positions through the file.)
While the update is occurring, these numbers must be
offset by earlier changes, but that is straightforward and
local to the update routine;
moreover, all the numbers have been computed
before the first is examined.
.PP
Treating the file as a transaction system has another advantage:
undo is trivial.
All it takes is to invert the transcript after it has been
implemented, converting insertions
into deletions and vice versa, and saving them in a holding
.CW Buffer .
The `do' transcript can then be deleted from
the transcript
.CW Buffer
and replaced by the `undo' transcript.
If an undo is requested, the transcript is rewound and the undo transcript
executed.
Because the transcript
.CW Buffer
is not truncated after each command, it accumulates
successive changes.
A sequence of undo commands
can therefore back up the file arbitrarily,
which is more helpful than the more commonly implemented self-inverse form of undo.
.CW Sam "" (
provides no way to undo an undo, but if it were desired,
it would be easy to provide by re-interpreting the `do' transcript.)
Each mark in the transcript contains a sequence number and the offset into
the transcript of the previous mark, to aid in unwinding the transcript.
Marks also contain the value of dot and the modified bit so these can be
restored easily.
Undoing multiple files is easy; it merely demands undoing all files whose
latest change has the same sequence number as the current file.
.PP
Another benefit of having a transcript is that errors encountered in the middle
of a complicated command need not leave the files in an intermediate state.
By rewinding the transcript to the mark beginning the command,
the partial command can be trivially undone.
.PP
When the update algorithm was first implemented, it was unacceptably slow,
so a cache was added to coalesce nearby changes,
replacing multiple small changes by a single larger one.
This reduced the number
of insertions into the transaction
.CW Buffer ,
and made a dramatic improvement in performance,
but made it impossible
to handle changes in non-monotonic order in the file; the caching method
only works if changes don't overlap.
Before the cache was added, the transaction could in principle be sorted
if the changes were out of order, although
this was never done.
The current status is therefore acceptable performance with a minor
restriction on global changes, which is sometimes, but rarely, an annoyance.
.PP
The update algorithm obviously paws the data more than simpler
algorithms, but it is not prohibitively expensive;
the caches help.
(The principle of avoiding copying the data is still honored here,
although not as piously:
the data is moved from contents' cache to
the transcript's all at once and through only one internal buffer.)
Performance figures confirm the efficiency.
To read from a dead start a hundred kilobyte file on a VAX-11/750
takes 1.4 seconds of user time, 2.5 seconds of system time,
and 5 seconds of real time.
Reading the same file in
.CW ed
takes 6.0 seconds of user time, 1.7 seconds of system time,
and 8 seconds of real time.
.CW Sam
uses about half the CPU time.
A more interesting example is the one stated above:
inserting a character between every pair of characters in the file.
The
.CW sam
command is
.P1
,y/@/ a/x/
.P2
and takes 3 CPU seconds per kilobyte of input file, of which
about a third is spent in the regular expression code.
This translates to about 500 changes per second.
.CW Ed
takes 1.5 seconds per kilobyte to make a similar change (ignoring newlines),
but cannot undo it.
The same example in
.CW ex ,\u\s-4\&9\s+4\d
a variant of
.CW ed
done at the University of California at Berkeley,
which allows one level of undoing, again takes 3 seconds.
In summary,
.CW sam 's
performance is comparable to that of other UNIX editors, although it solves
a harder problem.
.SH 2
Communications
.LP
The discussion so far has described the implementation of the host part of
.CW sam ;
the next few sections explain how a machine with mouse and bitmap display
can be engaged to improve interaction.
.CW Sam
is not the first editor to be written as two processes,\u\s-4\&16\s+4\d
but its implementation
has some unusual aspects.
.PP
There are several ways
.CW sam 's
host and terminal parts may be connected.
The first and simplest is to forgo the terminal part and use the host
part's command language to edit text on an ordinary terminal.
This mode is invoked by starting
.CW sam
with the
.CW -d
option.
With no options,
.CW sam
runs separate host and terminal programs,
communicating with a message protocol over the physical
connection that joins them.
Typically, the connection is an RS-232 link between a Blit
(the prototypical display for
.CW sam )
and a host running
the Ninth Edition of the UNIX operating system.\u\s-4\&8\s+4\d
(This is the version of the system used in the Computing Sciences Research
Center at AT&T Bell Laboratories [now Lucent Technologies, Bell Labs], where I work.  Its relevant
aspects are discussed in the Blit paper.\u\s-4\&1\s+4\d)
The implementation of
.CW sam
for the SUN computer runs both processes on the same machine and
connects them by a pipe.
.PP
The low bandwidth of an RS-232 link
necessitated the split between
the two programs.
The division is a mixed blessing:
a program in two parts is much harder to write and to debug
than a self-contained one,
but the split makes several unusual configurations possible.
The terminal may be physically separated from the host, allowing the conveniences
of a mouse and bitmap display to be taken home while leaving the files at work.
It is also possible to run the host part on a remote machine:
.P1
sam -r host
.P2
connects to the terminal in the usual way, and then makes a call
across the network to establish the host part of
.CW sam
on the named machine.
Finally, it cross-connects the I/O to join the two parts.
This allows
.CW sam
to be run on machines that do not support bitmap displays;
for example,
.CW sam
is the editor of choice on our Cray X-MP/24.
.CW Sam
.CW -r
involves
.I three
machines: the remote host, the terminal, and the local host.
The local host's job is simple but vital: it passes the data
between the remote host and terminal.
.PP
The host and terminal exchange messages asynchronously
(rather than, say, as remote procedure calls) but there is no
error detection or correction
because, whatever the configuration, the connection is reliable.
Because the terminal handles mundane interaction tasks such as
popping up menus and interpreting the responses, the messages are about
data, not actions.
For example, the host knows nothing about what is displayed on the screen,
and when the user types a character, the message sent to the host says
``insert a one-byte string at location 123 in file 7,'' not ``a character
was typed at the current position in the current file.''
In other words, the messages look very much like the transaction records
in the transcripts.
.PP
Either the host or terminal part of
.CW sam
may initiate a change to a file.
The command language operates on the host, while typing and some
mouse operations are executed directly in the terminal to optimize response.
Changes initiated by the host program must be transmitted to the terminal,
and
vice versa.
(A token is exchanged to determine which end is in control,
which means that characters typed while a time-consuming command runs
must be buffered and do not appear until the command is complete.)
To maintain consistent information,
the host and terminal track changes through a per-file
data structure that records what portions of the file
the terminal has received.
The data structure, called a
.CW Rasp
(a weak pun: it's a file with holes)
is held and updated by both the host and terminal.
A
.CW Rasp
is a list of
.CW Strings
holding those parts of the file known to the terminal,
separated by counts of the number of bytes in the interstices.
Of course, the host doesn't keep a separate copy of the data (it only needs
the lengths of the various pieces),
but the structure is the same on both ends.
.PP
The
.CW Rasp
in the terminal doubles as a cache.
Since the terminal keeps the text for portions of the file it has displayed,
it need not request data from the host when revisiting old parts of the file
or redrawing obscured windows, which speeds things up considerably
over low-speed links.
.PP
It's trivial for the terminal to maintain its
.CW Rasp ,
because all changes made on the terminal apply to parts of the file
already loaded there.
Changes made by the host are compared against the
.CW Rasp
during the update sequence after each command.
Small changes to pieces of the file loaded in the terminal
are sent in their entirety.
Larger changes, and changes that fall entirely in the holes,
are transmitted as messages without literal data:
only the lengths of the deleted and inserted strings are transmitted.
When a command is completed, the terminal examines its visible
windows to see if any holes in their
.CW Rasps
intersect the visible portion of the file.
It then requests the missing data from the host,
along with up to 512 bytes of surrounding data, to minimize
the number of messages when visiting a new portion of the file.
This technique provides a kind of two-level lazy evaluation for the terminal.
The first level sends a minimum of information about
parts of the file not being edited interactively;
the second level waits until a change is displayed before
transmitting the new data.
Of course,
performance is also helped by having the terminal respond immediately to typing
and simple mouse requests.
Except for small changes to active pieces of the file, which are
transmitted to the terminal without negotiation,
the terminal is wholly responsible for deciding what is displayed;
the host uses the
.CW Rasp
only to tell the terminal what might be relevant.
.PP
When a change is initiated by the host,
the messages to the terminal describing the change
are generated by the routine that applies the transcript of the changes
to the contents of the
.CW File .
Since changes are undone by the same update routine,
undoing requires
no extra code in the communications;
the usual messages describing changes to the file are sufficient
to back up the screen image.
.PP
The
.CW Rasp
is a particularly good example of the way caches are used in
.CW sam .
First, it facilitates access to the active portion of the text by placing
the busy text in main memory.
In so doing, it provides efficient access
to a large data structure that does not fit in memory.
Since the form of data is to be imposed by the user, not by the program,
and because characters will frequently be scanned sequentially,
files are stored as flat objects.
Caches help keep performance good and linear when working with such
data.
.PP
Second, the
.CW Rasp
and several of the other caches have some
.I read-ahead;
that is, the cache is loaded with more information than is needed for
the job immediately at hand.
When manipulating linear structures, the accesses are usually sequential,
and read-ahead can significantly reduce the average time to access the
next element of the object.
Sequential access is a common mode for people as well as programs;
consider scrolling through a document while looking for something.
.PP
Finally, like any good data structure,
the cache guides the algorithm, or at least the implementation.
The
.CW Rasp
was actually invented to control the communications between the host and
terminal parts, but I realized very early that it was also a form of
cache.  Other caches were more explicitly intended to serve a double
purpose: for example, the caches in
.CW Files
that coalesce updates not only reduce traffic to the
transcript and contents
.CW Buffers ,
they also clump screen updates so that complicated changes to the
screen are achieved in
just a few messages to the terminal.
This saved me considerable work: I did not need to write special
code to optimize the message traffic to the
terminal.
Caches pay off in surprising ways.
Also, they tend to be independent, so their performance improvements
are multiplicative.
.SH 2
Data structures in the terminal
.LP
The terminal's job is to display and to maintain a consistent image of
pieces of the files being edited.
Because the text is always in memory, the data structures are
considerably simpler than those in the host part.
.PP
.CW Sam
typically has far more windows than does
.CW mux ,
the window system within which its Blit implementation runs.
.CW Mux
has a fairly small number of asynchronously updated windows;
.CW sam
needs a large number of synchronously updated windows that are
usually static and often fully obscured.
The different tradeoffs guided
.CW sam
away from the memory-intensive implementation of windows, called
.CW Layers ,\u\s-4\&17\s+4\d
used in
.CW mux.
Rather than depending on a complete bitmap image of the display for each window,
.CW sam
regenerates the image from its in-memory text
(stored in the
.CW Rasp )
when necessary, although it will use such an image if it is available.
Like
.CW Layers ,
though,
.CW sam
uses the screen bitmap as active storage in which to update the image using
.CW bitblt .\u\s-4\&18,19\s+4\d
The resulting organization, pictured in Figure 6,
has a global array of windows, called
.CW Flayers ,
each of which holds an image of a piece of text held in a data structure
called a
.CW Frame ,
which in turn represents
a rectangular window full of text displayed in some
.CW Bitmap .
Each
.CW Flayer
appears in a global list that orders them all front-to-back
on the display, and simultaneously as an element of a per-file array
that holds all the open windows for that file.
The complement in the terminal of the
.CW File
on the host is called a
.CW Text ;
each connects its
.CW Flayers
to the associated
.CW Rasp .
.KF
.PS
copy "fig6.pic"
.PE
.Cs
Figure 6. Data structures in the terminal.
.CW Flayers
are also linked together into a front-to-back list.
.CW Boxes
are discussed in the next section.
.Ce
.KE
.PP
The
.CW Bitmap
for a
.CW Frame
contains the image of the text.
For a fully visible window, the
.CW Bitmap
will be the screen (or at least the
.CW Layer
in which
.CW sam
is being run),
while for partially obscured windows the
.CW Bitmap
will be off-screen.
If the window is fully obscured, the
.CW Bitmap
will be null.
.PP
The
.CW Bitmap
is a kind of cache.
When making changes to the display, most of the original image will
look the same in the final image, and the update algorithms exploit this.
The
.CW Frame
software updates the image in the
.CW Bitmap
incrementally; the
.CW Bitmap
is not just an image, it is a data structure.\u\s-4\&18,19\s+4\d
The job of the software that updates the display is therefore
to use as much as possible of the existing image (converting the
text from ASCII characters to pixels is expensive) in a sort of two-dimensional
string insertion algorithm.
The details of this process are described in the next section.
.PP
The
.CW Frame
software has no code to support overlapping windows;
its job is to keep a single
.CW Bitmap
up to date.
It falls to the
.CW Flayer
software to multiplex the various
.CW Bitmaps
onto the screen.
The problem of maintaining overlapping
.CW Flayers
is easier than for
.CW Layers \u\s-4\&17\s+4\d
because changes are made synchronously and because the contents of the window
can be reconstructed from the data stored in the
.CW Frame ;
the
.CW Layers
software
makes no such assumptions.
In
.CW sam ,
the window being changed is almost always fully visible, because the current
window is always fully visible, by construction.
However, when multi-file changes are being made, or when
more than one window is open on a file,
it may be necessary to update partially obscured windows.
.PP
There are three cases: the window is 
fully visible, invisible (fully obscured), or partially visible.
If fully visible, the
.CW Bitmap
is part of the screen, so when the
.CW Flayer
update routine calls the
.CW Frame
update routine, the screen will be updated directly.
If the window is invisible,
there is no associated
.CW Bitmap ,
and all that is necessary is to update the
.CW Frame
data structure, not the image.
If the window is partially visible, the
.CW Frame
routine is called to update the image in the off-screen
.CW Bitmap ,
which may require regenerating it from the text of the window.
The
.CW Flayer
code then clips this
.CW Bitmap
against the
.CW Bitmaps
of all
.CW Frames
in front of the
.CW Frame
being modified, and the remainder is copied to the display.
.PP
This is much faster than recreating the image off-screen
for every change, or clipping all the changes made to the image
during its update.
Unfortunately, these caches can also consume prohibitive amounts of
memory, so they are freed fairly liberally \(em after every change to the
front-to-back order of the
.CW Flayers .
The result is that
the off-screen
.CW Bitmaps
exist only while multi-window changes are occurring,
which is the only time the performance improvement they provide is needed.
Also, the user interface causes fully-obscured windows to be the
easiest to make \(em
creating a canonically sized and placed window requires only a button click
\(em which reduces the need for caching still further.
.PP
.SH 2
Screen update
.LP
Only two low-level primitives are needed for incremental update:
.CW bitblt ,
which copies rectangles of pixels, and
.CW string
(which in turn calls
.CW bitblt ),
which draws a null-terminated character string in a
.CW Bitmap .
A
.CW Frame
contains a list of
.CW Boxes ,
each of which defines a horizontal strip of text in the window
(see Figure 7).
A
.CW Box
has a character string
.CW str ,
and a
.CW Rectangle
.CW rect
that defines the location of the strip in the window.
(The text in
.CW str
is stored in the
.CW Box
separately from the
.CW Rasp
associated with the window's file, so
.CW Boxes
are self-contained.)
The invariant is that
the image of the
.CW Box
can be reproduced by calling
.CW string
with argument
.CW str
to draw the string in
.CW rect ,
and the resulting picture fits perfectly within
.CW rect .
In other words, the
.CW Boxes
define the tiling of the window.
The tiling may be complicated by long lines of text, which
are folded onto the next line.
Some editors use horizontal scrolling to avoid this complication,
but to be comfortable this technique requires that lines not be
.I too
long;
.CW sam
has no such restriction.
Also, and perhaps more importantly, UNIX programs and terminals traditionally fold
long lines to make their contents fully visible.
.PP
Two special kinds of
.CW Boxes
contain a single
character: either a newline or a tab.
Newlines and tabs are white space.
A newline
.CW Box
always extends to the right edge of the window,
forcing the following
.CW Box
to the next line.
The width of a tab depends on where it is located:
it forces the next
.CW Box
to begin at a tab location.
Tabs also
have a minimum width equivalent to a blank (blanks are
drawn by
.CW string
and are not treated specially); newlines have a minimum width of zero.
.KF
.PS
copy "fig7.pic"
.PE
.sp .5
.Cs
Figure 7. A line of text showing its
.CW Boxes .
The first two blank
.CW Boxes
contain tabs; the last contains a newline.
Spaces are handled as ordinary characters.
.Ce
.KE
.PP
The update algorithms always use the
.CW Bitmap
image of the text (either the display or cache
.CW Bitmap );
they never examine the characters within a
.CW Box
except when the
.CW Box
needs to be split in two.
Before a change, the window consists of a tiling of
.CW Boxes ;
after the change the window is tiled differently.
The update algorithms rearrange the tiles in place, without
backup storage.
The algorithms are not strictly optimal \(em for example, they can
clear a pixel that is later going to be written upon \(em
but they never move a tile that doesn't need to be moved,
and they move each tile at most once.
.CW Frinsert
on a Blit can absorb over a thousand characters a second if the strings
being inserted are a few tens of characters long.
.PP
Consider
.CW frdelete .
Its job is to delete a substring from a
.CW Frame
and restore the image of the
.CW Frame .
The image of a substring has a peculiar shape (see Figure 2) comprising
possibly a partial line,
zero or more full lines,
and possibly a final partial line.
For reference, call this the
.I
Z-shape.
.R
.CW Frdelete
begins by splitting, if necessary, the
.CW Boxes
containing the ends of
the substring so the substring begins and ends on
.CW Box
boundaries.
Because the substring is being deleted, its image is not needed,
so the Z-shape is then cleared.
Then, tiles (that is, the images of
.CW Boxes )
are copied, using
.CW bitblt ,
from immediately after the Z-shape to
the beginning of the Z-shape,
resulting in a new Z-shape.
.CW Boxes "" (
whose contents would span two lines in the new position must first be split.)
.PP
Copying the remainder of the
.CW Frame
tile by tile
this way will clearly accomplish the deletion but eventually,
typically when the copying algorithm encounters a tab or newline,
the old and new
.CW x
coordinates of the tile
to be copied are the same.
This correspondence implies
that the Z-shape has its beginning and ending edges aligned
vertically, and a sequence of at most two
.CW bitblts
can be used to copy the remaining tiles.
The last step is to clear out the resulting empty space at the bottom
of the window;
the number of lines to be cleared is the number of complete lines in the
Z-shape closed by the final
.CW bitblts.
The final step is to merge horizontally adjacent
.CW Boxes
of plain text.
The complete source to
.CW frdelete
is less than 100 lines of C.
.PP
.CW frinsert
is more complicated because it must do four passes:
one to construct the
.CW Box
list for the inserted string,
one to reconnoitre,
one to copy (in opposite order to
.CW frdelete )
the
.CW Boxes
to make the hole for the new text,
and finally one to copy the new text into place.
Overall, though,
.CW frinsert
has a similar flavor to
.CW frdelete ,
and needn't be described further.
.CW Frinsert
and its subsidiary routines comprise 211 lines of C.
.PP
The terminal source code is 3024 lines of C,
and the host source is 5797 lines.
.SH
Discussion
.SH 2
History
.LP
The immediate ancestor of
.CW sam
was the original text editor for the Blit, called
.CW jim .
.CW Sam
inherited
.CW jim 's
two-process structure and mouse language almost unchanged, but
.CW jim
suffered from several drawbacks that were addressed in the design of
.CW sam .
The most important of these was the lack of a command language.
Although
.CW jim
was easy to use for simple editing, it provided no direct help with
large or repetitive editing tasks.  Instead, it provided a command to pass
selected text through a shell pipeline,
but this was no more satisfactory than could be expected of a stopgap measure.
.PP
.CW Jim
was written primarily as a vehicle for experimenting with a mouse-based
interface to text, and the experiment was successful.
.CW Jim
had some spin-offs:
.CW mux ,
the second window system for the Blit, is essentially a multiplexed
version of the terminal part of
.CW jim ;
and the debugger
.CW pi 's
user interface\u\s-4\&20\s+4\d was closely modeled on
.CW jim 's.
But after a couple of years,
.CW jim
had become difficult to maintain and limiting to use,
and its replacement was overdue.
.PP
I began the design of
.CW sam
by asking
.CW jim
customers what they wanted.
This was probably a mistake; the answers were essentially a list of features
to be found in other editors, which did not provide any of the
guiding principles I was seeking.
For instance, one common request was for a ``global substitute,''
but no one suggested how to provide it within a cut-and-paste editor.
I was looking for a scheme that would
support such specialized features comfortably in the context of some
general command language.
Ideas were not forthcoming, though, particularly given my insistence
on removing all limits on file sizes, line lengths and so on.
Even worse, I recognized that, since the mouse could easily
indicate a region of the screen that was not an integral number of lines,
the command language would best forget about newlines altogether,
and that meant the command language had to treat the file as a single
string, not an array of lines.
.PP
Eventually, I decided that thinking was not getting me very far and it was
time to try building.
I knew that the terminal part could be built easily \(em
that part of
.CW jim
behaved acceptably well \(em and that most of the hard work was going
to be in the host part: the file interface, command interpreter and so on.
Moreover, I had some ideas about how the architecture of
.CW jim
could be improved without destroying its basic structure, which I liked
in principle but which hadn't worked out as well as I had hoped.
So I began by designing the file data structure,
starting with the way
.CW jim
worked \(em comparable to a single structure merging
.CW Disc
and
.CW Buffer ,
which I split to make the cache more general
\(em and thinking about how global substitute could be implemented.
The answer was clearly that it had to be done in two passes,
and the transcript-oriented implementation fell out naturally.
.PP
.CW Sam
was written bottom-up,
starting from the data structures and algorithms for manipulating text,
through the command language and up to the code for maintaining
the display.
In retrospect, it turned out well, but this implementation method is
not recommended in general.
There were several times when I had a large body of interesting code
assembled and no clue how to proceed with it.
The command language, in particular, took almost a year to figure out,
but can be implemented (given what was there at the beginning of that year)
in a day or two.  Similarly, inventing the
.CW Rasp
data structure delayed the
connection of the host and terminal pieces by another few months.
.CW Sam
took about two years to write, although only about four months were
spent actually working on it.
.PP
Part of the design process was unusual:
the subset of the protocol that maintains the
.CW Rasp
was simulated, debugged
and verified by an automatic protocol analyzer,\u\s-4\&21\s+4\d and was bug-free
from the start.
The rest of the protocol, concerned mostly
with keeping menus up to date,
was unfortunately too unwieldy for such analysis,
and was debugged by more traditional methods, primarily
by logging in a file all messages in and out of the host.
.SH 2
Reflections
.LP
.CW Sam
is essentially the only interactive editor used by the sixty or so members of
the computing science research center in which I work.
The same could not be said of
.CW jim ;
the lack of a command language kept some people from adopting it.
The union of a user interface as comfortable as
.CW jim 's
with a command language as powerful as
.CW ed 's†
.FS
.vs 9
†The people who criticize
.CW ed
as an interactive program often forget that it and its close relative
.CW sed \u\s-4\&7\s+4\d
still thrive as programmable editors.  The strength of these programs is
independent of their convenience for interactive editing.
.br
.vs
.FE
is essential to
.CW sam 's
success.
When
.CW sam
was first made available to the
.CW jim
community,
almost everyone switched to it within two or three days.
In the months that followed, even people who had never adopted
.CW jim
started using
.CW sam
exclusively.
.PP
To be honest,
.CW ed
still gets occasional use, but usually when
something quick needs to be done and the overhead of
downloading the terminal part of
.CW sam
isn't worth the trouble.
Also, as a `line' editor,
.CW sam
.CW -d
is a bit odd;
when using a good old ASCII terminal, it's comforting to have
a true line editor.
But it is fair to say that
.CW sam 's
command language has displaced
.CW ed 's
for most of the complicated editing that has kept line editors
(that is, command-driven editors) with us.
.PP
.CW Sam 's
command language is even fancier than
.CW ed 's,
and most
.CW sam
customers don't come near to using all its capabilities.
Does it need to be so sophisticated?
I think the answer is yes, for two reasons.
.PP
First, the
.I model
for
.CW sam 's
command language is really relatively simple, and certainly simpler than that of
.CW ed .
For instance, there is only one kind of textual loop in
.CW sam
\(em the
.CW x
command \(em
while
.CW ed
has three (the
.CW g
command, the global flag on substitutions, and the implicit loop over
lines in multi-line substitutions).
Also,
.CW ed 's
substitute command is necessary to make changes within lines, but in
.CW sam
the
.CW s
command is more of a familiar convenience than a necessity;
.CW c
and
.CW t
can do all the work.
.PP
Second,
given a community that expects an editor to be about as powerful as
.CW ed ,
it's hard to see how
.CW sam
could really be much simpler and still satisfy that expectation.
People want to do ``global substitutes,'' and most are content
to have the recipe for that and a few other fancy changes.
The sophistication of the command language is really just a veneer
over a design that makes it possible to do global substitutes
in a screen editor.
Some people will always want something more, however, and it's gratifying to
be able to provide it.
The real power of
.CW sam 's
command language comes from composability of the operators, which is by
nature orthogonal to the underlying model.
In other words,
.CW sam
is not itself complex, but it makes complex things possible.
If you don't want to do anything complex, you can ignore the
complexity altogether, and many people do so.
.PP
Sometimes I am asked the opposite question: why didn't I just make
.CW sam
a real programmable editor, with macros and variables and so on?
The main reason is a matter of taste: I like the editor
to be the same every time I use it.
There is one technical reason, though:
programmability in editors is largely a workaround for insufficient
interactivity.
Programmable editors are used to make particular, usually short-term,
things easy to do, such as by providing shorthands for common actions.
If things are generally easy to do in the first place,
shorthands are not as helpful.
.CW Sam
makes common editing operations very easy, and the solutions to
complex editing problems seem commensurate with the problems themselves.
Also, the ability to edit the
.CW sam
window makes it easy to repeat commands \(em it only takes a mouse button click
to execute a command again.
.SH 2
Pros and cons
.LP
.CW Sam
has several other good points,
and its share of problems.
Among the good things is the idea of
structural regular expressions,
whose usefulness has only begun to be explored.
They were arrived at serendipitously when I attempted to distill the essence of
.CW ed 's
way of doing global substitution and recognized that the looping command in
.CW ed
was implicitly imposing a structure (an array of lines) on the file.
.PP
Another of
.CW sam 's
good things is its undo capability.
I had never before used an editor with a true undo,
but I would never go back now.
Undo
.I must
be done well, but if it is, it can be relied on.
For example,
it's safe to experiment if you're not sure how to write some intricate command,
because if you make a mistake, it can be fixed simply and reliably.
I learned two things about undo from writing
.CW sam :
first, it's easy to provide if you design it in from the beginning, and
second, it's necessary, particularly if the system has some subtle
properties that may be unfamiliar or error-prone for users.
.PP
.CW Sam 's
lack of internal limits and sizes is a virtue.
Because it avoids all fixed-size tables and data structures,
.CW sam
is able to make global changes to files that some of our other
tools cannot even read.
Moreover, the design keeps the performance linear when doing such
operations, although I must admit
.CW sam
does get slow when editing a huge file.
.PP
Now, the problems.
Externally, the most obvious is that it is poorly integrated into the
surrounding window system.
By design, the user interface in
.CW sam
feels almost identical to that of
.CW mux ,
but a thick wall separates text in
.CW sam
from the programs running in
.CW mux .
For instance, the `snarf buffer' in
.CW sam
must be maintained separately from that in
.CW mux .
This is regrettable, but probably necessary given the unusual configuration
of the system, with a programmable terminal on the far end of an RS-232 link.
.PP
.CW Sam
is reliable; otherwise, people wouldn't use it.
But it was written over such a long time, and has so many new (to me)
ideas in it, that I would like to see it done over again to clean
up the code and remove many of the lingering problems in the implementation.
The worst part is in the interconnection of the host and terminal parts,
which might even be able to go away in a redesign for a more
conventional window system.
The program must be split in two to use the terminal effectively,
but the low bandwidth of the connection forces the separation to
occur in an inconvenient part of the design if performance is to be acceptable.
A simple remote procedure call
protocol driven by the host, emitting only graphics
commands, would be easy to write but wouldn't have nearly the
necessary responsiveness.  On the other hand, if the terminal were in control
and requested much simpler file services from the host, regular expression
searches would require that the terminal read the entire file over its RS-232
link, which would be unreasonably slow.
A compromise in which either end can take control is necessary.
In retrospect, the communications protocol should have been
designed and verified formally, although I do not know of any tool
that can adequately relate the protocol to
its implementation.
.PP
Not all of
.CW sam 's
users are comfortable with its command language, and few are adept.
Some (venerable) people use a sort of
.CW ed \& ``
subset'' of
.CW sam 's
command language,
and even ask why
.CW sam 's
command language is not exactly
.CW ed 's.
(The reason, of course, is that
.CW sam 's
model for text does not include newlines, which are central to
.CW ed .
Making the text an array of newlines to the command language would
be too much of a break from the seamless model provided by the mouse.
Some editors, such as
.CW vi ,
are willing to make this break, though.)
The difficulty is that
.CW sam 's
syntax is so close to
.CW ed 's
that people believe it
.I should
be the same.
I thought, with some justification in hindsight,
that making
.CW sam
similar to
.CW ed
would make it easier to learn and to accept.
But I may have overstepped and raised the users'
expectations too much.
It's hard to decide which way to resolve this problem.
.PP
Finally, there is a tradeoff in
.CW sam
that was decided by the environment in which it runs:
.CW sam
is a multi-file editor, although in a different system there might instead be
multiple single-file editors.
The decision was made primarily because starting a new program in a Blit is
time-consuming.
If the choice could be made freely, however, I would
still choose the multi-file architecture, because it allows
groups of files to be handled as a unit;
the usefulness of the multi-file commands is incontrovertible.
It is delightful to have the source to an entire program
available at your fingertips.
.SH
Acknowledgements
.LP
Tom Cargill suggested the idea behind the
.CW Rasp
data structure.
Norman Wilson and Ken Thompson influenced the command language.
This paper was improved by comments from
Al Aho,
Jon Bentley,
Chris Fraser,
Gerard Holzmann,
Brian Kernighan,
Ted Kowalski,
Doug McIlroy
and
Dennis Ritchie.
