/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include "tapefs.h"

/*
 * File system for old tap tapes.
 */

struct tap {
	unsigned char	name[32];
	unsigned char	mode[1];
	unsigned char	uid[1];
	unsigned char	size[2];
	unsigned char	tmod[4];
	unsigned char	taddress[2];
	unsigned char	unused[20];
	unsigned char	checksum[2];
} dir[192];

int	tapefile;
char	buffer[8192];
i32	cvtime(unsigned char *);
extern	int verbose;
extern	int newtap;

void
populate(char *name)
{
	int i, isabs;
	struct tap *tpp;
	Fileinf f;

	replete = 1;
	tapefile = open(name, OREAD);
	if (tapefile<0)
		error("Can't open argument file");
	read(tapefile, dir, sizeof dir);
	for (i=0, tpp=&dir[8]; i<192; i++, tpp++) {
		unsigned char *sp = (unsigned char *)tpp;
		int j, cksum = 0;
		for (j=0; j<32; j++, sp+=2)
			cksum += sp[0] + (sp[1]<<8);
		cksum &= 0xFFFF;
		if (cksum!=0) {
			print("cksum failure\n");
			continue;
		}
		if (tpp->name[0]=='\0')
			continue;
		f.addr = tpp->taddress[0] + (tpp->taddress[1]<<8);
		if (f.addr==0)
			continue;
		f.size = tpp->size[0] + (tpp->size[1]<<8);
		f.mdate = cvtime(tpp->tmod);
		f.mode = tpp->mode[0]&0777;
		f.uid = tpp->uid[0]&0377;
		isabs = tpp->name[0]=='/';
		f.name = (char *)tpp->name+isabs;
		if (verbose)
			print("%s mode %o uid %d, %s", f.name, f.mode, f.uid, ctime(f.mdate));
		poppath(f, 1);
	}
}

i32
cvtime(unsigned char *tp)
{
	unsigned long t = (tp[1]<<24)+(tp[0]<<16)+(tp[3]<<8)+(tp[2]<<0);
	if (!newtap) {
		t /= 60;
		t += 3*365*24*3600;
	}
	return t;
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
doread(Ram *r, i64 off, i32 cnt)
{
	if (cnt>sizeof(buffer))
		print("count too big\n");
	seek(tapefile, 512*r->addr+off, 0);
	read(tapefile, buffer, cnt);
	return buffer;
}

void
dowrite(Ram *r, char *buf, i32 off, i32 cnt)
{
	USED(r); USED(buf); USED(off); USED(cnt);
}

int
dopermw(Ram *r)
{
	USED(r);
	return 0;
}
