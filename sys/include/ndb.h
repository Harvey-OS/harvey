#pragma	lib	"libndb.a"

/*
 *  this include file requires includes of <u.h> and <bio.h>
 */
typedef struct Ndb	Ndb;
typedef struct Ndbtuple	Ndbtuple;
typedef struct Ndbhf	Ndbhf;
typedef struct Ndbs	Ndbs;

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
	Ndb	*next;

	Biobufhdr;		/* buffered input file */
	uchar	buf[256];	/* and it's buffer */
	ulong	offset;		/* current file offset */
	char	*line;		/* next unparsed line */
	int	linelen;	/* and its length */

	ulong	mtime;		/* mtime of db file */
	Qid	qid;		/* qid of db file */
	char	file[2*NAMELEN];/* path name of db file */

	Ndbhf	*hf;		/* open hash files */
};

/*
 *  a parsed entry, doubly linked
 */
struct Ndbtuple
{
	char		attr[Ndbalen];	/* attribute name */
	char		val[Ndbvlen];	/* value(s) */
	Ndbtuple	*entry;		/* next tuple in this entry */
	Ndbtuple	*line;		/* next tuple on this line */
	ulong		ptr;		/* (for the application - starts 0) */
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
	ulong	ptr;	/* current pointer */
	ulong	ptr1;	/* next pointer */
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
#define NDBPUTP(v,a) { (a)[0] = v; (a)[1] = (v)>>8; (a)[2] = (v)>>16; }
#define NDBGETP(a) ((a)[0] | ((a)[1]<<8) | ((a)[2]<<16))

/*
 *  macros for packing and unpacking unsigned longs
 */
#define NDBPUTUL(v,a) { (a)[0] = v; (a)[1] = (v)>>8; (a)[2] = (v)>>16; (a)[3] = (v)>>24; }
#define NDBGETUL(a) ((a)[0] | ((a)[1]<<8) | ((a)[2]<<16) | ((a)[3]<<24))

enum 
{
	DNScache=	128,
};

/*
 *  Domain name service cache
 */
typedef struct Dns	Dns;
struct Dns
{
	Ndbtuple	*cache[DNScache];
};

/*
 *  ip information about a system
 */
typedef struct Ipinfo	Ipinfo;
struct Ipinfo
{
	char	domain[128];		/* system domain name */
	char	bootf[128];		/* boot file */
	uchar	ipaddr[4];		/* ip address of system */
	uchar	ipmask[4];		/* ip network mask */
	uchar	ipnet[4];		/* ip network address (ipaddr & ipmask) */
	uchar	etheraddr[6];		/* ethernet address */
	uchar	gwip[4];		/* gateway ip address */
	uchar	fsip[4];		/* file system ip address */
	uchar	auip[4];		/* authentication server ip address */
};

ulong		ndbhash(char*, int);
Ndbtuple*	ndbparse(Ndb*);
void		ndbfree(Ndbtuple*);
Ndb*		ndbopen(char*);
int		ndbreopen(Ndb*);
void		ndbclose(Ndb*);
long		ndbseek(Ndb*, long, int);
Ndbtuple*	ndbsearch(Ndb*, Ndbs*, char*, char*);
Ndbtuple*	ndbsnext(Ndbs*, char*, char*);
Ndbtuple*	ndbgetval(Ndb*, Ndbs*, char*, char*, char*, char*);
char*		ipattr(char*);
int		ipinfo(Ndb*, char*, char*, char*, Ipinfo*);

