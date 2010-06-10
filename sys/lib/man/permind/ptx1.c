/*
	permuted title index
	ptx [-t] [-i ignore] [-o only] [-w num] [-r]
	    [-c commands] [-g gap] [-f] [input]

	Ptx reads the input file and permutes on words in it.
	It excludes all words in the ignore file.
	Alternately it includes words in the only file.
	if neither is given it excludes the words in
	/sys/lib/man/permind/ignore.

	The width of the output line (except for -r field)
	can be changed to num,
	which is a troff width measure, ens by default.
	with no -w, num is 72n, or 100n under -t.
	the -f flag tells the program to fold the output
	the -t flag says the output is for troff
	font specifier -F implies -t.
	-g sets the gutter
	-h sets the hole between wrapped segments
	-r takes the first word on each line and makes it
	into a fifth field.
	-c inserts troff commands for font-setting etc at beginning
 */

#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <ctype.h>

#define DEFLTX "/sys/lib/man/permind/ignore"
#define TILDE	0177		/* actually RUBOUT, not ~ */
#define	N	30
#define	MAX	N*BUFSIZ
#define LMAX	2048
#define MAXT	2048
#define MASK	03777
#define ON	1

#define isabreak(c) (btable[c])

char *getline(void);
void msg(char *, char *);
void extra(int);
void diag(char *, char *);
void cmpline(char *);
int cmpword(char *, char *, char *);
void putline(char *, char *);
void makek(void);
void getsort(void);
char *rtrim(char *, char *, int);
char *ltrim(char *, char *, int);
void putout(char *, char *);
void setlen(void);
void getlen(void);
int hash(char *, char *);
int storeh(int, char *);

int status;

char *hasht[MAXT];
char line[LMAX];
char mark[LMAX];
struct word {
	char *p;
	int w;
} word[LMAX/2];
char btable[256];
int ignore;
int only;
char *lenarg;
char *gutarg;
char *holarg;
int llen;
int spacesl;
int gutter;
int hole;
int mlen = LMAX;
int halflen;
int rflag;
char *strtbufp, *endbufp;


char *empty = "";
char *font = "R";
char *roff = "/bin/nroff";
char *troff = "/bin/troff";

char *infile = "/fd/0";
FILE *inptr;

FILE *outptr = stdout;

char *sortfile = "ptxsort";	/* output of sort program */
char nofold[] = {'-', 'd', 't', TILDE, 0};
char fold[] = {'-', 'd', 'f', 't', TILDE, 0};
char *sortopt = nofold;
FILE *sortptr;

char *kfile = "ptxmark";	/* ptxsort + troff goo for widths */
FILE *kptr;

char *wfile = "ptxwidth";	/* widths of words in ptxsort */
FILE *wptr;

char *bfile;	/*contains user supplied break chars */
FILE *bptr;

char *cmds;

main(int argc, char **argv)
{
	int c;
	char *bufp;
	char *pend;
	char *xfile;
	FILE *xptr;
	Waitmsg *w;

	/* argument decoding */
	xfile = DEFLTX;
	ARGBEGIN {
	case 'r':
		rflag = 1;
		break;
	case 'f':
		sortopt = fold;
		break;
	case 'w':
		if(lenarg)
			extra(ARGC());
		lenarg = ARGF();
		break;
	case 'c':
		if(cmds)
			extra(ARGC());
		cmds = ARGF();
	case 't':
		roff = troff;
		break;
	case 'g':
		if(gutarg)
			extra(ARGC());
		gutarg =  ARGF();
		break;
	case 'h':
		if(holarg)
			extra(ARGC());
		holarg =  ARGF();
		break;

	case 'i':
		if(only|ignore)
			extra(ARGC());
		ignore++;
		xfile = ARGF();
		break;

	case 'o':
		if(only|ignore)
			extra(ARGC());
		only++;
		xfile = ARGF();
		break;

	case 'b':
		if(bfile)
			extra(ARGC());
		bfile = ARGF();
		break;

	default:
		diag("Illegal argument:",*argv);
	} ARGEND

	if(lenarg == 0)
		lenarg = troff? "100n": "72n";
	if(gutarg == 0)
		gutarg = "3n";
	if(holarg == 0)
		holarg = gutarg;

	if(argc > 1)
		diag("Too many filenames",empty);
	if(argc == 1)
		infile = *argv;

	/* Default breaks of blank, tab and newline */
	btable[' '] = ON;
	btable['\t'] = ON;
	btable['\n'] = ON;
	if(bfile) {
		if((bptr = fopen(bfile,"r")) == NULL)
			diag("Cannot open break char file",bfile);

		while((c = getc(bptr)) != EOF)
			btable[c] = ON;
	}

	/*
	Allocate space for a buffer.  If only or ignore file present
	read it into buffer. Else read in default ignore file
	and put resulting words in buffer.
	*/

	if((strtbufp = calloc(N,BUFSIZ)) == NULL)
		diag("Out of memory space",empty);
	bufp = strtbufp;
	endbufp = strtbufp+MAX;

	if((xptr = fopen(xfile,"r")) == NULL)
		diag("Cannot open  file",xfile);

	while(bufp < endbufp && (c = getc(xptr)) != EOF)
		if(isabreak(c)) {
			if(storeh(hash(strtbufp,bufp),strtbufp))
				diag("Too many words",xfile);
			*bufp++ = '\0';
			strtbufp = bufp;
		} else
			*bufp++ = (isupper(c)?tolower(c):c);
	if (bufp >= endbufp)
		diag("Too many words in file",xfile);
	endbufp = --bufp;

	/* open output file for sorting */

	if((sortptr = fopen(sortfile, "w")) == NULL)
		diag("Cannot open output for sorting:",sortfile);

	/*
	get a line of data and compare each word for
	inclusion or exclusion in the sort phase
	*/
	if (infile!=0 && (inptr = fopen(infile,"r")) == NULL)
		diag("Cannot open data: ",infile);
	while((pend = getline()) != NULL)
		cmpline(pend);
	fclose(sortptr);

	if(fork()==0){
		execl("/bin/sort", "sort", sortopt, "+0", "-1", "+1",
			sortfile, "-o", sortfile, 0);
		diag("Sort exec failed","");
	}
	if((w = wait()) == NULL || w->msg[0] != '\0')
		diag("Sort failed","");
	free(w);

	makek();
	if(fork()==0){
		if(dup(create(wfile,OWRITE|OTRUNC,0666),1) == -1)
			diag("Cannot create width file:",wfile);
		execl(roff, roff, "-a", kfile, 0);
		diag("Sort exec failed","");
	}
	if((w = wait()) == NULL || w->msg[0] != '\0')
		diag("Sort failed","");
	free(w);

	getsort();
/*
	remove(sortfile);
	remove(kfile);
 */
	fflush(0);
	_exits(0);
/* I don't know what's wrong with the atexit func... */
/*	exits(0);	*/
}

