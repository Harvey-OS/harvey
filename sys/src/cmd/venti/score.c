#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include "libsec.h"

u8int zeroScore[VtScoreSize];

void
scoreMem(u8int *score, u8int *buf, int n)
{
	sha1(buf, n, score, nil);
}

static int
hexv(int c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

int
strScore(char *s, u8int *score)
{
	int i, c, d;

	for(i = 0; i < VtScoreSize; i++){
		c = hexv(s[2 * i]);
		if(c < 0)
			return 0;
		d = hexv(s[2 * i + 1]);
		if(d < 0)
			return 0;
		score[i] = (c << 4) + d;
	}
	return s[2 * i] == '\0';
}
