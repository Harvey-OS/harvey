#pragma	src	"/sys/src/alef/lib/libnet"
#pragma	lib	"/$M/lib/alef/libnet.a"

typedef aggr Ndb;
typedef aggr Ndbtuple;
typedef aggr Ndbhf;
typedef aggr Ndbs;

enum
{
	Ndbalen=	32,	/* max attribute length */
	Ndbvlen=	64,	/* max value length */
};

/*
 *  the database
 */
aggr Ndb
{
	Ndb	*next;

	Biobufhdr;		/* buffered input file */
	byte	buf[256];	/* and it's buffer */
	uint	offset;		/* current file offset */
	byte	*line;		/* next unparsed line */
	int	linelen;	/* and its length */

	uint	mtime;		/* mtime of db file */
	Qid	qid;		/* qid of db file */
	byte	file[2*NAMELEN];/* path name of db file */

	Ndbhf	*hf;		/* open hash files */
};

/*
 *  a parsed entry, doubly linked
 */
aggr Ndbtuple
{
	byte		attr[Ndbalen];	/* attribute name */
	byte		val[Ndbvlen];	/* value(s) */
	Ndbtuple	*entry;		/* next tuple in this entry */
	Ndbtuple	*line;		/* next tuple on this line */
	uint		ptr;		/* (for the application - starts 0) */
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
#define NDBULLEN	4		/* unsigned int length in bytes */
#define NDBPLEN		3		/* pointer length in bytes */
#define NDBHLEN		(2*NDBULLEN)	/* hash file header length in bytes */

/*
 *  finger pointing to current point in a search
 */
aggr Ndbs
{
	Ndb	*db;	/* data base file being searched */
	Ndbhf	*hf;	/* hash file being searched */
	int	type;
	uint	ptr;	/* current pointer */
	uint	ptr1;	/* next pointer */
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
 *  macros for packing and unpacking unsigned ints
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
aggr Dns
{
	Ndbtuple	*cache[DNScache];
};

/*
 *  ip information about a system
 */
aggr Ipinfo
{
	byte	domain[128];		/* system domain name */
	byte	bootf[128];		/* boot file */
	byte	ipaddr[4];		/* ip address of system */
	byte	ipmask[4];		/* ip network mask */
	byte	ipnet[4];		/* ip network address (ipaddr & ipmask) */
	byte	etheraddr[6];		/* ethernet address */
	byte	gwip[4];		/* gateway ip address */
	byte	fsip[4];		/* file system ip address */
	byte	auip[4];		/* authentication server ip address */
};

extern byte classmask[4][4];

#define CLASS(p) ((*(byte*)(p))>>6)

/*
 * for user level udp headers
 */
enum 
{
	Udphdrsize=	6,	/* size if a to/from user Udp header */
};

aggr Udphdr
{
	byte	ipaddr[4];
	byte	port[2];
};

uint		ndbhash(byte*, int);
Ndbtuple*	ndbparse(Ndb*);
void		ndbfree(Ndbtuple*);
Ndb*		ndbopen(byte*);
int		ndbreopen(Ndb*);
void		ndbclose(Ndb*);
int		ndbseek(Ndb*, int, int);
Ndbtuple*	ndbsearch(Ndb*, Ndbs*, byte*, byte*);
Ndbtuple*	ndbsnext(Ndbs*, byte*, byte*);
Ndbtuple*	ndbgetval(Ndb*, Ndbs*, byte*, byte*, byte*, byte*);
byte*		ipattr(byte*);
int		ipinfo(Ndb*, byte*, byte*, byte*, Ipinfo*);
int		eipconv(Printspec*);
uint		parseip(byte*, byte*);
int		parseether(byte*, byte*);
int		myipaddr(byte*, byte*);
int		myetheraddr(byte*, byte*);
void		maskip(byte*, byte*, byte*);
int		equivip(byte*, byte*);
uint		nhgetl(byte*);
usint		nhgets(byte*);
void		hnputl(byte*, uint);
void		hnputs(byte*, usint);
