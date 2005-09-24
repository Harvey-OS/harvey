#include <u.h>
#include <libc.h>
#include <bio.h>
#include <libsec.h>

enum
{
	ENCLEN = 26,
};

typedef struct Name Name;
struct Name {
	char	shortname[ENCLEN + 1];
	char*	longname;
	Name*	next;
};

Name *names;
void rename(char*, char*, char*);
void renamedir(char*);
void readnames(char*);

void
main(int argc, char **argv)
{
	char lnfile[256], *d;
	d = ".";
	if(argc > 1)
		d = argv[1];

	snprint(lnfile, sizeof(lnfile), "%s/.longnames", d);
	readnames(lnfile);
	renamedir(d);
}

void
renamedir(char *d)
{
	int n;
	Dir *dir;
	char *sub;
	int fd, i;
	Name *na;

	fd = open(d, OREAD);
	if (fd == -1)
		return;
	while((n = dirread(fd, &dir)) > 0){
		for(i = 0; i < n; i++){
			if(dir[i].mode & DMDIR){
				sub = malloc(strlen(d) + 1 + strlen(dir[i].name) + 1);
				sprint(sub, "%s/%s", d, dir[i].name);
				renamedir(sub);
				free(sub);
			}
			if(strlen(dir[i].name) != ENCLEN)
				continue;
			for (na = names; na != nil; na = na->next){
				if (strcmp(na->shortname, dir[i].name) == 0){
					rename(d, dir[i].name, na->longname);
					break;
				}
			}
		}
		free(dir);
	}
	close(fd);
}

void
rename(char *d, char *old, char *new)
{
	char *p;
	Dir dir;
	p = malloc(strlen(d) + 1 + strlen(old) + 1);
	sprint(p, "%s/%s", d, old);
	nulldir(&dir);
	dir.name = new;
	if(dirwstat(p, &dir) == -1)
		fprint(2, "unlnfs: cannot rename %s to %s: %r\n", p, new);
	free(p);
}
	
void
long2short(char shortname[ENCLEN+1], char *longname)
{
	uchar digest[MD5dlen];
	md5((uchar*)longname, strlen(longname), digest, nil);
	enc32(shortname, ENCLEN+1, digest, MD5dlen);
}

void
readnames(char *lnfile)
{
	Biobuf *bio;
	char *f;
	Name *n;

	bio = Bopen(lnfile, OREAD);
	if(bio == nil){
		fprint(2, "unlnfs: cannot open %s: %r\n", lnfile);
		exits("error");
	}
	while((f = Brdstr(bio, '\n', 1)) != nil){
		n = malloc(sizeof(Name));
		n->longname = f;
		long2short(n->shortname, f);
		n->next = names;
		names = n;
	}
	Bterm(bio);
}
