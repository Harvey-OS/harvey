#pragma lib "/$M/lib/alef/libbio.a"

enum
{
	Bsize		= 4096,
	Bungetsize	= 4,		/* space for ungetc */
	Bmagic		= 0x314159,
	Beof		= -1,
	Bbad		= -2,

	Binactive	= 0,		/* states */
	Bractive,
	Bwactive,
};

adt	Biobufhdr
{
	QLock;

	int	icount;		/* neg num of bytes at eob */
	int	ocount;		/* num of bytes at bob */
	int	nrdline;	/* num of bytes after rdline */
	int	runesize;	/* num of bytes of last getrune */
	int	state;		/* r/w/inactive */
	int	fid;		/* open file */
extern	int	flag;		/* magic if malloc'ed */
	int	off;		/* offset of buffer in file */
	int	bsize;		/* size of buffer */
	char*	bbuf;		/* pointer to beginning of buffer */
	char*	ebuf;		/* pointer to end of buffer */

intern	int	iflush(*Biobufhdr);

	int	buffered(*Biobufhdr);
	int	close(*Biobufhdr);
	void	erase(*Biobufhdr);
	int	fildes(*Biobufhdr);
	int	flush(*Biobufhdr);
	int	getc(*Biobufhdr);
	int	getrune(*Biobufhdr);
	int	inits(*Biobufhdr, int, int, char*, int);
	void	kill(*Biobufhdr);
	int	linelen(*Biobufhdr);
	int	offset(*Biobufhdr);
	int	print(*Biobufhdr, char*, ...);
	int	putc(*Biobufhdr, int);
	int	putrune(*Biobufhdr, int);
	void*	rdline(*Biobufhdr, int);
	int	read(*Biobufhdr, void*, int);
	int	seek(*Biobufhdr, int, int);
	int	ungetc(*Biobufhdr);
	int	ungetrune(*Biobufhdr);
	int	write(*Biobufhdr, void*, int);
};

adt	Biobuf
{
	Biobufhdr;

extern	char	b[Bungetsize+Bsize];

	int	init(*Biobuf, int, int);
};

Biobuf*	Bopen(char*, int);
void	batexit(void);
