#include <u.h>
#include <libc.h>

char	errbuf[ERRLEN];
Dir	*dirbuf;
long	ndirbuf = 0;
int	ignerr = 0;

void
err(char *f)
{
	if(!ignerr){
		errstr(errbuf);
		fprint(2, "rm: %s: %s\n", f, errbuf);
	}
}

/*
 * Read a whole directory before removing anything as the holes formed
 * by removing affect the read offset.
 */
long
readdirect(int fd)
{
	enum
	{
		N = 32
	};
	long m, n;

	m = 1;	/* prime the loop */
	for(n=0; m>0; n+=m/sizeof(Dir)){
		if(n == ndirbuf){
			dirbuf = realloc(dirbuf, (ndirbuf+N)*sizeof(Dir));
			if(dirbuf == 0){
				err("memory allocation");
				exits(errbuf);
			}
			ndirbuf += N;
		}
		m = dirread(fd, dirbuf+n, (ndirbuf-n)*sizeof(Dir));
	}
	return n;
}

/*
 * f is a non-empty directory. Remove its contents and then it.
 */
void
rmdir(char *f)
{
	char *name;
	Dir *db;
	int fd, i, j, n, ndir;

	fd = open(f, OREAD);
	if(fd < 0){
		err(f);
		return;
	}
	n = readdirect(fd);
	close(fd);

	name = malloc(strlen(f)+1+NAMELEN);
	if(name == 0){
		err("memory allocation");
		return;
	}

	ndir = 0;
	for(i=0; i<n; i++){
		sprint(name, "%s/%s", f, dirbuf[i].name);
		if(remove(name) != -1)
			dirbuf[i].qid.path = 0;	/* so we won't recurse */
		else{
			if(dirbuf[i].qid.path & CHDIR)
				ndir++;
			else
				err(name);
		}
	}
	if(ndir){
		/*
		 * Recursion will smash dirbuf, so make a local copy
		 */
		db = malloc(ndir*sizeof(Dir));
		if(db == 0){
			err("memory allocation");
			free(name);
			return;
		}
		for(j=0,i=0; i<n; i++)
			if(dirbuf[i].qid.path & 0x80000000L)
				db[j++] = dirbuf[i];
		/*
		 * Everything we need is in db now
		 */
		for(j=0; j<ndir; j++){
			sprint(name, "%s/%s", f, db[j].name);
			rmdir(name);
		}
		free(db);
	}
	if(remove(f) == -1)
		err(f);
	free(name);
}
void
main(int argc, char *argv[])
{
	int i;
	int recurse;
	char *f;
	Dir db;

	ignerr = 0;
	recurse = 0;
	ARGBEGIN{
	case 'r':
		recurse = 1;
		break;
	case 'f':
		ignerr = 1;
		break;
	default:
		fprint(2, "usage: rm [-fr] file ...\n");
		exits("usage");
	}ARGEND
	for(i=0; i<argc; i++){
		f = argv[i];
		if(remove(f) != -1)
			continue;
		if(recurse && dirstat(f, &db)!=-1 && (db.qid.path&CHDIR))
			rmdir(f);
		else
			err(f);
	}
	exits(errbuf);
}
