#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

int vflag, Vflag;

void
error(char* format, ...)
{
	char buf[512];
	int n;

	sequencer(0, 1);
	n = sprint(buf, "%s: ", argv0);
	doprint(buf+n, buf+sizeof(buf)-n, format, (&format+1));
	if(vflag)
		Bprint(&stdout, buf+n);
	Bflush(&stdout);
	fprint(2, buf);
	exits("error");
}

void
trace(char* format, ...)
{
	char buf[512];

	if(vflag || Vflag){
		if(curprintindex){
			curprintindex = 0;
			Bprint(&stdout, "\n");
		}
		doprint(buf, buf+sizeof(buf), format, (&format+1));
		Bprint(&stdout, buf);
		if(Vflag)
			print(buf);
	}
}
