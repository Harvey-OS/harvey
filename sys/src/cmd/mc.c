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
#include	<draw.h>
#include	<bio.h>

#define	WIDTH			80
#define	TAB	4
#define	WORD_ALLOC_QUANTA	1024
#define	ALLOC_QUANTA		4096

int linewidth=WIDTH;
int mintab=1;
int colonflag=0;
int tabflag=0;	/* -t flag turned off forever */
Rune *cbuf, *cbufp;
Rune **word;
int maxwidth=0;
int nalloc=ALLOC_QUANTA;
int nwalloc=WORD_ALLOC_QUANTA;
int nchars=0;
int nwords=0;
int tabwidth=0;
Font *font;
Biobuf	bin;
Biobuf	bout;

void getwidth(void), readbuf(int), error(char *);
void scanwords(void), columnate(void), morechars(void);
int wordwidth(Rune*, int);
int nexttab(int);

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
	if(lineset == 0){
		getwidth();
		if(linewidth <= 1){
			linewidth = WIDTH;
			font = nil;
		}
	}

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
			while(linesiz%TAB != 0) {
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
					Bputc(&bout, '\n');
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
	int i, w;

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
			w = wordwidth(q, p-q-1);
			if(w > maxwidth)
				maxwidth = w;
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
	maxwidth = nexttab(maxwidth+mintab-1);
	words_per_line = linewidth/maxwidth;
	if(words_per_line <= 0)
		words_per_line = 1;
	nlines=(nwords+words_per_line-1)/words_per_line;
	for(i = 0; i < nlines; i++){
		col = endcol = 0;
		for(j = i; j < nwords; j += nlines){
			endcol += maxwidth;
			Bprint(&bout, "%S", word[j]);
			col += wordwidth(word[j], runestrlen(word[j]));
			if(j+nlines < nwords){
				if(tabflag) {
					while(col < endcol){
						Bputc(&bout, '\t');
						col = nexttab(col);
					}
				}else{
					while(col < endcol){
						Bputc(&bout, ' ');
						col++;
					}
				}
			}
		}
		Bputc(&bout, '\n');
	}
}

int
wordwidth(Rune *w, int nw)
{
	if(font)
		return runestringnwidth(font, w, nw);
	return nw;
}

int
nexttab(int col)
{
	if(tabwidth){
		col += tabwidth;
		col -= col%tabwidth;
		return col;
	}
	return col+1;
}

void
morechars(void)
{
	nalloc += ALLOC_QUANTA;
	if((cbuf = realloc(cbuf, nalloc*sizeof(*cbuf))) == 0)
		error("out of memory");
	cbufp = cbuf+nchars-1;
}

/*
 * These routines discover the width of the display.
 * It takes some work.  If we do the easy calls to the
 * draw library, the screen flashes due to repainting
 * when mc exits.
 */

jmp_buf	drawjmp;

void
terror(Display*, char*)
{
	longjmp(drawjmp, 1);
}

void
getwidth(void)
{
	int n, fd;
	char buf[128], *f[10], *p;

	if(access("/dev/acme", OREAD) >= 0){
		if((fd = open("/dev/acme/ctl", OREAD)) < 0)
			return;
		n = read(fd, buf, sizeof buf-1);
		close(fd);
		if(n <= 0)
			return;
		buf[n] = 0;
		n = tokenize(buf, f, nelem(f));
		if(n < 7)
			return;
		if((font = openfont(nil, f[6])) == nil)
			return;
		if(n >= 8)
			tabwidth = atoi(f[7]);
		else
			tabwidth = 4*stringwidth(font, "0");
		mintab = stringwidth(font, "0");
		linewidth = atoi(f[5]);
		tabflag = 1;
		return;
	}

	if((p = getenv("font")) == nil)
		return;
	if((font = openfont(nil, p)) == nil)
		return;
	if((fd = open("/dev/window", OREAD)) < 0){
		font = nil;
		return;
	}
	n = read(fd, buf, 5*12);
	close(fd);
	if(n < 5*12){
		font = nil;
		return;
	}
	buf[n] = 0;
	
	/* window stucture:
		4 bit left edge
		1 bit gap
		12 bit scrollbar
		4 bit gap
		text
		4 bit right edge
	*/
	linewidth = atoi(buf+3*12) - atoi(buf+1*12) - (4+1+12+4+4);
	mintab = stringwidth(font, "0");
	if((p = getenv("tabstop")) != nil)
		tabwidth = atoi(p)*stringwidth(font, "0");
	if(tabwidth == 0)
		tabwidth = 4*stringwidth(font, "0");
	tabflag = 1;
}
