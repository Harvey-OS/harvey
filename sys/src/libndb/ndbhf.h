/* a hash file */
struct Ndbhf
{
	Ndbhf	*next;

	int	fd;
	ulong	dbmtime;	/* mtime of data base */
	int	hlen;		/* length (in entries) of hash table */
	char	attr[Ndbalen];	/* attribute hashed */

	uchar	buf[256];	/* hash file buffer */
	long	off;		/* offset of first byte of buffer */
	int	len;		/* length of valid data in buffer */
};

