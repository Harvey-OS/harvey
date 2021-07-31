/*
 * Control messages (regular priority)
 */
#define	M_DATA	0		/* regular data (not ctl) */
#define	M_BREAK	01		/* line break */
#define	M_HANGUP 02		/* line disconnect */
#define	M_DELIM	03		/* data delimiter */
#define	M_ECHO	04		/* request ACK (1 param) */
#define	M_ACK	05		/* response to ECHO (1 param) */
#define	M_IOCTL	06		/* ioctl; set/get params */
#define	M_DELAY 07		/* real-time xmit delay (1 param) */
#define	M_CTL	010		/* device-specific control message */
#define	M_PASS	011		/* pass file */
#define	M_YDEL	012		/* stream has started generating delims */
#define	M_NDEL	013		/* stream has stopped generating delims */

/*
 * Control messages (high priority; go to head of queue)
 */
#define	M_SIGNAL 0101		/* generate process signal */
#define	M_FLUSH	0102		/* flush your queues */
#define	M_STOP	0103		/* stop transmission immediately */
#define	M_START	0104		/* restart transmission after stop */
#define	M_IOCACK 0105		/* acknowledge ioctl */
#define	M_IOCNAK 0106		/* negative ioctl acknowledge */
#define	M_CLOSE	0107		/* channel closes (dk only) */
#define	M_IOCWAIT 0110		/* stop ioctl timeout, ack/nak follows later */

/*
 * ioctl message packet
 */

#define	STIOCSIZE	16
#define	STIOCHDR	4

struct stioctl {
	unsigned char com[STIOCHDR];	/* four-byte command, low order byte first */
	char data[STIOCSIZE];	/* depends on command */
};

/*
 * header for messages, see mesg.c
 */
#define MSLEN 2
struct mesg {
	char		type;
	unsigned char	magic;
	unsigned char	size[MSLEN];	/* 2 byte size, low order first */
};

#define	MSGMAGIC	0345
#define	MSGHLEN	4	/* true length of struct mesg in bytes */

/*
 * magic numbers of line disciplines
 */

#define	tty_ld		0	/* tty processing */
#define	cdkp_ld		1	/* URP protocol -- character mode (same as 1) */
#define	rdk_ld		2	/* raw datakit protocol */
#define	pk_ld		3	/* packet driver */
#define	mesg_ld		4	/* data message protocol */
#define	dkp_ld		5	/* URP protocol -- block mode */
#define	ntty_ld		6	/* new tty processing */
#define	buf_ld		7	/* buffer up characters till timeout */
#define	trc_ld		8	/* stream tracer */
#define	rmesg_ld	9	/* reverse message processing */
#define	ip_ld		10	/* IP - push on net interfaces (il, ec, ...) */
#define	tcp_ld		11	/* TCP (inet) - only one instance, on /dev/ip6 */
#define	chroute_ld	12	/* Chaosnet - push on net interfaces (il, ec, ...) */
#define	arp_ld		13	/* Ethernet address resolution - on net interfaces */
#define	udp_ld		14	/* UDP (inet) - only one instance, on /dev/ip */
#define	chaos_ld	15	/* Chaosnet - only one, above any chroute_ld */
#define	filter_ld	16	/* Delimiter filtering */
#define	dump_ld		17	/* Debug dumper */
#define	conn_ld		18	/* Connection line discipline */
#define	uxp_ld		19	/* unix common control protocol */
