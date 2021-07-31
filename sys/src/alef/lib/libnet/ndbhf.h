/* a hash file */
aggr Ndbhf
{
	Ndbhf	*next;

	int	fd;
	uint	dbmtime;	/* mtime of data base */
	int	hlen;		/* length (in entries) of hash table */
	byte	attr[Ndbalen];	/* attribute hashed */

	byte	buf[256];	/* hash file buffer */
	int	off;		/* offset of first byte of buffer */
	int	len;		/* length of valid data in buffer */
};

