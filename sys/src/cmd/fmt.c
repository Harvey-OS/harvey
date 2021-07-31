#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>

/*
 * block up paragraphs, possibly with indentation
 */

void	fmt(Biobuf*);
void	outchar(Rune);
void	addrune(Rune);
void	outword(void);
void	puncttest(void);
void	flush(void);
void nextline(void);

int join = 1;			/* can input lines be joined? */
int indent = 0;			/* how many spaces to indent */
int tindent = 0;		/* temporary extra indent */
int length = 70;		/* how many columns per output line */
int maxtab = 8;
Biobuf bin;
Biobuf bout;

void
usage(void)
{
	fprint(2, "usage: %s [-j] [-i indent] [-l length] [file...]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int i, f;
	char *s, *err;

	ARGBEGIN{
	case 'i':
		indent = atoi(EARGF(usage()));
		break;
	case 'j':
		join = 0;
		break;
	case 'w':
	case 'l':
		length = atoi(EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND

	if(length <= indent){
		fprint(2, "%s: line length<=indentation\n", argv0);
		exits("length");
	}

	s=getenv("tabstop");
	if(s!=nil && atoi(s)>0)
		maxtab=atoi(s);
	err = nil;
	Binit(&bout, 1, OWRITE);
	if(argc <= 0){
		Binit(&bin, 0, OREAD);
		fmt(&bin);
	}else{
		for(i=0; i<argc; i++){
			f = open(argv[i], OREAD);
			if(f < 0){
				fprint(2, "%s: can't open %s: %r\n", argv0, argv[i]);
				err = "open";
			}else{
				Binit(&bin, f, OREAD);
				fmt(&bin);
				Bterm(&bin);
				if(i != argc-1)
					Bputc(&bout, '\n');
			}
		}
	}
	exits(err);
}
void
fmt(Biobuf *f)
{
	long c;

	while((c = Bgetrune(f)) >= 0)
		outchar((Rune) c);
	flush();
}

#define	NWORD	(maxtab*32)

Rune *word;
int mword;
int wp;

int col = 0;		/* output column number */
int bol = 1;		/* at beginning of output line? */
int punct = 0;		/* last character out was punctuation? */
int newline = 1;	/* last char read was newline */
int nspace = 0;		/* number of spaces beginning this line */

int
addspace(int nspace, Rune c)
{
	if(c == ' ')
		return nspace+1;
	/* else tab */
	nspace = ((nspace/maxtab)+1)*maxtab;
	return nspace;
}

void
outchar(Rune c)
{
	switch(c){
	case '\0':
		break;
	case '\n':
		switch(newline){
		case 0:
			if(join)
				outword();
			else
				flush();
			break;
		case 1:
			flush();
		case 2:
			tindent = 0;
			Bputc(&bout, '\n');
			wp = 0;
		}
		newline = 1;
		break;
	case ' ':
	case '\t':
		switch(newline) {
		case 0:
			outword();
			break;
		case 1:
			nspace = addspace(nspace, c);
			outword();
			break;
		}
		break;
	default:
		if(nspace > 0){
			if(!newline && tindent != nspace){
				nextline();
				newline = 1;
			}
			tindent = nspace;
			nspace = 0;
		}else
			if(newline){
				if(tindent)
					nextline();
				tindent = 0;
			}
		addrune(c);
		newline = 0;
	}
}

void
addrune(Rune c)
{
	if(wp==mword) {
		if(mword == 0)
			mword = 10;
		else
			mword *= 2;
		word = realloc(word, sizeof(Rune)*mword);
		if(word == nil) {
			fprint(2, "fmt: out of memory\n");
			exits("out of memory");
		}
	}
	word[wp++] = c;
}

void
outword(void)
{
	int i;

	if(wp == 0)
		return;
	if(wp+col+(bol?0:punct?2:1) > length){
		Bputc(&bout, '\n');
		col = 0;
		bol = 1;
	}
	if(col == 0){
		for(i=0; i+maxtab <= indent+tindent; i+=maxtab)
			Bputc(&bout, '\t');
		while(i++ < indent+tindent)
			Bputc(&bout, ' ');
		col = indent+tindent;
	}
	if(bol)
		bol = 0;
	else{
		if(punct){
			Bputc(&bout, ' ');
			col++;
		}
		Bputc(&bout, ' ');
		col++;
	}
	puncttest();
	for (i = 0; i < wp; i++)
		Bputrune(&bout, word[i]);
	col += i;
	wp = 0;
}

/* is the word followed by major punctuation, .?:! */
/* disregard short things followed by periods; they are probably
   initials or titles like Mrs. and Dr. */
void
puncttest(void)
{
	int rp;

	punct = 0;
	for(rp=wp; --rp>=0; ) {
		switch(word[rp]) {
		case ')': case '\'': case '"':
			continue;
		case '.':
			if(isupper(*word)&&rp<=3)
				return;
		case '?': case '!': /*case ':':*/
			punct = 1;
		default:
			return;
		}
	}
}
void
nextline(void)
{
	Bputc(&bout, '\n');
	col = 0;
	bol = 1;
}
void
flush(void)
{
	outword();
	if(col != 0)
		nextline();
}
