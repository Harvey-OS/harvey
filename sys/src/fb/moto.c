#define	DCOLON	57346
#define	BEGIN	57347
#define	END	57348
#define	ID	57349
#define	NUMBER	57350
#define	TEXT	57351
#define	CMDLIST	57352
#define	EXPR	57353
#define	CMD	57354
#define	LERP2	57355
#define	LERP3	57356
#define	MOVIE	57357
#define	RANGE	57358
#define	FILENAME	57359
#define	SUBSHELL	57360
#define	AND	57361
#define	OR	57362
#define	LT	57363
#define	LE	57364
#define	EQ	57365
#define	NE	57366
#define	GT	57367
#define	GE	57368
#define	UMINUS	57369
#define	FABS	57370
#define	FLOOR	57371
#define	CEIL	57372
#define	SQRT	57373
#define	HYPOT	57374
#define	SIN	57375
#define	COS	57376
#define	TAN	57377
#define	ASIN	57378
#define	ACOS	57379
#define	ATAN	57380
#define	ATAN2	57381
#define	EXP	57382
#define	LOG	57383
#define	LOG10	57384
#define	SINH	57385
#define	COSH	57386
#define	TANH	57387
#define	GAMMA	57388

#line	22	"moto.y"
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

#line	43	"moto.y"
typedef union {
	int typ;
	Tree *tre;
} YYSTYPE;
extern	int	yyerrflag;
#ifndef	YYMAXDEPTH
#define	YYMAXDEPTH	150
#endif
YYSTYPE	yylval;
YYSTYPE	yyval;
#define YYEOFCODE 1
#define YYERRCODE 2

