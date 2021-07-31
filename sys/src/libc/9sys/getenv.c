#include <u.h>
#include <libc.h>

char*
getenv(char *name)
{
	int r, f;
	long s;
	char *ans;
	char *p, *ep, ename[200];

	sprint(ename, "/env/%s", name);
	f = open(ename, OREAD);
	if(f < 0)
		return 0;
	s = seek(f, 0, 2);
	ans = malloc(s+1);
	if(ans) {
		seek(f, 0, 0);
		r = read(f, ans, s);
		if(r >= 0) {
			ep = ans + s - 1;
			for(p = ans; p < ep; p++)
				if(*p == '\0')
					*p = ' ';
			ans[s] = '\0';
		}
	}
	close(f);
	return ans;
}
