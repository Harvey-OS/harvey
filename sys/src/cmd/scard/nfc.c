#include <u.h>
#include <libc.h>
#include "icc.h"
#include "nfc.h"

char *selstr[] = {
	[MifUlite] = "Mifare Ultralight",
	[Mif1k] = "Mifare 1k",
	[Mifmini] = "Mifare Mini",	
	[Mif4k] = "Mifare 4k",
	[MifDesfire] = "Mifare Desfire",
	[Jcop30]	= "Jcop30",
	[MifClass] = "Mifare classic",
	[GemPlus] = "GemPlus",
};

int
unpackatq(uchar *b, int bsz, Atq *a)
{
	uchar *s;

	s = b;
	if(bsz < 2){
		diprint(2, "nfc: bad atq len < 2\n");
		return -1;
	}
	b+= 2;
	a->ntagfnd = *b++;
	a->tagn = *b++;
	a->alen = *b++;
	if(bsz < b - s + a->alen){
		diprint(2, "nfc: bad atq len < 2\n");
		return -1;
	}
	b++;	/* no idea what this field is..., I suspect part of the length? */
	a->sensres = (b[0] << 8) + b[1];
	b += sizeof(a->sensres);
	a->selres = *b++;
	a->nuid = *b++;
	if(a->nuid > Maxuid){
		diprint(2, "nfc: UID len too big: %d\n", a->nuid);
		return -1;
	}
	a->uid = a->packuid;
	memmove(a->uid, b, a->nuid);
	return b - s;
}

enum{
	Maxatqstr = 256
};

char *
atq2str(Atq *a)
{
	char *s, *e, *st;
	int i;
	
	st = mallocz(Maxatqstr, 1);
	if(st == nil)
		return nil;
	s = st;
	e = s + Maxatqstr - 1;

	s = seprint(s, e, "#tag found: %d\n", a->ntagfnd);
	s = seprint(s, e, "tagn: %d\n", a->tagn);
	s = seprint(s, e, "sens res: %#4.4ux\n", a->sensres);
	s = seprint(s, e, "sel res: %#2.2ux\n", a->selres);
	if(a->selres > MaxCard)
		s = seprint(s, e, "	Unknown\n");
	s = seprint(s, e, "	%s\n", selstr[a->selres]);
		s = seprint(s, e, "UID: ");
	for(i = 0; i < a->nuid; i++)
		s = seprint(s, e, "%2.2ux ", a->uid[i]);
	seprint(s, e, "\n");

	return st;
}
