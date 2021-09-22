enum {
	Maxpayload =	32*1024,
	Maxrpcbuf =	8192,		/* devmnt's MAXRPC - IOHDRSZ */
	Copybufsz = 	4096,
	Blobsz =	512,
	Numbsz =	24,		/* big enough for 2^64 */

	Defstk =	80*1024,	/* debugging */
//	Defstk =	8*1024,		/* awfully small */
	Maxtoks =	32,

	Arbpathlen =	128,
	Arbbufsz =	256,
	Bigbufsz =	1024,
	Maxfactotum =	256*1024,	/* max bytes in /mnt/factotum/ctl */
};

char *esmprint(char *format, ...);
