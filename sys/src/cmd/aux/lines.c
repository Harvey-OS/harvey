#include <u.h>
#include <libc.h>
#include <bio.h>

void
cat(int f, char *s)
{
	Biobuf b;
	char *p;

	Binit(&b, f, OREAD);
	while((p = Brdline(&b, '\n')) != nil)
		if(write(1, p, Blinelen(&b)) != Blinelen(&b))
			sysfatal("write error copying %s: %r", s);
	Bterm(&b);
}

void
main(int argc, char *argv[])
{
	int f, i;

	argv0 = "lines";
	if(argc == 1)
		cat(0, "<stdin>");
	else for(i=1; i<argc; i++){
		f = open(argv[i], OREAD);
		if(f < 0)
			sysfatal("can't open %s: %r", argv[i]);
		else{
			cat(f, argv[i]);
			close(f);
		}
	}
	exits(0);
}

