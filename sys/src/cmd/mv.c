#include <u.h>
#include <libc.h>

void	split(char *, char **, char **);
int	samefile(char *, char *);
int	mv(char *from, char *todir, char *toelem);
int	copy1(int fdf, int fdt, char *from, char *to);

void
main(int argc, char *argv[])
{
	int i;
	Dir dirto, dirfrom;
	char *todir, *toelem;

	if(argc<3){
		fprint(2, "usage: mv fromfile tofile\n");
		fprint(2, "	  mv fromfile ... todir\n");
		exits("bad usage");
	}
	if(dirstat(argv[argc-1], &dirto) >= 0 && (dirto.mode&CHDIR)){
		if(argc == 3 && dirstat(argv[1], &dirfrom) >= 0 &&
				(dirfrom.mode&CHDIR)){
			split(argv[argc-1], &todir, &toelem);
		}else{
			todir = argv[argc-1];
			toelem = 0;	/* toelem will be fromelem */
		}
	}else
		split(argv[argc-1], &todir, &toelem);
	if(argc>3 && toelem != 0){
		fprint(2, "mv: %s not a directory\n", argv[argc-1]);
		exits("bad usage");
	}
	for(i=1; i < argc-1; i++)
		if (mv(argv[i], todir, toelem) < 0)
			exits("failure");
	exits(0);
}

int
mv(char *from, char *todir, char *toelem)
{
	Dir dirb, dirt;
	char toname[1024], fromname[1024];
	int fdf, fdt, i, j;
	int stat;
	char *fromdir, *fromelem;

	if(dirstat(from, &dirb)!=0){
		fprint(2, "mv: can't stat %s: ", from);
		perror("");
		return -1;
	}
	strncpy(fromname, from, sizeof fromname);
	split(from, &fromdir, &fromelem);
	if(toelem == 0)
		toelem = fromelem;
	i = strlen(toelem);
	if(i==0){
		fprint(2, "mv: null last name element moving %s\n", fromname);
		return -1;
	}
	if(i >= NAMELEN){
		fprint(2, "mv: name too big: %s\n", toelem);
		return -1;
	}
	j = strlen(todir);
	if(i + j + 2 > sizeof toname){
		fprint(2, "mv: path too big: %s/%s\n", todir, toelem);
		return -1;
	}
	memmove(toname, todir, j);
	toname[j] = '/';
	memmove(toname+j+1, toelem, i);
	toname[i+j+1] = 0;
	if(samefile(fromdir, todir)){
		if(samefile(fromname, toname)){
			fprint(2, "mv: %s and %s are the same\n", fromname, toname);
			return -1;
		}
		if(dirstat(toname, &dirt) == 0){
			if(remove(toname) == -1){
				fprint(2, "mv: can't remove %s\n", toname);
				exits("mv");
			}
			do; while(remove(toname) != -1);
		}
		strcpy(dirb.name, toelem);
		if(dirwstat(fromname, &dirb) >= 0)
			return 0;
		if(dirb.mode&CHDIR){
			fprint(2, "mv: can't rename directory %s: ", fromname);
			perror("");
			return -1;
		}
	}
	/*
	 * Renaming won't work --- have to copy
	 */
	if(dirb.mode&CHDIR){
		fprint(2, "mv: %s is a directory, not copied to %s\n", fromname, toname);
		return -1;
	}
	fdf = open(fromname, OREAD);
	if(fdf<0){
		fprint(2, "mv: can't open %s:", fromname);
		perror("");
		return -1;
	}
	fdt = create(toname, OWRITE, dirb.mode);
	if(fdt < 0){
		fprint(2, "mv: can't create %s:", toname);
		perror("");
		close(fdf);
		return -1;
	}
	if ((stat = copy1(fdf, fdt, fromname, toname)) != -1)
		if (remove(fromname) < 0) {
			fprint(2, "mv: can't remove %s", fromname);
			perror("");
			return -1;
		}
	close(fdf);
	close(fdt);
	return stat;
}

int
copy1(int fdf, int fdt, char *from, char *to)
{
	char buf[8192];
	long n, n1;

	for(;;) {
		n = read(fdf, buf, sizeof buf);
		if(n >= 0) {
			if(n == 0)
				break;
			n1 = write(fdt, buf, n);
			if(n1 != n) {
				fprint(2, "mv: error writing %s:", to);
				perror("");
				return -1;
			}
		}
	}
	if(n < 0) {
		fprint(2, "mv: error reading %s:", from);
		perror("");
		return -1;
	}
	return 0;
}

void
split(char *name, char **pdir, char **pelem)
{
	char *s;

	s = utfrrune(name, '/');
	if(s){
		*s = 0;
		*pelem = s+1;
		*pdir = name;
	}else{
		*pdir = ".";
		*pelem = name;
	}
}

int
samefile(char *a, char *b)
{
	Dir da, db;

	if(strcmp(a, b) == 0)
		return 1;
	if(dirstat(a, &da) < 0 || dirstat(b, &db) < 0)
		return 0;
	return (da.qid.path==db.qid.path && da.qid.vers==db.qid.vers &&
		da.dev==db.dev && da.type==db.type);
}

