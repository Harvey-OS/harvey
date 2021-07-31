#include <u.h>
#include <libc.h>

#define	N	8000

void
copy(int fd, int pid)
{
	int i, j, m, w, partial, nb;
	Rune r;
	char *t;
	char buf[32+N], *b;
	
	m = sprint(buf, "d%d\n", pid);
	b = buf+m;
	partial = 0;
	for(;;){
		nb = partial+read(0, b+partial, N-m-partial-1);	/* -1 to leave terminal \0 */
		if(nb <= 0)
			break;
		b[partial+nb] = '\0';
		if(strlen(b) < nb){	/* nulls in data */
			t = malloc(nb+1);
			if(t == nil){
				fprint(2, "wnew: malloc error: %r\n");
				exits("malloc");
			}
			for(i=j=0; i<nb; i++)
				if(b[i] != '\0')
					t[j++] = b[i];
			memmove(b, t, j);
			nb = j;
			free(t);
		}
		/* process bytes into runes, transferring terminal partial runes into next buffer */
		for(i=0; i<nb && fullrune(b+i, nb-i); i+=w)
			w = chartorune(&r, b+i);
		if(write(fd, buf, m+i) != m+i){
			fprint(2, "wnew: write error: %r\n");
			exits("write");
		}
		partial = nb-i;
		memmove(b, b+i, partial);
	}
}


void
main(int argc, char *argv[])
{
	char *srv;
	int i, fd, pid;
	char wdir[256];
	int dflag;

	dflag = 0;
	ARGBEGIN{
	case 'd':
		dflag = 1;
		break;
	default:
		fprint(2, "usage: wnew [-d] [label]\n");
	}ARGEND

	pid = getpid();
	wdir[0] = '\0';
	if(!dflag)
		getwd(wdir, sizeof wdir);
	if(argc>0)
		for(i=0; i<argc; i++)
			snprint(wdir, sizeof wdir, "%s%c%s", wdir, i==0? '/' : '-', argv[i]);
	else
		snprint(wdir, sizeof wdir, "%s/-win", wdir);

	srv = getenv("winsrv");
	if(srv == nil){
		fprint(2, "wnew: can't $winsrv not set\n");
		exits("no $winsrv");
	}
	fd = open(srv, OWRITE);
	if(fd < 0){
		fprint(2, "wnew: can't open $winsrv: %r\n");
		exits("open");
	}

	/* create window */
	if(fprint(fd, "n%d\n%s", pid, wdir+dflag) < 0){	/* skip leading / if -d */
		fprint(2, "wnew: can't create window: %r\n");
		exits("create");
	}

	/* copy */
	copy(fd, pid);

	/* terminate window */
	if(fprint(fd, "e%d\n", pid) < 0){
		fprint(2, "wnew: can't close window: %r\n");
		exits("write");
	}
	exits(nil);
}
