/*
 *  this file used by (at least) snoopy, tboot and bootp
 */
enum
{
	Bootrequest = 1,
	Bootreply   = 2,
};

typedef struct Bootp Bootp;
struct Bootp
{
	uchar	op;		/* opcode */
	uchar	htype;		/* hardware type */
	uchar	hlen;		/* hardware address len */
	uchar	hops;		/* hops */
	uchar	xid[4];		/* a random number */
	uchar	secs[2];	/* elapsed snce client started booting */
	uchar	flag[2];
	uchar	ciaddr[4];	/* client IP address (client tells server) */
	uchar	yiaddr[4];	/* client IP address (server tells client) */
	uchar	siaddr[4];	/* server IP address */
	uchar	giaddr[4];	/* gateway IP address */
	uchar	chaddr[16];	/* client hardware address */
	char	sname[64];	/* server host name (optional) */
	char	file[128];	/* boot file name */
	char	vend[128];	/* vendor-specific goo */
};
