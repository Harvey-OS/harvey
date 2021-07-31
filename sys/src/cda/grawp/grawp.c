#include <u.h>
#include <libc.h>
#include <bio.h>

#define	Pt(x,y)	(x), (y)

typedef struct Point {
	int x, y;
} Point;

char*	getpath(char*, char**, int);
int	grawf(char*, int);
int	grawp(Biobuf*);
char*	macname(char*);
int	nexti(char**);
Point	nextp(char**);
char*	nexts(char**);

char *xpos[] = {
	"ljust", "center", "rjust", "rjust",
};

char *ypos[] = {
	"below", " ", "above", "above",
};

char *libdirs[] = { "/lib/graw", 0, };

Biobuf	in;
Biobuf	out;
int	tflag;

void
main(int argc, char **argv)
{
	int i, errors = 0;

	ARGBEGIN{
	case 't':
		++tflag;
		break;
	default:
	Usage:
		fprint(2, "usage: %s [-t] [files...]\n", argv0);
		exits("usage");
	}ARGEND
	Binit(&out, 1, OWRITE);
	if(argc < 1)
		errors += grawf(0, tflag);
	else for(i=0; i<argc; i++)
		errors += grawf(argv[i], tflag);
	exits(errors ? "errors" : 0);
}

int
grawf(char *f, int noheader)
{
	int fd, errors;

	if(!f)
		fd = 0;
	else if ((fd = open(f, OREAD)) < 0) {
		fprint(2, "can't open %s\n", f);
		return 1;
	}
	if(!noheader){
		Bprint(&out, ".bp\n");
		if(f){
			Dir dir;
			char tbuf[32];
			dirfstat(fd, &dir);
			strcpy(tbuf, ctime(dir.mtime));
			tbuf[24] = 0;
			Bprint(&out, ".sp 4\n.ps 10\n.ft H\n");
			Bprint(&out, ".ce\n\\&%s\t\t%s\n", f, tbuf+4);
		}
		Bprint(&out, ".sp 2\n");
	}
	Bprint(&out, ".PS\n");
	Bprint(&out, ".nr VS \\n(.v\n.nr PQ \\n(.f\n.nr PS \\n(.s\n");
	Bprint(&out, ".vs 6\n.ft CW\n.ps 6\n");
	Bprint(&out, "scale = 16/0.125\n");
	Binit(&in, fd, OREAD);
	errors = grawp(&in);
	close(fd);
	Bprint(&out, ".vs \\n(VSu\n.ft \\n(PQ\n.ps \\n(PS\n");
	Bprint(&out, ".PE\n");
	return errors;
}

int
grawp(Biobuf *in)
{
	Point p, q;
	int flag, errors = 0;
	char *ol, *l, *s;
	Biobuf *np;

	while(ol=l=Brdline(in, '\n'))switch(l[BLINELEN(in)-1]=0, *l++){
	case 'b':
		p = nextp(&l); q = nextp(&l);
		Bprint(&out, "box ht %d wid %d with .nw at (%d,%d)\n",
			p.y-q.y, q.x-p.x, p);
		break;
	case 'e':
		Bprint(&out, "]}\n");
		break;
	case 'i':
		s = nexts(&l); p = nextp(&l);
		Bprint(&out, "%s with .nw at (%d,%d)\n", macname(s), p);
		break;
	case 'l':
	case 'w':
		p = nextp(&l); q = nextp(&l);
		Bprint(&out, "line from (%d,%d) to (%d,%d)\n", p, q);
		break;
	case 'm':
		s = nexts(&l);
		Bprint(&out, "define %s {[\n", macname(s));
		break;
	case 's':
		s = nexts(&l); flag = nexti(&l); p = nextp(&l);
		if (flag & 16)
			continue;
		Bprint(&out, "\"%s\" %s %s at (%d,%d)\n",
			s, xpos[flag&3], ypos[(flag>>2)&3], p);
		break;
	case 'r':
		s = getpath(nexts(&l), libdirs, 0);
		if(np = Bopen(s, OREAD)){	/* assign = */
			Bprint(&out, "# r \"%s\"\n", s);
			errors += grawp(np);
			Bclose(np);
			break;
		}else
			fprint(2, "can't open %s\n", s);
			goto Error;
	case 'z':
		p = nextp(&l); q = nextp(&l);
		Bprint(&out, "box dotted ht %d wid %d with .nw at (%d,%d)\n",
			p.y-q.y, q.x-p.x, p);
		break;
	default:
	Error:
		++errors;
		Bprint(&out, "#? %s\n", ol);
		break;
	}
	return errors;
}

int
nexti(char **pp)
{
	return strtol(*pp, pp, 10);
}

Point
nextp(char **pp)
{
	Point p;

	p.x = nexti(pp);
	p.y = -nexti(pp);
	return p;
}

char *
nexts(char **pp)
{
	char *p = *pp;
	char *val = 0;

	while(*p && *p != '"')
		p++;
	if(*p && *++p) {
		val = p;
		while(*p != 0 && *p != '"')
			p++;
		if(*p == '"')
			*p++ = 0;
	}
out:
	*pp = p;
	return val;
}

char *
macname(char *s)
{
	static char buf[512];
	char *p, *q;

	p = buf;
	*p++ = 'G';
	while(*s)switch (*s){
	default:
		*p++ = *s++;
		break;
	case '-':
		*p++ = '_';
		s++;
		break;
	case '/':
		q = "_slash_";
		s++;
		strcpy(p, q);
		p += strlen(q);
		break;
	}
	*p = 0;
	return buf;
}

char *
getpath(char *name, char **libdirs, int mode)
{
	char **lib; static char file[256];

	if(name && strchr(name, '/') == 0 && access(name, mode) != 0){
		for(lib=libdirs; *lib; lib++){
			sprint(file, "%s/%s", *lib, name);
			if(access(file, mode) == 0)
				return file;
		}
	}
	return name;
}
