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
implemented on a variety of computers and networks.
What distinguishes Plan 9 is its organization.
The goals of this organization were to
reduce administration
and to promote resource sharing. One of the keys to its success as a distributed
system is the organization and management of its networks.
.PP
A Plan 9 system comprises file servers, CPU servers and terminals.
The file servers and CPU servers are typically centrally
located multiprocessor machines with large memories and
high speed interconnects.
A variety of workstation-class machines
serve as terminals
connected to the central servers using several networks and protocols.
The architecture of the system demands a hierarchy of network
speeds matching the needs of the components.
Connections between file servers and CPU servers are high-bandwidth point-to-point
fiber links.
Connections from the servers fan out to local terminals
using medium speed networks
such as Ethernet [Met80] and Datakit [Fra80].
Low speed connections via the Internet and
the AT&T backbone serve users in Oregon and Illinois.
Basic Rate ISDN data service and 9600 baud serial lines provide slow
links to users at home.
.PP
Since CPU servers and terminals use the same kernel,
users may choose to run programs locally on
their terminals or remotely on CPU servers.
The organization of Plan 9 hides the details of system connectivity
allowing both users and administrators to configure their environment
to be as distributed or centralized as they wish.
Simple commands support the
construction of a locally represented namespace
spanning many machines and networks.
At work, users tend to use their terminals like workstations
running interactive programs locally and
reserving the CPU servers for data or compute intensive jobs
such as compiling and computing chess endgames.
At home or when connected over
a slow network, users tend to do most work on the CPU server to minimize
network traffic.
The goal of the network organization is to provide the same
environment to the user wherever resources are used.
.NH 
Kernel Network Support
.PP
Networks play a central role in any distributed system. This is particularly
true in Plan 9 where most resources are provided by servers external to the kernel.
The importance of the networking code within the kernel
is reflected by its size;
of 25,000 lines of kernel code, 12,500 are network and protocol related.
Networks are continually being added and the fraction of code
devoted to communications
is growing.
Moreover, the network code is complex.
Protocol implementations consist almost entirely of
synchronization and dynamic memory management; areas demanding 
subtle error recovery
strategies.
The kernel currently supports Datakit, point-to-point fiber links,
an Internet (IP) protocol suite and ISDN data service.
The variety of networks and machines
has raised issues not addressed by other systems running on commercial
hardware supporting only Ethernet or FDDI.
.NH 2
The File System protocol
.PP
A central idea in Plan 9 is the representation of a resource as a hierarchical
file system.
Each process assembles a view of the system by building a
.I namespace
[Needham] connecting its resources.
File systems need not represent disc files; in fact, most Plan 9 file systems have no
permanent storage.
A typical file system dynamically represents
some resource like a set of network connections or the process table.
Communication between the kernel, device drivers and local or remote file servers uses a
protocol called 9P. The protocol consists of 17 messages
describing operations on files and directories.
Kernel resident device and protocol drivers use a procedural version
of the protocol while external file servers use an RPC form.
Nearly all traffic between Plan 9 systems consists
of 9P messages.
9P relies on several properties of the underlying transport protocol.
It assumes messages arrive reliably and in sequence and
that delimiters between messages
are preserved.
When a protocol does not meet these
requirements (for example TCP does not preserve delimiters)
we provide mechanisms to marshal messages before handing them
to the system.
.PP
A kernel data structure, the
.I channel ,
is a handle to a file server.
Operations on a channel generate the following 9P messages.
The
.CW auth
and
.CW attach
messages authenticate a connection, established by means external to 9P,
and validate its user.
The result is an authenticated
channel
referencing the root of the
server.
The
.CW clone
message makes a new channel identical to an existing channel, much like
the
.CW dup
system call.
A
channel
may be moved to a file on the server using a
.CW walk
message to descend each level in the hierarchy.
The
.CW stat
and
.CW wstat
messages read and write the attributes of the file referenced by a channel.
The
.CW open
message prepares a channel for subsequent
.CW read
and
.CW write
messages to access the contents of the file.
.CW Create
and
.CW remove
perform the actions implied by their names on the file
referenced by the channel.
The
.CW clunk
message discards a channel without affecting the file.
.PP
A kernel resident file server called the
.I "mount driver"
converts the procedural version of 9P into RPC's.
The
.I mount
system call provides a file descriptor, which can be
a pipe to a user process or a network connection to a remote machine, to
be associated with the mount point.
After a mount, operations
on the file tree below the mount point are sent as messages to the file server.
The
mount
driver manages buffers, packs and unpacks parameters from
messages and demultiplexes among processes using the file server.
.NH 2
Kernel Organization
.PP
The network code in the kernel is divided into three layers: hardware interface,
protocol processing, and program interface.
A device driver typically uses streams to connect the two interface layers.
Additional stream modules may be pushed on
a device to process protocols.
Each device driver is a kernel-resident file system.
Simple device drivers serve a single level
directory containing just a few files;
for example, we represent each UART
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
The control file is used to control the device;
writing the string
.I b1200
to
.CW /dev/eia1ctl
sets the line to 1200 baud.
.PP
Multiplexed devices present
a more complex interface structure.
For example, the LANCE Ethernet driver
serves a two level file tree (Figure 1)
providing
.IP \(bu
device control and configuration
.IP \(bu
user-level protocols like
.I arp
.IP \(bu
diagnostic interfaces for snooping software.
.LP
The top directory contains a
.CW clone
file and a directory for each connection, numbered
.CW 1
to
.CW n .
Each connection directory corresponds to an Ethernet packet type.
Opening the
.CW clone
file finds an unused connection directory
and opens its
.CW ctl
file.
Reading the control file returns the ASCII connection number; the user
process can use this value to construct the name of the proper 
connection directory.
In each connection directory files named
.CW ctl, 
.CW data, 
.CW stats 
and 
.CW type
provide access to the connection.
Writing the string
.I "connect 2048"
to the
.CW ctl
file sets the packet type to 2048
and
configures the connection to receive
all IP packets sent to the machine.
Subsequent reads of the file
.CW type
yield the string
.I 2048 .
The
.CW data
file accesses the media;
reading it
returns the
next packet of the selected type.
Writing the file
queues a packet for transmission after
appending a packet header containing the source address and packet type.
The
.CW stats
file returns ASCII text containing the interface address,
packet input/output counts, error statistics and general information
about the state of the interface.
.so tree.pout
.PP
If several connections on an interface
are configured for a particular packet type each receives a
copy of the incoming packets.
The special packet type, 
.CW -1 ,
selects all packets.
Writing the strings
.I promiscuous
and
.I "connect -1"
to the
.CW ctl
file
configures a conversation to receive all packets on the Ethernet.
.PP
Although the driver interface may seem elaborate,
the representation of a device as a set of files using ASCII strings for
communication has several advantages.
Any mechanism supporting remote access to files immediately
allows a remote machine to use our interfaces as gateways.
Using ASCII strings to control the interface avoids byte order problems and
ensures a uniform representation for
devices on the same machine and even devices accessed remotely.
Representing dissimilar devices by the same set of files allows common tools
to serve
several networks or interfaces.
Programs like
.CW stty
are replaced by
.CW echo
and shell redirection.
.NH 2
Protocol devices
.PP
Network connections are represented as pseudo-devices called protocol devices.
Protocol device drivers exist for the Datakit URP protocol and for each of the
Internet IP protocols TCP, UDP, and IL.
IL, described below, is a new communication protocol used by Plan 9 for
transmitting file system RPC's.
All protocol devices look identical so user programs contain no
network-specific code.
.PP
Each protocol device driver serves a directory structure
similar to that of the Ethernet driver.
The top directory contains a
.CW clone
file and a directory for each connection numbered
.CW 1
to
.CW n .
Each connection directory contains files to control one
connection and to send and receive information.
A look at a TCP connection directory provides the following:
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
supply information about the state of the connection.
The
.CW data
and
.CW ctl
files
provide access to the process end of the stream implementing the protocol.
The
.CW listen
file is used to accept incoming calls from the network.
.PP
The following steps establish a connection.
.IP 1)
The clone device of the
appropriate protocol directory is opened to reserve an unused connection.
.IP 2)
The file descriptor returned by the open points to the
.CW ctl
file of the new connection.
Reading that file descriptor returns an ASCII string containing
the connection number.
.IP 3)
A protocol/network specific ASCII address string is written to the
.CW ctl
file.
.IP 4)
The path of the
.CW data
file is constructed using the connection number.
When the
.CW data
file is opened the connection is established.
.LP
A process can read and write this file descriptor
to send and receive messages from the network.
If the process opens the
.CW listen
file it blocks until an incoming call is received.
An address string written to the
.CW ctl
file before the listen selects the
ports or services the process is prepared to accept.
When an incoming call is received, the open completes
and returns a file descriptor
pointing to the
.CW ctl
file of the new connection.
Reading the
.CW ctl
file yields a connection number used to construct the path of the
.CW data
file.
A connection remains established while any of the files in the connection directory
are referenced or until a close is received from the network.
.so streams.ms
.NH
The IL Protocol
.PP
None of the standard IP protocols are suitable for transmission of
9P messages over an Ethernet or the Internet.
TCP has a high overhead and does not preserve delimiters.
UDP, while cheap, does not provide reliable sequenced delivery.
Early versions of the system used a custom protocol that was
efficient but unsatisfactory for internetwork transmission.
When we implemented IP, TCP and UDP we looked around for a suitable
replacement with the following properties:
.IP \(bu
Reliable datagram service with sequenced delivery
.IP \(bu
Runs over IP
.IP \(bu
Low complexity, high performance
.IP \(bu
Adaptive timeouts
.LP
None met our needs so a new protocol was designed.
IL is a lightweight protocol designed to be encapsulated by IP.
It is a connection-based protocol
providing reliable transmission of sequenced messages between machines.
No provision is made for flow control since the protocol is designed to transport RPC
messages between client and server.
A small outstanding message window prevents too
many incoming messages from being buffered;
messages outside the window are discarded
and must be retransmitted.
Connection setup uses a two way handshake to generate
initial sequence numbers at each end of the connection;
subsequent data messages increment the
sequence numbers allowing
the receiver to resequence out of order messages. 
In contrast to other protocols, IL does not do blind retransmission.
If a message is lost and a timeout occurs, a query message is sent.
The query message is a small control message containing the current
sequence numbers as seen by the sender.
The receiver responds to a query by retransmitting missing messages.
This allows the protocol to behave well in congested networks,
where blind retransmission would cause further
congestion.
Like TCP, IL has adaptive timeouts.
A round-trip timer is used
to calculate acknowledge and retransmission times in terms of the network speed.
This allows the protocol to perform well on both the Internet and on local Ethernets.
.PP
In keeping with the minimalist design of the rest of the kernel, IL is small.
The entire protocol is 847 lines of code, compared to 2200 lines for TCP.
IL is our protocol of choice.
.NH
Network Addressing
.PP
A uniform interface to protocols and devices is not sufficient to
support the transparency we require.
Since each network uses a different
addressing scheme,
the ASCII strings written to a control file have no common format.
As a result, every tool must know the specifics of the networks it
is capable of addressing.
Moreover, since each machine supplies a subset
of the available networks, each user must be aware of the networks supported
by every terminal and server machine.
This is obviously unacceptable.
.PP
Several possible solutions were considered and rejected; one deserves
more discussion.
We could have used a user-level file server
to represent the network namespace as a Plan 9 file tree. 
This global naming scheme has been implemented in other distributed systems.
The file hierarchy provides paths to
directories representing network domains.
Each directory contains
files representing the names of the machines in that domain;
an example might be the path
.CW /net/name/usa/edu/mit/ai .
Each machine file contains information like the IP address of the machine.
We rejected this representation for several reasons.
First, it is hard to devise a hierarchy encompassing all representations
of the various network addressing schemes in a uniform manner.
Datakit and Ethernet address strings have nothing in common.
Second, the address of a machine is
often only a small part of the information required to connect to a service on
the machine.
For example, the IP protocols require symbolic service names to be mapped into
numeric port numbers, some of which are privileged and hence special.
Information of this sort is hard to represent in terms of file operations.
Finally, the size and number of the networks being represented burdens users with
an unacceptably large amount of information about the organization of the network
and its connectivity.
In this case the Plan 9 representation of a
resource as a file is not appropriate.
.PP
If tools are to be network independent, a third-party server must resolve
network names.
A server on each machine, with local knowledge, can select the best network
for any particular destination machine or service.
Since the network devices present a common interface,
the only operation which differs between networks is name resolution.
A symbolic name must be translated to
the path of the clone file of a protocol
device and an ASCII address string to write to the
.CW ctl
file.
A connection server (CS) provides this service.
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
service is analogous to
.CW rlogin .
However, rather than emulating a terminal session
across the network,
.I cpu
creates a process on the remote machine whose namespace is an analogue of the window
in which it was invoked.
.I Exportfs
is a user level file server which allows a piece of namespace to be
exported from machine to machine across a network. It is used by the
.I cpu
command to serve the files in the terminal's namespace when they are
accessed from the
cpu server.
.PP
By convention the protocol and device driver file systems are mounted in a
directory called
.CW /net .
Although the per-process namespace allows users to configure an
arbitrary view of the system, in practice most user profiles build
a conventional namespace.
.NH 2
Exportfs
.PP
.I Exportfs
is invoked by an incoming network call.
The
.CW listener
(the Plan 9 equivalent of
.CW inetd )
runs the profile of the user
requesting the service to construct a namespace before starting
.I exportfs .
After an initial protocol
establishes the root of the file tree being
exported,
the remote process mounts the connection
allowing
.I exportfs
to act as a relay file server. Operations in the imported file tree
are executed on the remote server and the results returned.
As a result
the namespace of the remote machine appears to be exported into a
local file tree.
.PP
The
.I import
command calls
.I exportfs
on a remote machine, mounts the result in the local namespace
and then
exits.
No process is required locally to serve mounts;
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
system call but does allow processes to share file descriptors,
memory and other resources.
.I Exportfs
and the configurable namespace
provide a powerful means of sharing resources between machines.
It is a building block for constructing complex namespaces
served from many machines.
.PP
The simplicity of the interfaces encourages naive users to exploit the potential
of a richly connected environment.
Using these tools it is easy to gateway between networks.
For example on a terminal with only a Datakit connection:
.P1
import -a helix /net
telnet ai.mit.edu
.P2
The
.I import
command makes a Datakit connection to the machine helix where
it starts an instance
.I exportfs
to serve
.CW /net .
The
.I import
command mounts the remote
.CW /net
directory after (the
.I -a
option to
.I import )
the existing contents
of the local
.CW /net
directory.
The directory contains the union of the local and remote contents of
.CW /net .
Local entries supersede remote ones of the same name so
networks on the local machine are chosen in preference
to those supplied remotely.
However, unique entries in the remote directory are now visible in the local
.CW /net 
directory.
The networks helix supports over and above the Datakit
are available to the terminal. The effect on the namespace is shown by the following
example:
.P1
philw-gnot% ls /net
/net/cs
/net/dk
philw-gnot% import -a musca /net
philw-gnot% ls /net
/net/cs
/net/cs
/net/dk
/net/dk
/net/dns
/net/ether
/net/il
/net/tcp
/net/udp
.P2
.NH 2
Ftpfs
.PP
Fed up with the user unfriendly interface of the ftp
command on other systems, we decided to make our ftp
interface a file system.
Our command,
.I ftpfs,
dials the ftp port of a remote system, prompts for login and password, sets image mode,
and mounts the remote file system onto
.CW /n/ftp.
Files and directories are cached to reduce traffic.
The cache is updated whenever a file is created.
Ftpfs works with TOPS-20, VMS, and various Unix flavors
as the remote system.
.NH
Cyclone Fiber Links
.PP
The file servers and CPU servers are connected by
high-bandwidth
point-to-point links.
A link consists of two VME cards connected by a pair of optical
fibers.
The VME cards use 33Mhz Intel 960 processors and AMD's TAXI
fiber transmitter/receivers to drive the lines at 125 Mbit/sec.
Software in the VME card reduces latency by copying messages from system memory
to fiber without intermediate buffering.
.NH
Performance
.PP
We've measured both latency and throughput
of reading and writing bytes between two processes
for a number of different paths.
Measurements were made on two and four
CPU SGI Power Series processors.
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
More information about 9P can be found in the Plan 9 Programmers
manual available by anonymous ftp from research.att.com
in the directory dist/pla9doc.
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
XEROX Palo Alto Research Center, February 1980.
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
Litchfield Park AZ, December 1990.
.LP
[Rit84a] D. M. Ritchie, ``A Stream Input-Output System'',
.I
AT&T Bell Laboratories Technical Journal, 68(8),
.R
October 1984.
