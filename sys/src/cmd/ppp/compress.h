aggr Ip {
	byte	vihl;		/* Version and header length */
	byte	tos;		/* Type of service */
	byte	length[2];	/* packet length */
	byte	id[2];		/* Identification */
	byte	frag[2];	/* Fragment information */
	byte	ttl;		/* Time to live */
	byte	proto;		/* Protocol */
	byte	ipcksum[2];	/* Header checksum */
	byte	src[4];
	byte	dst[4];
};

aggr Tcpip {
	byte	vihl;		/* Version and header length */
	byte	tos;		/* Type of service */
	byte	length[2];	/* packet length */
	byte	id[2];		/* Identification */
	byte	frag[2];	/* Fragment information */
	byte	ttl;		/* Time to live */
	byte	proto;		/* Protocol */
	byte	ipcksum[2];	/* Header checksum */
#define	CONNIDSZ	12	/* src + dst + sport + dport */
	byte	src[4];		/* Ip source */
	byte	dst[4];		/* Ip destination */
	byte	sport[2];
	byte	dport[2];
	byte	seq[4];
	byte	ack[4];
	byte	flag[2];
	byte	win[2];
	byte	tcpcksum[2];
	byte	urg[2];
};

aggr Il {
	byte	vihl;		/* Version and header length */
	byte	tos;		/* Type of service */
	byte	length[2];	/* packet length */
	byte	id[2];		/* Identification */
	byte	frag[2];	/* Fragment information */
	byte	ttl;		/* Time to live */
	byte	proto;		/* Protocol */
	byte	ipcksum[2];	/* Header checksum */
	byte	src[4];		/* Ip source */
	byte	dst[4];		/* Ip destination */	
	byte	ilsum[2];	/* Checksum including header */
	byte	illen[2];	/* Packet length */
	byte	iltype;		/* Packet type */
	byte	ilspec;		/* Special */
	byte	ilsrc[2];	/* Src port */
	byte	ildst[2];	/* Dst port */
	byte	ilid[4];	/* Sequence id */
	byte	ilack[4];	/* Acked sequence */
};

enum
{
	URG		= 0x20,		/* Data marked urgent */
	ACK		= 0x10,		/* Aknowledge is valid */
	PSH		= 0x08,		/* Whole data pipe is pushed */
	RST		= 0x04,		/* Reset connection */
	SYN		= 0x02,		/* Pkt. is synchronise */
	FIN		= 0x01,		/* Start close down */

	TCP_PUSH_BIT	= 0x10,
	TCPMAX_STATES	= 16,		/* must fit in a byte */
	ILMAX_STATES	= 16,

	IP_TCPPROTO	= 6,
	IP_ILPROTO	= 40,
	IL_IPHDR	= 20,
};

aggr Ilcomp {
	byte	lastrecv;
	byte	lastxmit;
	byte	basexmit;
	byte	err;
	Il	tip[ILMAX_STATES];
	Il	rip[ILMAX_STATES];
};

aggr Tcpcomp {
	byte lastrecv;			/* last rcvd conn. id */
	byte lastxmit;			/* last sent conn. id */
	byte basexmit;			/* least conn. id */
	byte err;			/* nonzero if line error occurred */
	Tcpip tip[TCPMAX_STATES];	/* xmit connection states */
	Tcpip rip[TCPMAX_STATES];	/* receive connection states */
};

enum {	/* flag bits for what changed in a packet */
	NEW_U=(1<<0),	/* tcp only */
	NEW_W=(1<<1),	/* tcp only */
	NEW_A=(1<<2),	/* il tcp */
	NEW_S=(1<<3),	/* tcp only */
	NEW_P=(1<<4),	/* tcp only */
	NEW_I=(1<<5),	/* il tcp */
	NEW_C=(1<<6),	/* il tcp */
	NEW_T=(1<<7),	/* il only */
};

/* reserved, special-case values of above for tcp */
#define SPECIAL_I (NEW_S|NEW_W|NEW_U)		/* echoed interactive traffic */
#define SPECIAL_D (NEW_S|NEW_A|NEW_W|NEW_U)	/* unidirectional data */
#define SPECIALS_MASK (NEW_S|NEW_A|NEW_W|NEW_U)

/*
 * compress.l
 */
(usint, byte*, int)	compress(PPPstate *s, byte *);
(byte *, int)		iluncompress(PPPstate *s, byte *, int, usint);
(byte *, int)		tcpuncompress(PPPstate *s, byte *, int, usint);
void			compress_init(PPPstate *s);
