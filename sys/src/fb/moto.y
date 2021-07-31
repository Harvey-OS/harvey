/*% yacc % && cc y.tab.c -go #
 */
%term<tre>	DCOLON BEGIN END ID NUMBER TEXT CMDLIST
%term<typ>	EXPR CMD LERP2 LERP3 MOVIE RANGE FILENAME SUBSHELL
%term<typ>	'^' '=' '+' '-' '*' '/' '%' ',' ';' '?' ':' '!' '$'
%term<typ>	AND OR LT LE EQ NE GT GE
%term<typ>	UMINUS FABS FLOOR CEIL SQRT HYPOT
%term<typ>	SIN COS TAN ASIN ACOS ATAN ATAN2 EXP LOG LOG10
%term<typ>	SINH COSH TANH GAMMA
%type<tre>	movie group range elist e ee cmd
%right	'='
%right	'?' ':'
%left	OR
%left	AND
%binary	LT LE EQ NE GT GE
%left	'+' '-'
%left	'*' '/' '%'
%left	'[' ']'
%left	'^'
%left	UMINUS '!' '$'
%{
#include <u.h>
#include <libc.h>
int lexstate=EXPR;
struct sym{
	char *name;
	double value;
};
typedef struct Tree Tree;
struct Tree{
	int type;
	Tree *left, *right;
	double number;
	struct sym *id;
	char *text;
};
Tree *tree(int, Tree *, Tree *);
Tree *idtree(void);
Tree *texttree(void);
Tree *numtree(void);
Tree *movie;
%}
%union{
	int typ;
	Tree *tre;
}
%%
movie:					{$$=NULL;}
|	movie group			{$$=movie=tree(MOVIE, $1, $2);}
group:	range ':' {lexstate=CMD;} cmd '\n'
					{lexstate=EXPR; $$=tree(CMD, $1, $4);}
|	range '@' {lexstate=CMD;} cmd '\n'
					{lexstate=EXPR; $$=tree(FILENAME, $1, $4);}
|	range '!' {lexstate=CMD;} cmd '\n'
					{lexstate=EXPR; $$=tree(SUBSHELL, $1, $4);}
