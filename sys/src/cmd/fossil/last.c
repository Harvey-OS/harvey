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

void
usage(void)
{
	fprint(2, "usage: fossil/last disk\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int fd, bs, addr;
	char buf[20];

	ARGBEGIN{
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();

	if((fd = open(argv[0], OREAD)) < 0)
		sysfatal("open %s: %r", argv[0]);

	werrstr("end of file");
	if(seek(fd, 131072, 0) < 0 || readn(fd, buf, 20) != 20)
		sysfatal("error reading %s: %r", argv[0]);
	fmtinstall('H', encodefmt);
	if(memcmp(buf, "\x37\x76\xAE\x89", 4) != 0)
		sysfatal("bad magic %.4H != 3776AE89", buf);
	bs = buf[7]|(buf[6]<<8);
	addr = (buf[8]<<24)|(buf[9]<<16)|(buf[10]<<8)|buf[11];
	if(seek(fd, (vlong)bs*addr+34, 0) < 0 || readn(fd, buf, 20) != 20)
		sysfatal("error reading %s: %r", argv[0]);
	print("vac:%.20lH\n", buf);
	exits(0);
}
