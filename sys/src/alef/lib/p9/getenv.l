#include <alef.h>

byte*
getenv(byte *name)
{
	int f, s;
	byte *ans;
	byte ename[200];

	sprint(ename, "/env/%s", name);
	f = open(ename, OREAD);
	if(f < 0)
		return nil;
	s = seek(f, 0, 2);
	ans = malloc(s+1);
	if(ans) {
		seek(f, 0, 0);
		read(f, ans, s);
		ans[s] = 0;
	}
	close(f);
	return ans;
}
