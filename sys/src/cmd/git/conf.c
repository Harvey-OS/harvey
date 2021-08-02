#include <u.h>
#include <libc.h>
#include <ctype.h>

#include "git.h"

int	findroot;
int	showall;
int	nfile;
char	*file[32];

static int
showconf(char *cfg, char *sect, char *key)
{
	char *ln, *p;
	Biobuf *f;
	int foundsect, nsect, nkey, found;

	if((f = Bopen(cfg, OREAD)) == nil)
		return 0;

	found = 0;
	nsect = sect ? strlen(sect) : 0;
	nkey = strlen(key);
	foundsect = (sect == nil);
	while((ln = Brdstr(f, '\n', 1)) != nil){
		p = strip(ln);
		if(*p == '[' && sect){
			foundsect = strncmp(sect, ln, nsect) == 0;
		}else if(foundsect && strncmp(p, key, nkey) == 0){
			p = strip(p + nkey);
			if(*p != '=')
				continue;
			p = strip(p + 1);
			print("%s\n", p);
			found = 1;
			if(!showall){
				free(ln);
				goto done;
			}
		}
		free(ln);
	}
done:
	return found;
}


void
usage(void)
{
	fprint(2, "usage: %s [-f file] [-r] keys..\n", argv0);
	fprint(2, "\t-f:	use file 'file' (default: .git/config)\n");
	fprint(2, "\t r:	print repository root\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	char repo[512], *p, *s;
	int i, j;

	ARGBEGIN{
	case 'f':	file[nfile++]=EARGF(usage());	break;
	case 'r':	findroot++;			break;
	case 'a':	showall++;			break;
	default:	usage();			break;
	}ARGEND;

	if(findroot){
		if(findrepo(repo, sizeof(repo)) == -1)
			sysfatal("%r");
		print("%s\n", repo);
		exits(nil);
	}
	if(nfile == 0){
		file[nfile++] = ".git/config";
		if((p = getenv("home")) != nil)
			file[nfile++] = smprint("%s/lib/git/config", p);
		file[nfile++] = "/sys/lib/git/config";
	}

	for(i = 0; i < argc; i++){
		if((p = strchr(argv[i], '.')) == nil){
			s = nil;
			p = argv[i];
		}else{
			*p = 0;
			p++;
			s = smprint("[%s]", argv[i]);
		}
		for(j = 0; j < nfile; j++)
			if(showconf(file[j], s, p))
				break;
	}
	exits(nil);
}
