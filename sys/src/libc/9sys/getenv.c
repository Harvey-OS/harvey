#include <u.h>
#include <libc.h>

char*
getenv(char *name)
{
	char *ans;
	int f;
	char ename[200];
	long s;

	sprint(ename, "/env/%s", name);
	f = open(ename, OREAD);
	if(f < 0)
		return 0;
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
