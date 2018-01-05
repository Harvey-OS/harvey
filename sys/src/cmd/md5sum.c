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
#include <libsec.h>

#pragma	varargck	type	"M"	unsigned char*

static int
digestfmt(Fmt *fmt)
{
	char buf[MD5dlen*2+1];
	uint8_t *p;
	int i;

	p = va_arg(fmt->args, uint8_t*);
	for(i=0; i<MD5dlen; i++)
		sprint(buf+2*i, "%.2x", p[i]);
	return fmtstrcpy(fmt, buf);
}

static void
sum(int fd, char *name)
{
	int n;
	uint8_t buf[8192], digest[MD5dlen];
	DigestState *s;

	s = md5(nil, 0, nil, nil);
	while((n = read(fd, buf, sizeof buf)) > 0)
		md5(buf, n, nil, s);
	if(n < 0){
		fprint(2, "reading %s: %r\n", name ? name : "stdin");
		return;
	}
	md5(nil, 0, digest, s);
	if(name == nil)
		print("%M\n", digest);
	else
		print("%M\t%s\n", digest, name);
}

void
main(int argc, char *argv[])
{
	int i, fd;

	ARGBEGIN{
	default:
		fprint(2, "usage: md5sum [file...]\n");
		exits("usage");
	}ARGEND

	fmtinstall('M', digestfmt);

	if(argc == 0)
		sum(0, nil);
	else for(i = 0; i < argc; i++){
		fd = open(argv[i], OREAD);
		if(fd < 0){
			fprint(2, "md5sum: can't open %s: %r\n", argv[i]);
			continue;
		}
		sum(fd, argv[i]);
		close(fd);
	}
	exits(nil);
}
