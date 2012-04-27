#include <u.h>
#include <libc.h>
#include <oventi.h>

int
vtParseScore(char *buf, uint n, uchar score[VtScoreSize])
{
	int i, c;

	memset(score, 0, VtScoreSize);

	if(n != VtScoreSize*2)
		return 0;
	for(i=0; i<VtScoreSize*2; i++){
		if(buf[i] >= '0' && buf[i] <= '9')
			c = buf[i] - '0';
		else if(buf[i] >= 'a' && buf[i] <= 'f')
			c = buf[i] - 'a' + 10;
		else if(buf[i] >= 'A' && buf[i] <= 'F')
			c = buf[i] - 'A' + 10;
		else
			return 0;

		if((i & 1) == 0)
			c <<= 4;
	
		score[i>>1] |= c;
	}
	return 1;
}