void
msg(char *s, char *arg)
{
	fprintf(stderr,"ptx: %s %s\n",s,arg);
}

void
extra(int c)
{
	char s[] = "-x.";

	s[1] = c;
	diag("Extra option", s);
}

void
diag(char *s, char *arg)
{
	msg(s,arg);
/*
	remove(sortfile);
	remove(kfile);
*/
	exits(s);
}


char*
getline(void)
{
	int c;
	char *linep;
	char *endlinep;

	endlinep= line + mlen;
	linep = line;
	/* Throw away leading white space */

	while(isspace(c = getc(inptr)))
		;
	if(c==EOF)
		return(0);
	ungetc(c,inptr);
	while((c = getc(inptr)) != EOF)
		switch (c) {
		case '\t':
			if(linep<endlinep)
				*linep++ = ' ';
			break;
		case '\n':
			while(isspace(*--linep))
				;
			*++linep = '\n';
			return(linep);
		default:
			if(linep < endlinep)
				*linep++ = c;
			break;
		}
	return(0);
}

void
cmpline(char *pend)
{
	char *pstrt, *pchar, *cp;
	char **hp;
	int flag;

	pchar = line;
	if(rflag)
		while(pchar < pend && !isspace(*pchar))
			pchar++;
	while(pchar < pend){
		/* eliminate white space */
		if(isabreak(*pchar++))
			continue;
		pstrt = --pchar;

		flag = 1;
		while(flag){
			if(isabreak(*pchar)) {
				hp = &hasht[hash(pstrt,pchar)];
				pchar--;
				while(cp = *hp++){
					if(hp == &hasht[MAXT])
						hp = hasht;
					/* possible match */
					if(cmpword(pstrt,pchar,cp)){
						/* exact match */
						if(!ignore && only)
							putline(pstrt,pend);
						flag = 0;
						break;
					}
				}
				/* no match */
				if(flag){
					if(ignore || !only)
						putline(pstrt,pend);
					flag = 0;
				}
			}
			pchar++;
		}
	}
}

int
cmpword(char *cpp, char *pend, char *hpp)
{
	char c;

	while(*hpp != '\0'){
		c = *cpp++;
		if((isupper(c)?tolower(c):c) != *hpp++)
			return(0);
	}
	if(--cpp == pend)
		return(1);
	return(0);
}

void
putline(char *strt, char *end)
{
	char *cp;

	for(cp=strt; cp<end; cp++)
		putc(*cp, sortptr);
	/* Add extra blank before TILDE to sort correctly with -fd option */
	putc(' ',sortptr);
	putc(TILDE,sortptr);
	for (cp=line; cp<strt; cp++)
		putc(*cp,sortptr);
	putc('\n',sortptr);
}

