/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct Cfsmsg {
	uint32_t	n;			/* number of messages (of some type) */
	int64_t	t;			/* time spent in these messages */
	int64_t	s;			/* start time of last call */
};

struct Cfsstat {
	struct Cfsmsg cm[128];		/* client messages */
	struct Cfsmsg sm[128];		/* server messages */

	uint32_t ndirread;			/* # of directory read ops */
	uint32_t ndelegateread;		/* # of read ops delegated */
	uint32_t ninsert;			/* # of cache insert ops */
	uint32_t ndelete;			/* # of cache delete ops */
	uint32_t nupdate;			/* # of cache update ops */

	uint64_t bytesread;		/* # of bytes read by client */
	uint64_t byteswritten;		/* # of bytes written by client */
	uint64_t bytesfromserver;		/* # of bytes read from server */
	uint64_t bytesfromdirs;		/* # of directory bytes read from server */
	uint64_t bytesfromcache;		/* # of bytes read from cache */
	uint64_t bytestocache;		/* # of bytes written to cache */
};

extern struct Cfsstat cfsstat, cfsprev;
extern int statson;
