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
#include <mp.h>
#include <libsec.h>
#include "SConn.h"

static int32_t
ls(int8_t *p, Dir **dirbuf)
{
	int fd;
	int32_t n;
	Dir *db;

	if((db = dirstat(p)) == nil ||
		!(db->qid.type & QTDIR) ||
		(fd = open(p, OREAD)) < 0 )
		return -1;
	free(db);
	n = dirreadall(fd, dirbuf);
	close(fd);
	return n;
}

static uint8_t*
sha1file(int8_t *pfx, int8_t *nm)
{
	int n, fd, len;
	int8_t *tmp;
	uint8_t buf[8192];
	static uint8_t digest[SHA1dlen];
	DigestState *s;

	len = strlen(pfx)+1+strlen(nm)+1;
	tmp = emalloc(len);
	snprint(tmp, len, "%s/%s", pfx, nm);
	if((fd = open(tmp, OREAD)) < 0){
		free(tmp);
		return nil;
	}
	free(tmp);
	s = nil;
	while((n = read(fd, buf, sizeof buf)) > 0)
		s = sha1(buf, n, nil, s);
	close(fd);
	sha1(nil, 0, digest, s);
	return digest;
}

static int
compare(Dir *a, Dir *b)
{
	return strcmp(a->name, b->name);
}

/* list the (name mtime size sum) of regular, readable files in path */
int8_t *
dirls(int8_t *path)
{
	int8_t *list, *date, dig[30], buf[128];
	int m, nmwid, lenwid;
	int32_t i, n, ndir, len;
	Dir *dirbuf;

	if(path==nil || (ndir = ls(path, &dirbuf)) < 0)
		return nil;

	qsort(dirbuf, ndir, sizeof dirbuf[0], (int (*)(void *, void *))compare);
	for(nmwid=lenwid=i=0; i<ndir; i++){
		if((m = strlen(dirbuf[i].name)) > nmwid)
			nmwid = m;
		snprint(buf, sizeof(buf), "%ulld", dirbuf[i].length);
		if((m = strlen(buf)) > lenwid)
			lenwid = m;
	}
	for(list=nil, len=0, i=0; i<ndir; i++){
		date = ctime(dirbuf[i].mtime);
		date[28] = 0;  // trim newline
		n = snprint(buf, sizeof buf, "%*ulld %s", lenwid, dirbuf[i].length, date+4);
		n += enc64(dig, sizeof dig, sha1file(path, dirbuf[i].name), SHA1dlen);
		n += nmwid+3+strlen(dirbuf[i].name);
		list = erealloc(list, len+n+1);
		len += snprint(list+len, n+1, "%-*s\t%s %s\n", nmwid, dirbuf[i].name, buf, dig);
	}
	free(dirbuf);
	return list;
}

