#include <u.h>
#include <libc.h>
#include <bio.h>
#include "icc.h"
#include "nfc.h"
#include "reader.h"
#include "tlv.h"
#include "file.h"
#include "atr.h"

int iccdebug = 1;
uchar atr[128];
int natr;
ParsedAtr pa;
ParsedHist ph;

void
main(int , char *[])
{
	Biobuf *in;
	char *l, *p;
	int n, np;
	ParsedAtr pa;
	
	in = Bopen("/fd/0", OREAD);
	if(in == 0)
		sysfatal("open /fd/0: %r\n");
	
	l = Brdline(in, '\n');
	if(l == 0)
		sysfatal("no atr\n");
	
	n = Blinelen(in);
	if(n == 0)
		sysfatal("no atr\n");
		
	l[n-1] = 0;
	for(p = l; *p; p++){
		if(*p == ' ')
			continue;
		atr[natr++] = strtol(p, &p, 16);
	}
	np = parseatr(&pa, atr, natr);
	print("np is: %d\n", np);
	np = parsehist(&ph, pa.hist, pa.nhist);
	print("np is: %d\n", np);
}
