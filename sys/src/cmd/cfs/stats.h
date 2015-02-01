/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

struct Cfsmsg {
	ulong	n;			/* number of messages (of some type) */
	vlong	t;			/* time spent in these messages */
	vlong	s;			/* start time of last call */
};

struct Cfsstat {
	struct Cfsmsg cm[128];		/* client messages */
	struct Cfsmsg sm[128];		/* server messages */

	ulong ndirread;			/* # of directory read ops */
	ulong ndelegateread;		/* # of read ops delegated */
	ulong ninsert;			/* # of cache insert ops */
	ulong ndelete;			/* # of cache delete ops */
	ulong nupdate;			/* # of cache update ops */

	uvlong bytesread;		/* # of bytes read by client */
	uvlong byteswritten;		/* # of bytes written by client */
	uvlong bytesfromserver;		/* # of bytes read from server */
	uvlong bytesfromdirs;		/* # of directory bytes read from server */
	uvlong bytesfromcache;		/* # of bytes read from cache */
	uvlong bytestocache;		/* # of bytes written to cache */
};

extern struct Cfsstat cfsstat, cfsprev;
extern int statson;
