#include <u.h>
#include <libc.h>
#include <draw.h>
#include <plumb.h>
#include <regexp.h>
#include <bio.h>
#include "faces.h"

void
main(int argc, char **argv)
{
	Face f;
	char *q;

	if(argc != 3){
		fprint(2, "usage: dblook name domain\n");
		exits("usage");
	}

	q = findfile(&f, argv[2], argv[1]);
	print("%s\n", q);
}

void
killall(char *s)
{
	USED(s);
}
