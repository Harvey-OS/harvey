#include <u.h>
#include <libc.h>

void	error(char *f, char *s);
void	cat(int f, char *s);

void
main(int argc, char *argv[])
{
	int f, i;

	if(argc == 1)
		cat(0, "<stdin>");
	else for(i=1; i<argc; i++){
		f = open(argv[i], OREAD);
		if(f < 0)
			error("cat: can't open %s: ", argv[i]);
		else{
			cat(f, argv[i]);
			close(f);
		}
	}
	exits(0);
}

void
cat(int f, char *s)
{
	char buf[8192];
	long n;

	while((n=read(f, buf, (long)sizeof buf))>0)
		if(write(1, buf, n)!=n){
			error("cat: write error copying %s: ", s);
			return;
		}
	if(n < 0)
		error("cat: error reading %s: ", s);
}

void
error(char *f, char *s)
{
	char buf[ERRLEN];

	errstr(buf);
	fprint(2, f, s);
	fprint(2, "%s\n", buf);
	exits("error");
}