|	range DCOLON elist '\n'		{$$=tree(EXPR, $1, $3);}
|	error '\n' 			{lexstate=EXPR; $$=NULL;}
range:	BEGIN				{$$=tree(BEGIN, NULL, NULL);}
|	END				{$$=tree(END, NULL, NULL);}
|	e				{$$=tree(RANGE, $1, NULL);}
|	e ',' e				{$$=tree(RANGE, $1, $3);}
elist:	e
|	elist ';' e			{$$=tree($2, $1, $3);}
e:	ID
|	ID '=' e			{$$=tree($2, $1, $3);}
|	NUMBER
|	'$' e				{$$=tree($1, $2, NULL);}
|	'(' e ')'			{$$=$2;}
|	'[' e ',' e ']'			{$$=tree(LERP2, $2, $4);}
|	e '[' e ',' e ']'		{$$=tree(LERP3, $1, tree(LERP2, $3, $5));}
|	e '^' e				{$$=tree($2, $1, $3);}
|	e '*' '*' e	%prec '^'	{$$=tree('^', $1, $4);}
|	e '*' e				{$$=tree($2, $1, $3);}
|	e '/' e				{$$=tree($2, $1, $3);}
|	e '%' e				{$$=tree($2, $1, $3);}
|	e '+' e				{$$=tree($2, $1, $3);}
|	e '-' e				{$$=tree($2, $1, $3);}
|	'-' e		%prec UMINUS	{$$=tree(UMINUS, $2, NULL);}
|	e LT e				{$$=tree($2, $1, $3);}
|	e LE e				{$$=tree($2, $1, $3);}
|	e EQ e				{$$=tree($2, $1, $3);}
|	e NE e				{$$=tree($2, $1, $3);}
|	e GT e				{$$=tree($2, $1, $3);}
|	e GE e				{$$=tree($2, $1, $3);}
|	e '?' e ':' e			{$$=tree($2, $1, tree($4, $3, $5));}
|	'!' e				{$$=tree($1, $2, NULL);}
|	e AND e				{$$=tree($2, $1, $3);}
|	e OR e				{$$=tree($2, $1, $3);}
|	FABS '(' e ')'			{$$=tree($1, $3, NULL);}
|	FLOOR '(' e ')'			{$$=tree($1, $3, NULL);}
|	CEIL '(' e ')'			{$$=tree($1, $3, NULL);}
|	SQRT '(' e ')'			{$$=tree($1, $3, NULL);}
|	HYPOT '(' e ',' e ')'		{$$=tree($1, $3, $5);}
|	HYPOT '(' e ',' e ',' e ')'	{$$=tree($1, $3, tree($1, $5, $7));}
|	SIN '(' e ')'			{$$=tree($1, $3, NULL);}
|	COS '(' e ')'			{$$=tree($1, $3, NULL);}
|	TAN '(' e ')'			{$$=tree($1, $3, NULL);}
|	ASIN '(' e ')'			{$$=tree($1, $3, NULL);}
|	ACOS '(' e ')'			{$$=tree($1, $3, NULL);}
|	ATAN '(' e ')'			{$$=tree($1, $3, NULL);}
|	ATAN '(' e ',' e ')'		{$$=tree($1, $3, $5);}
|	EXP '(' e ')'			{$$=tree($1, $3, NULL);}
|	LOG '(' e ')'			{$$=tree($1, $3, NULL);}
|	LOG10 '(' e ')'			{$$=tree($1, $3, NULL);}
|	SINH '(' e ')'			{$$=tree($1, $3, NULL);}
|	COSH '(' e ')'			{$$=tree($1, $3, NULL);}
|	TANH '(' e ')'			{$$=tree($1, $3, NULL);}
|	GAMMA '(' e ')'			{$$=tree($1, $3, NULL);}
cmd:					{$$=NULL;}
|	cmd TEXT			{$$=tree(CMDLIST, $1, $2);}
|	cmd '[' {lexstate=EXPR;} ee ']'	{lexstate=CMD; $$=tree(CMDLIST, $1, $4);}
ee:	e
|	e ',' e				{$$=tree(LERP2, $1, $3);}
%%
#include <stdio.h>
char token[4096];
int nlpending=0;
int peekc = EOF;
int line=1;
FILE *ifd;
char *infile;
char **dolv;
int dolc;
struct res{
	char *name;
	int typ;
}res[]={
	"BEGIN",	BEGIN,
	"END",		END,
	"fabs",		FABS,
	"floor",	FLOOR,
	"ceil",		CEIL,
	"sqrt",		SQRT,
	"hypot",	HYPOT,
	"sin",		SIN,
	"cos",		COS,
	"tan",		TAN,
	"asin",		ASIN,
	"acos",		ACOS,
	"atan",		ATAN,
	"atan2",	ATAN,
	"exp",		EXP,
	"log",		LOG,
	"log10",	LOG10,
	"sinh",		SINH,
	"cosh",		COSH,
	"tanh",		TANH,
	"gamma",	GAMMA,
	NULL
};
int nextc(void){
	int c;
	if(peekc==EOF){
		c=getc(ifd);
		if(c=='\n')
			line++;
		return(c);
	}
	c=peekc;
	peekc=EOF;
	return(c);
}
int see(int look){
	int c=nextc();
	if(c==look){
		token[1]=c;
		return(1);
	}
	peekc=c;
	return(0);
}
int letter(int c){
	return('a'<=c && c<='z' || 'A'<=c && c<='Z' || c=='_');
}
int digit(int c){
	return('0'<=c && c<='9');
}
int yylex(void){
	int c, dot;
	char *s=token;
	struct res *resp;
	switch(lexstate){
	case CMD:		/* only recognize TEXT, '[', '\n', EOF */
		if(nlpending){
			strcpy(token, "\n");
			nlpending=0;
			return('\n');
		}
		c=nextc();
		if(c=='['){
			strcpy(token, "[");
			return(c);
		}
		if(c==EOF){
			strcpy(token, "EOF");
			return(c);
		}
		s=token;
		while(c!=EOF && c!='['){
			*s++=c;
			if(c=='\n'){
				c=nextc();
				if(c!=' ' && c!='\t' && c!='\n'){
					nlpending++;
					break;
				}
				*s++=c;
			}
			c=nextc();
		}
		*s='\0';
		peekc=c;
		yylval.tre=texttree();
		return TEXT;
	case EXPR:
		do
			c=nextc();
		while(c==' ' || c=='\t');
		if(c=='#'){
			do
				c=nextc();
			while(c!=EOF && c!='\n');
		}
		if(c==EOF){
			strcpy(token, "EOF");
			return c;
		}
		if(c=='\n'){
			peekc=nextc();
			if(peekc=='\n' || peekc==' ' || peekc=='\t'){
				strcpy(token, ";");
				yylval.typ=';';
				return(';');
			}
			strcpy(token, "<newline>");
			return '\n';
		}
		s=token;
		if(letter(c)){
			do{
				*s++=c;
				c=nextc();
			}while(letter(c) || digit(c));
			peekc=c;
			*s='\0';
			for(resp=res;resp->name!=NULL;resp++)
				if(strcmp(token, resp->name)==0){
					yylval.typ=resp->typ;
					return(resp->typ);
			}
			yylval.tre=idtree();
			return ID;
		}
		if(digit(c) || c=='.'){
			dot=0;
			do{
				if(c=='.'){
					if(dot)
						break;
					dot++;
				}
				*s++=c;
				c=nextc();
			}while(digit(c) || c=='.');
			if(c=='e' || c=='E'){
				*s++=c;
				c=nextc();
				if(c=='-' || c=='+'){
					*s++=c;
					c=nextc();
				}
				while(digit(c)){
					*s++=c;
					c=nextc();
				}
			}
			peekc=c;
			*s='\0';
			yylval.tre=numtree();
			return NUMBER;
		}
		*s++=c;
		*s++='\0';
		*s='\0';
		switch(c){
		case ':': return yylval.typ=see(':')?DCOLON:':';
		case '=': return yylval.typ=see('=')?EQ:'=';
		case '<': return yylval.typ=see('=')?LE:LT;
		case '>': return yylval.typ=see('=')?GE:GT;
		case '!': return yylval.typ=see('=')?NE:'!';
		case '&': return yylval.typ=see('&')?AND:'&';
		case '|': return yylval.typ=see('|')?OR:'|';
		default:  return yylval.typ=c;
		}
	}
}
int nerror;
void yyerror(char *m){
	if(infile!=NULL)
		fprintf(stderr, "\"%s\", ", infile);
	fprintf(stderr, "line %d:", line);
	if(token[0]!='\0')
		fprintf(stderr, " (token `%s')", token);
	fprintf(stderr, " %s\n", m);
	nerror++;
}
char *copy(char *s){
	char *t=(char *)malloc(strlen(s)+1);
	strcpy(t, s);
	return(t);
}
#define	NSYM	512
struct sym sym[NSYM]={
	"skip", 1.,
	"frame", 0.,
};
#define	skip	sym[0].value
#define	frame	sym[1].value
struct sym *lookup(char *name){
	struct sym *sp;
	for(sp=sym;sp!=&sym[NSYM] && sp->name!=NULL;sp++)
		if(strcmp(name, sp->name)==0)
			return(sp);
	if(sp==&sym[NSYM]){
		yyerror("Symbol table full");
		exits("no space");
	}
	sp->name=copy(name);
	sp->value=0.;
	return(sp);
}
Tree *tree(int type, Tree *left, Tree *right){
	Tree *tp=(Tree *)malloc(sizeof(Tree));
	if(tp==NULL){
		yyerror("Out of space");
		exits("no space");
	}
	tp->type=type;
	tp->left=left;
	tp->right=right;
	return tp;
}
Tree *texttree(void){
	Tree *tp=tree(TEXT, NULL, NULL);
	tp->text=copy(token);
	return tp;
}
Tree *idtree(void){
	Tree *tp=tree(ID, NULL, NULL);
	tp->id=lookup(token);
	return tp;
}
Tree *numtree(void){
	Tree *tp=tree(NUMBER, NULL, NULL);
	tp->number=atof(token);
	return tp;
}
double startf, endf;
int rangeset;
#define	degrees(x)	((x)*57.295779513082321)
#define	radians(x)	((x)*.01745329251994330)
double dollar(double f){
	int n=f-1;
	return 0<=n && n<dolc?atof(dolv[n]):0.;
}
double eval(Tree *tp, double f0, double f1){
	double l, r;
	switch(tp->type){
	case ID:	return(tp->id->value);
	case NUMBER:	return(tp->number);
	case '=':	return(tp->left->id->value=eval(tp->right, f0, f1));
	}
	l=eval(tp->left, f0, f1);
	switch(tp->type){
	case '$':	return(dollar(l));
	case UMINUS:	return(-l);
	case FABS:	return(fabs(l));
	case FLOOR:	return(floor(l));
	case CEIL:	return(ceil(l));
	case SQRT:	return(sqrt(l));
	case SIN:	return(sin(radians(l)));
	case COS:	return(cos(radians(l)));
	case TAN:	return(tan(radians(l)));
	case ASIN:	return(degrees(asin(l)));
	case ACOS:	return(degrees(acos(l)));
	case ATAN:	return(degrees(atan(l)));
	case EXP:	return(exp(l));
	case LOG:	return(log(l));
	case LOG10:	return(log10(l));
	case SINH:	return(sinh(l));
	case COSH:	return(cosh(l));
	case TANH:	return(tanh(l));
	case GAMMA:	return(gamma(l));	/* wrong */
	case '!':	return(l==0.);
	case '?':	return(eval(l!=0.?tp->right->left:tp->right->right,
				f0, f1));
	case AND:	return(l==0.?l:eval(tp->right, f0, f1));
	case OR:	return(l==0.?eval(tp->right, f0, f1):l);
	case LERP3:
		tp=tp->right;
		r=eval(tp->left, f0, f1);
		return(r+l*(eval(tp->right, f0, f1)-r));
	}
	r=eval(tp->right, f0, f1);
	switch(tp->type){
	case HYPOT:	return(hypot(l, r));
	case ATAN2:	return(degrees(atan2(l, r)));
	case LERP2:	return(f0==f1?l:l+(r-l)*(frame-f0)/(f1-f0));
	case '^':	return(pow(l, r));
	case '+':	return(l+r);
	case '-':	return(l-r);
	case '*':	return(l*r);
	case '%':	return(l-r*(int)(l/r));
	case ';':	return(r);
	case LT:	return(l<r);
	case LE:	return(l<=r);
	case EQ:	return(l==r);
	case NE:	return(l!=r);
	case GT:	return(l>r);
	case GE:	return(l>=r);
	case '/':
		if(r==0.){
			yyerror("Divide check");
			r=1.;
		}
		return(l/r);
	default:
		if(' '<tp->type && tp->type<0177)
			fprintf(stderr, "Missed case `%c' in eval\n", tp->type);
		else
			fprintf(stderr, "Missed case %d in eval\n", tp->type);
		return(0.);
	}
}
void emit(Tree *tp, double f0, double f1){
	if(tp!=NULL){
		emit(tp->left, f0, f1);
		if(tp->right->type==TEXT)
			printf("%s", tp->right->text);
		else
			printf("%.14g", eval(tp->right, f0, f1));
	}
}
char filename[4096];
void filecat(Tree *tp, double f0, double f1){
	char num[100];
	if(tp!=NULL){
		filecat(tp->left, f0, f1);
		if(tp->right->type==TEXT)
			strcat(filename, tp->right->text);
		else{
			sprintf(num, "%.14g", eval(tp->right, f0, f1));
			strcat(filename, num);
		}
	}
}
void file(Tree *tp, double f0, double f1){
	char *s;
	filename[0]='\0';
	filecat(tp, f0, f1);
	fclose(stdout);
	s=filename+strlen(filename);
	do{
		--s;
	}while(*s==' ' || *s=='\t' || *s=='\n');
	*++s='\0';
	for(s=filename;*s==' ' || *s=='\t' || *s=='\n';s++);
	if(freopen(s, "w", stdout)==NULL){
		perror(s);
		exits("create file");
	}
}
void subshell(Tree *tp, double f0, double f1){
	Waitmsg w;
	int pid;
	filename[0]='\0';
	filecat(tp, f0, f1);
	fflush(stdout);
	switch(pid=fork()){
	case -1:
		perror("moto");
		exits("can't fork");
	case 0:
		execl("/bin/rc", "rc", "-c", filename, 0);
		perror("/bin/rc");
		exits("can't exec");
	default:
		do;while(wait(&w)>=0 && atoi(w.pid)!=pid);
	}
}
void execute(Tree *s, double f0, double f1){
	switch(s->type){
	case CMD:	emit(s->right, f0, f1); break;
	case EXPR:	eval(s->right, f0, f1); break;
	case FILENAME:	file(s->right, f0, f1); break;
	case SUBSHELL:	subshell(s->right, f0, f1); break;
	}
}
void execbegin(Tree *s){
	if(s->left->type==BEGIN)
		execute(s, 0., 0.);
}
void execend(Tree *s){
	if(s->left->type==END)
		execute(s, 0., 0.);
}
void rangeframe(double v){
	if(!rangeset){
		startf=endf=v;
		rangeset++;
	}
	else if(v<startf)
		startf=v;
	else if(endf<v)
		endf=v;
}
void framerange(Tree *s){
	s=s->left;
	if(s->type==RANGE){
		rangeframe(eval(s->left,0., 0.));
		if(s->right)
			rangeframe(eval(s->right, 0., 0.));
	}
}
void execinrange(Tree *s){
	double v0, v1, t;
	if(s->left->type==RANGE){
		v0=eval(s->left->left, 0., 0.);
		if(s->left->right)
		v1=s->left->right?eval(s->left->right, 0., 0.):v0;
		if(v1<v0){t=v0; v0=v1; v1=t;}
		if(v0<=frame && frame<=v1)
			execute(s, v0, v1);
	}
}
void stmtloop(void (*p)(Tree *), Tree *tp){
	if(tp!=NULL){
		stmtloop(p, tp->left);
		(*p)(tp->right);
	}
}
yyparse(void);
main(int argc, char *argv[]){
	int startset=0;
	struct movie *mp;
	while(argc>1 && argv[1][0]=='-'){
		switch(argv[1][1]){
		case 'f':
			if(argc<4)
				goto Usage;
			startf=atof(argv[2]);
			endf=atof(argv[3]);
			startset++;
			argc-=2;
			argv+=2;
			break;
		case 's':
			if(argc<3)
				goto Usage;
			skip=atof(argv[2]);
			--argc;
			argv++;
			break;
		default:
		Usage:
			fprintf(stderr,
				"Usage: %s [-f start end] [-s skip] [file]\n", argv[0]);
			exits("usage");
		}
		--argc;
		argv++;
	}
	if(argc>1){
		ifd=fopen(argv[1], "r");
		if(ifd==NULL){
			perror(argv[1]);
			exits("open input");
		}
		infile=argv[1];
		dolv=argv+2;
		dolc=argc-2;
	}
	else{
		ifd=stdin;
		dolv=argv+2;
		dolc=0;
	}
	yyparse();
	if(nerror)
		exits("syntax error");
	stmtloop(execbegin, movie);
	if(!startset)
		stmtloop(framerange, movie);
	for(frame=startf;startf<=frame && frame<=endf;frame+=skip)
		stmtloop(execinrange, movie);
	stmtloop(execend, movie);
	exits("");
}
