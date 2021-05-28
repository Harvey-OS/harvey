#include "rc.h"
#include "exec.h"
#include "io.h"
#include "fns.h"

Rune getnext(void);

int
wordchr(Rune c)		/* is c in the alphabet of words (non-delimiters)? */
{
	return c != EOF &&
		(c >= Runeself || strchr("\n \t#;&|^$=`'{}()<>", c) == nil);
}

/*
 * is c in the alphabet of identifiers?  as in the c compiler, treat
 * non-ascii as alphabetic.
 */
int
idchr(Rune c)
{
	/*
	 * Formerly:
	 * return 'a'<=c && c<='z' || 'A'<=c && c<='Z' || '0'<=c && c<='9'
	 *	|| c=='_' || c=='*';
	 */
	return c != EOF && (c >= Runeself ||
		c > ' ' &&
		  strchr("!\"#$%&'()+,-./:;<=>?@[\\]^`{|}~", c) == nil);
}

Rune future = EOF;
int doprompt = 1;
int inquote;		/* are we processing a quoted word ('...')? */
int incomm;		/* are we ignoring input in a comment (#...\n)? */
/*
 * Look ahead in the input stream
 */

Rune
nextc(void)
{
	if(future==EOF)
		future = getnext();
	return future;
}
/*
 * Consume the lookahead character.
 */

Rune
advance(void)
{
	Rune c = nextc();

	lastc = future;
	future = EOF;
	return c;
}
/*
 * read a character from the input stream
 */

Rune
getnext(void)
{
	Rune c;
	char buf[UTFmax+1];
	static Rune peekc = EOF;

	if(peekc!=EOF){
		c = peekc;
		peekc = EOF;
		return c;
	}
	if(runq->eof)
		return EOF;
	if(doprompt)
		pprompt();
	rutf(runq->cmdfd, buf, &c);
	if(!inquote && c=='\\'){
		rutf(runq->cmdfd, buf, &c);
		if(c=='\n' && !incomm){		/* don't continue a comment */
			doprompt = 1;
			c=' ';
		}
		else{
			peekc = c;
			c='\\';
		}
	}
	doprompt = doprompt || c=='\n' || c==EOF;
	if(c==EOF)
		runq->eof++;
	else if(flag['V'] || ndot>=2 && flag['v']) pchr(err, c);
	return c;
}

void
pprompt(void)
{
	var *prompt;
	if(runq->iflag){
		pstr(err, promptstr);
		flush(err);
		prompt = vlook("prompt");
		if(prompt->val && prompt->val->next)
			promptstr = prompt->val->next->word;
		else
			promptstr="\t";
	}
	runq->lineno++;
	doprompt = 0;
}

void
skipwhite(void)
{
	Rune c;

	for(;;){
		c = nextc();
		/* Why did this used to be  if(!inquote && c=='#') ?? */
		if(c=='#'){
			incomm = 1;
			for(;;){
				c = nextc();
				if(c=='\n' || c==EOF) {
					incomm = 0;
					break;
				}
				advance();
			}
		}
		if(c==' ' || c=='\t')
			advance();
		else return;
	}
}

void
skipnl(void)
{
	Rune c, c0;

	for(c0 = nextc(); ; c0 = c){
		skipwhite();
		c = nextc();
		if(c != c0)
			lastword = 0; /* change of whitespace or c is not ws */
		if(c!='\n')
			return;
		lastword = 0;			/* new line; continue */
		advance();
	}
}

int
nextis(Rune c)
{
	if(nextc()==c){
		advance();
		return 1;
	}
	return 0;
}

char*
addutf(char *p, Rune c)
{
	if(p==0)
		return 0;
	if(p >= &tok[NTOK-1-UTFmax*2]){
		*p = 0;
		yyerror("token buffer too short");
		return 0;
	}
	p += runetochar(p, &c);
	return p;
}

int lastdol;	/* was the last token read '$' or '$#' or '"'? */
int lastword;	/* was the last token read a word or compound word terminator? */

