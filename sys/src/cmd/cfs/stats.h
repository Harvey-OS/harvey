struct Cfsmsg {
	ulong	n;			/* number of messages (of some type) */
	vlong	t;			/* time spent in these messages */
	vlong	s;			/* start time of last call */
};

struct Cfsstat {
	struct Cfsmsg cm[128];	/* client messages */
	struct Cfsmsg sm[128];	/* server messages */

	ulong ndirread;			/* # of directory read ops */
	ulong ndelegateread;	/* # of read ops delegated */
	ulong ninsert;			/* # of cache insert ops */
	ulong ndelete;			/* # of cache delete ops */
	ulong nupdate;			/* # of cache update ops */

	ulong bytesread;		/* # of bytes read by client */
	ulong byteswritten;		/* # of bytes written by client */
	ulong bytesfromserver;	/* # of bytes read from server */
	ulong bytesfromdirs;	/* # of directory bytes read from server */
	ulong bytesfromcache;	/* # of bytes read from cache */
	ulong bytestocache;		/* # of bytes written to cache */
};

extern struct Cfsstat cfsstat, cfsprev;
extern int statson;