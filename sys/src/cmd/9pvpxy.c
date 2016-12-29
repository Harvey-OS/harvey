/*
 * This file is part of the Harvey operating system.  It is subject to the
 * license terms of the GNU GPL v2 in LICENSE.gpl found in the top-level
 * directory of this distribution and at http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * No part of Harvey operating system, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms
 * contained in the LICENSE.gpl file.
 */

// 9pvpxy.c A 9p proxy over virtqueue. Read 9p messages from standard input, send them to the virtqueue.
// Read responses from the virtqueue, send them to the standard output.
// The operation is single-threaded: write to virtqueue is expected to complete before the response can be read.

#include <u.h>
#include <libc.h>
#include <fcall.h>

#define BUFLEN 256*1024

static uint8_t ibuf[BUFLEN], obuf[BUFLEN];

void
exerr(char *s)
{
	char err[200];
	err[0] = 0;
	errstr(err, 199);
	fprint(2, "%s: %s\n", s, err);
	exits(err);
}

void
exerr2(char *s, char *err)
{
	fprint(2, "%s: %s\n", s, err);
	exits(err);
}

void
main(int argc, char *argv[])
{
	if(argc == 0) {
		fprint(2, "usage: %s file\n", argv[0]);
		exits("usage");
	}
	int fd = open(argv[1], ORDWR);
	if(fd < 0)
		exerr(argv[0]);
	while(1) {
		int rc = read9pmsg(0, obuf, BUFLEN);
		if(rc < 0)
		{
			close(fd);
			exerr(argv[0]);
		}
		int rc2 = write(fd, obuf, rc);
		if(rc2 < rc)
		{
			close(fd);
			exerr2(argv[0], "short write to vq");
		}
		rc = read(fd, ibuf, BUFLEN);
		if(rc < BUFLEN)
		{
			close(fd);
			exerr(argv[0]);
		}
		uint32_t ml = GBIT32(ibuf);
		rc = write(1, ibuf, ml);
		if(rc < 0)
		{
			close(fd);
			exerr2(argv[0], "short write to stdout");
		}
	}
}
