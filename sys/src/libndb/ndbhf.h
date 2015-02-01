/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

char*		_ndbparsetuple(char*, Ndbtuple**);
Ndbtuple*	_ndbparseline(char*);

#define ISWHITE(x) ((x) == ' ' || (x) == '\t' || (x) == '\r')
#define EATWHITE(x) while(ISWHITE(*(x)))(x)++

extern Ndbtuple *_ndbtfree;

/* caches */
void	_ndbcacheflush(Ndb *db);
int	_ndbcachesearch(Ndb *db, Ndbs *s, char *attr, char *val, Ndbtuple **t);
Ndbtuple* _ndbcacheadd(Ndb *db, Ndbs *s, char *attr, char *val, Ndbtuple *t);
