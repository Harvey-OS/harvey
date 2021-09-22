/*
 * tftp definitions
 */
enum {
	Maxpath=	128,
	Maxerr=		256,

	Opsize=		sizeof(short),
	Blksize=	sizeof(short),
	Hdrsize=	Opsize + Blksize,

	Ackerr=		-1,
	Ackok=		0,
	Ackrexmit=	1,

	/* op codes */
	Tftp_READ	= 1,
	Tftp_WRITE	= 2,
	Tftp_DATA	= 3,
	Tftp_ACK	= 4,
	Tftp_ERROR	= 5,
	Tftp_OACK	= 6,		/* option acknowledge */

	Errnotdef	= 0,		/* see textual error instead */
	Errnotfound	= 1,
	Errnoaccess	= 2,
	Errdiskfull	= 3,
	Errbadop	= 4,
	Errbadtid	= 5,
	Errexists	= 6,
	Errnouser	= 7,
	Errbadopt	= 8,		/* really bad option value */

	Defsegsize	= 512,
	Maxsegsize	= 65464,	/* from rfc2348 */

	/*
	 * bandt (viaduct) tunnels use smaller mtu than ether's
	 * (1400 bytes for tcp mss of 1300 bytes).
	 */
	Bandtmtu	= 1400,
	/*
	 * maximum size of block's data content, excludes hdrs,
	 * notably IP/UDP and TFTP, using worst-case (IPv6) sizes.
	 */
	Bandtblksz	= Bandtmtu - 40 - 8,
	Bcavium		= 1432,		/* cavium's u-boot demands this size */
};
