

enum {
	/* Mifare classic commands */
	MCAUTHA	= 0x60,
	MCAUTHB	= 0x61,
	MCREAD	= 0x30,
	MCWRITE	= 0xA0,
	MCTRANS	= 0xB0,
	MCDEC	= 0xC0,
	MCINC	= 0xC1,
	MCSTORE	 = 0xC2,
	
	Multrablksz = 15,	/* BUG: should be 16, but you'd be suprised */
};

CmdiccR *		mifareop(int fd, int ncard, uchar mc, uchar nblk);
int		mifareblkread(int fd, uchar *blk, uchar nbytes, uchar off, uchar nblk);
