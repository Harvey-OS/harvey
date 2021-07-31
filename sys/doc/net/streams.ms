.NH 2
Streams
.PP
A
.I stream 
[Rit84a][Presotto] is a bidirectional channel connecting a
physical or pseudo-device to user processes.
The user processes insert and remove data at one end of the stream.
Kernel processes acting on behalf of a device insert data at
the other end.
Asynchronous communications channels such as pipes,
TCP conversations, Datakit conversations, and RS232 lines are implemented using
streams.
.PP
A stream comprises a linear list of
.I "processing modules" .
Each module has both an upstream (toward the process) and
downstream (toward the device)
.I "put routine" .
Calling the put routine of the module on either end of the stream
inserts data into the stream.
Each module calls the succeeding one to send data up or down the stream.
.PP
An instance of a processing module is represented by a pair of
.I Queues ,
one for each direction.
The queues point to the put procedures and can be used
to queue information traveling along the stream.
Some put routines queue data locally and send it along the stream at some
later time either due to a subsequent call or an asynchronous
event such as a retransmission timer or a device interrupt.
Processing modules create helper kernel processes to
provide a context for handling asynchronous events.
For example, a helper kernel process awakens periodically
to perform any necessary TCP retransmissions.
The use of kernel processes instead of serialized run-to-completion service routines
differs from the implementation of Unix streams.
Unix service routines cannot
use any blocking kernel resource and they lack a local long-lived state.
Helper kernel processes solve these problems and simplify the stream code.
.PP
There is no implicit synchronization in our streams.
Each processing module must ensure that concurrent processes using the stream
are synchronized.
This maximizes concurrency but introduces the
possibility of deadlock.
However deadlocks are easily avoided by careful programming; to
date they have not caused us problems.
.PP
Information is represented by linked lists of kernel structures called
.I blocks .
Each block contains a type, some state flags, and pointers to
an optional buffer.
Block buffers can hold either data or control information, i.e., directives
to the processing modules.
Blocks and block buffers are dynamically allocated from kernel memory.
.NH 3
User Interface
.PP
A stream is represented at user level as two files, 
.CW ctl
and
.CW data .
The actual names can be changed by the device driver using the stream,
as we saw earlier in the example of the UART driver.
The first process to open either file creates the stream automatically.
The last close destroys it.
Writing to the
.CW data
file copies the data into kernel blocks
and passes them to the downstream put routine of the first processing module.
A write of less than 32K is guaranteed to be contained by a single block.
Concurrent writes to the same stream are not synchronized although the
32K block size assures atomic writes for most protocols.
The last block written is flagged with a delimiter
to alert downstream modules that care about write boundaries.
In most cases the first put routine calls the second, the second
calls the third, and so on until the data is output.
As a consequence most data is output without context switching.
.PP
Reading from the
.CW data
file returns data queued at the top of the stream.
The read terminates when the read count is reached
or when the end of a delimited block is encountered.
A per stream read lock ensures only one process
can read from a stream at a time and guarantees
that the bytes read were contiguous bytes from the
stream.
.PP
Like UNIX streams
.[ [
bltj
.] ] ,
Plan 9 streams can be dynamically configured.
The stream system intercepts and interprets
the following control blocks:
.IP "push \fIname\fR" 12
adds an instance of the processing module 
.I name
to the top of the stream.
.IP pop 12
removes the top module of the stream.
.IP hangup 12
sends a hangup message
up the stream from the device end.
.LP
Other control blocks are module-specific and are interpreted by each
processing module
as they pass.
.PP
The convoluted syntax and semantics of the UNIX
.CW ioctl
system call convinced us to leave it out of Plan 9.
Instead,
.CW ioctl
is replaced by the
.CW ctl
file.
Writing to the
.CW ctl
file
is identical to writing to a
.CW data
file except the blocks are of type
.I control .
A processing module parses each control block it sees.
Commands in control blocks are ASCII strings, so
byte ordering is not an issue when one system
controls streams in a name space implemented on another processor.
The time to parse control blocks is not important, since control
operations are rare.
.NH 3
Device Interface
.PP
The module at the downstream end of the stream is part of a device interface.
The particulars of the interface vary with the device.
Most device interfaces consist of an interrupt routine, an output
put routine, and a kernel process.
The output put routine stages data for the
device and starts the device if it is stopped.
The interrupt routine wakes up the kernel process whenever
the device has input to be processed or needs more output staged.
The kernel process puts information up the stream or stages more data for output.
The division of labor among the different pieces varies depending on
how much must be done at interrupt level.
However, the interrupt routine may not allocate blocks or call
a put routine since both actions require a process context.
.NH 3
Multiplexing
.PP
The conversations using a protocol device must be
multiplexed onto a single physical wire.
We push a multiplexor processing module
onto the physical device stream to group the conversations.
The device end modules on the conversations add the necessary header
onto downstream messages and then put them to the module downstream
of the multiplexor.
The multiplexing module looks at each message moving up its stream and
puts it to the correct conversation stream after stripping
the header controlling the demultiplexing.
.PP
This is similar to the Unix implementation of multiplexor streams.
The major difference is that we have no general structure that
corresponds to a multiplexor.
Each attempt to produce a generalized multiplexor created a more complicated
structure and underlined the basic difficulty of generalzing this mechanism.
We now code each multiplexor from scratch and favour simplicity over
generality.
.NH 3
Reflections
.PP
Despite five year's experience and the efforts of many programmers,
we remain dissatisfied with the stream mechanism.
Performance is not an issue;
the time to process protocols and drive
device interfaces continues to dwarf the
time spent allocating, freeing, and moving blocks
of data.
However the mechanism remains inordinately
complex.
Much of the complexity results from our efforts
to make streams dynamically configurable, to
reuse processing modules on different devices
and to provide kernel synchronization
to ensure data structures
don't disappear under foot.
This is particularly irritating since we seldom use these properties.
.PP
Streams remain in our kernel because we are unable to
devise a better alternative.
Larry Peterson's X-kernel [Pet89a]
is the closest contender but
doesn't offer enough advantage to switch.
If we were to rewrite the streams code, we would probably statically
allocate resources for a large fixed number of conversations and burn
memory in favor of less complexity.
