#include	"rework.h"
#include	<sys/types.h>
#include	<fcntl.h>
#include	<unistd.h>

int verbose;
FILE *UN, *RE, *NEW;
char *debug = 0;
int oflag = 0;
int eflag = 0;
int qflag = 0;
Pintab pins(3011);
Net *oldnets, *newnets;

void
usage()
{
	fprintf(stderr, "Usage:\trework [-q] [-e] [-v] [-dnet] [-s] [-o] old new\n");
	exit(1);
}

char buf[50000];

main(int argc, char **argv)
{
	char *s;
	FILE *fp;
	int sflag = 0;
	register Net *n, *nn;
	char buf[BUFSIZ];
	extern etext();
/*
	monitor((int)main, (int)etext, buf, ((int)etext)-((int)main)+12+8*300, 300);
*/
	for(argv++, argc--; *argv && (**argv == '-'); argv++, argc--)
		switch(argv[0][1])
		{
		case 'd':	debug = &argv[0][2]; break;
		case 'e':	verbose = eflag = 1; break;
		case 'o':	oflag = 1; break;
		case 'q':	qflag = 1; break;
		case 's':	sflag = 1; break;
		case 'v':	verbose = 1; break;
		default:	usage();
		}
	if(argc != 2)
		usage();
	if(!verbose){
		if((UN = fopen("UN.wr", "w")) == 0){
			perror("UN.wr");
			exit(1);
		}
		if((RE = fopen("RE.wr", "w")) == 0){
			perror("RE.wr");
			exit(1);
		}
		if((NEW = fopen("NEW.wr", "w")) == 0){
			perror("NEW.wr");
			exit(1);
		}
	}
	setfields(" \t");
	if((fp = fopen(*argv, "r")) == 0){
		perror(*argv);
		exit(1);
	}
	fgets(buf, (int) sizeof(buf), fp);
	fgets(buf, (int) sizeof(buf), fp);
	fgets(buf, (int) sizeof(buf), fp);
	oldnets = readnets(fp, 1);
	fclose(fp);
	if(sflag){
		pins.stats();
	}
	if((fp = fopen(*++argv, "r")) == 0){
		perror(*argv);
		exit(1);
	}
	s = fgets(buf, (int) sizeof(buf), fp);
	if(!verbose)
		fprintf(NEW, "%s", s), fprintf(UN, "%s", s), fprintf(RE, "%s", s);
	s = fgets(buf, (int) sizeof(buf), fp);
	if(!verbose)
		fprintf(NEW, "%s", s), fprintf(UN, "%s", s), fprintf(RE, "%s", s);
	s = fgets(buf, (int) sizeof(buf), fp);
	if(!verbose)
		fprintf(NEW, "%s", s), fprintf(UN, "%s", s), fprintf(RE, "%s", s);
	newnets = readnets(fp, 0);
	for(n = newnets; n; n = n->next)
		n->eqelim();
	/*
		eliminate done entries
	*/
	while(newnets && newnets->done)
		newnets = newnets->next;
	if(newnets){
		for(n = newnets; n; n = n->next){
			for(nn = n->next; nn && nn->done; nn = nn->next)
				;
			n->next = nn;
		}
	}
	while(oldnets && oldnets->done)
		oldnets = oldnets->next;
	if(oldnets){
		for(n = oldnets; n; n = n->next){
			for(nn = n->next; nn && nn->done; nn = nn->next)
				;
			n->next = nn;
		}
	}
	/*
		now do the diffs
	*/
	for(n = newnets; n; n = n->next)
		if(!n->done)
			n->diff();
	for(n = oldnets; n; n = n->next)
		if(!n->done)
			n->rm();
	exit(0);
}
