#include <lib9.h>
#include <a.out.h>
#include <sysexptab.h>
#include <dynld.h>

void
main(int argc, char **argv)
{
	void (*libmain)(int argc, char **argv);
	Dynobj* sc;

	ARGBEGIN{
	}ARGEND

	fd = open(argv[0], OREAD);
	if(fd < 0)
		sysfatal("couldn't open %s: %r", argv[0]);
	sc = dynloadfd(fd, sysexptab, nelem(sysexptab), 0);
	libmmain = dynimport(sc, "main", signof(main));
	if(libmmain == nil)
		sysfatal("couldn't get main symbol");
	libmain(argc, argv);
}

