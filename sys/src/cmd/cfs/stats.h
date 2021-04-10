/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct Cfsmsg {
	u32	n;			/* number of messages (of some type) */
	i64	t;			/* time spent in these messages */
	i64	s;			/* start time of last call */
};

struct Cfsstat {
	struct Cfsmsg cm[128];		/* client messages */
	struct Cfsmsg sm[128];		/* server messages */

	u32 ndirread;			/* # of directory read ops */
	u32 ndelegateread;		/* # of read ops delegated */
	u32 ninsert;			/* # of cache insert ops */
	u32 ndelete;			/* # of cache delete ops */
	u32 nupdate;			/* # of cache update ops */

	u64 bytesread;		/* # of bytes read by client */
	u64 byteswritten;		/* # of bytes written by client */
	u64 bytesfromserver;		/* # of bytes read from server */
	u64 bytesfromdirs;		/* # of directory bytes read from server */
	u64 bytesfromcache;		/* # of bytes read from cache */
	u64 bytestocache;		/* # of bytes written to cache */
};

extern struct Cfsstat cfsstat, cfsprev;
extern int statson;
