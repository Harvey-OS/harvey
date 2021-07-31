#include <ctype.h>
#include "common.h"
#include "code.h"
#include "sym.h"
#include "y.tab.h"

int yyline = 1;
char token_buffer[MAXIDSIZE+1];
extern YYSTYPE yylval;
static Code *curCdBlock;

static void ScanCodeBlock(void);
static void ScanComment(char (*)(void));
static char get(void);

int
yylex(void)
{
	register c;
	register char *cp;

	yylval.y_nodep = (struct node *) NULL;
	cp = token_buffer;
	while((c = Bgetc(bin)) != Beof) {
		switch(c) {
		case ' ': case '\t': case '\f':
			continue;
		case '@': case '[': case ']': case ';': case ':':
		case '(': case ')': case ',': case '=':
		case '*':
			if(debug_flag&DB_LEX)
				fprint(2, "%c\n", c);
			*cp++ = c;
			*cp = '\0';
			return c;

		case '{':
			ScanCodeBlock();
			yylval.y_code = curCdBlock;
			curCdBlock = NULL;
			*cp++ = '{'; *cp = '}';
			return CBLOCK;

		case '\n':
			yyline++;
			continue;
		case '/':
			if (Bgetc(bin) == '*') {
				ScanComment(get);
				continue;
			} else {
				Bungetc(bin);
				c = '/';
			}
			/* FALL THRU */

		default:
			if (isdigit(c)) {
				int errs = 0;
				do {
					if(cp > &token_buffer[MAXIDSIZE]) {
						token_buffer[MAXIDSIZE] = '\0';
						yyerror("number too long");
						errs++;
					} else *cp++ = c;
					c = Bgetc(bin);
				} while (isdigit(c));
				if(isalpha(c))
					yyerror2("illegal digit '%c'", c);
				Bungetc(bin);
				if(errs)
					return(ERROR);
				yylval.y_int = atoi(token_buffer);
				return(NUMBER);
			}
			if (isalpha(c)) {
				SymbolEntry *sp;
				int errs = 0;
				do {
					if(cp > &token_buffer[MAXIDSIZE]) {
						token_buffer[MAXIDSIZE] = '\0';
						yyerror("ID too long");
						errs++;
					} else *cp++ = c;
					c = Bgetc(bin);
				} while (isalpha(c)||isdigit(c)||c=='_');
				Bungetc(bin);
				if(errs)
					return(ERROR);
				*cp = '\0';

				sp = SymbolLookup (token_buffer);
				if (sp == NULL) {
					/* undefined */
				    yylval.y_symp = SymbolAllocate(token_buffer);
				} else {
				    /* already defined */
				    if (sp->attr == A_KEYWORD)
						return (sp->sd.keyword);
				    yylval.y_symp = sp;
				}
				if(debug_flag&DB_LEX)
					fprint(2, "ID\n");
				return(ID);
			}
			yyerror2("illegal character (\\%03o)", c);
		}
	}
	strcpy(token_buffer, "EOF");
	return 0;
}

void
LexInit(void)
{
}

void
lexCleanup(void)
{
}

/*
 * Beware: ungets of the characters from these routines may screw up the
 * line count
 */
static char
getput(void)
{
	int c;
	c = Bgetc(bin);
	if(c == '\n')
		yyline++;
	if(c != Beof)
		curCdBlock = CodeStoreChar(curCdBlock, c);
	return c;
}

static char
get(void)
{
	int c;

	c = Bgetc(bin);
	if(c == '\n')
		yyline++;
	return c;
}

extern int nerrors;

static void
ScanComment(char (*rdfunc)(void))
{
	int startline = yyline;
	int c;
	int saw_star = 0;

	while((c = rdfunc()) != Beof) {
		if (c == '*')
			saw_star++;
		else if(c=='/' && saw_star) {
			return;
		} else saw_star = 0;
	}
	yyerror2("unexpected EOF in comment beginning on line %d", startline);
	nerrors++;
	cleanup(1);
}

static void
ScanString(char (*rdfunc)(void))
{
	int startline = yyline;
	int c;
	int saw_backsl = 0;

	while((c=rdfunc()) != Beof) {
		if (c=='"' && !saw_backsl)
			return;
		if (c=='\\' && !saw_backsl) {
			saw_backsl = 1;
			continue;
		}
		saw_backsl = 0;
	}
	/* fall thru due to EOF */
	yyerror2("unexpected EOF in string beginning on line %d", startline);
	nerrors++;
	cleanup(1);
}

static void
ScanChar(void)
{
	int startline = yyline;
	int c;
	int saw_backsl = 0;

	while((c=getput()) != Beof) {
		if (c=='\'' && !saw_backsl)
			return;
		if (c=='\\' && !saw_backsl) {
			saw_backsl = 1;
			continue;
		}
		saw_backsl = 0;
	}
	/* fall thru due to EOF */
	yyerror2("unexpected EOF in character constant beginning on line %d",
		 startline);
	nerrors++;
	cleanup(1);
}

static void
ScanTreeReference(void)
{

	int c;
	c = Bgetc(bin);
	if(c=='%') {
		curCdBlock = CodeStoreString (curCdBlock, "_ll[(");
		while ((c = Bgetc(bin)) != Beof && c != '$') {
			if(!isdigit(c)) {
				yyerror("unclosed $ reference");
				Bungetc(bin);
				break;
			}
			curCdBlock = CodeStoreChar(curCdBlock, c);
		}
		curCdBlock = CodeStoreString (curCdBlock, ")-1]");
		return;
	}
	else if(c=='$') {
		curCdBlock = CodeStoreString(curCdBlock, "root");
		return;
	} else curCdBlock = CodeStoreString(curCdBlock, "_mtG(root,");
	do {
		if(!isdigit(c) && c!='.') {
			yyerror("unclosed $ reference");
			Bungetc(bin);
			break;
		}
		curCdBlock = CodeStoreChar(curCdBlock, c=='.' ? ',' : c);
	} while((c = Bgetc(bin)) != Beof && c != '$');
	curCdBlock = CodeStoreString(curCdBlock, ", -1)");
}

static void
ScanCodeBlock(void)
{
	int startline = yyline;
	int c;
	if(curCdBlock==NULL) {
		curCdBlock = CodeGetBlock();
		curCdBlock = CodeMarkLine(curCdBlock,yyline);
	}
	while((c = Bgetc(bin)) != Beof){
		if(c=='}')
			return;
		else if(c=='$')
			ScanTreeReference();
		else
			curCdBlock = CodeStoreChar(curCdBlock, c);
		if(c=='\n')
			yyline++;
		if(c=='"')
			ScanString(getput);
		else if(c=='\'')
			ScanChar();
		else if(c=='/'){
			if(Bgetc(bin) == '*'){
				curCdBlock = CodeStoreChar(curCdBlock, '*');
				ScanComment(getput);
			}else
				Bungetc(bin);
		}else if (c=='{'){
			ScanCodeBlock();
			curCdBlock = CodeStoreChar (curCdBlock, '}');
		}
	}
	yyerror2("{ on line %d has no closing }", startline);
	nerrors++;
}
