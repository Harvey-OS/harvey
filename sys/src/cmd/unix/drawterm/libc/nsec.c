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

static uint64_t order = (uint64_t)0x0001020304050607ULL;

static void
be2vlong(int64_t *to, uint8_t *f)
{
	uint8_t *t, *o;
	int i;

	t = (uint8_t *)to;
	o = (uint8_t *)&order;
	for(i = 0; i < 8; i++)
		t[o[i]] = f[i];
}

/*
 *  After a fork with fd's copied, both fd's are pointing to
 *  the same Chan structure.  Since the offset is kept in the Chan
 *  structure, the seek's and read's in the two processes can
 *  compete at moving the offset around.  Hence the retry loop.
 *
 *  Since the bintime version doesn't need a seek, it doesn't
 *  have the loop.
 */
int64_t
nsec(void)
{
	char b[12 + 1];
	static int f = -1;
	static int usebintime;
	int retries;
	int64_t t;

	if(f < 0) {
		usebintime = 1;
		f = open("/dev/bintime", OREAD | OCEXEC);
		if(f < 0) {
			usebintime = 0;
			f = open("/dev/nsec", OREAD | OCEXEC);
			if(f < 0)
				return 0;
		}
	}

	if(usebintime) {
		if(read(f, b, sizeof(uint64_t)) < 0)
			goto error;
		be2vlong(&t, (uint8_t *)b);
		return t;
	} else {
		for(retries = 0; retries < 100; retries++) {
			if(seek(f, 0, 0) >= 0 && read(f, b, sizeof(b) - 1) >= 0) {
				b[sizeof(b) - 1] = 0;
				return strtoll(b, 0, 0);
			}
		}
	}

error:
	close(f);
	f = -1;
	return 0;
}
