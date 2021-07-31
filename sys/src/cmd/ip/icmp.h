/* ICMP for IP v4 and v6 */
enum
{
	/* Packet Types, icmp v4 (rfc 792) */
	EchoReply	= 0,
	Unreachable	= 3,
	SrcQuench	= 4,
	Redirect	= 5,
	EchoRequest	= 8,
	TimeExceed	= 11,
	InParmProblem	= 12,
	Timestamp	= 13,
	TimestampReply	= 14,
	InfoRequest	= 15,
	InfoReply	= 16,
	AddrMaskRequest = 17,
	AddrMaskReply   = 18,
	Traceroute	= 30,
	IPv6WhereAreYou	= 33,
	IPv6IAmHere	= 34,

	/* packet types, icmp v6 (rfc 2463) */

        /* error messages */
	UnreachableV6	= 1,
	PacketTooBigV6	= 2,
	TimeExceedV6	= 3,
	ParamProblemV6	= 4,

        /* informational messages (rfc 2461 also) */
	EchoRequestV6	= 128,
	EchoReplyV6	= 129,
	RouterSolicit	= 133,
	RouterAdvert	= 134,
	NbrSolicit	= 135,
	NbrAdvert	= 136,
	RedirectV6	= 137,

	Maxtype6	= 137,

	ICMP_HDRSIZE	= 8,
};

/* the icmp payload has the same format in v4 and v6 */
typedef struct Icmphdr Icmphdr;
struct Icmphdr {
	uchar	type;
	uchar	code;
	uchar	cksum[2];
	uchar	icmpid[2];
	uchar	seq[2];
	uchar	data[1];
};

typedef struct Icmp Icmp;
struct Icmp
{
	uchar	vihl;		/* Version and header length */
	uchar	tos;		/* Type of service */
	uchar	length[2];	/* packet length */
	uchar	id[2];		/* Identification */
	uchar	frag[2];	/* Fragment information */
	uchar	ttl;		/* Time to live */
	uchar	proto;		/* Protocol */
	uchar	ipcksum[2];	/* Header checksum */
	uchar	src[4];		/* Ip source */
	uchar	dst[4];		/* Ip destination */

	Icmphdr;
};

typedef struct Icmp6 Icmp6;
struct Icmp6
{
	Ip6hdr;
	Icmphdr;
};
