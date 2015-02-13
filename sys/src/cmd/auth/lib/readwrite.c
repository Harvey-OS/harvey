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
readfile(int8_t *file, int8_t *buf, int n)
{
	int fd;

	fd = open(file, OREAD);
	if(fd < 0){
		werrstr("%s: %r", file);
		return -1;
	}
	n = read(fd, buf, n);
	close(fd);
	return n;
}

int
writefile(int8_t *file, int8_t *buf, int n)
{
	int fd;

	fd = open(file, OWRITE);
	if(fd < 0)
		return -1;
	n = write(fd, buf, n);
	close(fd);
	return n;
}

int8_t*
findkey(int8_t *db, int8_t *user, int8_t *key)
{
	int n;
	int8_t filename[Maxpath];

	snprint(filename, sizeof filename, "%s/%s/key", db, user);
	n = readfile(filename, key, DESKEYLEN);
	if(n != DESKEYLEN)
		return 0;
	else
		return key;
}

int8_t*
findsecret(int8_t *db, int8_t *user, int8_t *secret)
{
	int n;
	int8_t filename[Maxpath];

	snprint(filename, sizeof filename, "%s/%s/secret", db, user);
	n = readfile(filename, secret, SECRETLEN-1);
	secret[n]=0;
	if(n <= 0)
		return 0;
	else
		return secret;
}

int8_t*
setkey(int8_t *db, int8_t *user, int8_t *key)
{
	int n;
	int8_t filename[Maxpath];

	snprint(filename, sizeof filename, "%s/%s/key", db, user);
	n = writefile(filename, key, DESKEYLEN);
	if(n != DESKEYLEN)
		return 0;
	else
		return key;
}

int8_t*
setsecret(int8_t *db, int8_t *user, int8_t *secret)
{
	int n;
	int8_t filename[Maxpath];

	snprint(filename, sizeof filename, "%s/%s/secret", db, user);
	n = writefile(filename, secret, strlen(secret));
	if(n != strlen(secret))
		return 0;
	else
		return secret;
}
