/*
 * mc - columnate
 *
 * mc[-][-LINEWIDTH][-t][file...]
 *	- causes break on colon
 *	-LINEWIDTH sets width of line in which to columnate(default 80)
 *	-t suppresses expanding multiple blanks into tabs
 *
 */
#include	<u.h>
#include	<libc.h>
#include	<libg.h>
#include	<bio.h>

#define WIDTH			80
#define WORD_ALLOC_QUANTA	1024
#define ALLOC_QUANTA		4096

int linewidth=WIDTH;
int colonflag=0;
int tabflag=0;	/* -t flag turned off forever */
Rune *cbuf, *cbufp;
Rune **word;
int maxwidth=0;
int nalloc=ALLOC_QUANTA;
int nwalloc=WORD_ALLOC_QUANTA;
int nchars=0;
int nwords=0;
Biobuf	bin;
Biobuf	bout;

void getwidth(void), readbuf(int), error(char *);
void scanwords(void), columnate(void), morechars(void);

void
main(int argc, char *argv[])
{
	int i;
	int lineset;
	int ifd;

	lineset = 0;
	Binit(&bout, 1, OWRITE);
	while(argc > 1 && argv[1][0] == '-'){
		--argc; argv++;
		switch(argv[0][1]){
		case '\0':
			colonflag = 1;
			break;
		case 't':
			tabflag = 0;
			break;
		default:
			linewidth = atoi(&argv[0][1]);
			if(linewidth <= 1)
				linewidth = WIDTH;
			lineset = 1;
			break;
		}
	}
	if(lineset == 0)
		getwidth();
	cbuf = cbufp = malloc(ALLOC_QUANTA*(sizeof *cbuf));
	word = malloc(WORD_ALLOC_QUANTA*(sizeof *word));
	if(word == 0 || cbuf == 0)
		error("out of memory");
	if(argc == 1)
		readbuf(0);
	else{
		for(i = 1; i < argc; i++){
			if((ifd = open(*++argv, OREAD)) == -1)
				fprint(2, "mc: can't open %s (%r)\n", *argv);
			else{
				readbuf(ifd);
				Bflush(&bin);
				close(ifd);
			}
		}
	}
	columnate();
	exits(0);
}
void
error(char *s)
{
	fprint(2, "mc: %s\n", s);
	exits(s);
}
void
readbuf(int fd)
{
	int lastwascolon = 0;
	long c;
	int linesiz = 0;

	Binit(&bin, fd, OREAD);
	do{
		if(nchars++ >= nalloc)
			morechars();
		*cbufp++ = c = Bgetrune(&bin);
		linesiz++;
		if(c == '\t') {
			cbufp[-1] = L' ';
			while(linesiz%8 != 0) {
				if(nchars++ >= nalloc)
					morechars();
				*cbufp++ = L' ';
				linesiz++;
			}
		}
		if(colonflag && c == ':')
			lastwascolon++;
		else if(lastwascolon){
			if(c == '\n'){
				--nchars; 	/* skip newline */
				*cbufp = L'\0';
				while(nchars > 0 && cbuf[--nchars] != '\n')
					;
				if(nchars)
					nchars++;
				columnate();
				if (nchars)
					BPUTC(&bout, '\n');
				Bprint(&bout, "%S", cbuf+nchars);
				nchars = 0;
				cbufp = cbuf;
			}
			lastwascolon = 0;
		}
		if(c == '\n')
			linesiz = 0;
	}while(c >= 0);
}
void
scanwords(void)
{
	Rune *p, *q;
	int i;

	nwords=0;
	maxwidth=0;
	for(p = q = cbuf, i = 0; i < nchars; i++){
		if(*p++ == L'\n'){
			if(nwords >= nwalloc){
				nwalloc += WORD_ALLOC_QUANTA;
				if((word = realloc(word, nwalloc*sizeof(*word)))==0)
					error("out of memory");
			}
			word[nwords++] = q;
			p[-1] = L'\0';
			if(p-q > maxwidth)
				maxwidth = p-q;
			q = p;
		}
	}
}

void
columnate(void)
{
	int i, j;
	int words_per_line;
	int nlines;
	int col;
	int endcol;


	scanwords();
	if(nwords==0)
		return;
	words_per_line = linewidth/maxwidth;
	if(words_per_line <= 0)
		words_per_line = 1;
	nlines=(nwords+words_per_line-1)/words_per_line;
	for(i = 0; i < nlines; i++){
		col = endcol = 0;
		for(j = i; j < nwords; j += nlines){
			endcol += maxwidth;
			Bprint(&bout, "%S", word[j]);
			col += word[j+1]-word[j]-1;
			if(j+nlines < nwords){
				if(tabflag) {
					int tabcol = (col|07)+1;
					while(tabcol <= endcol){
						BPUTC(&bout, '\t');
						col = tabcol;
						tabcol += 8;
					}
				}
				while(col < endcol){
					BPUTC(&bout, ' ');
					col++;
				}
			}
		}
		BPUTC(&bout, '\n');
	}
}

void
morechars(void)
{
	nalloc += ALLOC_QUANTA;
	if((cbuf = realloc(cbuf, nalloc*sizeof(*cbuf))) == 0)
		error("out of memory");
	cbufp = cbuf+nchars-1;
}

void
getwidth(void)
{
	Rectangle r;
	char buf[5*12];
	int fd, width;

		/* window stucture:
			4 bit left edge
			1 bit gap
			12 bit scrollbar
			4 bit gap
			text
			4 bit right edge
		*/
	fd = open("/dev/window", OREAD);
	if(fd < 0)
		fd = open("/dev/screen", OREAD);
	if(fd < 0)
		return;
	if(read(fd, buf, sizeof buf) != sizeof buf){
		close(fd);
		return;
	}
	close(fd);
	r.min.x = atoi(buf+1*12);
	r.min.y = atoi(buf+2*12);
	r.max.x = atoi(buf+3*12);
	r.max.y = atoi(buf+4*12);
	fd = open("/dev/bitblt", ORDWR);
	if(fd < 0)
		width = 9;
	else{
		close(fd);
		binit(0, 0, 0);
		width = charwidth(font, ' ');
		bclose();
	}
	linewidth = (r.max.x-r.min.x-(4+1+12+4+4))/width;
}