#line	114	"moto.y"

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
short	yyexca[] =
{-1, 1,
	1, -1,
	-2, 0,
-1, 93,
	34, 0,
	35, 0,
	36, 0,
	37, 0,
	38, 0,
	39, 0,
	-2, 32,
-1, 94,
	34, 0,
	35, 0,
	36, 0,
	37, 0,
	38, 0,
	39, 0,
	-2, 33,
-1, 95,
	34, 0,
	35, 0,
	36, 0,
	37, 0,
	38, 0,
	39, 0,
	-2, 34,
-1, 96,
	34, 0,
	35, 0,
	36, 0,
	37, 0,
	38, 0,
	39, 0,
	-2, 35,
-1, 97,
	34, 0,
	35, 0,
	36, 0,
	37, 0,
	38, 0,
	39, 0,
	-2, 36,
-1, 98,
	34, 0,
	35, 0,
	36, 0,
	37, 0,
	38, 0,
	39, 0,
	-2, 37,
};
#define	YYNPROD	68
#define	YYPRIVATE 57344
#define	YYLAST	1285
short	yyact[] =
{
   7, 123,  36,  78,  77,  76,  75,  74,  73,  72,
  71,  56,  57,  58,  59,  60,  40,  70,  44,  45,
  41,  42,  43,  69,  68,  52,  67,  33,  35,  53,
  54,  46,  47,  48,  49,  50,  51,  83,  66,  84,
  85,  86,  88,  89,  90,  91,  92,  93,  94,  95,
  96,  97,  98,  99, 100, 101, 102,  39, 163,  65,
  64,  34, 105, 106, 107, 108, 109, 110, 111, 112,
 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
  40, 152, 124, 125,  41,  42,  43,  63, 129,  40,
  62,  44,  45,  41,  42,  43, 152,  40,  61,  44,
  45,  41,  42,  43, 165, 131,  52, 152,  37, 170,
  53,  54,  46,  47,  48,  49,  50,  51, 127,  55,
  40,  39, 162,  81,  80,  79, 167,  82, 156, 157,
  39, 158, 153,   3, 155,   2,   4, 160,  39,   5,
   6,   8,   9, 164, 161,   1,   0, 153,   0, 154,
   0,   0,   0, 126,   0,   0,  13,   0, 153,   0,
 151,  39,   0, 168,  14,  10, 169,   0,   0,   0,
   0,   0, 173,   0,   0,  15,  16,  17,  18,  19,
  20,  21,  22,  23,  24,  25,   0,  26,  27,  28,
  29,  30,  31,  32,  12,   8,   9,   0,  11,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  13,  87,   0,   0,   0,   0,   0,   0,  14,  10,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  15,
  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,
   0,  26,  27,  28,  29,  30,  31,  32,  12,   8,
   9,   0,  11,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,  13,   0,   0,   0,   0,   0,
   0,   0,  14,  10,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  15,  16,  17,  18,  19,  20,  21,
  22,  23,  24,  25,   0,  26,  27,  28,  29,  30,
  31,  32,  12,   0,   0,  40,  11,  44,  45,  41,
  42,  43, 143,   0,  52,   0,   0,   0,  53,  54,
  46,  47,  48,  49,  50,  51,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  40,
   0,  44,  45,  41,  42,  43,  39,   0,  52,   0,
   0, 142,  53,  54,  46,  47,  48,  49,  50,  51,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  40,   0,  44,  45,  41,  42,  43,
  39,   0,  52,   0,   0, 172,  53,  54,  46,  47,
  48,  49,  50,  51,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  40,   0,  44,
  45,  41,  42,  43,  39,   0,  52,   0,   0, 166,
  53,  54,  46,  47,  48,  49,  50,  51,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  40,   0,  44,  45,  41,  42,  43,  39,   0,
  52,   0,   0, 150,  53,  54,  46,  47,  48,  49,
  50,  51,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  40,   0,  44,  45,  41,
  42,  43,  39,   0,  52,   0,   0, 149,  53,  54,
  46,  47,  48,  49,  50,  51,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  40,
   0,  44,  45,  41,  42,  43,  39,   0,  52,   0,
   0, 148,  53,  54,  46,  47,  48,  49,  50,  51,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  40,   0,  44,  45,  41,  42,  43,
  39,   0,  52,   0,   0, 147,  53,  54,  46,  47,
  48,  49,  50,  51,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  40,   0,  44,
  45,  41,  42,  43,  39,   0,  52,   0,   0, 146,
  53,  54,  46,  47,  48,  49,  50,  51,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  40,   0,  44,  45,  41,  42,  43,  39,   0,
  52,   0,   0, 145,  53,  54,  46,  47,  48,  49,
  50,  51,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  40,   0,  44,  45,  41,
  42,  43,  39,   0,  52,   0,   0, 144,  53,  54,
  46,  47,  48,  49,  50,  51,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  40,
   0,  44,  45,  41,  42,  43,  39,   0,  52,   0,
   0, 141,  53,  54,  46,  47,  48,  49,  50,  51,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  40,   0,  44,  45,  41,  42,  43,
  39,   0,  52,   0,   0, 140,  53,  54,  46,  47,
  48,  49,  50,  51,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  40,   0,  44,
  45,  41,  42,  43,  39,   0,  52,   0,   0, 139,
  53,  54,  46,  47,  48,  49,  50,  51,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  40,   0,  44,  45,  41,  42,  43,  39,   0,
  52,   0,   0, 138,  53,  54,  46,  47,  48,  49,
  50,  51,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  40,   0,  44,  45,  41,
  42,  43,  39,   0,  52,   0,   0, 137,  53,  54,
  46,  47,  48,  49,  50,  51,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  40,
   0,  44,  45,  41,  42,  43,  39,   0,  52,   0,
   0, 135,  53,  54,  46,  47,  48,  49,  50,  51,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  40,   0,  44,  45,  41,  42,  43,
  39,   0,  52,   0,   0, 134,  53,  54,  46,  47,
  48,  49,  50,  51,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,  40,   0,  44,
  45,  41,  42,  43,  39,   0,  52,   0,   0, 133,
  53,  54,  46,  47,  48,  49,  50,  51,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  40,   0,  44,  45,  41,  42,  43,  39,   0,
  52,   0,   0, 132,  53,  54,  46,  47,  48,  49,
  50,  51,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  40,   0,  44,  45,  41,
  42,  43,  39,   0,  52,   0,   0, 103,  53,  54,
  46,  47,  48,  49,  50,  51,   0,   0,   0,   0,
   0,   0,   0,   0,   0,  40,   0,  44,  45,  41,
  42,  43, 171,   0,  52,   0,  39, 159,  53,  54,
  46,  47,  48,  49,  50,  51,   0,   0,   0,   0,
   0,   0,   0,   0,  40,   0,  44,  45,  41,  42,
  43, 136,   0,  52,   0,   0,  39,  53,  54,  46,
  47,  48,  49,  50,  51,   0,   0,   0,   0,   0,
   0,   0,   0,  40,   0,  44,  45,  41,  42,  43,
   0,   0,  52, 130,   0,  39,  53,  54,  46,  47,
  48,  49,  50,  51,   0,   0,   0,   0,   0,   0,
   0,   0,  40,   0,  44,  45,  41,  42,  43, 128,
   0,  52,   0,   0,  39,  53,  54,  46,  47,  48,
  49,  50,  51,   0,   0,   0,   0,   0,   0,   0,
   0,  40,   0,  44,  45,  41,  42,  43, 104,   0,
  52,   0,   0,  39,  53,  54,  46,  47,  48,  49,
  50,  51,   0,   0,   0,   0,   0,   0,   0,   0,
  40,   0,  44,  45,  41,  42,  43,  38,   0,  52,
   0,   0,  39,  53,  54,  46,  47,  48,  49,  50,
  51,   0,   0,   0,   0,   0,   0,   0,   0,  40,
   0,  44,  45,  41,  42,  43,   0,   0,  52,   0,
   0,  39,  53,  54,  46,  47,  48,  49,  50,  51,
   0,   0,   0,   0,   0,   0,  40,   0,  44,  45,
  41,  42,  43,   0,   0,   0,   0,   0,   0,  53,
  39,  46,  47,  48,  49,  50,  51,   0,   0,   0,
   0,   0,   0,  40,   0,  44,  45,  41,  42,  43,
   0,   0,   0,   0,   0,   0,   0,  39,  46,  47,
  48,  49,  50,  51,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,  39
};
short	yypact[] =
{
-1000, 134,-1000,  -2,  46,-1000,-1000,1141,  99,-1000,
 242, 242, 242, 242, 242,  34,  26,  23,  -4,  -5,
 -26, -38, -40, -41, -47, -54, -55, -56, -57, -58,
 -59, -60, -61,-1000,-1000,-1000, 242,-1000, 242, 242,
 242, 188, 242, 242, 242, 242, 242, 242, 242, 242,
 242, 242, 242, 242, 242, 242,-1000, 932,1112,-1000,
-1000, 242, 242, 242, 242, 242, 242, 242, 242, 242,
 242, 242, 242, 242, 242, 242, 242, 242, 242,-1000,
-1000,-1000,  91,1170,1170,1083,-1000, 242, 101, 101,
 101,  61,  61,  70,  70,  70,  70,  70,  70,1054,
1224,1197,1170,-1000, 242, 898, 864, 830, 796,1025,
 762, 728, 694, 660, 626, 286, 592, 558, 524, 490,
 456, 422, 388,  98,  87,  72,-1000, 242, 242,-1000,
 242, 966,-1000,-1000,-1000,-1000, 242,-1000,-1000,-1000,
-1000,-1000,-1000, 242,-1000,-1000,-1000,-1000,-1000,-1000,
-1000,-1000,-1000,-1000,-1000,-1000,1170,  -3,1170,-1000,
  78, 354, 242,-1000,-1000, 242,-1000,  48, 996, 320,
-1000, 242,-1000,1170
};
short	yypgo[] =
{
   0, 145, 135, 133, 127,   0, 126,   1, 125, 124,
 123, 122
};
short	yyr1[] =
{
   0,   1,   1,   8,   2,   9,   2,  10,   2,   2,
   2,   3,   3,   3,   3,   4,   4,   5,   5,   5,
   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,
   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,
   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,
   5,   5,   5,   5,   5,   5,   5,   5,   5,   5,
   5,   5,   7,   7,  11,   7,   6,   6
};
short	yyr2[] =
{
   0,   0,   2,   0,   5,   0,   5,   0,   5,   4,
   2,   1,   1,   1,   3,   1,   3,   1,   3,   1,
   2,   3,   5,   6,   3,   4,   3,   3,   3,   3,
   3,   2,   3,   3,   3,   3,   3,   3,   5,   2,
   3,   3,   4,   4,   4,   4,   6,   8,   4,   4,
   4,   4,   4,   4,   6,   4,   4,   4,   4,   4,
   4,   4,   0,   2,   0,   5,   1,   3
};
short	yychk[] =
{
-1000,  -1,  -2,  -3,   2,   5,   6,  -5,   7,   8,
  31,  64,  60,  22,  30,  41,  42,  43,  44,  45,
  46,  47,  48,  49,  50,  51,  53,  54,  55,  56,
  57,  58,  59,  29,  63,  30,   4,  62,  26,  60,
  19,  23,  24,  25,  21,  22,  34,  35,  36,  37,
  38,  39,  28,  32,  33,  20,  -5,  -5,  -5,  -5,
  -5,  64,  64,  64,  64,  64,  64,  64,  64,  64,
  64,  64,  64,  64,  64,  64,  64,  64,  64,  -8,
  -9, -10,  -4,  -5,  -5,  -5,  -5,  23,  -5,  -5,
  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,
  -5,  -5,  -5,  65,  26,  -5,  -5,  -5,  -5,  -5,
  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5,
  -5,  -5,  -5,  -7,  -7,  -7,  62,  27,  26,  -5,
  29,  -5,  65,  65,  65,  65,  26,  65,  65,  65,
  65,  65,  65,  26,  65,  65,  65,  65,  65,  65,
  65,  62,   9,  60,  62,  62,  -5,  -5,  -5,  61,
  -5,  -5, -11,  61,  65,  26,  65,  -6,  -5,  -5,
  61,  26,  65,  -5
};
short	yydef[] =
{
   1,  -2,   2,   0,   0,  11,  12,  13,  17,  19,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   3,   5,   7,   0,  10,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  20,   0,   0,  31,
  39,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  62,
  62,  62,   0,  15,  14,   0,  24,   0,  26,  27,
  28,  29,  30,  -2,  -2,  -2,  -2,  -2,  -2,   0,
  40,  41,  18,  21,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   9,   0,   0,  25,
   0,   0,  42,  43,  44,  45,   0,  48,  49,  50,
  51,  52,  53,   0,  55,  56,  57,  58,  59,  60,
  61,   4,  63,  64,   6,   8,  16,   0,  38,  22,
   0,   0,   0,  23,  46,   0,  54,   0,  66,   0,
  65,   0,  47,  67
};
short	yytok1[] =
{
   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,
  62,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,  30,   0,   0,  31,  25,   0,   0,
  64,  65,  23,  21,  26,  22,   0,  24,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  29,  27,
   0,  20,   0,  28,  63,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  60,   0,  61,  19
};
short	yytok2[] =
{
   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,
  12,  13,  14,  15,  16,  17,  18,  32,  33,  34,
  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
  45,  46,  47,  48,  49,  50,  51,  52,  53,  54,
  55,  56,  57,  58,  59
};
long	yytok3[] =
{
   0
};
#define YYFLAG 		-1000
#define	yyclearin	yychar = -1
#define	yyerrok		yyerrflag = 0

