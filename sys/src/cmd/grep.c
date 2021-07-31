#include <u.h>
#include <libc.h>
#include <bio.h>
#include <regexp.h>

int	grep(Biobuf*, Reprog*, char*);
char*	lower(char*);

int	negate = 0;
int	cflag = 0;
int	hflag = 0;
int	iflag = 0;
int	lflag = 0;
int	Lflag = 0;
int	nflag = 0;
int	sflag = 0;

void
main(int argc, char *argv[])
{
	Reprog *r;
	Biobuf *b;
	Biobuf bin;
	int i, match;

	ARGBEGIN{
	case 'c':
		cflag = 1;
		break;
	case 'h':
		hflag = 1;
		break;
	case 'i':
		iflag = 1;
		break;
	case 'L':
		Lflag = 1; /* fall through */
	case 'l':
		lflag = 1;
		break;
	case 'n':
		nflag = 1;
		break;
	case 's':
		sflag = 1;
		break;
	case 'v':
		negate = 1;
		break;
	default:
		goto Usage;
	}ARGEND;
	if(argc < 1)
    Usage:
		regerror("usage: grep [-chiLlnsv] pattern [file ...]");
	r = regcomp(lower(argv[0]));
	match = 0;
	if(argc == 1){
		hflag = 1;
		Binit(&bin, 0, OREAD);
		match = grep(&bin, r, "stdin");
	}else for(i=1; i<argc; i++){
		if(argc == 2)
			hflag = 1;
		b = Bopen(argv[i], OREAD);
		if(b == 0)
			fprint(2, "grep: can't open %s: %r\n", argv[i]);
		else{
			match |= grep(b, r, argv[i]);
			Bclose(b);
		}
	}
	if(match)
		exits(0);
	exits("no matches");
}

int
grep(Biobuf *b, Reprog *r, char *file)
{
	char *buf;
	int count;
	long line;
	char map[Bsize];

	count = 0;
	line = 0;
    Loop:
	while((buf=Brdline(b, '\n')) != 0){
		line++;
		buf[BLINELEN(b)-1] = 0;
		if(iflag){
			memmove(map, buf, BLINELEN(b));
			lower(map);
		}
		if(regexec(r, iflag? map : buf, 0, 0) ^ negate){
			count++;
			if(lflag==0 && cflag==0 && sflag==0)
				switch((nflag<<1)|hflag){
				case 0:
					print("%s:%s\n", file, buf);
					break;
				case (1<<1)|0:
					print("%s:%d:%s\n", file, line, buf);
					break;
				case (0<<1)|1:
					print("%s\n", buf);
					break;
				case (1<<1)|1:
					print("%d:%s\n", line, buf);
					break;
				}
		}
	}
	if(BLINELEN(b) > 0){	/* line too long; skip and continue */
		Bseek(b, BLINELEN(b), 1);
		goto Loop;
	}
	if(cflag && !sflag){
		if(hflag)
			print("%d\n", count);
		else
			print("%s:%d\n", file, count);
	}
	if(lflag){
		if((count==0) == Lflag)
			print("%s\n", file);
	}
	return count;
}

char*
lower(char *s)
{
	char *t;

		/* we assume the 'A'-'Z' only appear as themselves
		 * in a utf encoding.
		 */
	if(iflag)
		for (t = s; *t; t++)
			*t = tolower(*t);
	return s;
}

void
regerror(char *s)
{
	fprint(2, "grep: %s\n", s);
	exits(s);
}
