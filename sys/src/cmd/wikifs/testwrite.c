#include <u.h>
#include <libc.h>
#include <bio.h>
#include <String.h>
#include <thread.h>
#include "wiki.h"

char *wikidir = ".";

void
usage(void)
{
	fprint(2, "usage: testwrite [-d dir] wikifile n\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	ulong t;
	int i;
	Biobuf *b;
	String *h;
	Whist *doc;
	char tmp[20];

	t = 0;
	ARGBEGIN{
	case 't':
		t = strtoul(EARGF(usage()), 0, 0);
		break;
	default:
		usage();
	}ARGEND

	if(argc != 2)
		usage();

	if((b = Bopen(argv[0], OREAD)) == nil)
		sysfatal("Bopen: %r");

	if((doc = Brdwhist(b)) == nil)
		sysfatal("Brdwtxt: %r");

	sprint(tmp, "D%lud\n", time(0));
	if((h = pagetext(s_copy(tmp), (doc->doc+doc->ndoc-1)->wtxt, 1))==nil)
		sysfatal("wiki2text: %r");

	if(writepage(atoi(argv[1]), t, h, doc->title) <0)
		sysfatal("writepage: %r");
	exits(0);
}
