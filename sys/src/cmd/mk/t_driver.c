#include	"mk.h"
#include	<ar.h>

#define	AR	123456L

static long split(char *name, char *buf, char **p2);

struct ftype
{
	long magic;
	long (*time)(int, char*, char*);
	void (*touch)(char*, char*, char*);
	void (*delete)(char*, char*, char*);
} ftab[] =
{
	{ 0L,	ftimeof,	ftouch,		fdelete },
	{ AR,	atimeof,	atouch,		adelete },
	{ 0,}
};

long
timeof(char *name, int force)
{
	char buf[BIGBLOCK], *part2;
	struct ftype *f;
	long magic;

	magic = split(name, buf, &part2);
	for(f = ftab; f->time; f++)
		if(f->magic == magic)
			return((*f->time)(force, name, buf));
	fprint(2, "mk: '%s' appears to have an unknown magic number (%ld)\n", name, magic);
	Exit();
	return(0L);	/* shut cyntax up */
}

void
touch(char *name)
{
	char buf[BIGBLOCK], *part2;
	struct ftype *f;
	long magic;

	magic = split(name, buf, &part2);
	Bprint(&stdout, "touch(%s)\n", name);
	if(nflag)
		return;
	for(f = ftab; f->time; f++)
		if(f->magic == magic){
			(*f->touch)(name, buf, part2);
			return;
		}
	fprint(2, "mk: hoon off! I never heard of magic=%ld\n", magic);
	Exit();
}

void
delete(char *name)
{
	char buf[BIGBLOCK], *part2;
	struct ftype *f;
	long magic;

	magic = split(name, buf, &part2);
	for(f = ftab; f->time; f++)
		if(f->magic == magic){
			(*f->delete)(name, buf, part2);
			return;
		}
	fprint(2, "mk: hoon off! I never heard of magic=%ld\n", magic);
	Exit();
}

static long
type(char *file)
{
	int fd;
	char buf[SARMAG];
	short m;
	long goo;

	if((fd = open(file, 0)) < 0){
		{
			if(symlook(file, S_BITCH, (char *)0) == 0){
				Bprint(&stdout, "%s doesn't exist: assuming it will be an archive\n", file);
				symlook(file, S_BITCH, file);
			}
			return(AR);
		}
	}
	if(read(fd, buf, (COUNT)SARMAG) != (COUNT)SARMAG){
		close(fd);
		return(-1L);
	}
	if(strncmp(ARMAG, buf, (COUNT)SARMAG) == 0)
		goo = AR;
	else {
		LSEEK(fd, 0L, 0);
		if(read(fd, (char *)&m, (COUNT)sizeof m) == (COUNT)sizeof m)
			goo = m;
		else
			goo = -1;
	}
	close(fd);
	return(goo);
}

static long
split(char *name, char *buf, char **p2)
{
	char *s;

	strcpy(buf, name);
	if(s = utfrune(buf, '(')){
		*s++ = 0;
		*p2 = s;
		s = utfrune(s, ')');
		if (s)
			*s = 0;
		return(type(buf));
	} else
		return(0L);
}