void
makek(void)
{
	int i, c;
	int nr = 0;

	if((sortptr = fopen(sortfile,"r")) == NULL)
		diag("Cannot open sorted data:",sortfile);
	if((kptr = fopen(kfile,"w")) == NULL)
		diag("Cannot create mark file:",kfile);
	if(cmds)
		fprintf(kptr,"%s\n",cmds);
	fprintf(kptr,
		".nf\n"
		".pl 1\n"
		".tr %c\\&\n", TILDE);
	setlen();

	while((c = getc(sortptr)) != EOF) {
		if(nr == 0) {
			fprintf(kptr,".di xx\n");
			nr++;
		}
		if(c == '\n') {
			fprintf(kptr,"\n.di\n");
			for(i=1; i<nr; i++)
				fprintf(kptr,"\\n(%.2d ",i);
			fprintf(kptr,"\n");
			nr = 0;
			continue;
		}
		if(isspace(c))
			fprintf(kptr,"\\k(%.2d",nr++);
		putc(c,kptr);
	}
	fclose(sortptr);
	fclose(kptr);
}

void
getsort(void)
{
	char *tilde, *linep, *markp;
	int i0, i1, i2, i3, i4, i5, i6, i7, w0, w6;

	if((sortptr = fopen(sortfile, "r")) == NULL)
		diag("Cannot open sorted data:", sortfile);
	if((wptr = fopen(wfile, "r")) == NULL)
		diag("Cannot open width file:", wfile);
	getlen();

	halflen = (llen-gutter)/2;

	while(fgets(line, sizeof(line), sortptr) != NULL) {
		if(fgets(mark, sizeof(mark), wptr) == NULL)
			diag("Phase error 1: premature EOF on width file",
				wfile);
		linep = line;
		markp = mark;
		i3 = i7 = 0;
		word[i7].p = linep;
		word[i7].w = 0;
		for(linep=line; *linep; linep++) {
			if(*linep == TILDE)
				i3 = i7;
			else if(*linep == '\n')
				break;
			else if(isspace(*linep)) {
				i7++;
				word[i7].p = linep;
				if(!markp)
					diag("Phase error 2: no widths for summary",
						line);
				word[i7].w = atoi(markp);
				markp = strchr(markp+1, ' ');
			}
		}
		i0 = 0;
		for(i1=i0; i1<i3; i1++)
			if(word[i1+1].w - word[i0].w >= halflen - spacesl)
				break;
		w0 = word[i1].w - word[i0].w;
		i4 = i3 + rflag;
		for(i6 = i7; i6>i4; i6--)
			if(word[i7].w - word[i6-1].w >= halflen)
				break;
		w6 = word[i7].w - word[i6].w - spacesl;
		for(i2=i1 ; i2<i3; i2++)
			if(word[i2+1].w - word[i1].w + w6 >= halflen-hole)
				break;
		for(i5=i6; i5>i4; i5--)
			if(word[i6].w - word[i5-1].w + w0 >= halflen-hole)
				break;

		printf(".xx \"");
		putout(word[i1].p+1,word[i2].p);
		if(i1<i2 && i2<i3) putchar('/');
		printf("\" \"");
		if(i5>i4 && i6==i5) putchar('/');
		putout(word[i6].p+1+(i6==i3),word[i7].p);
		printf("\" \"");
		putout(word[i0].p,word[i1].p);
		if(i2<i3 && i1==i2) putchar('/');
		printf("\" \"");
		if(i5>i4 && i6>i5) putchar('/');
		putout(word[i5].p+1+(i5==i3),word[i6].p);
		if(rflag) {
			printf("\" \"");
			putout(word[i3].p+2,word[i4].p);
		}
		printf("\"\n");
	}
}

void
putout(char *strt, char *end)
{
	char *cp;

	for(cp=strt; cp<end; )
		putc(*cp++,outptr);
}

void
setlen(void)
{
	fprintf(kptr,
		"\\w'\\h'%s''\n"
		"\\w' /'\n"
		"\\w'\\h'%s''\n"
		"\\w'\\h'%s''\n",lenarg,gutarg,holarg);
}

void
getlen(void)
{
	char s[128];

	s[0] = '\0';
	fgets(s,sizeof(s),kptr);
	llen = atoi(s);

	fgets(s,sizeof(s),kptr);
	spacesl = atoi(s);

	fgets(s,sizeof(s),kptr);
	gutter = atoi(s);

	fgets(s,sizeof(s),kptr);
	hole = atoi(s);
	if(hole < 2*spacesl)
		hole = 2*spacesl;
}

int
hash(char *strtp, char *endp)
{
	char *cp, c;
	int i, j, k;

	/* Return zero hash number for single letter words */
	if((endp - strtp) == 1)
		return(0);

	cp = strtp;
	c = *cp++;
	i = (isupper(c)?tolower(c):c);
	c = *cp;
	j = (isupper(c)?tolower(c):c);
	i = i*j;
	cp = --endp;
	c = *cp--;
	k = (isupper(c)?tolower(c):c);
	c = *cp;
	j = (isupper(c)?tolower(c):c);
	j = k*j;
	return (i ^ (j>>2)) & MASK;
}

int
storeh(int num, char *strtp)
{
	int i;

	for(i=num; i<MAXT; i++)
		if(hasht[i] == 0) {
			hasht[i] = strtp;
			return(0);
		}
	for(i=0; i<num; i++)
		if(hasht[i] == 0) {
			hasht[i] = strtp;
			return(0);
		}
	return(1);
}
