.nr PS 12
.nr VS 14
.FP palatino
.
.TL
My Dream Ethernet Interface
.AU
Geoff Collyer
.AI
Plan 9 Foundation
.
.AB
Most existing Ethernet interfaces have lots of registers
that I, as a system programmer, have no interest in.
This paper is a thought experiment in designing my ideal
minimal interface.
.AE
.
.NH
Requirements
.LP
I don't write diagnostics, so I only care about
production use and prefer that the hardware come up
in an immediately-usable state.
Diagnostic registers and those that control fancy features
for jumping on passing bandwagons should be controlled by other,
disjoint registers.
These are the functions that an operating system needs:
.IP \(bu 3
establish buffer rings or NVME-style submit and completion queues
for input and output;
.IP \(bu
read and set MAC addresses;
.IP \(bu
enable and disable interrupts for received packet(s);
.IP \(bu
start and stop transmission and reception;
and
.IP \(bu
notification of link speed.
.LP
Note that none of this requires
the use of proprietary techniques
nor interfaces,
nor does it require software fiddling with
PHYs nor EPROMs nor S?R?G?MII nor DMA settings.
Auto-negotiation of speed and link state,
and the like should happen automatically
and without intervention from the operating system.
.LP
The interface should be cache coherent (including DMA)
with all CPUs in the system.
.LP
Errors are generally uninteresting; Ethernet is not guaranteed reliable
and higher-level protocols such as TCP and UDP will have to detect
errors or loss.
.LP
Multicast filtering should be performed internally and not require
the operating system to know the hash function used, if any.
Broadcast packets should always be accepted (ARP must work).
.LP
If IP checksum offloading is performed, it
.I must
be thoroughly validated by automated means,
such as the Spin verifier.
.[
%T Using SPIN
%A Gerard J. Holzmann
.]
A zero checksum must be understood as `no checksum'.
Offloading must be optional.
When it's wrong, it breaks things, and Intel has got this wrong in the past.
.
.NH
Strawman Proposal
.LP
Byte order of registers and descriptors is assumed to be little-endian,
since that seems to be what we are cursed with.
Integer types are those of Plan 9.
.br
.ne 8
.LP
Each transmit and receive buffer needs a descriptor:
.DS
.ft CW
struct Desc {
	uvlong	address;
	ushort	length;
	ushort	bytesread;
	uint	flags;	/* bits: rx filled, tx sent */
};
.ft
.DE
.br
.ne 11
.LP
The registers interface can be fairly brief:
.DS
.ft CW
struct Etherifc {
	uchar	magic[4];	/* "eth" */
	ulong	control; /* bits: rx go, tx go, speed, link, */
			/* promisc, rx intr enable, one-shot intr */
	uvlong	rxring;		/* base address of rx Descs */
	ulong	rxrlen;
	uvlong	txring;		/* base address of tx Descs */
	ulong	txrlen;
	uchar	macs[64][6];	/* many [um]*cast addresses */
};
.ft
.DE
.CW macs[0]
should contain the default MAC address
and all others should be zero at reset.
.
.SH
References
.LP
.[
$LIST$
.]
