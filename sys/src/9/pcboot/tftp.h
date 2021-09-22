enum {
	TFTPport	= 69,
	Timeout		= 3000,	/* milliseconds */
	Tftp_READ	= 1,
	Tftp_WRITE	= 2,
	Tftp_DATA	= 3,
	Tftp_ACK	= 4,
	Tftp_ERROR	= 5,
	Tftp_OACK	= 6,		/* extension: option(s) ack */

	Defsegsize	= 512,
	Tftpusehdrs	= 0,	/* flag: use announce+headers for tftp? */

	Ok =		0,
	Err =		-1,
	Nonexist =	-2,
};

int	tftpboot(Openeth *oe, char *file, Bootp *rep, Boot *b);
int	tftpopen(Openeth *oe, char *file, Bootp *rep);
long	tftprdfile(Openeth *oe, int openread, void* va, long len);
int	tftpread(Openeth *oe, Pxenetaddr *a, Tftp *tftp, int dlen);

extern int tftpphase;