#ifdef	yydebug
#include	"y.debug"
#else
#define	yydebug		0
#endif

/*	parser for yacc output	*/

int	yynerrs = 0;		/* number of errors */
int	yyerrflag = 0;		/* error recovery flag */

char*	yytoknames[1];		/* for debugging */
char*	yystates[1];		/* for debugging */

extern	int	fprint(int, char*, ...);
extern	int	sprint(char*, char*, ...);

char*
yytokname(int yyc)
{
	static char x[10];

	if(yyc > 0 && yyc <= sizeof(yytoknames)/sizeof(yytoknames[0]))
	if(yytoknames[yyc-1])
		return yytoknames[yyc-1];
	sprint(x, "<%d>", yyc);
	return x;
}

char*
yystatname(int yys)
{
	static char x[10];

	if(yys >= 0 && yys < sizeof(yystates)/sizeof(yystates[0]))
	if(yystates[yys])
		return yystates[yys];
	sprint(x, "<%d>\n", yys);
	return x;
}

long
yylex1(void)
{
	long yychar;
	long *t3p;
	int c;

	yychar = yylex();
	if(yychar < sizeof(yytok1)/sizeof(yytok1[0])) {
		if(yychar <= 0) {
			c = yytok1[0];
			goto out;
		}
		c = yytok1[yychar];
		goto out;
	}
	if(yychar >= YYPRIVATE)
		if(yychar < YYPRIVATE+sizeof(yytok2)/sizeof(yytok2[0])) {
			c = yytok2[yychar-YYPRIVATE];
			goto out;
		}
	for(t3p=yytok3;; t3p+=2) {
		c = t3p[0];
		if(c == yychar) {
			c = t3p[1];
			goto out;
		}
		if(c == 0)
			break;
	}
	c = 0;

out:
	if(c == 0)
		c = yytok2[1];	/* unknown char */
	if(yydebug >= 3)
		fprint(2, "lex %.4lux %s\n", yychar, yytokname(c));
	return c;
}

