.TL
The Organization of Networks in Plan 9
.AU
Dave Presotto
Phil Winterbottom
.AI
.MH
USA
.AB
In a distributed system networks are of paramount importance. This
paper describes the implementation, design philosophy and organization
of network support in Plan 9. Topics include network requirements
for distributed systems, our kernel implementation, network naming, user interfaces
and performance. We also observe that much of this organization is relevant to
current systems.
.AE
.NH
Introduction
.PP
Plan 9 [Pike90] is a general-purpose, multi-user, portable distributed system
implemented on a variety of computers and networks. What distinguishes Plan 9
is its organization. The goals of this organization were to reduce administration
and to promote resource sharing. One of the keys to its success as a distributed
system is the organization and management of its networks.
.PP
A Plan 9 system is comprised of file servers, CPU servers and terminals. The File
servers and CPU servers are typically multiprocessor machines with large memories
in a central location. A variety of workstation class machines are used as terminals
and are connected to the central servers using a variety of networks and protocols.
The architecture of the system demands a hierarchy of network
speeds matching the needs of the components.
Connections between File servers and CPU servers are high bandwidth point to point
fiber links. Connections from the central servers fan out to medium speed networks
in the building at Murray Hill over Ethernet [Met80] and Datakit [Fra80].
Low speed connections via the Internet and
the AT&T backbone serve users in Oregon and Illinois. Finally
Basic Rate ISDN data service and 9.6K baud serial lines provide very slow
links to users at home.
.PP
Since both CPU servers and terminals use the same kernel,
users may choose whether to run programs locally on
their terminals or remotely on CPU servers.
By hiding the system connectivity from the users Plan 9 provides
this flexibility without constraining the choice.
Simple commands allow a locally
represented namespace to transparently span many machines and networks.
Therefore, both users and administrators can configure their environment
to be as distributed or centralized as they wish.
At work, users tend to use their terminals more like workstations
running all interactive programs locally and
reserving the CPU servers for data or compute intensive jobs
such as compiling and computing chess endgames.
At home or connected over
a slow network users tend to do most of the work on the CPU server to minimize
network traffic.
The goal of the network organization is to provide the same
environment to the user wherever resources are used.
.NH 
Kernel Network Support
.PP
The importance of the networking code within the kernel is reflected by its size.
Of 25,000 lines of kernel code 12,500 are network and protocol related.
Networks are being added all the time, so the fraction of code devoted to communications
is growing. Networks play a central role in any distributed system; this is particularly
true in Plan 9 where most resources are provided by servers external to the kernel.
Moreover the network code is complex.
Protocol implementations consist almost entirely of
synchronization and dynamic memory management and are an area where good error recovery
is essential.
The networks currently supported by the kernel are Datakit, point to point fiber links,
an Internet (IP) protocol suite and ISDN data service.
The diversity of networks and variety of machines which run
Plan 9 has raised issues not addressed by other systems running on commercial
hardware supporting only Ethernet or FDDI.
.NH 2
The File System protocol
.PP
A central idea in Plan 9 is the representation of a resource as a hierarchical
file system.
Each process assembles a view of the system by building a
.I namespace
[Needham] that connects those resources.
File systems need not represent disc files; in fact most Plan 9 file systems have no
permanent storage.
A typical file system builds a dynamic representation
of some resource, like a set of network connections or the process table.
Communication between the kernel, device drivers and local or remote file servers uses a
protocol called 9P. The protocol consists of 17 messages
which describe operations on files and directories.
Device and protocol drivers built into the kernel use a procedural version
of the protocol while external file servers use an RPC form
of the protocol.
Nearly all traffic between Plan 9 systems consists
of 9P messages.
9P makes several assumptions about the underlying transport protocol.
The protocol assumes messages arrive reliably and in sequence.
Delimiters between messages
must be preserved.
Where a protocol is unable to meet these
requirements (for example TCP does not preserve delimiters)
a means must be provided to marshal messages before they are handed to the system.
.PP
A kernel data structure, the
.I channel ,
is a handle to a file server object.
9P messages are generated for each operation on a
channel.
The following is a brief description of the messages.
The
.CW auth
and
.CW attach
messages authenticate a connection, established by means outside 9P,
and validate its user.
The result is an authenticated
.I channel
that points to the root of the
server.
The
.CW clone
message makes a new channel identical to an existing channel, much like a kernel
version of the dup system call.
A
.I channel
may be moved to a file on the server using a
.CW walk
message to descend each level in the hierarchy.
The
.CW stat
and
.CW wstat
messages read and write the attributes of the file pointed to by a channel.
The
.CW open
message prepares a channel for subsequent
.CW read
and
.CW write
messages to access the contents of the file, while
.CW create
and
.CW remove
perform, on the files, the actions implied by their names.
The
.CW clunk
message discards a channel without affecting the file.
.PP
A kernel resident file server called the
.I mount
driver converts the procedural version of 9P into RPC's.
The file descriptor
is converted into a channel which  can be a pipe to a user process or a network
connection to a remote machine.
After a mount, operations
on the file tree below the mount point are sent as messages to the file server.
The
.I mount
driver performs the associated buffer management, packs and unpacks parameters from
messages and demultiplexes between processes using the file server.
.NH 2
Cyclone Fiber Links
.PP
The central file servers and CPU servers are connected together by point
to point links.
A link consists of two VME cards connected by a pair of optical
fibers. The links allow high bandwidth communication between machines with VME buses.
The VME cards use 33Mhz Intel 960 processors and AMD's TAXI
fiber transmitter/receivers. The line speed is 125 Mbit/sec.
Software in the VME card allows messages to be copied from system memory
to fiber without intermediate buffering, so reducing latency.
.NH 2
Kernel Organization
.PP
The network code in the kernel can be divided into three layers; hardware interface,
protocol processing, and program interface.
A device driver typically provides the two interface layers using streams
to provide the plumbing between the interfaces.
Streams modules pushed onto the devices process the protocols.
Each device driver is a kernel resident file system.
Simple device drivers serve a single level
file structure containing just a few files.
For example the UART's on our processors are each represented
by a data and a control file.
.P1
helix% cd /dev
helix% ls -l eia*
--rw-rw-rw- t 0 bootes bootes 0 Jul 16 17:28 eia1
--rw-rw-rw- t 0 bootes bootes 0 Jul 16 17:28 eia1ctl
--rw-rw-rw- t 0 bootes bootes 0 Jul 16 17:28 eia2
--rw-rw-rw- t 0 bootes bootes 0 Jul 16 17:28 eia2ctl
helix%
.P2
The control file is used to control the device.
For example, writing
.I b1200
to
.CW /dev/eia1ctl
will set that line to 1200 baud.
.PP
Multiplexed devices like the Ethernet interfaces present
a more structured interface.
The LANCE Ethernet driver provides an typical example.
This driver is used to
.IP \(bu
provide device control and configuration
.IP \(bu
implement protocols like
.I arp
at user level.
.IP \(bu
diagnose problems using snooping software.
.LP
The driver serves a two level file tree (Figure 1).
The top directory contains a
.CW clone
file and a directory for each conversation, numbered
.CW 1
to
.CW n .
Each conversation directory corresponds to an Ethernet packet type.
Opening the clone file finds an unused conversation directory
and opens its
.CW ctl
file.
Reading the control file returns the ASCII number of the conversation chosen.
This allows the user process to find the corresponding directory.
Each connection directory contains files which represent the state
of the connection.
The LANCE driver supplies the files 
.CW ctl, 
.CW data, 
.CW stats 
and 
.CW type.
Writing the string
.I "connect 2048"
to the
.CW ctl
file sets the packet type to 2048.
This configures a connection to receive
all IP packets sent to the machine.
Subsequent reads of the file
.CW type
yields the string 2048.
The
.CW data
file accesses the media.
A read from the
.CW data
file will return the
next packet of the type selected by the connection.
A write to the file will
append a packet header containing the source address and packet type to the data
and then queue the packet for transmission on the wire.
The
.CW stats
file returns ASCII text containing the address of the interface,
packet input/output counts, error statistics and general information
about the state of the interface.
.so tree.pout
.PP
If more than one connection on an interface
is configured for a particular type then each will receive a
copy of the incoming packets.
The special packet type, 
.CW -1 ,
is a wildcard matching any packet type.
Writing the strings
.I promiscuous
and
.I "connect -1"
to
.CW ctl
will cause that conversation to receive all packets on the Ethernet.
.PP
The driver interface may seem broader and more elaborate than necessary.
However the representation of the device as a set of files using ASCII strings to
communicate with the driver has several advantages.
If we have a mechanism which allows files to be accessed remotely we immediately have
the means to allow a remote machine to use our interfaces as gateways.
By using ASCII strings to control the interface we avoid byte order problems and
have a common representation which is easy to keep consistent between
devices on the same machine and even devices accessed remotely.
By representing dissimilar devices by the same set of files, common tools can be used 
on more than one network or interface.
Programs like
.CW stty
are eliminated and are replaced by
.CW echo
and shell redirection.
.NH 2
Protocol devices
.PP
Network connections are represented as pseudo-devices called protocol devices.
Protocol device drivers exist for the Datakit URP protocol and for each of the
Internet IP protocols TCP, UDP, and IL.
IL, described below, is a new communication protocol used by Plan 9 for
transmitting file system RPC.
All protocol devices look exactly the same so that programs need not
include network specific code.
.PP
Each protocol device driver serves the same directory structure
as the Ethernet driver.
The top directory contains a
.CW clone
file and a directory for each conversation numbered
.CW 1
to
.CW n .
Each directory contains files needed to control one
conversation and to send and receive information.
A look at a tcp conversation provides the following:
.P1
helix% cd /net/tcp/2
helix% ls -l
--rw-rw---- I 0 ehg    bootes 0 Jul 13 21:14 ctl
--rw-rw---- I 0 ehg    bootes 0 Jul 13 21:14 data
--rw-rw---- I 0 ehg    bootes 0 Jul 13 21:14 listen
--r--r--r-- I 0 bootes bootes 0 Jul 13 21:14 local
--r--r--r-- I 0 bootes bootes 0 Jul 13 21:14 remote
--r--r--r-- I 0 bootes bootes 0 Jul 13 21:14 status
helix% cat local remote status
135.104.9.31 5012
135.104.53.11 564
tcp/2 1 Established connect
helix%
.P2
The files
.CW local,
.CW remote
and
.CW status
are informational and represent the state of the connection.
The
.CW data
and
.CW ctl
files are supplied by the streams code and
provide the interface to the process end of the stream implementing the protocol.
The
.CW listen
file provides means to accept incoming calls from the network.
.PP
The following steps are used to establish a connection.
.IP 1)
The clone device of the
appropriate protocol directory is opened.
Opening the clone device reserves an unused connection.
.IP 2)
The file descriptor from the open points to the
.CW ctl
file of the new connection.
Reading the file descriptor returns an ASCII string containing
the connection number.
.IP 3)
A protocol/network specific ASCII address string is written to the
.CW ctl
file.
.IP 4)
The path of the
.CW data
file is generated from the connection number.
When the data file is opened the connection is established.
.LP
A process can then read and write the file descriptor to the
.CW data
file to send and receive messages from the network.
If the process opens the
.CW listen
file the process will block until an incoming call is received.
An address string written to the control file before the listen selects the
ports or services the server is prepared to accept.
After an incoming call is received the open completes.
The file descriptor returned from the
open of a
.CW listen
file will point to the control file of the new connection.
The path of the
.CW data
file can be obtained by reading the control file.
A connection remains established while any of the files in the connection directory
are referenced or until a close is received from the network.
.so streams.ms
.NH
The IL Protocol
.PP
None of the standard IP protocols are suitable for transmission of
9P messages over an Ethernet or the Internet.
TCP has a high overhead and does not preserve delimiters.
UDP while cheap does not provide reliable sequenced delivery.
Early versions of the system used a protocol called
.CW Nonet
on local Ethernets.
The protocol was efficient but had some problems.
Packets were addressed by Ethernet source and destination addresses.
Most routing hardware does not  preserve Ethernet addresses.
We were unable to supply a service across the
Internet or AT&T backbone because the protocol was not encapsulated by IP.
Finally, implementations of the protocol were unreliable because
the protocol was ill defined.
When IP, TCP and UDP were implemented we decided to look around for a suitable
replacement.
None met our needs:
.IP \(bu
Reliable datagram service with sequenced delivery
.IP \(bu
Run over IP
.IP \(bu
Low complexity, high performance
.IP \(bu
Adaptive timeouts to allow good performance over the Internet
.LP
IL is a lightweight protocol designed to be encapsulated by IP.
It provides reliable transmission of sequenced messages between machines.
The protocol is connection based.
No provision is made for flow control since the protocol was designed to transport RPC
messages between client and server.
A small outstanding message window prevents too
many incoming messages from being buffered.
Messages outside of the window are discarded
and must be made up by retransmissions.
Connection setup uses a two way handshake to generate
initial sequence numbers at each end of the connection. Each data message increments the
sequence number.
Messages out of order are resequenced by the receiver. 
In contrast to other protocols IL does not do blind retransmission.
If a message is lost and a timeout occurs, a querey message is sent.
The query message is a small control message containing the current
sequence numbers as seen by the sender.
The receiver responds to a query by retransmitting missing messages.
This allows the protocol to behave well in congested networks,
where blind retransmission would cause further
congestion.
Like TCP, IL has adaptive timeouts.
A round-trip timer is used
to calculate acknowledge and retransmission times in terms of the network speed.
This allows the protocol to perform well on the Internet and on local Ethernets.
.PP
In keeping with the minimalist design of the rest of the kernel, IL is small.
It has the
minimum mechanism to fulfill our needs.
The entire protocol is 847 lines of code, compared to 2200 lines for TCP.
IL has become our protocol of choice.
.NH
Network Addressing
.PP
Defining a uniform interface to the protocols and devices is not sufficient to
allow the transparency we require.
Since each of the networks uses a different
addressing scheme the ASCII strings written to a control file have no common format.
That implies that each tool needs to know the specifics of the networks it
is capable of addressing.
Moreover since each machine supplies a different subset
of the available networks each user would have to be aware of the networks supported
by each terminal and server machine.
This is obviously an unacceptable situation.
.PP
Several possible solutions were discussed and rejected; one deserves
more discussion.
We could have used a user level file server
to represent the network namespace as a Plan 9 file tree. 
This global naming scheme has been implemented in other distributed systems.
In such a system a file hierarchy provides paths to
directories which represent network domains.
Each directory contains a list of
files by machine name.
A file represents each machine in that domain;
an example might be the path
.CW /net/name/usa/edu/mit/ai .
That file contains information like the IP address of the machine.
This representation was rejected for several reasons.
First, it is hard to devise a hierarchy which encompasses all representations
of the various network addressing schemes in a uniform manner.
Datakit and Ethernet address strings have nothing in common.
Second, the address of a machine is
often only a small part of the information required to connect to a service on a
particular machine.
For example the IP protocols require symbolic service names to be mapped into
numeric port numbers, some of which are privileged and hence special.
Information of this sort is hard to represent in terms of file operations.
Finally the size and number of the networks being represented burdens users with
an unacceptably large amount of information about the organization of the network
and its connectivity.
This is an example where the Plan 9 representation of an
object as a file is not appropriate.
.PP
If the tools are to be network independent a third party server must be used to resolve
network names.
A server on each machine, with local knowledge, can select the best network
for any particular machine or service.
Since the network devices have a common interface
the only operation which differs between networks is name resolution.
A symbolic name must be translated into an ASCII string
containing a path to the clone file of a protocol
device and an address string which can be written to the
.CW ctl
file.
A connection server (CS) provides this service.
Translation is performed by writing a symbolic name to
.I /net/cs
and reading back a translated address.
Symbolic names have the form
.I network!machine!service .
In most cases both the
.I network
and
.I service
can be omitted.
A default network is selected by CS and the service,
for instance rlogin, is implied by the tool.
.so cs.ms
.so library.ms
.NH
User Level
.PP
Communication between Plan 9 machines is done almost exclusively in
terms of 9P messages. Only two services
.I cpu
and
.I exportfs
are used.
The
.I cpu
service is analogous to rlogin.
However, rather than emulating a terminal session
across the network,
.I cpu
creates a process on the remote machine whose namespace is an analogue of the window
in which it was invoked.
.I Exportfs
is a user level file server which allows a piece of namespace to be
exported from machine to machine across a network. It is used by the
.I cpu
command to serve the files in the namespace of the terminal which are accessed from the
cpu server.
.PP
By convention the protocol and device driver file systems are mounted in a
directory called
.I /net .
Since the namespace
is per process a user may configure any view of the system he chooses. However
in practice most people's profiles build a similar namespace.
.NH 2
Exportfs
.PP
Exportfs is invoked by an incoming network call.
The profile of the user requesting the
service is run by the listener (Plan 9 equivalent of inetd).
The profile constructs a namespace for the user.
After the profile has executed the
.I exportfs
program is started.
An initial protocol establishes the root of the file tree being
exported.
The connection to
.I exportfs
is then mounted by the remote process.
.I Exportfs
then acts as a relay file server; operations in the imported file tree
are executed on the remote server and the results returned.
This gives the appearance of exporting a name space from a remote machine into a
local file tree.
.PP
The
.I import
command calls
.I exportfs
on a remote machine and mounts the result in the local namespace. After the connection
has been mounted the import exits.
No process is required locally to serve mounts.
9P messages are generated by the mount driver in the kernel and sent
directly over the network.
.PP
.I Exportfs
must be multithreaded since the system calls
.I open,
.I read
and
.I write
may block.
Plan 9 does not implement the 
.I select
system call but does have mechanisms for sharing file descriptor tables,
memory and other resources between processes.
This eases the burden of writing multithreaded servers.
Together with the namespace
.I Exportfs
provides a powerful means of sharing resources between machines.
It is used as a building block for complex namespaces served from many machines.
The simplicity of the interfaces enables naive users to exploit the potential
of a richly connected environment.
.PP
Using these tools it is possible to gateway between networks.
For example, on a terminal connected only to the Datakit:
.P1
import -a helix /net
telnet ai.mit.edu
.P2
The
.I import
command makes a Datakit connection to the machine helix.
An
.I exportfs
process is started to serve
.I /net
from helix.
The import command mounts the remote
.I /net
directory after (the -a option to import) the existing local contents
of
.I /net .
The contents of the directory has become the union of the local and remote versions of
.I /net .
Duplicate entries in the directories will be matched by the local entry.
Networks supplied by the local machine will continue to be chosen in preference
to those supplied remotely.
Unique entries in the remote directory however are now visible in the local
.I /net .
The networks helix supports over and above the Datakit are now available in the terminal.
.NH
Performance
.PP
We've measured both latency and throughput
of reading and writing bytes between two processes
for a number of different paths.
Measurements were made on a two and four
CPU SGI Power Series processor.
The CPU's are 25 MHZ MIPS 3000's.
The latency is measured as the round trip time
for a byte sent from one process to another and
back again.
Throughput is measured using 16k writes from
one process to another.
.DS C
.TS
box, tab(:);
c s s
c | c | c
l | n | n.
Table 1 - Performance
_
test:throughput:latency
:MBytes/sec:millisec
_
pipes:8.15:.255
_
IL/ether:1.02:1.42
_
URP/datakit:0.22:1.75
_
Cyclone:3.2:0.375
.TE
.DE
.NH
Conclusion
.PP
The representation of all resources as file systems
coupled with an ASCII interface has proved more powerful
than we had originally imagined.
Resources can be used by any computer in our networks
independent of byte ordering or CPU type.
The connection server provides an elegant means
of decoupling tools from the networks they use.
Users successfully use Plan 9 without knowing the
topology of the system or the networks they use.
.NH
References
.LP
[Pike90] R. Pike, D. Presotto, K. Thompson, H. Trickey,
``Plan 9 from Bell Labs'',
.I
UKUUG Proc. of the Summer 1990 Conf. ,
London, England,
1990
.LP
[Needham] R. Needham, ``Names'', in
.I
Distributed systems,
.R
S. Mullender, ed.,
Addison Wesley, 1989
.LP
[Presotto] D. Presotto, ``Multiprocessor Streams for Plan 9'',
.I
UKUUG Proc. of the Summer 1990 Conf. ,
.R
London, England, 1990
.LP
[Met80] R. Metcalfe, D. Boggs, C. Crane, E. Taf and J. Hupp, ``The
Ethernet Local Network: Three reports'',
.I
CSL-80-2,
.R
XEROX Palo Alto Research Center, Feburary 1980.
.LP
[Fra80] A. G. Fraser, ``Datakit - A Modular Network for Synchronous
and Asynchronous Traffic'', 
.I
Proc. International Conference on Communication ,
Boston Mass., June 1980.
.LP
[Pet89a] L. Peterson, ``RPC in the X-Kernel: Evaluating new Design Techniques'',
.I
Proc. Twelfth Symposium on Operating Systems Principles,
.R
Lictfield Park AZ, December 1990.
.LP
[Rit84a] D. M. Ritchie, ``A Stream Input-Output System'',
.I
AT&T Bell Laboratories Technical Journal, 68(8),
.R
October 1984.
