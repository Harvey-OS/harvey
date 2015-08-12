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
#include <authsrv.h>
#include <bio.h>
#include "authcmdlib.h"

int
readfile(char *file, char *buf, int n)
{
	int fd;

	fd = open(file, OREAD);
	if(fd < 0) {
		werrstr("%s: %r", file);
		return -1;
	}
	n = read(fd, buf, n);
	close(fd);
	return n;
}

int
writefile(char *file, char *buf, int n)
{
	int fd;

	fd = open(file, OWRITE);
	if(fd < 0)
		return -1;
	n = write(fd, buf, n);
	close(fd);
	return n;
}

char *
findkey(char *db, char *user, char *key)
{
	int n;
	char filename[Maxpath];

	snprint(filename, sizeof filename, "%s/%s/key", db, user);
	n = readfile(filename, key, DESKEYLEN);
	if(n != DESKEYLEN)
		return 0;
	else
		return key;
}

char *
findsecret(char *db, char *user, char *secret)
{
	int n;
	char filename[Maxpath];

	snprint(filename, sizeof filename, "%s/%s/secret", db, user);
	n = readfile(filename, secret, SECRETLEN - 1);
	secret[n] = 0;
	if(n <= 0)
		return 0;
	else
		return secret;
}

char *
setkey(char *db, char *user, char *key)
{
	int n;
	char filename[Maxpath];

	snprint(filename, sizeof filename, "%s/%s/key", db, user);
	n = writefile(filename, key, DESKEYLEN);
	if(n != DESKEYLEN)
		return 0;
	else
		return key;
}

char *
setsecret(char *db, char *user, char *secret)
{
	int n;
	char filename[Maxpath];

	snprint(filename, sizeof filename, "%s/%s/secret", db, user);
	n = writefile(filename, secret, strlen(secret));
	if(n != strlen(secret))
		return 0;
	else
		return secret;
}
