%start file
%token NRAD NSTR SINGLETON THREAD
%{
#include <u.h>
#include <libc.h>
#include <bio.h>

typedef struct Rset	Rset;

struct Rset
{
	ushort	index;
	ushort	size;
	ushort	strokes[1];
};

int	nrsets;
Rset **	rsets;
Rset **	rpp;

#define	Sflag	(1<<16)

extern void	yyerror(char*);
extern int	yyparse(void);
extern int	yylval;

void		mktable(void);
void		showrset(Rset*);

int	debug;
int	pflag;
int	yylineno;
int	yyeof;
char *	yyfile;
Biobuf *yyin;

int	lookrad;
char *	line;

int	thread;
int	topbits;
int	scount;

int	nrad;
int	radical;
int	ns;
int	strsize;
int *	str;

Biobuf *out;

void
main(int argc, char **argv)
{
	int i;

	ARGBEGIN{
	case 'p':
		++pflag;
		break;
	case 'D':
		++debug;
		break;
	}ARGEND
	if(argc > 0)
		yyfile = argv[0];
	else
		yyfile = "/fd/0";
	yyin = Bopen(yyfile, OREAD);
	if(yyin == 0){
		fprint(2, "%s: can't open %s: %r\n", argv0, yyfile);
		exits("open");
	}
	yylineno = 1;
	yyeof = 0;
	lookrad = 1;
	yyparse();
	fprint(2, "radical %d %.4ux, thread %.4x\n", nrad, radical, thread);
	fprint(2, "%d radical sets\n", nrsets);
	out = Bopen("/fd/1", OWRITE);
	if(pflag){
		for(i=0; i<nrsets; i++)
			showrset(rsets[i]);
	}else
		mktable();
	exits(0);
}

void
mktable(void)
{
	int i, j, k;
	Rset *rp;
	int *addr;

	addr = malloc(nrsets*sizeof(int));
	k = 0;

	Bprint(out, "#include <u.h>\n\n");
	Bprint(out, "static ushort rsd[] = {\n");
	for(i=0; i<nrsets; i++){
		rp = rsets[i];
		addr[i] = k;
		k += rp->size+2;
		Bprint(out, "\t%d, %d,\n", rp->index, rp->size);
		for(j=0; j<rp->size; j++){
			if(j%10 == 0)
				Bprint(out, "\t");
			else
				Bprint(out, " ");
			if(rp->strokes[j] < 0x4000)
				Bprint(out, "%d,", rp->strokes[j]);
			else
				Bprint(out, "0x%x,", rp->strokes[j]);
			if(j%10 == 9)
				Bprint(out, "\n");
		}
		if(j%10 != 0)
			Bprint(out, "\n");
	}
	Bprint(out, "};\n\n");
	Bprint(out, "typedef struct Rset\tRset;\n\n");
	Bprint(out, "#define\tA(x)\t(Rset *)(rsd+x)\n\n");
	Bprint(out, "Rset *rsets[] = {\n");
	for(i=0; i<nrsets; i++){
		if(i%10 == 0)
			Bprint(out, "\t");
		else
			Bprint(out, " ");
		Bprint(out, "A(%d),", addr[i]);
		if(i%10 == 9)
			Bprint(out, "\n");
	}
	if(i%10 != 0)
		Bprint(out, "\n");
	Bprint(out, "};\n\n");
	Bprint(out, "int nrsets = %d;\n", nrsets);
}

void
showrset(Rset *rp)
{
	int i = 0, j, s = 0;

	Bprint(out, "%d:", rp->index);
	while(i<rp->size){
		s = rp->strokes[i++];
		if(s <= 0xff)
			break;
		Bprint(out, " %.4ux %C", s, s);
	}
	Bprint(out, "\n");
	while(i<rp->size){
		Bprint(out, "%6d", s);
		j = 0;
		while(i<rp->size){
			s = rp->strokes[i++];
			if(s <= 0xff)
				break;
			Bprint(out, "%c%.4ux %C", (j==0 ? '\t' : ' '), s, s);
			if(j == 9){
				j = 0;
				Bprint(out, "\n");
			}else
				j++;
		}
		if(j != 0)
			Bprint(out, "\n");
	}
}

int
yylex(void)
{
	int c;
	char *p;

	if(yyeof)
		return -1;
	if(line == 0){
		line = Brdline(yyin, '\n');
		if(line == 0){
			++yyeof;
			return -1;
		}
	}
	c = *line;
	switch(c){
	case '\n':
		++yylineno;
		line = 0;
		return '\n';
	case '-':
		yylval = strtol(++line, &line, 10);
		return NSTR;
	case '^':
		yylval = strtol(++line, &p, 16);
		if(p-line != 4){
			yyerror("bad ^");
			exits("^");
		}
		line = p;
		return THREAD;
	}
	if(lookrad){
		if('0' <= c && c <= '9'){
			yylval = strtol(line, &line, 10);
			lookrad = 0;
			return NRAD;
		}
		yyerror("bad NRAD");
		exits("NRAD");
	}
	if('0' <= c && c <= '9' || 'a' <= c && c <= 'f'){
		yylval = strtol(line, &p, 16);
		switch(p-line){
		case 2:
			line = p;
			return THREAD;
		case 4:
			line = p;
			return SINGLETON;
		}
		yyerror("bad THREAD/SINGLETON");
		exits("T/S");
	}
	++line;
	return (c == ' ') ? '_' : c;
}

void
yyerror(char *p)
{
	fprint(2, "%s.%d: %s\n", yyfile, yylineno, p);
}

void
startrad(int n, int code, int flag)
{
	if(n < nrad){
		yyerror("decreasing nrad");
		exits("nrad");
	}
	if(code <= thread && !flag){
		yyerror("non-increasing thread");
		exits("thread");
	}
	nrad = n;
	radical = code;
	if(!flag){
		thread = code;
		topbits = thread & 0xff00;
	}
	scount = 0;
	++nrsets;
	rsets = realloc(rsets, nrsets*sizeof(Rset *));
	rpp = rsets+nrsets-1;
	*rpp = malloc(sizeof(Rset));
	(*rpp)->index = nrad;
	(*rpp)->size = 1;
	(*rpp)->strokes[0] = radical;
}

void
startstr(int n)
{
	if(n <= scount && !(n == scount && n == 0)){
		yyerror("non-increasing stroke count");
		exits("count");
	}
	ns = 0;
	scount = n;
}

void
incrset(int x)
{
	int size;

	size = (*rpp)->size;
	*rpp = realloc(*rpp, sizeof(Rset)+size*sizeof(ushort));
	(*rpp)->size = size+1;
	(*rpp)->strokes[size] = x;
}

void
dostrset(void)
{
	int i, j, k;

	if(debug)
		print("%d %.4ux %d:", nrad, radical, scount);
	if(scount != 0)
		incrset(scount);
	for(i=0; i<ns; i++){
		k = str[i];
		if(k&Sflag){
			k &= ~Sflag;
			if(k != radical){
				if(debug)
					print(" (%.4ux)", k);
				incrset(k);
			}
		}else if(i+1<ns && !(str[i+1]&Sflag)){
			++i;
			if(debug)
				print(" %.4ux-%.4ux", k, str[i]);
			for(j=k; j<=str[i]; j++)
				incrset(j);
		}else{
			if(debug)
				print(" %.4ux", k);
			incrset(k);
		}
	}
	if(debug)
		print("\n");
}

void
dostr(int code)
{
	if(ns >= strsize)
		str = realloc(str, (strsize+=32)*sizeof(int));
	if(!(code & Sflag)){
		if(!(code & 0xff00))
			code |= topbits;
		else
			topbits = code & 0xff00;
		if(code <= thread){
			yyerror("non-increasing thread");
			exits("thread");
		}
		thread = code;
	}
	str[ns++] = code;
}
%}
%%
file: /* empty */
	| file radical stroke-groups '\n'	{ lookrad = 1; }
	;

radical: NRAD '\t' opt-twiddle SINGLETON '\n'	{ startrad($1, $4, $3); }
	;

opt-twiddle: /* empty */	{ $$ = 0; }
	| '~'			{ $$ = 1; }
	;

stroke-groups: stroke-set
	| stroke-groups stroke-set
	;

stroke-set: '\t' opt-count stroke-specs '\n'	{ dostrset(); }
	;

opt-count: /* empty */		{ startstr(scount+1); }
	| NSTR '_'		{ startstr($1); }
	;

stroke-specs:	stroke-spec
	| stroke-specs '_' stroke-spec
	;

stroke-spec: THREAD	{ dostr($$); }
	| SINGLETON	{ dostr(Sflag|$$); }
	;
