/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 * tar archive manipulation functions
 */

#include <u.h>
#include <libc.h>
#include <ctype.h>
#include "tar.h"

enum {
	Blocksxfr = 32,
};

/* exports */
char *thisnm, *lastnm;

/* private data */
static u64 outoff = 0;		/* maintained by newarch, writetar */

unsigned
checksum(Hblock *hp)
{
	int i;
	u8 *cp, *csum, *end;

	i = ' ' * sizeof hp->header.chksum;	/* pretend blank chksum field */
	csum = (u8 *)hp->header.chksum;
	end = &hp->dummy[Tblock];
	/*
	 * Unixware gets this wrong; it adds *signed* chars.
	 *	i += (Uflag? *(schar *)cp: *cp);
	 */
	for (cp = hp->dummy; cp < csum; )
		i += *cp++;
	/* skip checksum field */
	for (cp += sizeof hp->header.chksum; cp < end; )
		i += *cp++;
	return i;
}

void
readtar(int in, char *buffer, i32 size)
{
	int i;
	unsigned bytes;

	bytes = i = readn(in, buffer, size);
	if (i <= 0)
		sysfatal("archive read error: %r");
	if (bytes % Tblock != 0)
		sysfatal("archive blocksize error");
	if (bytes != size) {
		/*
		 * buffering would be screwed up by only partially
		 * filling tbuf, yet this might be the last (short)
		 * record in a tar disk archive, so just zero the rest.
		 */
		fprint(2, "%s: warning: short archive block\n", argv0);
		memset(buffer + bytes, '\0', size - bytes);
	}
}

void
newarch(void)
{
	outoff = 0;
}

u64
writetar(int outf, char *buffer, u32 size)
{
	if (write(outf, buffer, size) < size) {
		fprint(2, "%s: archive write error: %r\n", argv0);
		fprint(2, "%s: archive seek offset: %llu\n", argv0, outoff);
		exits("write");
	}
	outoff += size;
	return outoff;
}

u32
otoi(char *s)
{
	int c;
	u32 ul = 0;

	while (isascii(*s) && isspace(*s))
		s++;
	while ((c = *s++) >= '0' && c <= '7') {
		ul <<= 3;
		ul |= c - '0';
	}
	return ul;
}

int
getdir(Hblock *hp, int in, i64 *lenp)
{
	*lenp = 0;
	readtar(in, (char*)hp, Tblock);
	if (hp->header.name[0] == '\0') { /* zero block indicates end-of-archive */
		lastnm = strdup(thisnm);
		return 0;
	}
	*lenp = otoi(hp->header.size);
	if (otoi(hp->header.chksum) != checksum(hp))
		sysfatal("directory checksum error");
	if (lastnm != nil)
		free(lastnm);
	lastnm = thisnm;
	thisnm = strdup(hp->header.name);
	return 1;
}

u64
passtar(Hblock *hp, int in, int outf, i64 len)
{
	u32 bytes;
	i64 off;
	u64 blks;
	char bigbuf[Blocksxfr*Tblock];		/* 2*(8192 == MAXFDATA) */

	off = outoff;
	if (islink(hp->header.linkflag))
		return off;
	for (blks = TAPEBLKS((u64)len); blks >= Blocksxfr;
	    blks -= Blocksxfr) {
		readtar(in, bigbuf, sizeof bigbuf);
		off = writetar(outf, bigbuf, sizeof bigbuf);
	}
	if (blks > 0) {
		bytes = blks*Tblock;
		readtar(in, bigbuf, bytes);
		off = writetar(outf, bigbuf, bytes);
	}
	return off;
}

void
putempty(int out)
{
	static char buf[Tblock];

	writetar(out, buf, sizeof buf);
}

/* emit zero blocks at end */
int
closeout(int outf, char *c, int prflag)
{
	if (outf < 0)
		return -1;
	putempty(outf);
	putempty(outf);
	if (lastnm && prflag)
		fprint(2, " %s\n", lastnm);
	close(outf);		/* guaranteed to succeed on plan 9 */
	return -1;
}