int
yylex(void)
{
	Rune c, d = nextc();
	char *w = tok;
	struct tree *t;

	yylval.tree = 0;
	/*
	 * Embarrassing sneakiness: if the last token read was a quoted or
	 * unquoted WORD then we alter the meaning of what follows.  If the
	 * next character is `(', we return SUB (a subscript paren) and
	 * consume the `('.  Otherwise, if the next character is the first
	 * character of a simple or compound word, we insert a `^' before it.
	 */
	if(lastword){
		lastword = 0;
		if(d=='('){
			advance();
			strcpy(tok, "( [SUB]");
			return SUB;
		}
		if(wordchr(d) || d=='\'' || d=='`' || d=='$' || d=='"'){
			strcpy(tok, "^");
			return '^';
		}
	}
	skipwhite();
	switch(c = advance()){
	case EOF:
		lastdol = 0;
		strcpy(tok, "EOF");
		return EOF;
	case '$':
		lastdol = 1;
		if(nextis('#')){
			strcpy(tok, "$#");
			return COUNT;
		}
		if(nextis('"')){
			strcpy(tok, "$\"");
			return '"';
		}
		strcpy(tok, "$");
		return '$';
	case '&':
		lastdol = 0;
		if(nextis('&')){
			skipnl();
			strcpy(tok, "&&");
			return ANDAND;
		}
		strcpy(tok, "&");
		return '&';
	case '|':
		lastdol = 0;
		if(nextis(c)){
			skipnl();
			strcpy(tok, "||");
			return OROR;
		}
	case '<':
	case '>':
		lastdol = 0;
		/*
		 * funny redirection tokens:
		 *	redir:	arrow | arrow '[' fd ']'
		 *	arrow:	'<' | '<<' | '>' | '>>' | '|'
		 *	fd:	digit | digit '=' | digit '=' digit
		 *	digit:	'0'|'1'|'2'|'3'|'4'|'5'|'6'|'7'|'8'|'9'
		 * some possibilities are nonsensical and get a message.
		 */
		*w++=c;
		t = newtree();
		switch(c){
		case '|':
			t->type = PIPE;
			t->fd0 = 1;
			t->fd1 = 0;
			break;
		case '>':
			t->type = REDIR;
			if(nextis(c)){
				t->rtype = APPEND;
				*w++=c;
			}
			else t->rtype = WRITE;
			t->fd0 = 1;
			break;
		case '<':
			t->type = REDIR;
			if(nextis(c)){
				t->rtype = HERE;
				*w++=c;
			} else if (nextis('>')){
				t->rtype = RDWR;
				*w++=c;
			} else t->rtype = READ;
			t->fd0 = 0;
			break;
		}
		if(nextis('[')){
			*w++='[';
			c = advance();
			*w++=c;
			if(c<'0' || '9'<c){
			RedirErr:
				*w = 0;
				yyerror(t->type==PIPE?"pipe syntax"
						:"redirection syntax");
				return EOF;
			}
			t->fd0 = 0;
			do{
				t->fd0 = t->fd0*10+c-'0';
				*w++=c;
				c = advance();
			}while('0'<=c && c<='9');
			if(c=='='){
				*w++='=';
				if(t->type==REDIR)
					t->type = DUP;
				c = advance();
				if('0'<=c && c<='9'){
					t->rtype = DUPFD;
					t->fd1 = t->fd0;
					t->fd0 = 0;
					do{
						t->fd0 = t->fd0*10+c-'0';
						*w++=c;
						c = advance();
					}while('0'<=c && c<='9');
				}
				else{
					if(t->type==PIPE)
						goto RedirErr;
					t->rtype = CLOSE;
				}
			}
			if(c!=']'
			|| t->type==DUP && (t->rtype==HERE || t->rtype==APPEND))
				goto RedirErr;
			*w++=']';
		}
		*w='\0';
		yylval.tree = t;
		if(t->type==PIPE)
			skipnl();
		return t->type;
	case '\'':
		lastdol = 0;
		lastword = 1;
		inquote = 1;
		for(;;){
			c = advance();
			if(c==EOF)
				break;
			if(c=='\''){
				if(nextc()!='\'')
					break;
				advance();
			}
			w = addutf(w, c);
		}
		if(w!=0)
			*w='\0';
		t = token(tok, WORD);
		t->quoted = 1;
		yylval.tree = t;
		inquote = 0;
		return t->type;
	}
	if(!wordchr(c)){
		lastdol = 0;
		addutf(tok, c);
		return c;
	}
	for(;;){
		if(c=='*' || c=='[' || c=='?' || c==GLOB)
			w = addutf(w, GLOB);
		w = addutf(w, c);
		c = nextc();
		if(lastdol?!idchr(c):!wordchr(c)) break;
		advance();
	}
	lastword = 1;
	lastdol = 0;
	if(w!=0)
		*w='\0';
	t = klook(tok);
	if(t->type!=WORD)
		lastword = 0;
	t->quoted = 0;
	yylval.tree = t;
	return t->type;
}
