#include <u.h>
#include <libc.h>

int	copy1(int fdf, int fdt, char *from, char *to);
void	hardremove(char *);
int	mv(char *from, char *todir, char *toelem);
int	mv1(char *from, Dir *dirb, char *todir, char *toelem);
int	samefile(char *, char *);
void	split(char *, char **, char **);

void
main(int argc, char *argv[])
{
	int i, failed;
	Dir *dirto, *dirfrom;
	char *todir, *toelem;

	if(argc<3){
		fprint(2, "usage: mv fromfile tofile\n");
		fprint(2, "	  mv fromfile ... todir\n");
		exits("bad usage");
	}

	/* prepass to canonicalise names before splitting, etc. */
	for(i=1; i < argc; i++)
		cleanname(argv[i]);

	if((dirto = dirstat(argv[argc-1])) != nil && (dirto->mode&DMDIR)){
		dirfrom = nil;
		if(argc == 3
		&& (dirfrom = dirstat(argv[1])) != nil
		&& (dirfrom->mode & DMDIR)) 
			split(argv[argc-1], &todir, &toelem); /* mv dir1 dir2 */
		else{				/* mv file... dir */
			todir = argv[argc-1];
			toelem = nil;		/* toelem will be fromelem */
		}
		free(dirfrom);
	}else
		split(argv[argc-1], &todir, &toelem);	/* mv file1 file2 */
	free(dirto);
	if(argc>3 && toelem != nil){
		fprint(2, "mv: %s not a directory\n", argv[argc-1]);
		exits("bad usage");
	}

	failed = 0;
	for(i=1; i < argc-1; i++)
		if(mv(argv[i], todir, toelem) < 0)
			failed++;
	if(failed)
		exits("failure");
	exits(0);
}

int
mv(char *from, char *todir, char *toelem)
{
	int stat;
	Dir *dirb;

	dirb = dirstat(from);
	if(dirb == nil){
		fprint(2, "mv: can't stat %s: %r\n", from);
		return -1;
	}
	stat = mv1(from, dirb, todir, toelem);
	free(dirb);
	return stat;
}

int
mv1(char *from, Dir *dirb, char *todir, char *toelem)
{
	int fdf, fdt, i, j, stat;
	char toname[4096], fromname[4096];
	char *fromdir, *fromelem;
	Dir *dirt, null;

	strncpy(fromname, from, sizeof fromname);
	split(from, &fromdir, &fromelem);
	if(toelem == 0)
		toelem = fromelem;
	i = strlen(toelem);
	if(i==0){
		fprint(2, "mv: null last name element moving %s\n", fromname);
		return -1;
	}
	j = strlen(todir);
	if(i + j + 2 > sizeof toname){
		fprint(2, "mv: path too big (max %d): %s/%s\n",
			sizeof toname, todir, toelem);
		return -1;
	}
	memmove(toname, todir, j);
	toname[j] = '/';
	memmove(toname+j+1, toelem, i);
	toname[i+j+1] = 0;

	if(samefile(fromdir, todir)){
		if(samefile(fromname, toname)){
			fprint(2, "mv: %s and %s are the same\n",
				fromname, toname);
			return -1;
		}

		/* remove target if present */
		dirt = dirstat(toname);
		if(dirt != nil) {
			hardremove(toname);
			free(dirt);
		}

		/* try wstat */
		nulldir(&null);
		null.name = toelem;
		if(dirwstat(fromname, &null) >= 0)
			return 0;
		if(dirb->mode & DMDIR){
			fprint(2, "mv: can't rename directory %s: %r\n",
				fromname);
			return -1;
		}
	}
	/*
	 * Renaming won't work --- must copy
	 */
	if(dirb->mode & DMDIR){
		fprint(2, "mv: %s is a directory, not copied to %s\n",
			fromname, toname);
		return -1;
	}
	fdf = open(fromname, OREAD);
	if(fdf < 0){
		fprint(2, "mv: can't open %s: %r\n", fromname);
		return -1;
	}

	dirt = dirstat(toname);
	if(dirt != nil && (dirt->mode & DMAPPEND))
		hardremove(toname);  /* because create() won't truncate file */
	free(dirt);

	fdt = create(toname, OWRITE, dirb->mode);
	if(fdt < 0){
		fprint(2, "mv: can't create %s: %r\n", toname);
		close(fdf);
		return -1;
	}
	stat = copy1(fdf, fdt, fromname, toname);
	close(fdf);

	if(stat >= 0){
		nulldir(&null);
		null.mtime = dirb->mtime;
		null.mode = dirb->mode;
		dirfwstat(fdt, &null);	/* ignore errors; e.g. user none always fails */
		if(remove(fromname) < 0){
			fprint(2, "mv: can't remove %s: %r\n", fromname);
			stat = -1;
		}
	}
	close(fdt);
	return stat;
}

int
copy1(int fdf, int fdt, char *from, char *to)
{
	char buf[8192];
	long n, n1;

	while ((n = read(fdf, buf, sizeof buf)) > 0) {
		n1 = write(fdt, buf, n);
		if(n1 != n){
			fprint(2, "mv: error writing %s: %r\n", to);
			return -1;
		}
	}
	if(n < 0){
		fprint(2, "mv: error reading %s: %r\n", from);
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
	}else if(strcmp(name, "..") == 0){
		*pdir = "..";
		*pelem = ".";
	}else{
		*pdir = ".";
		*pelem = name;
	}
}

int
samefile(char *a, char *b)
{
	Dir *da, *db;
	int ret;

	if(strcmp(a, b) == 0)
		return 1;
	da = dirstat(a);
	db = dirstat(b);
	ret = (da != nil && db != nil &&
		da->qid.type==db->qid.type &&
		da->qid.path==db->qid.path &&
		da->qid.vers==db->qid.vers &&
		da->dev==db->dev &&
		da->type==db->type);
	free(da);
	free(db);
	return ret;
}

void
hardremove(char *a)
{
	if(remove(a) == -1){
		fprint(2, "mv: can't remove %s: %r\n", a);
		exits("mv");
	}
	while(remove(a) != -1)
		;
}
