/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<mach.h>

int
size(char *file)
{
	int fd;
	Fhdr f;

	if((fd = open(file, OREAD)) < 0){
		fprint(2, "size: ");
		perror(file);
		return 1;
	}
	if(crackhdr(fd, &f)) {
		print("%ldt + %ldd + %ldb = %ld\t%s\n", f.txtsz, f.datsz,
			f.bsssz, f.txtsz+f.datsz+f.bsssz, file);
		close(fd);
		return 0;
	}
	fprint(2, "size: %s not an a.out\n", file);
	close(fd);
	return 1;
}

void
main(int argc, char *argv[])
{
	char *err;
	int i;

	ARGBEGIN {
	default:
		fprint(2, "usage: size [a.out ...]\n");
		exits("usage");
	} ARGEND;

	err = 0;
	if(argc == 0)
		if(size("8.out"))
			err = "error";
	for(i=0; i<argc; i++)
		if(size(argv[i]))
			err = "error";
	exits(err);
}
