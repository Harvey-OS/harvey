#include <u.h>
#include <libc.h>
#include <bio.h>
#include <String.h>
#include <thread.h>
#include "wiki.h"

char *wikidir;

void
usage(void)
{
	fprint(2, "usage: wiki2html [-hoDP ] [-d dir] wikifile\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int t;
	int parse;
	String *h;
	Whist *doc;

	rfork(RFNAMEG);

	t = Tpage;
	ARGBEGIN{
	default:
		usage();
	case 'd':
		wikidir = EARGF(usage());
		break;
	case 'h':
		t = Thistory;
		break;
	case 'o':
		t = Toldpage;
		break;
	case 'D':
		t = Tdiff;
		break;
	case 'P':
		parse = 1;
	}ARGEND

	if(argc != 1)
		usage();

	if(t == Thistory || t==Tdiff)
		doc = gethistory(atoi(argv[0]));
	else
		doc = getcurrent(atoi(argv[0]));

	if(doc == nil)
		sysfatal("doc: %r");

	if(parse){
		printpage(doc->doc->wtxt);
		exits(0);
	}
	if((h = tohtml(doc, doc->doc+doc->ndoc-1, t)) == nil)
		sysfatal("wiki2html: %r");

	write(1, s_to_c(h), s_len(h));
	exits(0);
}
