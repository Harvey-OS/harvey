#include <u.h>
#include <libc.h>

#define	ISDIR	0x80000000L

long	du(char *, Dir *);
long	k(long);
void	err(char *);
int	warn(char *);
int	seen(Dir *);
int	aflag=0;
char	fmt[] = "%lud\t%s\n";
long	blocksize = 1024;

void
main(int argc, char *argv[])
{
	int i;
	char *s, *ss;

	ARGBEGIN{
	case 'a':	aflag=1; break;
	case 'b':
		s = ARGF();
		blocksize = strtoul(s, &ss, 0);
		if(s == ss)
			blocksize = 1;
		if(*ss == 'k')
			blocksize *= 1024;
		break;
	}ARGEND
	if(argc==0)
		print(fmt, du(".", (Dir*)0), ".");
	else
	for(i=0; i<argc; i++)
		print(fmt, du(argv[i], (Dir*)0), argv[i]);
	exits(0);
}

long
du(char *name, Dir *dir)
{
	int fd, i, n;
	Dir buf[25], b;
	char file[256];
	long nk, t;

	if(dir==0){
		dir=&buf[0];
		if(dirstat(name, dir)<0)
			return warn(name);
		if((dir->mode&ISDIR)==0)
			return k(dir->length);
	}
	fd=open(name, OREAD);
	if(fd<0)
		return warn(name);
	nk=0;
	while((n=dirread(fd, buf, sizeof buf))>0){
		n/=sizeof(Dir);
		dir=buf;
		for(i=0; i<n; i++, dir++){
			if((dir->mode&ISDIR)==0){
				t=k(dir->length);
				nk+=t;
				if(aflag){
					sprint(file, "%s/%s", name, dir->name);
					print(fmt, t, file);
				}
				continue;
			}
			if(strcmp(dir->name, ".")==0 || strcmp(dir->name, "..")==0 || seen(dir))
				continue;
			sprint(file, "%s/%s", name, dir->name);
			if(dirstat(file, &b)<0){
				warn(file);
				continue;
			}
			if(b.qid.path!=dir->qid.path ||
			   b.dev!=dir->dev || b.type!=dir->type)
				continue;	/* file is hidden */
			t=du(file, dir);
			print(fmt, t, file);
			nk+=t;
		}
	}
	if(n<0)
		warn("name");
	close(fd);
	return nk;
}

int
seen(Dir *dir)
{
	static Dir *cache=0;
	static int n=0, ncache=0;
	Dir *dp;
	int i;

	dp=cache;
	for(i=0; i<n; i++,dp++)
		if(dir->qid.path==dp->qid.path &&
		   dir->type==dp->type && dir->dev==dp->dev)
			return 1;
	if(n==ncache){
		cache=realloc(cache, (ncache+=20)*sizeof(Dir));
		if(cache==0)
			err("malloc failure");
	}
	cache[n++]=*dir;
	return 0;
}

void
err(char *s)
{
	fprint(2, "du: ");
	perror(s);
	exits(s);
}

int
warn(char *s)
{
	fprint(2, "du: ");
	perror(s);
	return 0;
}

long
k(long n)
{
	n = (n+blocksize-1)/blocksize;
	return n*blocksize/1024;
}
