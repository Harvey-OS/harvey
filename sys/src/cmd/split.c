#include <u.h>
#include <libc.h>
#include <bio.h>
#include <regexp.h>

char	digit[] = "0123456789";
char	*suffix = "";
char	*stem = "x";
char	suff[] = "aa";
char	name[200];
Biobuf	bout;
Biobuf	*output = &bout;

extern int nextfile(void);
extern int matchfile(Resub*);
extern void openf(void);
extern char *fold(char*,int);
extern void usage(void);
extern void badexp(void);

void
main(int argc, char *argv[])
{
	Reprog *exp;
	char *pattern = 0;
	register n = 1000;
	char *line;
	int xflag = 0;
	int iflag = 0;
	Biobuf bin;
	Biobuf *b = &bin;

	ARGBEGIN{
	case 'n':
		n=atoi(ARGF());
		break;
	case 'e':
		pattern = strdup(ARGF());
		break;
	case 'f':
		stem = strdup(ARGF());
		break;
	case 's':
		suffix = strdup(ARGF());
		break;
	case 'x':
		xflag++;
		break;
	case 'i':
		iflag++;
		break;
	default:
		goto Usage;
	}ARGEND;
	if(argc < 0 || argc > 1)
    Usage:	usage();

	if(argc == 0){
		Binit(b, 0, OREAD);
	}else{
		b = Bopen(argv[0], OREAD);
		if(b == 0)
			fprint(2, "split: can't open %s: %r\n", argv[0]);
	}
	if(pattern) {
		if(!(exp = regcomp(iflag? fold(pattern,strlen(pattern)): pattern)))
			badexp();
		while((line=Brdline(b,'\n')) != 0) {
			Resub match[2];
			line[BLINELEN(b)-1] = 0;
			if(regexec(exp,iflag?fold(line,BLINELEN(b)-1):line,match,2)) {
				if(matchfile(match) && xflag)
					continue;
			} else if(output == 0)
				nextfile();	/* at most once */
			Bwrite(output, line, BLINELEN(b)-1);
			Bputc(output, '\n');
		}
	} else {
		register linecnt = n;
		while((line=Brdline(b,'\n')) != 0) {
			if(++linecnt > n) {
				nextfile();
				linecnt = 1;
			}
			Bwrite(output, line, BLINELEN(b));
		}
	}
	if(b != 0)
		Bclose(b);
	exits(0);
}

int
nextfile(void)
{
	static canopen = 1;
	if(suff[0] > 'z') {
		if(canopen)
			fprint(2, "split: file %szz not split\n",stem);
		canopen = 0;
	} else {
		strcpy(name, stem);
		strcat(name, suff);
		if(++suff[1] > 'z') 
			suff[1] = 'a', ++suff[0];
		openf();
	}
	return canopen;
}

int
matchfile(Resub *match)
{
	if(match[1].sp) {
		int len = match[1].ep - match[1].sp;
		strncpy(name, match[1].sp, len);
		strcpy(name+len, suffix);
		openf();
		return 1;
	} 
	return nextfile();
}

void
openf(void)
{
	int fd;
	Bflush(output);
	Bclose(output);
	if((fd=create(name,OWRITE,0666)) == -1) {
		fprint(2, "grep: can't open %s: %r\n", name);
		exits("open failed");
	}
	Binit(output,fd,OWRITE);
}

char *
fold(char *s, int n)
{
	static char *fline;
	static int linesize = 0;
	char *t;
	if(linesize < n+1){
		fline = realloc(fline,n+1);
		linesize = n+1;
	}
	for(t=fline; *t++=tolower(*s++); )
		continue;
		/* we assume the 'A'-'Z' only appear as themselves
		 * in a utf encoding.
		 */
	return fline;
}

void
usage(void)
{
	fprint(2, "usage: split [-n num] [-e exp] [-f stem] [-s suff] [-x] [-i] [file]\n");
	exits("split usage");
}

void
badexp(void)
{
	fprint(2, "split: bad regular expression\n");
	exits("bad regular expression");
}
