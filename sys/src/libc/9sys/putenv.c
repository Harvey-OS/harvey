#include <u.h>
#include <libc.h>

int
putenv(char *name, char *val)
{
	int f;
	char ename[100];
	long s;

	if(strchr(name, '/') != nil)
		return -1;
	snprint(ename, sizeof ename, "/env/%s", name);
	if(strcmp(ename+5, name) != 0)
		return -1;
	f = create(ename, OWRITE, 0664);
	if(f < 0)
		return -1;
	s = strlen(val);
	if(write(f, val, s) != s){
		close(f);
		return -1;
	}
	close(f);
	return 0;
}
