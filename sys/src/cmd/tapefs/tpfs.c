#include <u.h>
#include <libc.h>
#include "tapefs.h"

unsigned char tape[300000];
int	tapefile;

struct tp {
	unsigned char	name[32];
	unsigned char	mode[2];
	unsigned char	uid[1];
	unsigned char	gid[1];
	unsigned char	unused[1];
	unsigned char	size[3];
	unsigned char	tmod[4];
	unsigned char	taddress[2];
	unsigned char	unused[16];
	unsigned char	checksum[2];
} dir[192];

void
populate(char *name)
{
	int i, isabs;
	struct tp *tpp;
	Fileinf f;

	replete = 1;
	tapefile = open(name, OREAD);
	if (tapefile<0)
		error("Can't open argument file");
	read(tapefile, tape, sizeof tape);
	tpp = (struct tp *)(tape+512);
	for (i=0;  i<192; i++, tpp++) {
		if (tpp->name[0]=='\0')
			continue;
		f.addr = (void *)(tpp->taddress[0] + (tpp->taddress[1]<<8));
		f.size = (tpp->size[0]<<16) + (tpp->size[1]<<0) + (tpp->size[2]<<8);
		f.mdate = (tpp->tmod[2]<<0) + (tpp->tmod[3]<<8)
		     +(tpp->tmod[0]<<16) + (tpp->tmod[1]<<24);
		f.mode = tpp->mode[0]&0777;
		isabs = tpp->name[0]=='/';
		f.name = (char *)tpp->name+isabs;
		poppath(f, 1);
	}
}

void
popdir(Ram *r)
{
	USED(r);
}

void
dotrunc(Ram *r)
{
	USED(r);
}

void
docreate(Ram *r)
{
	USED(r);
}

char *
doread(Ram *r, long off, long cnt)
{
	USED(cnt);
	return (char *)r->data+off;
}

void
dowrite(Ram *r, char *buf, long off, long cnt)
{
	USED(r); USED(buf); USED(off); USED(cnt);
}

int
dopermw(Ram *r)
{
	USED(r);
	return 0;
}