int
yyparse(void)
{
	struct
	{
		YYSTYPE	yyv;
		int	yys;
	} yys[YYMAXDEPTH], *yyp, *yypt;
	short *yyxi;
	int yyj, yym, yystate, yyn, yyg;
	long yychar;

	yystate = 0;
	yychar = -1;
	yynerrs = 0;
	yyerrflag = 0;
	yyp = &yys[-1];

yystack:
	/* put a state and value onto the stack */
	if(yydebug >= 4)
		fprint(2, "char %s in %s", yytokname(yychar), yystatname(yystate));

	yyp++;
	if(yyp >= &yys[YYMAXDEPTH]) { 
		yyerror("yacc stack overflow"); 
		return 1; 
	}
	yyp->yys = yystate;
	yyp->yyv = yyval;

yynewstate:
	yyn = yypact[yystate];
	if(yyn <= YYFLAG)
		goto yydefault; /* simple state */
	if(yychar < 0)
		yychar = yylex1();
	yyn += yychar;
	if(yyn < 0 || yyn >= YYLAST)
		goto yydefault;
	yyn = yyact[yyn];
	if(yychk[yyn] == yychar) { /* valid shift */
		yychar = -1;
		yyval = yylval;
		yystate = yyn;
		if(yyerrflag > 0)
			yyerrflag--;
		goto yystack;
	}

yydefault:
	/* default state action */
	yyn = yydef[yystate];
	if(yyn == -2) {
		if(yychar < 0)
			yychar = yylex1();

		/* look through exception table */
		for(yyxi=yyexca;; yyxi+=2)
			if(yyxi[0] == -1 && yyxi[1] == yystate)
				break;
		for(yyxi += 2;; yyxi += 2) {
			yyn = yyxi[0];
			if(yyn < 0 || yyn == yychar)
				break;
		}
		yyn = yyxi[1];
		if(yyn < 0)
			return 0;
	}
	if(yyn == 0) {
		/* error ... attempt to resume parsing */
		switch(yyerrflag) {
		case 0:   /* brand new error */
			yyerror("syntax error");
			yynerrs++;
			if(yydebug >= 1) {
				fprint(2, "%s", yystatname(yystate));
				fprint(2, "saw %s\n", yytokname(yychar));
			}

		case 1:
		case 2: /* incompletely recovered error ... try again */
			yyerrflag = 3;

			/* find a state where "error" is a legal shift action */
			while(yyp >= yys) {
				yyn = yypact[yyp->yys] + YYERRCODE;
				if(yyn >= 0 && yyn < YYLAST) {
					yystate = yyact[yyn];  /* simulate a shift of "error" */
					if(yychk[yystate] == YYERRCODE)
						goto yystack;
				}

				/* the current yyp has no shift onn "error", pop stack */
				if(yydebug >= 2)
					fprint(2, "error recovery pops state %d, uncovers %d\n",
						yyp->yys, (yyp-1)->yys );
				yyp--;
			}
			/* there is no state on the stack with an error shift ... abort */
			return 1;

		case 3:  /* no shift yet; clobber input char */
			if(yydebug >= 2)
				fprint(2, "error recovery discards %s\n", yytokname(yychar));
			if(yychar == YYEOFCODE)
				return 1;
			yychar = -1;
			goto yynewstate;   /* try again in the same state */
		}
	}

	/* reduction by production yyn */
	if(yydebug >= 2)
		fprint(2, "reduce %d in:\n\t%s", yyn, yystatname(yystate));

	yypt = yyp;
	yyp -= yyr2[yyn];
	yyval = (yyp+1)->yyv;
	yym = yyn;

	/* consult goto table to find next state */
	yyn = yyr1[yyn];
	yyg = yypgo[yyn];
	yyj = yyg + yyp->yys + 1;

	if(yyj >= YYLAST || yychk[yystate=yyact[yyj]] != -yyn)
		yystate = yyact[yyg];
	switch(yym) {
		
case 1:
#line	48	"moto.y"
{yyval.tre=NULL;} break;
case 2:
#line	49	"moto.y"
{yyval.tre=movie=tree(MOVIE, yypt[-1].yyv.tre, yypt[-0].yyv.tre);} break;
case 3:
#line	50	"moto.y"
{lexstate=CMD;} break;
case 4:
#line	51	"moto.y"
{lexstate=EXPR; yyval.tre=tree(CMD, yypt[-4].yyv.tre, yypt[-1].yyv.tre);} break;
case 5:
#line	52	"moto.y"
{lexstate=CMD;} break;
case 6:
#line	53	"moto.y"
{lexstate=EXPR; yyval.tre=tree(FILENAME, yypt[-4].yyv.tre, yypt[-1].yyv.tre);} break;
case 7:
#line	54	"moto.y"
{lexstate=CMD;} break;
case 8:
#line	55	"moto.y"
{lexstate=EXPR; yyval.tre=tree(SUBSHELL, yypt[-4].yyv.tre, yypt[-1].yyv.tre);} break;
case 9:
#line	56	"moto.y"
{yyval.tre=tree(EXPR, yypt[-3].yyv.tre, yypt[-1].yyv.tre);} break;
case 10:
#line	57	"moto.y"
{lexstate=EXPR; yyval.tre=NULL;} break;
case 11:
#line	58	"moto.y"
{yyval.tre=tree(BEGIN, NULL, NULL);} break;
case 12:
#line	59	"moto.y"
{yyval.tre=tree(END, NULL, NULL);} break;
case 13:
#line	60	"moto.y"
{yyval.tre=tree(RANGE, yypt[-0].yyv.tre, NULL);} break;
case 14:
#line	61	"moto.y"
{yyval.tre=tree(RANGE, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 16:
#line	63	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 18:
#line	65	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 20:
#line	67	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-0].yyv.tre, NULL);} break;
case 21:
#line	68	"moto.y"
{yyval.tre=yypt[-1].yyv.tre;} break;
case 22:
#line	69	"moto.y"
{yyval.tre=tree(LERP2, yypt[-3].yyv.tre, yypt[-1].yyv.tre);} break;
case 23:
#line	70	"moto.y"
{yyval.tre=tree(LERP3, yypt[-5].yyv.tre, tree(LERP2, yypt[-3].yyv.tre, yypt[-1].yyv.tre));} break;
case 24:
#line	71	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 25:
#line	72	"moto.y"
{yyval.tre=tree('^', yypt[-3].yyv.tre, yypt[-0].yyv.tre);} break;
case 26:
#line	73	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 27:
#line	74	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 28:
#line	75	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 29:
#line	76	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 30:
#line	77	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 31:
#line	78	"moto.y"
{yyval.tre=tree(UMINUS, yypt[-0].yyv.tre, NULL);} break;
case 32:
#line	79	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 33:
#line	80	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 34:
#line	81	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 35:
#line	82	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 36:
#line	83	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 37:
#line	84	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 38:
#line	85	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-4].yyv.tre, tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre));} break;
case 39:
#line	86	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-0].yyv.tre, NULL);} break;
case 40:
#line	87	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 41:
#line	88	"moto.y"
{yyval.tre=tree(yypt[-1].yyv.typ, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
case 42:
#line	89	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 43:
#line	90	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 44:
#line	91	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 45:
#line	92	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 46:
#line	93	"moto.y"
{yyval.tre=tree(yypt[-5].yyv.typ, yypt[-3].yyv.tre, yypt[-1].yyv.tre);} break;
case 47:
#line	94	"moto.y"
{yyval.tre=tree(yypt[-7].yyv.typ, yypt[-5].yyv.tre, tree(yypt[-7].yyv.typ, yypt[-3].yyv.tre, yypt[-1].yyv.tre));} break;
case 48:
#line	95	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 49:
#line	96	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 50:
#line	97	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 51:
#line	98	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 52:
#line	99	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 53:
#line	100	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 54:
#line	101	"moto.y"
{yyval.tre=tree(yypt[-5].yyv.typ, yypt[-3].yyv.tre, yypt[-1].yyv.tre);} break;
case 55:
#line	102	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 56:
#line	103	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 57:
#line	104	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 58:
#line	105	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 59:
#line	106	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 60:
#line	107	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 61:
#line	108	"moto.y"
{yyval.tre=tree(yypt[-3].yyv.typ, yypt[-1].yyv.tre, NULL);} break;
case 62:
#line	109	"moto.y"
{yyval.tre=NULL;} break;
case 63:
#line	110	"moto.y"
{yyval.tre=tree(CMDLIST, yypt[-1].yyv.tre, yypt[-0].yyv.tre);} break;
case 64:
#line	111	"moto.y"
{lexstate=EXPR;} break;
case 65:
#line	111	"moto.y"
{lexstate=CMD; yyval.tre=tree(CMDLIST, yypt[-4].yyv.tre, yypt[-1].yyv.tre);} break;
case 67:
#line	113	"moto.y"
{yyval.tre=tree(LERP2, yypt[-2].yyv.tre, yypt[-0].yyv.tre);} break;
	}
	goto yystack;  /* stack new state and value */
}
