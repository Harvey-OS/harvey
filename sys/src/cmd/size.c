#include	<u.h>
#include	<libc.h>
#include	<bio.h>
#include	<mach.h>

int	size(char*);

void
main(int argc, char *argv[])
{
	int err = 0;

	USED(argc);
	if(*++argv == 0)
		err |= size("v.out");
	else
		while(*argv)
			err |= size(*argv++);
	if(err)
		exits("error");
	else
		exits(0);
}

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
	if (crackhdr(fd, &f)) {
		print("%ldt + %ldd + %ldb = %ld\t%s\n",f.txtsz,f.datsz,
			f.bsssz, f.txtsz+f.datsz+f.bsssz, file);
		close(fd);
		return 0;
	}
	fprint(2, "size: %s not an a.out\n", file);
	close(fd);
	return 1;
}
