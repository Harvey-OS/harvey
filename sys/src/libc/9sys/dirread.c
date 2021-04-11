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
#include <fcall.h>

static
i32
dirpackage(u8 *buf, i32 ts, Dir **d)
{
	char *s;
	i32 ss, i, n, nn, m;

	*d = nil;
	if(ts <= 0)
		return 0;

	/*
	 * first find number of all stats, check they look like stats, & size all associated strings
	 */
	ss = 0;
	n = 0;
	for(i = 0; i < ts; i += m){
		m = BIT16SZ + GBIT16(&buf[i]);
		if(statcheck(&buf[i], m) < 0)
			break;
		ss += m;
		n++;
	}

	if(i != ts)
		return -1;

	*d = malloc(n * sizeof(Dir) + ss);
	if(*d == nil)
		return -1;

	/*
	 * then convert all buffers
	 */
	s = (char*)*d + n * sizeof(Dir);
	nn = 0;
	for(i = 0; i < ts; i += m){
		m = BIT16SZ + GBIT16((u8*)&buf[i]);
		if(nn >= n || convM2D(&buf[i], m, *d + nn, s) != m){
			free(*d);
			*d = nil;
			return -1;
		}
		nn++;
		s += m;
	}

	return nn;
}

i32
dirread(int fd, Dir **d)
{
	u8 *buf;
	i32 ts;

	buf = malloc(DIRMAX);
	if(buf == nil)
		return -1;
	ts = read(fd, buf, DIRMAX);
	if(ts >= 0)
		ts = dirpackage(buf, ts, d);
	free(buf);
	return ts;
}

i32
dirreadall(int fd, Dir **d)
{
	u8 *buf, *nbuf;
	i32 n, ts;

	buf = nil;
	ts = 0;
	for(;;){
		nbuf = realloc(buf, ts+DIRMAX);
		if(nbuf == nil){
			free(buf);
			return -1;
		}
		buf = nbuf;
		n = read(fd, buf+ts, DIRMAX);
		if(n <= 0)
			break;
		ts += n;
	}
	if(ts >= 0)
		ts = dirpackage(buf, ts, d);
	free(buf);
	if(ts == 0 && n < 0)
		return -1;
	return ts;
}
