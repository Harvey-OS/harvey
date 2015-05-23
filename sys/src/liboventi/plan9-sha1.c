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
#include <oventi.h>
#include <libsec.h>

static void encode(uint8_t*, uint32_t*, uint32_t);
extern void vtSha1Block(uint32_t *s, uint8_t *p, uint32_t len);

struct VtSha1
{
	DigestState *s;
};

VtSha1 *
vtSha1Alloc(void)
{
	VtSha1 *s;

	s = vtMemAlloc(sizeof(VtSha1));
	vtSha1Init(s);
	return s;
}

void
vtSha1Free(VtSha1 *s)
{
	if(s == nil)
		return;
	if(s->s != nil)
		free(s->s);
	vtMemFree(s);
}

void
vtSha1Init(VtSha1 *s)
{
	s->s = nil;
}

void
vtSha1Update(VtSha1 *s, uint8_t *p, int len)
{
	s->s = sha1(p, len, nil, s->s);
}

void
vtSha1Final(VtSha1 *s, uint8_t *digest)
{
	sha1(nil, 0, digest, s->s);
	s->s = nil;
}

void
vtSha1(uint8_t sha1[VtScoreSize], uint8_t *p, int n)
{
	VtSha1 s;

	vtSha1Init(&s);
	vtSha1Update(&s, p, n);
	vtSha1Final(&s, sha1);
}

int
vtSha1Check(uint8_t score[VtScoreSize], uint8_t *p, int n)
{
	VtSha1 s;
	uint8_t score2[VtScoreSize];

	vtSha1Init(&s);
	vtSha1Update(&s, p, n);
	vtSha1Final(&s, score2);

	if(memcmp(score, score2, VtScoreSize) != 0) {
		vtSetError("vtSha1Check failed");
		return 0;
	}
	return 1;
}
