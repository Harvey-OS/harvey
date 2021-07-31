typedef struct Icmp {
#define	ETHERIPHDR	34
	uchar	d[6];
	uchar	s[6];
	uchar	ethertype[2];
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
#define	ICMPHDR	8
	uchar	type;
	uchar	code;
	uchar	cksum[2];
	uchar	icmpid[2];
	uchar	seq[2];
	uchar	data[1];
} Icmp;

enum {
	EchoReply=0,
	EchoRequest=8,
};

enum {
	ICMP_PROTO=1,
};
