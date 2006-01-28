.HTML "The IL Protocol
.TL
The IL protocol
.AU
Dave Presotto
Phil Winterbottom
.sp
presotto,philw@plan9.bell-labs.com
.AB
To transport the remote procedure call messages of the Plan 9 file system
protocol 9P, we have implemented a new network protocol, called IL.
It is a connection-based, lightweight transport protocol that carries
datagrams encapsulated by IP.
IL provides retransmission of lost messages and in-sequence delivery, but has
no flow control and no blind retransmission.
.AE
.SH
Introduction
.PP
Plan 9 uses a file system protocol, called 9P [PPTTW93], that assumes
in-sequence guaranteed delivery of delimited messages
holding remote procedure call
(RPC) requests and responses.
None of the standard IP protocols [RFC791] is suitable for transmission of
9P messages over an Ethernet or the Internet.
TCP [RFC793] has a high overhead and does not preserve delimiters.
UDP [RFC768], while cheap and preserving message delimiters, does not provide
reliable sequenced delivery.
When we were implementing IP, TCP, and UDP in our system we
tried to choose a protocol suitable for carrying 9P.
The properties we desired were:
.IP \(bu
Reliable datagram service
.IP \(bu
In-sequence delivery
.IP \(bu
Internetworking using IP
.IP \(bu
Low complexity, high performance
.IP \(bu
Adaptive timeouts
.LP
No standard protocol met our needs so we designed a new one,
called IL (Internet Link).
.PP
IL is a lightweight protocol encapsulated by IP.
It is connection-based and
provides reliable transmission of sequenced messages.
No provision is made for flow control since the protocol
is designed to transport RPC
messages between client and server, a structure with inherent flow limitations.
A small window for outstanding messages prevents too
many incoming messages from being buffered;
messages outside the window are discarded
and must be retransmitted.
Connection setup uses a two-way handshake to generate
initial sequence numbers at each end of the connection;
subsequent data messages increment the
sequence numbers to allow
the receiver to resequence out of order messages. 
In contrast to other protocols, IL avoids blind retransmission.
This helps performance in congested networks,
where blind retransmission could cause further
congestion.
Like TCP, IL has adaptive timeouts,
so the protocol performs well both on the
Internet and on local Ethernets.
A round-trip timer is used
to calculate acknowledge and retransmission times
that match the network speed.
.SH
Connections
.PP
An IL connection carries a stream of data between two end points.
While the connection persists,
data entering one side is sent to the other side in the same sequence.
The functioning of a connection is described by the state machine in Figure 1,
which shows the states (circles) and transitions between them (arcs).
Each transition is labeled with the list of events that can cause
the transition and, separated by a horizontal line,
the messages sent or received on that transition.
The remainder of this paper is a discussion of this state machine.
.KF
\s-2
.PS 5.5i
copy "transition.pic"
.PE
\s+2
.RS
.IP \fIackok\fR 1.5i
any sequence number between id0 and next inclusive
.IP \fI!x\fR 1.5i
any value except x
.IP \- 1.5i
any value
.RE
.sp
.ce
.I "Figure 1 - IL State Transitions
.KE
.PP
The IL state machine has five states:
.I Closed ,
.I Syncer ,
.I Syncee ,
.I Established ,
and
.I Closing .
The connection is identified by the IP address and port number used at each end.
The addresses ride in the IP protocol header, while the ports are part of the
18-byte IL header.
The local variables identifying the state of a connection are:
.RS
.IP state 10
one of the states
.IP laddr 10
32-bit local IP address
.IP lport 10
16-bit local IL port
.IP raddr 10
32-bit remote IP address
.IP rport 10
16-bit remote IL port
.IP id0 10
32-bit starting sequence number of the local side
.IP rid0 10
32-bit starting sequence number of the remote side
.IP next 10
sequence number of the next message to be sent from the local side
.IP rcvd 10
the last in-sequence message received from the remote side
.IP unacked 10
sequence number of the first unacked message
.RE
.PP
Unused connections are in the
.I Closed
state with no assigned addresses or ports.
Two events open a connection: the reception of
a message whose addresses and ports match no open connection
or a user explicitly opening a connection.
In the first case, the message's source address and port become the
connection's remote address and port and the message's destination address
and port become the local address and port.
The connection state is set to
.I Syncee
and the message is processed.
In the second case, the user specifies both local and remote addresses and ports.
The connection's state is set to
.I Syncer
and a
.CW sync
message is sent to the remote side.
The legal values for the local address are constrained by the IP implementation.
.SH
Sequence Numbers
.PP
IL carries data messages.
Each message corresponds to a single write from
the operating system and is identified by a 32-bit
sequence number.
The starting sequence number for each direction in a
connection is picked at random and transmitted in the initial
.CW sync
message.
The number is incremented for each subsequent data message.
A retransmitted message contains its original sequence number.
.SH
Transmission/Retransmission
.PP
Each message contains two sequence numbers:
an identifier (ID) and an acknowledgement.
The acknowledgement is the last in-sequence
data message received by the transmitter of the message.
For
.CW data
and
.CW dataquery
messages, the ID is its sequence number.
For the control messages
.CW sync ,
.CW ack ,
.CW query ,
.CW state ,
and
.CW close ,
the ID is one greater than the sequence number of
the highest sent data message.
.PP
The sender transmits data messages with type
.CW data .
Any messages traveling in the opposite direction carry acknowledgements.
An
.CW ack
message will be sent within 200 milliseconds of receiving the data message
unless a returning message has already piggy-backed an
acknowledgement to the sender.
.PP
In IP, messages may be delivered out of order or
may be lost due to congestion or faults.
To overcome this,
IL uses a modified ``go back n'' protocol that also attempts
to avoid aggravating network congestion.
An average round trip time is maintained by measuring the delay between
the transmission of a message and the
receipt of its acknowledgement.
Until the first acknowledge is received, the average round trip time
is assumed to be 100ms.
If an acknowledgement is not received within four round trip times
of the first unacknowledged message
.I "rexmit timeout" "" (
in Figure 1), IL assumes the message or the acknowledgement
has been lost.
The sender then resends only the first unacknowledged message,
setting the type to
.CW dataquery .
When the receiver receives a
.CW dataquery ,
it responds with a
.CW state
message acknowledging the highest received in-sequence data message.
This may be the retransmitted message or, if the receiver has been
saving up out-of-sequence messages, some higher numbered message.
Implementations of the receiver are free to choose whether to save out-of-sequence messages.
Our implementation saves up to 10 packets ahead.
When the sender receives the
.CW state
message, it will immediately resend the next unacknowledged message
with type
.CW dataquery .
This continues until all messages are acknowledged.
.PP
If no acknowledgement is received after the first
.CW dataquery ,
the transmitter continues to timeout and resend the
.CW dataquery
message.
The intervals between retransmissions increase exponentially.
After 300 times the round trip time
.I "death timeout" "" (
in Figure 1), the sender gives up and
assumes the connection is dead.
.PP
Retransmission also occurs in the states
.I Syncer ,
.I Syncee ,
and
.I Close .
The retransmission intervals are the same as for data messages.
.SH
Keep Alive
.PP
Connections to dead systems must be discovered and torn down
lest they consume resources.
If the surviving system does not need to send any data and
all data it has sent has been acknowledged, the protocol
described so far will not discover these connections.
Therefore, in the
.I Established
state, if no other messages are sent for a 6 second period,
a
.CW query
is sent.
The receiver always replies to a
.CW query
with a
.CW state
message.
If no messages are received for 30 seconds, the
connection is torn down.
This is not shown in Figure 1.
.SH
Byte Ordering
.PP
All 32- and 16-bit quantities are transmitted high-order byte first, as
is the custom in IP.
.SH
Formats
.PP
The following is a C language description of an IP+IL
header, assuming no IP options:
.P1
typedef unsigned char byte;
struct IPIL
{
	byte	vihl;       /* Version and header length */
	byte	tos;        /* Type of service */
	byte	length[2];  /* packet length */
	byte	id[2];      /* Identification */
	byte	frag[2];    /* Fragment information */
	byte	ttl;        /* Time to live */
	byte	proto;      /* Protocol */
	byte	cksum[2];   /* Header checksum */
	byte	src[4];     /* Ip source */
	byte	dst[4];     /* Ip destination */
	byte	ilsum[2];   /* Checksum including header */
	byte	illen[2];   /* Packet length */
	byte	iltype;     /* Packet type */
	byte	ilspec;     /* Special */
	byte	ilsrc[2];   /* Src port */
	byte	ildst[2];   /* Dst port */
	byte	ilid[4];    /* Sequence id */
	byte	ilack[4];   /* Acked sequence */
};
.P2
.LP
Data is assumed to immediately follow the header in the message.
.CW Ilspec
is an extension reserved for future protocol changes.
.PP
The checksum is calculated with
.CW ilsum
and
.CW ilspec
set to zero.
It is the standard IP checksum, that is, the 16-bit one's complement of the one's
complement sum of all 16 bit words in the header and text.  If a
message contains an odd number of header and text bytes to be
checksummed, the last byte is padded on the right with zeros to
form a 16-bit word for the checksum.
The checksum covers from
.CW cksum
to  the end of the data.
.PP
The possible
.I iltype
values are:
.P1
enum {
	sync=		0,
	data=		1,
	dataquery=	2,
	ack=		3,
	query=		4,
	state=		5,
	close=		6,
};
.P2
.LP
The
.CW illen
field is the size in bytes of the IL header (18 bytes) plus the size of the data.
.SH
Numbers
.PP
The IP protocol number for IL is 40.
.PP
The assigned IL port numbers are:
.RS
.IP 7 15
echo all input to output
.IP 9 15
discard input
.IP 19 15
send a standard pattern to output
.IP 565 15
send IP addresses of caller and callee to output
.IP 566 15
Plan 9 authentication protocol
.IP 17005 15
Plan 9 CPU service, data
.IP 17006 15
Plan 9 CPU service, notes
.IP 17007 15
Plan 9 exported file systems
.IP 17008 15
Plan 9 file service
.IP 17009 15
Plan 9 remote execution
.IP 17030 15
Alef Name Server
.RE
.SH
References
.LP
[PPTTW93] Rob Pike, Dave Presotto, Ken Thompson, Howard Trickey, and Phil Winterbottom,
``The Use of Name Spaces in Plan 9'',
.I "Op. Sys. Rev.,
Vol. 27, No. 2, April 1993, pp. 72-76,
reprinted in this volume.
.br
[RFC791] RFC791,
.I "Internet Protocol,
.I "DARPA Internet Program Protocol Specification,
September 1981.
.br
[RFC793] RFC793,
.I "Transmission Control Protocol,
.I "DARPA Internet Program Protocol Specification,
September 1981.
.br
[RFC768] J. Postel, RFC768,
.I "User Datagram Protocol,
.I "DARPA Internet Program Protocol Specification,
August 1980.
