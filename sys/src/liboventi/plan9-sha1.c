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
#include <mp.h>
#include <libsec.h>

extern void vtSha1Block(u32 *s, u8 *p, u32 len);

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
vtSha1Update(VtSha1 *s, u8 *p, int len)
{
	s->s = sha1(p, len, nil, s->s);
}

void
vtSha1Final(VtSha1 *s, u8 digest[VtScoreSize])
{
	sha1(nil, 0, digest, s->s);
	s->s = nil;
}

void
vtSha1(u8 sha1[VtScoreSize], u8 *p, int n)
{
	VtSha1 s;

	vtSha1Init(&s);
	vtSha1Update(&s, p, n);
	vtSha1Final(&s, sha1);
}

int
vtSha1Check(u8 score[VtScoreSize], u8 *p, int n)
{
	VtSha1 s;
	u8 score2[VtScoreSize];

	vtSha1Init(&s);
	vtSha1Update(&s, p, n);
	vtSha1Final(&s, score2);

	if(memcmp(score, score2, VtScoreSize) != 0) {
		vtSetError("vtSha1Check failed");
		return 0;
	}
	return 1;
}
