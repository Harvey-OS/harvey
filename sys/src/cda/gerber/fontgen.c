#include	<u.h>
#include	<libc.h>
#include	<bio.h>

#define		min(x,y)	(((x)<(y))?(x):(y))

void
main(int argc, char **argv)
{
	Biobuf *bp;
	uchar *s;
	int n, i;
	int first;
	int map[256];		/* ascii, not runes */
	char *todo;
	int nmax, j, k;
	Biobuf stdout;
	char buf[1024];

	ARGBEGIN{
	}ARGEND
	if(argc != 2){
		fprint(2, "Usage: %s mapfile fontfile\n", argv0);
		exits("usage");
	}
	if((bp = Bopen(argv[0], OREAD)) == 0){
		fprint(2, "%s: open(%s) failed; %r\n", argv0, argv[0]);
		exits("open fail");
	}
	first = 1;
	nmax = -1;
	while(s = Brdline(bp, '\n')){
		n = strtol((char *)s+1, 0, 10);
		if(first){
			for(i = 0; i < 256; i++)
				map[i] = n;
			first = 0;
		}
		map[*s] = n;
		if(nmax < n)
			nmax = n;
	}
	Bclose(bp);
	todo = malloc(nmax+1);
	memset(todo, 0, nmax+1);
	for(i = 0; i < 256; i++)
		todo[map[i]] = 1;
	Binit(&stdout, 1, OWRITE);
	Bprint(&stdout, "#define	ENDVEC		-9999\n");
	Bprint(&stdout, "#define	ENDCHAR		-9998\n\n");
	if((bp = Bopen(argv[1], OREAD)) == 0){
		fprint(2, "%s: open(%s) failed; %r\n", argv0, argv[1]);
		exits("open fail");
	}
	while(Bread(bp, buf, 8) == 8){
		buf[8] = 0;
		i = strtol(buf+5, 0, 10);
		buf[5] = 0;
		n = strtol(buf, 0, 10);
		/* char n; i chars */
		j = 0;
		k = min(i, 32);
#define	RD(x)	if(Bread(bp,buf+j,2*x)!=2*x){fprint(2,"%s: char %d bad read: %r\n",argv0,n);exits("bad read");} j += 2*x
		RD(k);
		if(i >= 32)
			Bgetc(bp);	/* swallow \n */
		if(i > 32){
			i -= 32;
			k = min(i, 36);
			RD(k);
			if(i >= 36)
				Bgetc(bp);	/* swallow \n */
			if(i > 36){
				i -= 36;
				k = min(i, 32);
				RD(k);
				if(i >= 32)
					Bgetc(bp);	/* swallow \n */
				if(i > 32){
					i -= 32;
					RD(i);
				}
			}
		}
		if((i = Bgetc(bp)) != '\n'){
			fprint(2, "%s: char %d bad end, expected \\n, got '%c'\n", argv0, n, i);
			exits("bad char");
		}
		if(todo[n] == 0)
			continue;
		Bprint(&stdout, "static int char%d[] = { %d,", n, (int)buf[1] - (int)buf[0]);
		for(i = 2; i < j; i += 2){
			if(buf[i] == ' ')
				Bprint(&stdout, " ENDVEC,");
			else
				Bprint(&stdout, " %d,%d,", ((int)buf[i])-'R', ((int)buf[i+1])-'R');
		}
		Bprint(&stdout, " ENDVEC, ENDCHAR };\n");
	}
	Bprint(&stdout, "\n\nstatic int *vecs[256] = {");
	for(i = 0; i < 256; i++){
		if((i%8) == 0)
			Bprint(&stdout, "\n\tchar%d,", map[i]);
		else
			Bprint(&stdout, " char%d,", map[i]);
	}
	Bprint(&stdout, "\n};\n");
	exits(0);
}
