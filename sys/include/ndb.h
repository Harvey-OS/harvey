/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *  this include file requires includes of <u.h> and <bio.h>
 */
typedef struct Ndb	Ndb;
typedef struct Ndbtuple	Ndbtuple;
typedef struct Ndbhf	Ndbhf;
typedef struct Ndbs	Ndbs;
typedef struct Ndbcache	Ndbcache;

enum
{
	Ndbalen=	32,	/* max attribute length */
	Ndbvlen=	64,	/* max value length */
};

/*
 *  the database
 */
struct Ndb
{
	Ndb		*next;

	Biobufhdr	b;		/* buffered input file */
	uint8_t		buf[256];	/* and its buffer */

	uint32_t		mtime;		/* mtime of db file */
	Qid		qid;		/* qid of db file */
	char		file[128];/* path name of db file */
	uint32_t		length;		/* length of db file */

	int		nohash;		/* don't look for hash files */
	Ndbhf		*hf;		/* open hash files */

	int		ncache;		/* size of tuple cache */
	Ndbcache	*cache;		/* cached entries */
};

/*
 *  a parsed entry, doubly linked
 */
struct Ndbtuple
{
	char		attr[Ndbalen];		/* attribute name */
	char		*val;			/* value(s) */
	Ndbtuple	*entry;			/* next tuple in this entry */
	Ndbtuple	*line;			/* next tuple on this line */
	uint32_t		ptr;			/* (for the application - starts 0) */
	char		valbuf[Ndbvlen];	/* initial allocation for value */
};

/*
 *  each hash file is of the form
 *
 *		+---------------------------------------+
 *		|	mtime of db file (4 bytes)	|
 *		+---------------------------------------+
 *		|  size of table (in entries - 4 bytes)	|
 *		+---------------------------------------+
 *		|		hash table		|
 *		+---------------------------------------+
 *		|		hash chains		|
 *		+---------------------------------------+
 *
 *  hash collisions are resolved using chained entries added to the
 *  the end of the hash table.
 *
 *  Hash entries are of the form
 *
 *		+-------------------------------+
 *		|	offset	(3 bytes) 	|
 *		+-------------------------------+
 *
 *  Chain entries are of the form
 *
 *		+-------------------------------+
 *		|	offset1	(3 bytes) 	|
 *		+-------------------------------+
 *		|	offset2	(3 bytes) 	|
 *		+-------------------------------+
 *
 *  The top bit of an offset set to 1 indicates a pointer to a hash chain entry.
 */
#define NDBULLEN	4		/* unsigned long length in bytes */
#define NDBPLEN		3		/* pointer length in bytes */
#define NDBHLEN		(2*NDBULLEN)	/* hash file header length in bytes */

/*
 *  finger pointing to current point in a search
 */
struct Ndbs
{
	Ndb	*db;	/* data base file being searched */
	Ndbhf	*hf;	/* hash file being searched */
	int	type;
	uint32_t	ptr;	/* current pointer */
	uint32_t	ptr1;	/* next pointer */
	Ndbtuple *t;	/* last attribute value pair found */
};

/*
 *  bit defs for pointers in hash files
 */
#define NDBSPEC 	(1<<23)
#define NDBCHAIN	NDBSPEC		/* points to a collision chain */
#define NDBNAP		(NDBSPEC|1)	/* not a pointer */

/*
 *  macros for packing and unpacking pointers
 */
#define NDBPUTP(v,a) { (a)[0] = (uint8_t)v; (a)[1] = (uint8_t)(v)>>8; (a)[2] = (uint8_t)(v)>>16; }
#define NDBGETP(a) ((a)[0] | ((a)[1]<<8) | ((a)[2]<<16))

/*
 *  macros for packing and unpacking unsigned longs
 */
#define NDBPUTUL(v,a) { (a)[0] = (uint8_t)v; (a)[1] = (uint8_t)(v)>>8; (a)[2] = (uint8_t)(v)>>16; (a)[3] = (uint8_t)(v)>>24; }
#define NDBGETUL(a) ((a)[0] | ((a)[1]<<8) | ((a)[2]<<16) | ((a)[3]<<24))

#define NDB_IPlen 16

Ndbtuple*	csgetval(char*, char*, char*, char*, char*);
char*		csgetvalue(char*, char*, char*, char*,
				  Ndbtuple**);
Ndbtuple*	csipinfo(char*, char*, char*, char**, int);
Ndbtuple*	dnsquery(char*, char*, char*);
char*		ipattr(char*);
Ndb*		ndbcat(Ndb*, Ndb*);
int		ndbchanged(Ndb*);
void		ndbclose(Ndb*);
Ndbtuple*	ndbconcatenate(Ndbtuple*, Ndbtuple*);
Ndbtuple*	ndbdiscard(Ndbtuple*, Ndbtuple*);
void		ndbfree(Ndbtuple*);
Ndbtuple*	ndbgetipaddr(Ndb*, char*);
Ndbtuple*	ndbgetval(Ndb*, Ndbs*, char*, char*, char*, char*);
char*		ndbgetvalue(Ndb*, Ndbs*, char*, char*, char*,
				   Ndbtuple**);
Ndbtuple*	ndbfindattr(Ndbtuple*, Ndbtuple*, char*);
uint32_t		ndbhash(char*, int);
Ndbtuple*	ndbipinfo(Ndb*, char*, char*, char**, int);
Ndbtuple*	ndblookval(Ndbtuple*, Ndbtuple*, char*, char*);
Ndbtuple*	ndbnew(char*, char*);
Ndb*		ndbopen(char*);
Ndbtuple*	ndbparse(Ndb*);
int		ndbreopen(Ndb*);
Ndbtuple*	ndbreorder(Ndbtuple*, Ndbtuple*);
Ndbtuple*	ndbsearch(Ndb*, Ndbs*, char*, char*);
void		ndbsetval(Ndbtuple*, char*, int);
Ndbtuple*	ndbsnext(Ndbs*, char*, char*);
Ndbtuple*	ndbsubstitute(Ndbtuple*, Ndbtuple*, Ndbtuple*);
void		ndbsetmalloctag(Ndbtuple*, uintptr);
