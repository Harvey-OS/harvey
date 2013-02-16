enum {
	Maxpayload =	32*1024,
	Maxrpcbuf =	8192,		/* devmnt's MAXRPC - IOHDRSZ */
	Copybufsz = 	4096,
	Blobsz =	512,
	Numbsz =	24,		/* enough digits for 2^64 */

	Defstk =	80*1024,	/* was 8K, which seems small */
	Maxtoks =	32,

	Arbpathlen =	128,
	Arbbufsz =	256,
	Bigbufsz =	1024,
	Maxfactotum =	256*1024,	/* max bytes in /mnt/factotum/ctl */
};

typedef struct Conn Conn;
#pragma incomplete Conn

#pragma	varargck argpos	esmprint 1
#pragma	varargck argpos	ssdebug	2
#pragma	varargck argpos	sshlog	2

char *esmprint(char *format, ...);
void sshdebug(Conn *, char *format, ...);
void sshlog(Conn *, char *format, ...);

void freeptr(void **);
int readfile(char *file, char *buf, int size);
