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
static uint64_t outoff = 0;		/* maintained by newarch, writetar */

unsigned
checksum(Hblock *hp)
{
	int i;
	uint8_t *cp, *csum, *end;

	i = ' ' * sizeof hp->chksum;	/* pretend blank chksum field */
	csum = (uint8_t *)hp->chksum;
	end = &hp->dummy[Tblock];
	/*
	 * Unixware gets this wrong; it adds *signed* chars.
	 *	i += (Uflag? *(schar *)cp: *cp);
	 */
	for (cp = hp->dummy; cp < csum; )
		i += *cp++;
	/* skip checksum field */
	for (cp += sizeof hp->chksum; cp < end; )
		i += *cp++;
	return i;
}

void
readtar(int in, char *buffer, int32_t size)
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

uint64_t
writetar(int outf, char *buffer, uint32_t size)
{
	if (write(outf, buffer, size) < size) {
		fprint(2, "%s: archive write error: %r\n", argv0);
		fprint(2, "%s: archive seek offset: %llu\n", argv0, outoff);
		exits("write");
	}
	outoff += size;
	return outoff;
}

uint32_t
otoi(char *s)
{
	int c;
	uint32_t ul = 0;

	while (isascii(*s) && isspace(*s))
		s++;
	while ((c = *s++) >= '0' && c <= '7') {
		ul <<= 3;
		ul |= c - '0';
	}
	return ul;
}

int
getdir(Hblock *hp, int in, int64_t *lenp)
{
	*lenp = 0;
	readtar(in, (char*)hp, Tblock);
	if (hp->name[0] == '\0') { /* zero block indicates end-of-archive */
		lastnm = strdup(thisnm);
		return 0;
	}
	*lenp = otoi(hp->size);
	if (otoi(hp->chksum) != checksum(hp))
		sysfatal("directory checksum error");
	if (lastnm != nil)
		free(lastnm);
	lastnm = thisnm;
	thisnm = strdup(hp->name);
	return 1;
}

uint64_t
passtar(Hblock *hp, int in, int outf, int64_t len)
{
	uint32_t bytes;
	int64_t off;
	uint64_t blks;
	char bigbuf[Blocksxfr*Tblock];		/* 2*(8192 == MAXFDATA) */

	off = outoff;
	if (islink(hp->linkflag))
		return off;
	for (blks = TAPEBLKS((uint64_t)len); blks >= Blocksxfr;
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
closeout(int outf, char *, int prflag)
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
