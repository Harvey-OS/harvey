.HTML "Using SPIN
.\" runoff as:
.\" eqn file | tbl | troff -ms
.\"
.ds P \\s-1PROMELA\\s0
.ds V \\s-1SPIN\\s0
.ds C pcc
.\" .tr -\(hy
.EQ
delim $#
.EN
.TL
Using \*V
.AU
Gerard J. Holzmann
gerard@plan9.bell-labs.com
.AB
\*V can be used for proving or disproving logical properties
of concurrent systems.
To render the proofs, a concurrent system is first
modeled in a formal specification language called \*P.
The language allows one to specify the behaviors
of asynchronously executing
processes that may interact through synchronous
or asynchronous message passing, or through direct
access to shared variables.
.LP
System models specified in this way can be verified
for both safety and liveness properties. The specification
of general properties in linear time temporal logic is
also supported.
.LP
The first part of this manual
discusses the basic features of the specification language \*P.
The second part describes the verifier \*V.
.AE
.NH 1
The Language \*P
.LP
\*P is short for Protocol Meta Language [Ho91].
\*P is a \f2modeling\f1 language, not a programming language.
A formal model differs in two essential ways from an implementation.
First, a model is meant to be an abstraction of a design
that contains only those aspects of the design that are
directly relevant to the properties one is interested in proving.
Second, a formal model must contain things that are typically not part
of an implementation, such as worst-case assumptions about
the behavior of the environment that may interact with the
system being studied, and a formal statement of relevant correctness
properties. It is possible to mechanically extract abstract models
from implementation level code, as discussed, for instance in [HS99].
.LP
Verification with \*V is often performed in a series of steps,
with the construction of increasingly detailed models.
Each model can be verified under different types of
assumptions about the environment and for different
types of correctness properties.
If a property is not valid for the given assumptions about
system behavior, the verifier can produce a counter-example
that demonstrates how the property may be violated.
If a property is valid, it may be possible to simplify the
model based on that fact, and prove still other properties.
.LP
Section 1.1 covers the basic building blocks of the language.
Section 1.2 introduces the control flow structures.
Section 1.3 explains how correctness properties are specified.
Section 1.4 concludes the first part with a discussion of
special predefined variables and functions that can be used to
express some correctness properties.
.LP
Up to date manual pages for \*V can always be found online at:
.CW
http://cm.bell-labs.com/cm/cs/what/spin/Man/
.R
.NH 2
Basics
.LP
A \*P model can contain three different types of objects:
.IP
.RS
\(bu Processes (section 1.1.1),
.br
\(bu Variables (section 1.1.2),
.br
\(bu Message channels (section 1.1.3).
.RE
.LP
All processes are global objects.
For obvious reasons, a \*P model must contain at least one
process to be meaningful.
Since \*V is specifically meant to prove properties of
concurrent systems, a model typically contains more than
one process.
.LP
Message channels and variables, the two basic types of data objects,
can be declared with either a global scope or a local scope.
A data object with global scope can be referred to by all processes.
A data object with a local scope can be referred to by just a
single process: the process that declares and instantiates the object.
As usual, all objects must be declared in the specification
before they are referenced.
.NH 3
Processes
.LP
Here is a simple process that does nothing except print
a line of text:
.P1
init {
	printf("it works\en")
}
.P2
There are a few things to note.
.CW Init
is a predefined keyword from the language.
It can be used to declare and instantiate
a single initial process in the model.
(It is comparable to the
.CW main
procedure of a C program.)
The
.CW init
process does not take arguments, but it can
start up (instantiate) other processes that do.
.CW Printf
is one of a few built-in procedures in the language.
It behaves the same as the C version.
Note, finally, that no semicolon follows the single
.CW printf
statement in the above example.
In \*P, semicolons are used as statement separators,
not statement terminators.  (The \*V parser, however, is
lenient on this issue.)
.LP
Any process can start new processes by using another
built-in procedure called
.CW run .
For example,
.P1
proctype you_run(byte x)
{
	printf("my x is: %d\en", x)
}
.P2
.P1
init {
	run you_run(1);
	run you_run(2)
}
.P2
The word
.CW proctype
is again a keyword that introduces the declaration
of a new type of process.
In this case, we have named that type
.CW you_run
and declared that all instantiations of processes
of this type will take one argument:  a data object
of type
.CW byte ,
that can be referred to within this process by the name
.CW x .
Instances of a
.CW proctype
can be created with the predefined procedure
.CW run ,
as shown in the example.
When the
.CW run
statement completes, a copy of the process
has been started, and all its arguments have been
initialized with the arguments provided.
The process may, but need not, have performed
any statement executions at this point.
It is now part of the concurrent system,
and its execution can be interleaved arbitrarily with
those of the other, already executing processes.
(More about the semantics of execution follows shortly.)
.LP
In many cases, we are only interested in creating a
single instance of each process type that is declared,
and the processes require no arguments.
We can define this by prefixing the keyword
.CW proctype
from the process declaration with another keyword:
.CW active .
Instances of all active proctypes are created when the
system itself is initialized.
We could, for instance, have avoided the use of
.CW init
by declaring the corresponding process in the last example
as follows:
.P1
active proctype main() {
	run you_run(1);
	run you_run(2)
}
.P2
Note that there are no parameters to instantiate in this
case.  Had they been declared, they would default to a
zero value, just like all other data objects
that are not explicitly instantiated.
.LP
Multiple copies of a process type can also be created in
this way.  For example:
.P1
active [4] proctype try_me() {
	printf("hi, i am process %d\en", _pid)
}
.P2
creates four processes.
A predefined variable
.CW _pid
is assigned to each running process, and holds
its unique process instantiation number.
In some cases, this number is needed when a reference
has to be made to a specific process.
.LP
Summarizing:  process behavior is declared in
.CW proctype
definitions, and it is instantiated with either
.CW run
statements or with the prefix
.CW active .
Within a proctype declaration, statements are separated
(not terminated) by semicolons.
As we shall see in examples that follow, instead of the
semicolon, one can also use the alternative separator
.CW "->"
(arrow), wherever that may help to clarify the structure
of a \*P model.
.SH
Semantics of Execution
.LP
In \*P there is no difference between a condition or
expression and a statement.
Fundamental to the semantics of the language is the
notion of the \f2executability\f1 of statements.
Statements are either executable or blocked.
Executability is the basic means of enforcing
synchronization between the processes in a distributed system.
A process can wait for an event to happen by waiting
for a statement to become executable.
For instance, instead of writing a busy wait loop:
.P1
while (a != b)	/* not valid Promela syntax */
	skip;	/* wait for a==b */
\&...
.P2
we achieve the same effect in \*P with the statement
.P1
(a == b);
\&...
.P2
Often we indicate that the continuation of an execution
is conditional on the truth of some expression by using
the alternate statement separator:
.P1
(a == b) -> \&...
.P2
Assignments and
.CW printf
statements are always executable in \*P.
A condition, however, can only be executed (passed) when it holds.
If the condition does not hold, execution blocks until it does.
There are similar rules for determining the executability
of all other primitive and compound statements in the
language.
The semantics of each statement is defined in terms of
rules for executability and effect.
The rules for executability set a precondition on the state
of the system in which a statement can be executed.
The effect defines how a statement will alter a
system state when executed.
.LP
\*P assumes that all individual statements are executed
atomically: that is, they model the smallest meaningful entities
of execution in the system being studied.
This means that \*P defines the standard asynchronous interleaving
model of execution, where a supposed scheduler is free at
each point in the execution to select any one of the processes
to proceed by executing a single primitive statement.
Synchronization constraints can be used to influence the
interleaving patterns.  It is the purpose of a concurrent system's
design to constrain those patterns in such a way that no
correctness requirements can be violated, and all service
requirements are met.  It is the purpose of the verifier
either to find counter-examples to a designer's claim that this
goal has been met, or to demonstrate that the claim is indeed valid.
.NH 3
Variables
.LP
The table summarizes the five basic data types used in \*P.
.CW Bit
and
.CW bool
are synonyms for a single bit of information.
The first three types can store only unsigned quantities.
The last two can hold either positive or negative values.
The precise value ranges of variables of types
.CW short
and
.CW int
is implementation dependent, and corresponds
to those of the same types in C programs
that are compiled for the same hardware.
The values given in the table are most common.
.KS
.TS
center;
l l
lw(10) lw(12).
=
\f3Type	Range\f1
_
bit   	0..1
bool   	0..1
byte   	0..255
short	   $-2 sup 15# .. $2 sup 15 -1#
int	   $-2 sup 31# .. $2 sup 31 -1#
_
.TE
.KE
.LP
The following example program declares a array of
two elements of type
.CW bool
and a scalar variable
.CW turn
of the same type.
Note that the example relies on the fact that
.CW _pid
is either 0 or 1 here.
.MT _sec5critical
.P1
/*
 * Peterson's algorithm for enforcing
 * mutual exclusion between two processes
 * competing for access to a critical section
 */
bool turn, want[2];

active [2] proctype user()
{
again:
	want[_pid] = 1; turn = _pid;

	/* wait until this condition holds: */
	(want[1 - _pid] == 0 || turn == 1 - _pid);

	/* enter */
critical:	skip;
	/* leave */

	want[_pid] = 0;
	goto again
}
.P2
In the above case, all variables are initialized to zero.
The general syntax for declaring and instantiating a
variable, respectively for scalar and array variables, is:
.P1
type name = expression;
type name[constant] = expression
.P2
In the latter case, all elements of the array are initialized
to the value of the expression.
A missing initializer fields defaults to the value zero.
As usual, multiple variables of the same type can be grouped
behind a single type name, as in:
.P1
byte a, b[3], c = 4
.P2
In this example, the variable
.CW c
is initialized to the value 4; variable
.CW a
and the elements of array
.CW b
are all initialized to zero.
.LP
Variables can also be declared as structures.
For example:
.P1
typedef Field {
        short f = 3;
        byte  g
};

typedef Msg {
        byte a[3];
        int fld1;
        Field fld2;
        chan p[3];
        bit b
};

Msg foo;
.P2
introduces two user-defined data types, the first named
.CW Field
and the second named
.CW Msg .
A single variable named
.CW foo
of type
.CW Msg
is declared.
All fields of
.CW foo
that are not explicitly initialized (in the example, all fields except
.CW foo.fld2.f )
are initialized to zero.
References to the elements of a structure are written as:
.P1
foo.a[2] = foo.fld2.f + 12
.P2
A variable of a user-defined type can be passed as a single
argument to a new process in
.CW run
statements.
For instance,
.P1
proctype me(Msg z) {
	z.a[2] = 12
}
init {
	Msg foo;
	run me(foo)
}
.P2
.LP
Note that even though \*P supports only one-dimensional arrays,
a two-dimensional array can be created indirectly with user-defined
structures, for instance as follows:
.P1
typedef Array {
	byte el[4]
};

Array a[4];
.P2
This creates a data structure of 16 elements that can be
referenced, for instance, as
.CW a[i].el[j] .
.LP
As in C, the indices of an array of
.CW N
elements range from 0 to
.CW N-1 .
.SH
Expressions
.LP
Expressions must be side-effect free in \*P.
Specifically, this means that an expression cannot
contain assignments, or send and receive operations (see section 1.1.3).
.P1
c = c + 1; c = c - 1
.P2
and
.P1
c++; c--
.P2
are assignments in \*P, with the same effects.
But, unlike in C,
.P1
b = c++
.P2
is not a valid assignment, because the right-hand side
operand is not a valid expression in \*P (it is not side-effect free).
.LP
It is also possible to write a side-effect free conditional
expression, with the following syntax:
.P1
(expr1 -> expr2 : expr3)
.P2
The parentheses around the conditional expression are required to
avoid misinterpretation of the arrow.
The example expression has the value of \f(CWexpr2\f1 when \f(CWexpr1\f1
evaluates to a non-zero value, and the value of \f(CWexpr3\f1 otherwise.
.LP
In assignments like
.P1
variable = expression
.P2
the values of all operands used inside the expression are first cast to
signed integers before the operands are applied.
After the evaluation of the expression completes, the value produced
is cast to the type of the target variable before the assignment takes place.
.NH 3
Message Channels
.LP
Message channels are used to model the transfer of data
between processes.
They are declared either locally or globally,
for instance as follows:
.P1
chan qname = [16] of { short, byte }
.P2
The keyword
.CW chan
introduces a channel declaration.
In this case, the channel is named
.CW qname ,
and it is declared to be capable of storing up
to 16 messages.
Each message stored in the channel is declared here to
consist of two fields: one of type
.CW short
and one of type
.CW byte .
The fields of a message can be any one of the basic types
.CW bit ,
.CW bool ,
.CW byte ,
.CW short ,
.CW int ,
and
.CW chan ,
or any user-defined type.
Message fields cannot be declared as arrays.
.LP
A message field of type
.CW chan
can be used to pass a channel identifier
through a channel from one process to another.
.LP
The statement
.P1
qname!expr1,expr2
.P2
sends the values of expressions
.CW expr1
and
.CW expr2
to the channel that we just created.  It appends
the message field created from the values of the two
expressions (and cast to the appropriate types of the
message fields declared for
.CW qname )
to the tail of the message buffer of 16 slots that belongs
to channel
.CW qname .
By default the send statement is only executable if the target
channel is non-full.
(This default semantics can be changed in the verifier into
one where the send statement is always executable, but the
message will be lost when an attempt is made to append it to
a full channel.)
.LP
The statement
.P1
qname?var1,var2
.P2
retrieves a message from the head of the same buffer,
and stores the two expressions in variables
.CW var1
and
.CW var2 .
.LP
The receive statement is executable only if the source channel
is non-empty.
.LP
If more parameters are sent per message than were declared
for the message channel, the redundant parameters are lost.
If fewer parameters are sent than declared,
the value of the remaining parameters is undefined.
Similarly, if the receive operation tries to retrieve more
parameters than available, the value of the extra parameters is
undefined; if it receives fewer than the number of parameters
sent, the extra information is lost.
.LP
An alternative, and equivalent, notation for the
send and receive operations is to structure the
message fields with parentheses, as follows:
.P1
qname!expr1(expr2,expr3)
qname?var1(var2,var3)
.P2
In the above case, we assume that
.CW qname
was declared to hold messages consisting of three fields.
.PP
Some or all of the arguments of the receive operation
can be given as constants instead of as variables:
.P1
qname?cons1,var2,cons2
.P2
In this case, an extra condition on the executability of the
receive operation is that the value of all message fields
specified as constants match the value of the corresponding
fields in the message that is to be received.
.LP
Here is an example that uses some of the mechanisms introduced
so far.
.P1
proctype A(chan q1)
{	chan q2;
	q1?q2;
	q2!123
}
.P2
.P1
proctype B(chan qforb)
{	int x;
	qforb?x;
	printf("x = %d\en", x)
}
.P2
.P1
init {
	chan qname = [1] of { chan };
	chan qforb = [1] of { int };
	run A(qname);
	run B(qforb);
	qname!qforb
}
.P2
The value printed by the process of type
.CW B
will be
.CW 123 .
.LP
A predefined function
.CW len(qname)
returns the number of messages currently
stored in channel
.CW qname .
Two shorthands for the most common uses of this
function are
.CW empty(qname)
and
.CW full(qname) ,
with the obvious connotations.
.LP
Since all expressions must be side-effect free,
it is not valid to say:
.P1
(qname?var == 0)
.P2
or
.P1
(a > b && qname!123)
.P2
We could rewrite the second example (using an atomic sequence,
as explained further in section 1.2.1):
.P1
atomic { (a > b && !full(qname)) -> qname!123 }
.P2
The meaning of the first example is ambiguous.  It could mean
that we want the condition to be true if the receive operation
is unexecutable.  In that case, we can rewrite it without
side-effects as:
.P1
empty(qname)
.P2
It could also mean that we want the condition
to be true when the channel does contain a message with
value zero.
We can specify that as follows:
.P1
atomic { qname?[0] -> qname?var }
.P2
The first statement of this atomic sequence is
an expression without side-effects that
evaluates to a non-zero value only if the
receive operation
.P1
qname?0
.P2
would have been executable at that
point (i.e., channel
.CW qname
contains at least one message and the oldest
message stored consists of one message field
equal to zero).
Any receive statement can be turned into
a side-effect free expression by placing square
brackets around the list of all message parameters.
The channel contents remain undisturbed by the
evaluation of such expressions.
.LP
Note carefully, however, that in non-atomic sequences
of two statements such as
.P1
!full(qname) -> qname!msgtype
.P2
and
.P1
qname?[msgtype] -> qname?msgtype
.P2
the second statement is not necessarily executable
after the first one has been executed.
There may be race conditions when access to the channels
is shared between several processes.
Another process can send a message to the channel
just after this process determined that it was not full,
or another process can steal away the
message just after our process determined its presence.
.LP
Two other types of send and receive statements are used
less frequently: sorted send and random receive.
A sorted send operation is written with two, instead of one,
exclamation marks, as follows:
.P1
qname!!msg
.P2
A sorted send operation will insert a message into the channel's buffer
in numerical order, instead of in FIFO order.
The channel contents are scanned from the first message towards the
last, and the message is inserted immediately before the first message
that follows it in numerical order.
To determine the numerical order, all message fields are
taken into account.
.LP
The logical counterpart of the sorted send operation
is the random receive.
It is written with two, instead of one, question marks:
.P1
qname??msg
.P2
A random receive operation is executable if it is executable for \f2any\f1
message that is currently buffered in a message channel (instead of
only for the first message in the channel).
Normal send and receive operations can freely be combined with
sorted send and random receive operations.
.SH
Rendezvous Communication
.LP
So far we have talked about asynchronous communication between processes
via message channels, declared in statements such as
.P1
chan qname = [N] of { byte }
.P2
where
.CW N
is a positive constant that defines the buffer size.
A logical extension is to allow for the declaration
.P1
chan port = [0] of { byte }
.P2
to define a rendezvous port.
The channel size is zero, that is, the channel
.CW port
can pass, but cannot store, messages.
Message interactions via such rendezvous ports are
by definition synchronous.
Consider the following example:
.P1
#define msgtype 33

chan name = [0] of { byte, byte };

active proctype A()
{	name!msgtype(124);
	name!msgtype(121)
}
.P2
.P1
active proctype B()
{	byte state;
	name?msgtype(state)
}
.P2
Channel
.CW name
is a global rendezvous port.
The two processes will synchronously execute their first statement:
a handshake on message
.CW msgtype
and a transfer of the value 124 to local variable
.CW state .
The second statement in process
.CW A
will be unexecutable,
because there is no matching receive operation in process
.CW B .
.LP
If the channel
.CW name
is defined  with a non-zero buffer capacity,
the behavior is different.
If the buffer size is at least 2, the process of type
.CW A
can complete its execution, before its peer even starts.
If the buffer size is 1, the sequence of events is as follows.
The process of type
.CW A
can complete its first send action, but it blocks on the
second, because the channel is now filled to capacity.
The process of type
.CW B 
can then retrieve the first message and complete.
At this point
.CW A
becomes executable again and completes,
leaving its last message as a residual in the channel.
.LP
Rendezvous communication is binary: only two processes,
a sender and a receiver, can be synchronized in a
rendezvous handshake.
.LP
As the example shows, symbolic constants can be defined
with preprocessor macros using
.CW #define .
The source text of a \*P model is translated by the standard
C preprocessor.
The disadvantage of defining symbolic names in this way is,
however, that the \*P parser will only see the expanded text,
and cannot refer to the symbolic names themselves.
To prevent that, \*P also supports another way to define
symbolic names, which are preserved in error reports.
For instance, by including the declaration
.P1
mtype = { ack, msg, error, data };
.P2
at the top of a \*P model, the names provided between the
curly braces are equivalent to integers of type
.CW byte ,
but known by their symbolic names to the \*V parser and the
verifiers it generates.
The constant values assigned start at 1, and count up.
There can be only one
.CW mtype
declaration per model.
.NH 2
Control Flow
.LP
So far, we have seen only some of the basic statements
of \*P, and the way in which they can be combined to
model process behaviors.
The five types of statements we have mentioned are:
.CW printf ,
.CW assignment ,
.CW condition ,
.CW send ,
and
.CW receive .
.LP
The pseudo-statement
.CW skip
is syntactically and semantically equivalent to the
condition
.CW (1)
(i.e., to true), and is in fact quietly replaced with this
expression by the lexical analyzer of \*V.
.LP
There are also five types of compound statements.
.IP
.RS
\(bu
Atomic sequences (section 1.2.1),
.br
\(bu
Deterministic steps (section 1.2.2),
.br
\(bu
Selections (section 1.2.3),
.br
\(bu
Repetitions (section 1.2.4),
.br
\(bu
Escape sequences (section 1.2.5).
.RE
.LP
.NH 3
Atomic Sequences
.LP
The simplest compound statement is the
.CW atomic
sequence:
.P1
atomic {	/* swap the values of a and b */
	tmp = b;
	b = a;
	a = tmp
}
.P2
In the example, the values of two variables
.CW a
and
.CW b
are swapped in a sequence of statement executions
that is defined to be uninterruptable.
That is, in the interleaving of process executions, no
other process can execute statements from the moment that
the first statement of this sequence begins to execute until
the last one has completed.
.LP
It is often useful to use
.CW atomic
sequences to start a series of processes in such a
way that none of them can start executing statements
until all of them have been initialized:
.P1
init {
	atomic {
		run A(1,2);
		run B(2,3);
		run C(3,1)
	}
}
.P2
.CW Atomic
sequences may be non-deterministic.
If any statement inside an
.CW atomic
sequence is found to be unexecutable, however,
the atomic chain is broken, and another process can take over
control.
When the blocking statement becomes executable later,
control can non-deterministically return to the process,
and the atomic execution of the sequence resumes as if
it had not been interrupted.
.NH 3
Deterministic Steps
.LP
Another way to define an indivisible sequence of actions
is to use the
.CW d_step
statement.
In the above case, for instance, we could also have written:
.P1
d_step {	/* swap the values of a and b */
	tmp = b;
	b = a;
	a = tmp
}
.P2
The difference between a
.CW d_step
sequence
and an
.CW atomic
sequence are:
.IP \(bu
A
.CW d_step
sequence must be completely deterministic.
(If non-determinism is nonetheless encountered,
it is always resolved in a fixed and deterministic
way: i.e., the first true guard in selection or
repetition structures is always selected.)
.IP \(bu
No
.CW goto
jumps into or out of a
.CW d_step
sequence are permitted.
.IP \(bu
The execution of a
.CW d_step
sequence cannot be interrupted when a
blocking statement is encountered.
It is an error if any statement other than
the first one in a
.CW d_step
sequence is found to be unexecutable.
.IP \(bu
A
.CW d_step
sequence is executed as one single statement.
In a way, it is a mechanism for adding new types
of statements to the language.
.LP
None of the items listed above apply to
.CW atomic
sequences.
This means that the keyword
.CW d_step
can always be replaced with the keyword
.CW atomic ,
but the reverse is not true.
(The main, perhaps the only, reason for using
.CW d_step
sequences is to improve the efficiency of
verifications.)
.NH 3
Selection Structures
.LP
A more interesting construct is the selection structure.
Using the relative values of two variables
.CW a
and
.CW b
to choose between two options, for instance, we can write:
.P1
if
:: (a != b) -> option1
:: (a == b) -> option2
fi
.P2
The selection structure above contains two execution sequences,
each preceded by a double colon.
Only one sequence from the list will be executed.
A sequence can be selected only if its first statement is executable.
The first statement is therefore called a \f2guard\f1.
.LP
In the above example the guards are mutually exclusive, but they
need not be.
If more than one guard is executable, one of the corresponding sequences
is selected nondeterministically.
If all guards are unexecutable the process will block until at least
one of them can be selected.
There is no restriction on the type of statements that can be used
as a guard: it may include sends or receives, assignments,
.CW printf ,
.CW skip ,
etc.
The rules of executability determine in each case what the semantics
of the complete selection structure will be.
The following example, for instance, uses receive statements
as guards in a selection.
.P1
mtype = { a, b };

chan ch = [1] of { byte };

active proctype A()
{	ch!a
}
.P2
.P1
active proctype B()
{	ch!b
}
.P2
.P1
active proctype C()
{	if
	:: ch?a
	:: ch?b
	fi
}
.P2
The example defines three processes and one channel.
The first option in the selection structure of the process
of type
.CW C
is executable if the channel contains
a message named
.CW a ,
where
.CW a
is a symbolic constant defined in the
.CW mtype
declaration at the start of the program.
The second option is executable if it contains a message
.CW b ,
where, similarly,
.CW b
is a symbolic constant.
Which message will be available depends on the unknown
relative speeds of the processes.
.LP
A process of the following type will either increment
or decrement the value of variable
.CW count
once.
.P1
byte count;

active proctype counter()
{
	if
	:: count++
	:: count--
	fi
}
.P2
Assignments are always executable, so the choice made
here is truly a non-deterministic one that is independent
of the initial value of the variable (zero in this case).
.NH 3
Repetition Structures
.LP
We can modify the above program as follows, to obtain
a cyclic program that randomly changes the value of
the variable up or down, by replacing the selection
structure with a repetition.
.P1
byte count;

active proctype counter()
{
	do
	:: count++
	:: count--
	:: (count == 0) -> break
	od
}
.P2
Only one option can be selected for execution at a time.
After the option completes, the execution of the structure
is repeated.
The normal way to terminate the repetition structure is
with a
.CW break
statement.
In the example, the loop can be
broken only when the count reaches zero.
Note, however, that it need not terminate since the other
two options remain executable.
To force termination we could modify the program as follows.
.P1
active proctype counter()
{
	do
	:: (count != 0) ->
		if
		:: count++
		:: count--
		fi
	:: (count == 0) -> break
	od
}
.P2
A special type of statement that is useful in selection
and repetition structures is the
.CW else
statement.
An
.CW else
statement becomes executable only if no other statement
within the same process, at the same control-flow point,
is executable.
We could try to use it in two places in the above example:
.P1
active proctype counter()
{
	do
	:: (count != 0) ->
		if
		:: count++
		:: count--
		:: else
		fi
	:: else -> break
	od
}
.P2
The first
.CW else ,
inside the nested selection structure, can never become
executable though, and is therefore redundant (both alternative
guards of the selection are assignments, which are always
executable).
The second usage of the
.CW else ,
however, becomes executable exactly when
.CW "!(count != 0)"
or
.CW "(count == 0)" ,
and is therefore equivalent to the latter to break from the loop.
.LP
There is also an alternative way to exit the do-loop, without
using a
.CW break
statement:  the infamous
.CW goto .
This is illustrated in the following implementation of
Euclid's algorithm for finding the greatest common divisor
of two non-zero, positive numbers:
.P1
proctype Euclid(int x, y)
{
	do
	:: (x >  y) -> x = x - y
	:: (x <  y) -> y = y - x
	:: (x == y) -> goto done
	od;
done:
	skip
}
.P2
.P1
init { run Euclid(36, 12) }
.P2
The
.CW goto
in this example jumps to a label named
.CW done .
Since a label can only appear before a statement,
we have added the dummy statement
.CW skip .
Like a
.CW skip ,
a
.CW goto
statement is always executable and has no other
effect than to change the control-flow point
of the process that executes it.
.LP
As a final example, consider the following implementation of
a Dijkstra semaphore, which is implemented with the help of
a synchronous channel.
.P1
#define p	0
#define v	1

chan sema = [0] of { bit };
.P2
.P1
active proctype Dijkstra()
{	byte count = 1;

	do
	:: (count == 1) ->
		sema!p; count = 0
	:: (count == 0) ->
		sema?v; count = 1
	od	
}
.P2
.P1
active [3] proctype user()
{	do
	:: sema?p;
	   /* critical section */
	   sema!v;
	   /* non-critical section */
	od
}
.P2
The semaphore guarantees that only one of the three user processes
can enter its critical section at a time.
It does not necessarily prevent the monopolization of
the access to the critical section by one of the processes.
.LP
\*P does not have a mechanism for defining functions or
procedures.  Where necessary, though, these may be
modeled with the help of additional processes.
The return value of a function, for instance, can be passed
back to the calling process via global variables or messages.
The following program illustrates this by recursively
calculating the factorial of a number
.CW n .
.P1
proctype fact(int n; chan p)
{	chan child = [1] of { int };
	int result;

	if
	:: (n <= 1) -> p!1
	:: (n >= 2) ->
		run fact(n-1, child);
		child?result;
		p!n*result
	fi
}
.P2
.P1
init
{	chan child = [1] of { int };
	int result;

	run fact(7, child);
	child?result;
	printf("result: %d\en", result)
}
.P2
Each process creates a private channel and uses it
to communicate with its direct descendant.
There are no input statements in \*P.
The reason is that models must always be complete to
allow for logical verifications, and input statements
would leave at least the source of some information unspecified.
A way to read input
would presuppose a source of information that is not
part of the model.
.LP
We have already discussed a few special types of statement:
.CW skip ,
.CW break ,
and
.CW else .
Another statement in this class is the
.CW timeout .
The
.CW timeout
is comparable to a system level
.CW else
statement: it becomes executable if and only if no other
statement in any of the processes is executable.
.CW Timeout
is a modeling feature that provides for an escape from a
potential deadlock state.
The
.CW timeout
takes no parameters, because the types of properties we
would like to prove for \*P models must be proven independent
of all absolute and relative timing considerations.
In particular, the relative speeds of processes can never be
known with certainty in an asynchronous system.
.NH 3
Escape Sequences
.LP
The last type of compound structure to be discussed is the
.CW unless
statement.
It is used as follows:
.MT _sec5unless
.P1
{ P } unless { E }
.P2
where the letters
.CW P
and
.CW E
represent arbitrary \*P fragments.
Execution of the
.CW unless
statement begins with the execution of statements from
.CW P .
Before each statement execution in
.CW P
the executability of the first statement of
.CW E
is checked, using the normal \*P semantics of executability.
Execution of statements from
.CW P
proceeds only while the first statement of
.CW E
remains unexecutable.
The first time that this `guard of the escape sequence'
is found to be executable, control changes to it,
and execution continues as defined for
.CW E .
Individual statement executions remain indivisible,
so control can only change from inside
.CW P
to the start of
.CW E
in between individual statement executions.
If the guard of the escape sequence
does not become executable during the
execution of
.CW P ,
then it is skipped entirely when
.CW P
terminates.
.LP
An example of the use of escape sequences is:
.P1
A;
do
:: b1 -> B1
:: b2 -> B2
\&...
od
unless { c -> C };
D
.P2
As shown in the example, the curly braces around the main sequence
(or the escape sequence) can be deleted if there can be no confusion
about which statements belong to those sequences.
In the example, condition
.CW c
acts as a watchdog on the repetition construct from the main sequence.
Note that this is not necessarily equivalent to the construct
.P1
A;
do
:: b1 -> B1
:: b2 -> B2
\&...
:: c -> break
od;
C; D
.P2
if
.CW B1
or
.CW B2
are non-empty.
In the first version of the example, execution of the iteration can
be interrupted at \f2any\f1 point inside each option sequence.
In the second version, execution can only be interrupted at the
start of the option sequences.
.NH 2
Correctness Properties
.LP
There are three ways to express correctness properties in \*P,
using:
.IP
.RS
.br
\(bu
Assertions (section 1.3.1),
.br
\(bu
Special labels (section 1.3.2),
.br
\(bu
.CW Never
claims (section 1.3.3).
.RE
.LP
.NH 3
Assertions
.LP
Statements of the form
.P1
assert(expression)
.P2
are always executable.
If the expression evaluates to a non-zero value (i.e., the
corresponding condition holds), the statement has no effect
when executed.
The correctness property expressed, though, is that it is
impossible for the expression to evaluate to zero (i.e., for
the condition to be false).
A failing assertion will cause execution to be aborted.
.NH 3
Special Labels
.LP
Labels in a \*P specification ordinarily serve as
targets for unconditional
.CW goto
jumps, as usual.
There are, however, also three types of labels that
have a special meaning to the verifier.
We discuss them in the next three subsections.
.NH 4
End-State Labels
.LP
When a \*P model is checked for reachable deadlock states
by the verifier, it must be able to distinguish valid \f2end state\f1s
from invalid ones.
By default, the only valid end states are those in which
every \*P process that was instantiated has reached the end of
its code.
Not all \*P processes, however, are meant to reach the
end of their code.
Some may very well linger in a known wait
state, or they may sit patiently in a loop
ready to spring into action when new input arrives.
.LP
To make it clear to the verifier that these alternate end states
are also valid, we can define special end-state labels.
We can do so, for instance, in the process type
.CW Dijkstra ,
from an earlier example:
.P1
proctype Dijkstra()
{	byte count = 1;

end:	do
	:: (count == 1) ->
		sema!p; count = 0
	:: (count == 0) ->
		sema?v; count = 1
	od	
}
.P2
The label
.CW end
defines that it is not an error if, at the end of an
execution sequence, a process of this type
has not reached its closing curly brace, but waits at the label.
Of course, such a state could still be part of a deadlock state, but
if so, it is not caused by this particular process.
.LP
There may be more than one end-state label per \*P model.
If so, all labels that occur within the same process body must
be unique.
The rule is that every label name with the prefix
.CW end
is taken to be an end-state label.
.NH 4
Progress-State Labels
.LP
In the same spirit, \*P also allows for the definition of
.CW progress
labels.
Passing a progress label during an execution is interpreted
as a good thing:  the process is not just idling while
waiting for things to happen elsewhere, but is making
effective progress in its execution.
The implicit correctness property expressed here is that any
infinite execution cycle allowed by the model that does not
pass through at least one of these progress labels is a
potential starvation loop.
In the
.CW Dijkstra
example, for instance, we can label the
successful passing of a semaphore test as progress and
ask a verifier to make sure that there is no cycle elsewhere
in the system.
.P1
proctype Dijkstra()
{	byte count = 1;

end:	do
	:: (count == 1) ->
progress:	sema!p; count = 0
	:: (count == 0) ->
		sema?v; count = 1
	od	
}
.P2
If more than one state carries a progress label,
variations with a common prefix are again valid.
.NH 4
Accept-State Labels
.LP
The last type of label, the accept-state label, is used
primarily in combination with
.CW never
claims.
Briefly, by labeling a state with any label starting
with the prefix
.CW accept
we can ask the verifier to find all cycles that \f2do\f1
pass through at least one of those labels.
The implicit correctness claim is that this cannot happen.
The primary place where accept labels are used is inside
.CW never
claims.
We discuss
.CW never
claims next.
.NH 3
Never Claims
.LP
Up to this point we have talked about the specification
of correctness criteria with assertions
and with three special types of labels.
Powerful types of correctness criteria can already
be expressed with these tools, yet so far our only option is
to add them to individual
.CW proctype
declarations.
We can, for instance, express the claim ``every system state
in which property
.CW P
is true eventually leads to a system state in which property
.CW Q
is true,'' with an extra monitor process, such as:
.P1
active proctype monitor()
{
progress:
	do
	:: P -> Q
	od
}
.P2
If we require that property
.CW P
must \f2remain\f1 true while we are waiting
.CW Q
to become true, we can try to change this to:
.P1
active proctype monitor()
{
progress:
	do
	:: P -> assert(P || Q)
	od
}
.P2
but this does not quite do the job.
Note that we cannot make any assumptions about the
relative execution speeds of processes in a \*P model.
This means that if in the remainder of the system the
property
.CW P
becomes true, we can move to the state just before the
.CW assert ,
and wait there for an unknown amount of time (anything between
a zero delay and an infinite delay is possible here, since
no other synchronizations apply).
If
.CW Q
becomes true, we may pass the assertion, but we need not
do so.
Even if
.CW P
becomes false only \f2after\f1
.CW Q
has become true, we may still fail the assertion,
as long as there exists some later state where neither
.CW P
nor
.CW Q
is true.
This is clearly unsatisfactory, and we need another mechanism
to express these important types of liveness properties.
.SH
The Connection with Temporal Logic
.LP
A general way to express system properties of the type we
have just discussed is to use linear time temporal logic (LTL)
formulae.
Every \*P expression is automatically also a valid LTL formula.
An LTL formula can also contain the unary temporal operators □
(pronounced always), ◊ (pronounced eventually), and
two binary temporal operators
.CW U
(pronounced weak until) and
.BI U
(pronounced strong until).
.LP
Where the value of a \*P expression without temporal operators can be
defined uniquely for individual system states, without further context,
the truth value of an LTL formula is defined for sequences of states:
specifically, it is defined for the first state of a given infinite
sequence of system states (a trace).
Given, for instance, the sequence of system states:
.P1
s0;s1;s2;...
.P2
the LTL formula
.CW pUq ,
with
.CW p
and
.CW q
standard \*P expressions, is true for
.CW s0
either if
.CW q
is true in
.CW s0 ,
or if
.CW p
is true in
.CW s0 
and
.CW pUq
holds for the remainder of the sequence after
.CW s0 .
.LP
Informally,
.CW pUq
says that
.CW p
is required to hold at least until
.CW q
becomes true.
If, instead, we would write \f(CWp\f(BIU\f(CWq\f1,
then we also require that there exists at least
one state in the sequence where
.CW q
does indeed become true.
.LP
The temporal operators □ and ◊
can be defined in terms of the strong until operator
.BI U ,
as follows.
.P1
□ p = !◊ !p = !(true \f(BIU\f(CW !p)
.P2
Informally, □
.CW p
says that property
.CW p
must hold in all states of a trace, and ◊
.CW p
says that
.CW p
holds in at least one state of the trace.
.LP
To express our original example requirement: ``every system state
in which property
.CW P
is true eventually leads to a system state in which property
.CW Q
is true,''
we can write the LTL formula:
.P1
□ (P -> ◊ Q)
.P2
where the logical implication symbol
.CW ->
is defined in the usual way as
.P1
P => Q means !P || Q
.P2
.SH
Mapping LTL Formulae onto Never Claims
.LP
\*P does not include syntax for specifying LTL formulae
directly, but it relies on the fact that every such
formula can be translated into a special type of
automaton, known as a Büchi automaton.
In the syntax of \*P this automaton is called a
.CW never
claim.
If you don't care too much about the details of
.CW never
claims, you can skip the remainder of this section and
simple remember that \*V can convert any LTL formula
automatically into the proper never claim syntax with
the command:
.P1
spin -f "...formula..."
.P2
Here are the details.
The syntax of a never claim is:
.P1
never {
	\&...
}
.P2
where the dots can contain any \*P fragment, including
arbitrary repetition, selection, unless constructs,
jumps, etc.
.LP
There is an important difference in semantics between a
.CW proctype
declaration and a
.CW never
claim.
Every statement inside a
.CW never
claim is interpreted as a proposition, i.e., a condition.
A
.CW never
claim should therefore only contain expressions and never
statements that can have side-effects (assignments, sends or
receives, run-statements, etc.)
.LP
.CW Never
claims are used to express behaviors that are considered
undesirable or illegal.
We say that a
.CW never
claim is `matched' if the undesirable behavior can be realized,
contrary to the claim, and thus the correctness requirement violated.
The claims are evaluated over system executions, that is, the
propositions that are listed in the claim are evaluated over the
traces from the remainder of the system.
The claim, therefore, should not alter that behavior: it merely
monitors it.
Every time that the system reaches a new state, by asynchronously
executing statements from the model, the claim will evaluate the
appropriate propositions to determine if a counter-example can
be constructed to the implicit LTL formula that is specified.
.LP
Since LTL formulae are only defined for infinite executions,
the behavior of a
.CW never
claim can only be matched by an infinite system execution.
This by itself would restrict us to the use of progress labels
and accept labels as the only means we have discussed so far
for expressing properties of infinite behaviors.
To conform to standard omega automata theory, the behaviors of
.CW never
claims are expressed exclusively with
.CW accept
labels (never with
.CW progress
labels).
To match a claim, therefore, an infinite sequence of true propositions
must exist, at least one of which is labeled with an
.CW accept
label (inside the never claim).
.LP
Since \*P models can also express terminating system behaviors,
we have to define the semantics of the
.CW never
claims also for those behaviors.
To facilitate this, it is defined that a
.CW never
claim can also be matched when it reaches its closing curly brace
(i.e., when it appears to terminate).
This semantics is based on what is usually referred to as a `stuttering
semantics.'
With stuttering semantics, any terminating execution can be extended
into an equivalent infinite execution (for the purposes of evaluating
LTL properties) by repeating (stuttering) the final state infinitely often.
As a syntactical convenience, the final state of a
.CW never
claim is defined to be accepting, i.e., it could be replaced with
the explicit repetition construct:
.P1
accept: do :: skip od
.P2
Every process behavior, similarly, is (for the purposes of evaluating the
.CW never
claims) thought to be extended with a dummy self-loop on all final states:
.P1
	do :: skip od
.P2
(Note the
.CW accept
labels only occur in the
.CW never
claim, not in the system.)
.SH
The Semantics of a Never Claim
.LP
.CW Never
claims are probably the hardest part of the language to understand,
so it is worth spending a few extra words on them.
On an initial reading, feel free to skip the remainder of this
section.
.LP
The difference between a
.CW never
claim and the remainder of a \*P system can be explained
as follows.
A \*P model defines an asynchronous interleaving product of the
behaviors of individual processes.
Given an arbitrary system state, its successor states are
conceptually obtained in two steps.
In a first step, all the executable statements in the
individual processes are identified.
In a second step, each one of these statements is executed,
each one producing one potential successor for the current state.
The complete system behavior is thus defined recursively and
represents all possible interleavings of the individual process behaviors.
It is this asynchronous product machine that we call the `global
system behavior'.
.LP
The addition of a
.CW never
claim defines a \f2synchronous\f1 product of the global system behavior
with the behavior expressed in the claim.
This synchronous product can be thought of as the construction of a
new global state machine, in which every state is defined as a pair
.CW (s,n)
with
.CW s
a state from the global system (the asynchronous product of processes), and
.CW n
a state from the claim.
Every transition in the new global machine is similarly defined by a pair
of transitions, with the first element a statement from the system, and the
second a proposition from the claim.
In other words, every transition in this final synchronous product is
defined as a joint transition of the system and the claim.
Of course, that transition can only occur if the proposition from the
second half of the transition pair evaluates to true in the current state
of the system (the first half of the state pair).
.SH
Examples
.LP
To manually translate an LTL formula into a
.CW never
claim (e.g. foregoing the builtin translation that \*V
offers), we must carefully consider whether the
formula expresses a positive or a negative property.
A positive property expresses a good behavior that we
would like our system to have.
A negative property expresses a bad behavior that we
claim the system does not have.
A
.CW never
claim can express only negative claims, not positive ones.
Fortunately, the two are exchangeable:  if we want to express
that a good behavior is unavoidable, we can formalize all
ways in which the good behavior could be violated, and express
that in the
.CW never
claim.
.LP
Suppose that the LTL formula ◊□
.CW p ,
with
.CW p
a \*P expression, expresses a negative claim
(i.e., it is considered a correctness violation if
there exists any execution sequence in which
.CW p
can eventually remain true infinitely long).
This can be written in a
.CW never
claim as:
.P1
never {	/* <>[]p */
	do
	:: skip	/* after an arbitrarily long prefix */
	:: p -> break	/* p becomes true */
	od;
accept:	do
	:: p	/* and remains true forever after */
	od
}
.P2
Note that in this case the claim does not terminate, and
also does not necessarily match all system behaviors.
It is sufficient if it precisely captures all violations
of our correctness requirement, and no more.
.LP
If the LTL formula expressed a positive property, we first
have to invert it to the corresponding negative property
.CW ◊!p
and translate that into a
.CW never
claim.
The requirement now says that it is a violation if
.CW p
does not hold infinitely long.
.P1
never {	/* <>!p*/
	do
	:: skip
	:: !p -> break
	od
}
.P2
We have used the implicit match of a claim upon reaching the
closing terminating brace.
Since the first violation of the property suffices to disprove
it, we could also have written:
.P1
never {	/* <>!p*/
	do
	:: p
	:: !p -> break
	od
}
.P2
or, if we abandon the connection with LTL for a moment,
even more tersely as:
.P1
never { do :: assert(p) od }
.P2
Suppose we wish to express that it is a violation of our
correctness requirements if there exists any execution in
the system where
.CW "□ (p -> ◊ q)"
is violated (i.e., the negation of this formula is satisfied).
The following
.CW never
claim expresses that property:
.P1
never {
	do
	:: skip
	:: p && !q -> break
	od;
accept:
	do
	:: !q
	od
}
.P2
Note that using
.CW "(!p || q)"
instead of
.CW skip
in the first repetition construct would imply a check for just
the first occurrence of proposition
.CW p
becoming true in the execution sequence, while
.CW q
is false.
The above formalization checks for all occurrences, anywhere in a trace.
.LP
Finally, consider a formalization of the LTL property
.CW "□ (p -> (q U r))" .
The corresponding claim is:
.P1
never {
	do
	:: skip		/* to match any occurrence */
	:: p &&  q && !r -> break
	:: p && !q && !r -> goto error
	od;
	do
	::  q && !r
	:: !q && !r -> break
	od;
error:	skip
}
.P2
Note again the use of
.CW skip
instead of
.CW "(!p || r)"
to avoid matching just the first occurrence of
.CW "(p && !r)"
in a trace.
.NH 2
Predefined Variables and Functions
.LP
The following predefined variables and functions
can be especially useful in
.CW never
claims.
.LP
The predefined variables are:
.CW _pid
and
.CW _last .
.LP
.CW _pid
is a predefined local variable in each process
that holds the unique instantiation number for
that process.
It is always a non-negative number.
.LP
.CW _last
is a predefined global variable that always holds the
instantiation number of the process that performed the last
step in the current execution sequence.
Its value is not part of the system state unless it is
explicitly used in a specification.
.P1
never {
	/* it is not possible for the process with pid=1
	 * to execute precisely every other step forever
	 */
accept:
	do
	:: _last != 1 -> _last == 1
	od
}
.P2
The initial value of
.CW _last
is zero.
.LP
Three predefined functions are specifically intended to be used in
.CW never
claims, and may not be used elsewhere in a model:
.CW pc_value(pid) ,
.CW enabled(pid) ,
.CW procname[pid]@label .
.LP
The function
.CW pc_value(pid)
returns the current control state
of the process with instantiation number
.CW pid ,
or zero if no such process exists.
.LP
Example:
.P1
never {
	/* Whimsical use: claim that it is impossible
	 * for process 1 to remain in the same control
	 * state as process 2, or one with smaller value.
	 */
accept:	do
	:: pc_value(1) <= pc_value(2)
	od
}
.P2
The function
.CW enabled(pid)
tells whether the process with instantiation number
.CW pid
has an executable statement that it can execute next.
.LP
Example:
.P1
never {
	/* it is not possible for the process with pid=1
	 * to remain enabled without ever executing
	 */
accept:
	do
	:: _last != 1 && enabled(1)
	od
}
.P2
The last function
.CW procname[pid]@label
tells whether the process with instantiation number
.CW pid
is currently in the state labeled with
.CW label
in
.CW "proctype procname" .
It is an error if the process referred to is not an instantiation
of that proctype.
.NH 1
Verifications with \*V
.LP
The easiest way to use \*V is probably on a Windows terminal
with the Tcl/Tk implementation of \s-1XSPIN\s0.
All functionality of \*V, however, is accessible from
any plain ASCII terminal, and there is something to be
said for directly interacting with the tool itself.
.LP
The description in this paper gives a short walk-through of
a common mode of operation in using the verifier.
A more tutorial style description of the verification
process can be found in [Ho93].
More detail on the verification of large systems with the
help of \*V's supertrace (bitstate) verification algorithm
can be found in [Ho95].
.IP
.RS
.br
\(bu
Random and interactive simulations (section 2.1),
.br
\(bu
Generating a verifier (section 2.2),
.br
\(bu
Compilation for different types of searches (section 2.3),
.br
\(bu
Performing the verification (section 2.4),
.br
\(bu
Inspecting error traces produced by the verifier (section 2.5),
.br
\(bu
Exploiting partial order reductions (section 2.6).
.RE
.LP
.NH 2
Random and Interactive Simulations
.LP
Given a model in \*P, say stored in a file called
.CW spec ,
the easiest mode of operation is to perform a random simulation.
For instance,
.P1
spin -p spec
.P2
tells \*V to perform a random simulation, while printing the
process moves selected for execution at each step (by default
nothing is printed, other than explicit
.CW printf
statements that appear in the model itself).
A range of options exists to make the traces more verbose,
e.g., by adding printouts of local variables (add option
.CW -l ),
global variables (add option
.CW -g ),
send statements (add option
.CW -s ),
or receive statements (add option
.CW -r ).
Use option
.CW -n N
(with N any number) to fix the seed on \*V's internal
random number generator, and thus make the simulation runs
reproducible.
By default the current time is used to seed the random number
generator.
For instance:
.P1
spin -p -l -g -r -s -n1 spec
.P2
.LP
If you don't like the system randomly resolving non-deterministic
choices for you, you can select an interactive simulation:
.P1
spin -i -p spec
.P2
In this case you will be offered a menu with choices each time
the execution could proceed in more than one way.
.LP
Simulations, of course, are intended primarily for the
debugging of a model.  They cannot prove anything about it.
Assertions will be evaluated during simulation runs, and
any violations that result will be reported, but none of
the other correctness requirements can be checked in this way.
.NH 2
Generating the Verifier
.LP
A model-specific verifier is generated as follows:
.P1
spin -a spec
.P2
This generates a C program in a number of files (with names
starting with
.CW pan ).
.NH 2
Compiling the Verifier
.LP
At this point it is good to know the physical limitations of
the computer system that you will run the verification on.
If you know how much physical (not virtual) memory your system
has, you can take advantage of that.
Initially, you can simply compile the verifier for a straight
exhaustive verification run (constituting the strongest type
of proof if it can be completed).
Compile as follows.
.P1
\*C -o pan pan.c		# standard exhaustive search
.P2
If you know a memory bound that you want to restrict the run to
(e.g., to avoid paging), find the nearest power of 2 (e.g., 23
for the bound $2 sup 23# bytes) and compile as follows.
.P1
\*C '-DMEMCNT=23' -o pan pan.c
.P2
or equivalently in terms of MegaBytes:
.P1
\*C '-DMEMLIM=8' -o pan pan.c
.P2
If the verifier runs out of memory before completing its task,
you can decide to increase the bound or to switch to a frugal
supertrace verification.  In the latter case, compile as follows.
.P1
\*C -DBITSTATE -o pan pan.c
.P2
.NH 2
Performing the Verification
.LP
There are three specific decisions to make to
perform verifications optimally: estimating the
size of the reachable state space (section 2.4.1),
estimating the maximum length of a unique execution
sequence (2.4.2), and selecting the type of correctness
property (2.4.3).
No great harm is done if the estimates from the first two
steps are off.  The feedback from the verifier usually provides
enough clues to determine quickly what the optimal settings
for peak performance should be.
.NH 3
Reachable States
.LP
For a standard exhaustive run, you can override the default choice
for the size for the hash table ($2 sup 18# slots) with option
.CW -w .
For instance,
.P1
pan -w23
.P2
selects $2 sup 23# slots.
The hash table size should optimally be roughly equal to the number of
reachable states you expect (within say a factor of two or three).
Too large a number merely wastes memory, too low a number wastes
CPU time, but neither can affect the correctness of the result.
.sp
For a supertrace run, the hash table \f2is\f1 the memory arena, and
you can override the default of $2 sup 22# bits with any other number.
Set it to the maximum size of physical memory you can grab without
making the system page, again within a factor of say two or three.
Use, for instance
.CW -w23
if you expect 8 million reachable states and have access to at least
8 million ($2 sup 23#) bits of memory (i.e., $2 sup 20# or 1 Megabyte of RAM).
.NH 3
Search Depth
.LP
By default the analyzers have a search depth restriction of 10,000 steps.
If this isn't enough, the search will truncate at 9,999 steps (watch for
it in the printout).
Define a different search depth with the -m flag.
.P1
pan -m100000
.P2
If you exceed also this limit, it is probably good to take some
time to consider if the model you have specified is indeed finite.
Check, for instance, if no unbounded number of processes is created.
If satisfied that the model is finite, increase the search depth at
least as far as is required to avoid truncation completely.
.LP
If you find a particularly nasty error that takes a large number of steps
to hit, you may also set lower search depths to find the shortest variant
of an error sequence.
.P1
pan -m40
.P2
Go up or down by powers of two until you find the place where the
error first appears or disappears and then home in on the first
depth where the error becomes apparent, and use the error trail of
that verification run for guided simulation.
.sp
Note that if a run with a given search depth fails to find
an error, this does not necessarily mean that no violation of a
correctness requirement is possible within that number of steps.
The verifier performs its search for errors by using a standard
depth-first graph search.  If the search is truncated at N steps,
and a state at level N-1 happens to be reachable also within fewer
steps from the initial state, the second time it is reached it
will not be explored again, and thus neither will its successors.
Those successors may contain errors states that are reachable within
N steps from the initial state.
Normally, the verification should be run in such a way that no
execution paths can be truncated, but to force the complete exploration
of also truncated searches one can override the defaults with a compile-time
flag
.CW -DREACH .
When the verifier is compiled with that additional directive, the depth at
which each state is visited is remembered, and a state is now considered
unvisited if it is revisited via a shorter path later in the search.
(This option cannot be used with a supertrace search.)
.NH 3
Liveness or Safety Verification
.LP
For the last, and perhaps the most critical, runtime decision:
it must be decided if the system is to be checked for safety
violations or for liveness violations.
.P1
pan -l	# search for non-progress cycles
pan -a	# search for acceptance cycles
.P2
(In the first case, though, you must compile pan.c with -DNP as an
additional directive. If you forget, the executable will remind you.)
If you don't use either of the above two options, the default types of
correctness properties are checked (assertion violations,
completeness, race conditions, etc.).
Note that the use of a
.CW never
claim that contains
.CW accept
labels requires the use of the
.CW -a
flag for complete verification.
.LP
Adding option
.CW -f
restricts the search for liveness properties further under
a standard \f2weak fairness\f1 constraint:
.P1
pan -f -l	# search for weakly fair non-progress cycles
pan -f -a	# search for weakly fair acceptance cycles
.P2
With this constraint, each process is required to appear
infinitely often in the infinite trace that constitutes
the violation of a liveness property (e.g., a non-progress cycle
or an acceptance cycle), unless it is permanently blocked
(i.e., has no executable statements after a certain point in
the trace is reached).
Adding the fairness constraint increases the time complexity
of the verification by a factor that is linear in the number
of active processes.
.LP
By default, the verifier will report on unreachable code in
the model only when a verification run is successfully
completed.
This default behavior can be turned off with the runtime option
.CW -n ,
as in:
.P1
pan -n -f -a
.P2
(The order in which the options such as these are listed is
always irrelevant.)
A brief explanation of these and other runtime options can
be determined by typing:
.P1
pan --
.P2
.NH 2
Inspecting Error Traces
.LP
If the verification run reports an error,
any error, \*V dumps an error trail into a file named
.CW spec.trail ,
where
.CW spec
is the name of your original \*P file.
To inspect the trail, and determine the cause of the error,
you must use the guided simulation option.
For instance:
.P1
spin -t -c spec
.P2
gives you a summary of message exchanges in the trail, or
.P1
spin -t -p spec
.P2
gives a printout of every single step executed.
Add as many extra or different options as you need to pin down the error:
.P1
spin -t -r -s -l -g spec
.P2
Make sure the file
.CW spec
didn't change since you generated the analyzer from it.
.sp
If you find non-progress cycles, add or delete progress labels
and repeat the verification until you are content that you have found what
you were looking for.
.sp
If you are not interested in the first error reported,
use pan option
.CW -c
to report on specific others:
.P1
pan -c3
.P2
ignores the first two errors and reports on the third one that
is discovered.
If you just want to count all errors and not see them, use
.P1
pan -c0
.P2
.SH
State Assignments
.LP
Internally, the verifiers produced by \*V deal with a formalization of
a \*P model in terms of extended finite state machines.
\*V therefore assigns state numbers to all statements in the model.
The state numbers are listed in all the relevant output to make it
completely unambiguous (source line references unfortunately do not
have that property).
To confirm the precise state assignments, there is a runtime option
to the analyzer generated:
.P1
pan -d	# print state machines
.P2
which will print out a table with all state assignments for each
.CW proctype
in the model.
.NH 2
Exploiting Partial Order Reductions
.LP
The search algorithm used by \*V is optimized
according to the rules of a partial order theory explained in [HoPe94].
The effect of the reduction, however, can be increased considerably if the verifier
has extra information about the access of processes to global
message channels.
For this purpose, there are two keywords in the language that
allow one to assert that specific channels are used exclusively
by specific processes.
For example, the assertions
.P1
xr q1;
xs q2;
.P2
claim that the process that executes them is the \f2only\f1 process
that will receive messages from channel
.CW q1 ,
and the \f2only\f1 process that will send messages to channel
.CW q2 .
.LP
If an exclusive usage assertion turns out to be invalid, the
verifier will be able to detect this, and report it as a violation
of an implicit correctness requirement.
.LP
Every read or write access to a message channel can introduce
new dependencies that may diminish the maximum effect of the
partial order reduction strategies.
If, for instance, a process uses the
.CW len
function to check the number of messages stored in a channel,
this counts as a read access, which can in some cases invalidate
an exclusive access pattern that might otherwise exist.
There are two special functions that can be used to poll the
size of a channel in a safe way that is compatible with the
reduction strategy.
.LP
The expression
.CW nfull(qname)
returns true if channel
.CW qname
is not full, and
.CW nempty(qname)
returns true if channel
.CW qname
contains at least one message.
Note that the parser will not recognize the free form expressions
.CW !full(qname)
and
.CW !empty(qname)
as equally safe, and it will forbid constructions such as
.CW !nfull(qname)
or
.CW !nempty(qname) .
More detail on this aspect of the reduction algorithms can be
found in [HoPe94].
.SH
Keywords
.LP
For reference, the following table contains all the keywords,
predefined functions, predefined variables, and
special label-prefixes of the language \*P,
and refers to the section of this paper in
which they were discussed.
.KS
.TS
center;
l l l l.
_last (1.4)	_pid (1.1.1)	accept (1.3.2)	active (1.1.1)
assert (1.3.1)	atomic (1.2.1)	bit (1.1.2)	bool (1.1.2)
break (1.2.4)	byte (1.1.2)	chan (1.1.3)	d_step (1.2.2)
do (1.2.4)	else (1.2.4)	empty (1.1.3)	enabled (1.4)
end (1.3.2)	fi (1.2.3)	full (1.1.3)	goto (1.2.2)
hidden (not discussed)	if (1.2.3)	init (1.1.1)	int (1.1.2)
len (1.1.3)	mtype (1.1.3)	nempty (2.6)	never (1.3.3)
nfull (2.6)	od (1.2.4)	of (1.1.3)	pc_value (1.4)
printf (1.1.1)	proctype (1.1.1)	progress (1.3.2)	run (1.1.1)
short (1.1.2)	skip (1.2)	timeout (1.2.4)	typedef (1.1.2)
unless (1.2.5)	xr (2.6)	xs (2.6)
.TE
.KE
.SH
References
.LP
[Ho91]
G.J. Holzmann,
.I
Design and Validation of Computer Protocols,
.R
Prentice Hall, 1991.
.LP
[Ho93]
G.J. Holzmann, ``Tutorial: Design and Validation of Protocols,''
.I
Computer Networks and ISDN Systems,
.R
1993, Vol. 25, No. 9, pp. 981-1017.
.LP
[HoPe94]
G.J. Holzmann and D.A. Peled, ``An improvement in
formal verification,''
.I
Proc. 7th Int. Conf. on Formal Description Techniques,
.R
FORTE94, Berne, Switzerland. October 1994.
.LP
[Ho95]
G.J. Holzmann, ``An Analysis of Bitstate Hashing,''
technical report 2/95, available from author.
.LP
[HS99]
G.J. Holzmann, ``Software model checking: extracting
verification models from source code,''
.I
Proc. Formal Methods in Software Engineering and Distributed
Systems,
.R
PSTV/FORTE99, Beijng, China, Oct. 1999, Kluwer,pp. 481-497.
