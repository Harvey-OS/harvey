#include <u.h>
#include <libc.h>

char*
getenv(char *name)
{
	int r, f;
	long s;
	char *ans;
	char *p, *ep, ename[100];

	if(strchr(name, '/') != nil)
		return nil;
	snprint(ename, sizeof ename, "/env/%s", name);
	if(strcmp(ename+5, name) != 0)
		return nil;
	f = open(ename, OREAD);
	if(f < 0)
		return 0;
	s = seek(f, 0, 2);
	ans = malloc(s+1);
	if(ans) {
		setmalloctag(ans, getcallerpc(&name));
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
