.NH 2
Streams
.PP
A
.I stream 
[Rit84a][Presotto] is a bidirectional channel connecting a
physical or pseudo device to user processes.
User processes insert and remove data at one end of the stream.
Kernel processes acting on behalf of a device insert data at
the other end.
Asynchronous communications channels such as pipes,
TCP conversations, Datakit conversations, and RS232 lines are implemented using
streams.
.PP
A stream is made up of a linear list of
.I "processing modules" .
Each module has both an upstream (toward the process) and
downstream (toward the device)
.I "put routine" .
Data is inserted into the stream by calling the put routine of the
module at either end of the stream.
Each module calls the succeeding one to send data up or down the stream.
.PP
An instance of a processing module is represented by a pair of
.I Queue
structures, one for each direction.
The queues point to the put procedures and can be used
to queue information traveling along the stream.
.PP
Some put routines queue data locally and send it along the stream at some
later time either due to a subsequent call or an asynchronous
event such as a retransmission timer or a device interrupt.
Processing modules create helper kernel processes to
provide a context to handle these asynchronous events.
For example, a helper kernel process exists that awakens periodically
to perform any necessary TCP retransmissions.
The use of kernel processes instead of serialized run-to-completion service routines
is a major deviation from Unix streams.
The two major problems with Unix service routines are their inability
to use any blocking kernel resource and their lack of local long lived state.
Using kernel processes solves these problems and makes the stream code considerably
less complex.
.PP
There is no implicit synchronization in our streams.
Each processing module must ensure that concurrent processes using the stream
are synchronized.
While this allows a maximum of concurrency it creates the
possibility of deadlock that did not exist in Unix streams.
However deadlocking has not caused us problems.
.PP
Information is represented by linked lists of kernel structures called
.I blocks .
Each block has a type, a delimiter flag, and pointers to an
optional buffer.
Block buffers can hold either data or control information, i.e., directives
to the processing modules.
Blocks and block buffers are dynamically allocated from kernel memory.
.P1
typedef struct Block	Block;
struct Block
{
	Block	*next;
	uchar	*rptr;		/* first unconsumed byte in buffer */
	uchar	*wptr;		/* first empty byte in buffer */
	uchar	*base;		/* start of the buffer */
	uchar	*lim;		/* 1 past the end of the buffer */
	uchar	type;		/* M_DATA, M_CTL, M_HANGUP */
	uchar	flags;
};
.P2
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
The last process to close destroys the stream.
Writing to the
.CW data
file copies the data into kernel data blocks
and passes them to the downstream put routine of the first processing module.
A write of less than 32K is guaranteed to be contained by a single block.
Concurrent writes to the same stream are not synchronized although the
32K block size assures atomic writes for most protocols.
The last block of a write is flagged with a delimiter
in the event that the downstream module cares about write boundaries.
In most cases the first put routine calls the second, the second
calls the third, and so on until the data is output.
As a consequence most data is output without context switching.
.PP
Like UNIX streams
.[ [
bltj
.]] ,
Plan 9 streams can be dynamically configured.
The stream system intercepts the control blocks which control
configuration of the stream.
These control blocks are:
.IP "push \fIname\fR" 12
to add an instance of the processing module 
.I name
to the top of the stream.
.IP pop 12
to remove the top module of the stream.
.IP hangup 12
to send a hangup message
up the stream from the device end.
.LP
Other control blocks are stream specific and interpreted by each module
as they pass through.
.PP
Our system has no
.CW ioctl 
system call.
The syntax and semantics of
.CW ioctl
in
UNIX
are so uncontrolled that we left it out.
Writing to the
.CW ctl
file takes the place of
.CW ioctl .
Writing to the control file
is the same as writing to a data file except that the blocks created are of type
control.
A processing module parses each control block put to it.
The commands in the control blocks are simple ASCII strings.
Therefore, there is no problem with byte ordering when one system
is controlling streams in a name space implemented on another processor.
The time to parse control blocks is not important since control
operations are rare.
.PP
Reading from the
.CW data
file returns data queued at the top of the stream.
The read terminates either when the read count is reached
or when the end of a delimited block is read.
There is a per stream read lock that ensures that only one process
can read from the stream at a time.
This ensures that the bytes read were contiguous bytes from the
stream.
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
How work is divided between these different pieces varies depending on
how much needs to be done at interrupt level.
However, the interrupt routine may not allocate blocks or call
a put routine since both actions require a process context.
.NH 3
Multiplexing
.PP
The conversations belonging to a protocol device need to be
multiplexed onto a single physical device.
We group the conversations by pushing a multiplexing processing module
onto the physical device stream.
The device end modules on the conversations add the necessary header
onto downstream messages and then put them to the module downstream
of the multiplexor.
The multiplexing module looks at each message moving up its stream and
puts it to the correct conversation stream after stripping
whatever header it used to do the demultiplexing.
.PP
This is similar to how Unix streams implement multiplexors.
The major difference is that we have no general structure that
corresponds to a multiplexor.
Each attempt to introduce one made the code more complicated.
Therefore, each multiplexor is written from scratch.
.NH 3
Reflections
.PP
After 5 years' experience with streams and many programmers writing stream
code, we are still dissatisfied with the mechanism.
Performance is not an issue.
The time to process protocols and drive
device interfaces continues to dwarf the
time for the stream mechanism itself, i.e.,
allocating, freeing, and moving blocks
of data.
The problem is that the mechanism remains one of the most
complex in our kernel.
Much of the complexity exists to make streams
dynamically configurable and to make processing
modules reusable on different devices.
This includes synchronization in the
kernel to ensure that data structures
don't disappear under foot.
This is irritating since we seldom use these properties.
The same was true on our research Unix systems.
Only the tty module was ever heavily reused and
Plan 9 needs no such module.
.PP
Streams remain in our kernel because we are unable to
devise a better alternative.
Larry Peterson's X-kernel [Pet89a]
is the closest contender but
doesn't offer enough advantage to switch.
