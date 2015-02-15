/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ndb.h>

extern char* secureidcheck(char *user, char *response);

Ndb *db;

void
main(int argc, char **argv)
{
	Ndb *db2;

	if(argc!=2){
		fprint(2, "usage: %s pinsecurid\n", argv[0]);
		exits("usage");
	}

	db = ndbopen("/lib/ndb/auth");
	if(db == 0)
		syslog(0, "secstore", "no /lib/ndb/auth");
	db2 = ndbopen(0);
	if(db2 == 0)
		syslog(0, "secstore", "no /lib/ndb/local");
	db = ndbcat(db, db2);

	print("user=%s\n", getenv("user"));
	print("%s\n", secureidcheck(getenv("user"), argv[1]));
	exits(0);
}
