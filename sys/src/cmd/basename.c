#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	char *pr;
	int n;

	if(argc < 2){
		fprint(2, "usage: basename string [suffix]\n");
		exits("usage");
	}
	pr = utfrrune(argv[1], '/');
	if(pr)
		pr++;
	else
		pr = argv[1];
	if(argc==3){
		n = strlen(pr)-strlen(argv[2]);
		if(n >= 0 && !strcmp(pr+n, argv[2]))
			pr[n] = 0;
	}
	print("%s\n", pr);
	exits(0);
}
