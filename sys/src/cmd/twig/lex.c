#include <ctype.h>
#include "common.h"
#include "code.h"
#include "sym.h"
#include "y.tab.h"

FILE *fin;
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
	while((c=getc(fin))!=EOF) {
		switch(c) {
		case ' ': case '\t': case '\f':
			continue;
		case '@': case '[': case ']': case ';': case ':':
		case '(': case ')': case ',': case '=':
		case '*':
			if(debug_flag&DB_LEX) {
				putc(c,stderr);
				putc('\n', stderr);
			}
			*cp++ = c;
			*cp = '\0';
			return(c);

		case '{':
			ScanCodeBlock();
			yylval.y_code = curCdBlock;
			curCdBlock = NULL;
			*cp++ = '{'; *cp = '}';
			return(CBLOCK);

		case '\n':
			yyline++;
			continue;
		case '/':
			if ((c=getc(fin))=='*') {
				ScanComment(get);
				continue;
			} else {
				ungetc(c, fin);
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
					c = getc(fin);
				} while (isdigit(c));
				if(isalpha(c))
					yyerror2("illegal digit '%c'", c);
				ungetc(c, fin);
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
					c = getc(fin);
				} while (isalpha(c)||isdigit(c)||c=='_');
				ungetc(c, fin);
				if(errs)
					return(ERROR);
				*cp = '\0';

				sp = SymbolLookup (token_buffer);
				if (sp==NULL) {
					/* undefined */
				    yylval.y_symp = SymbolAllocate(token_buffer);
				} else {
				    /* already defined */
				    if (sp->attr == A_KEYWORD)
						return (sp->sd.keyword);
				    yylval.y_symp = sp;
				}
				if(debug_flag&DB_LEX)
					fprintf(stderr, "ID\n");
				return(ID);
			}
			yyerror2("illegal character (\\%03o)", c);
		}
	}
	strcpy(token_buffer, "EOF");
	return(0);
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
	/* keutzer
	char c;
	*/
	int c;
	c = getc(fin);
	if(c=='\n') yyline++;
	if(c!=EOF)
		curCdBlock = CodeStoreChar(curCdBlock, c);
	return(c);
}

static char
get(void)
{
	/* keutzer
	char c;
	*/
	int c;
	c = getc(fin);
	if(c=='\n') yyline++;
	return(c);
}

extern int nerrors;

static void
ScanComment(char (*rdfunc)(void))
{
	int startline = yyline;
	/* keutzer
	char c;
	*/
	int c;
	int saw_star = 0;
	while ((c=rdfunc())!=EOF) {
		if (c=='*')
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
	/* keutzer
	char c;
	*/
	int c;
	int saw_backsl = 0;

	while((c=rdfunc())!=EOF) {
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
	/* keutzer
	char c;
	*/
	int c;
	int saw_backsl = 0;

	while((c=getput())!=EOF) {
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

	/* keutzer
	char c;
	*/
	int c;
	c = getc(fin);
	if(c=='%') {
		curCdBlock = CodeStoreString (curCdBlock, "_ll[(");
		while ((c=getc(fin))!=EOF && c!='$') {
			if(!isdigit(c)) {
				yyerror("unclosed $ reference");
				ungetc(c,fin);
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
			ungetc(c,fin);
			break;
		}
		curCdBlock = CodeStoreChar(curCdBlock, c=='.' ? ',' : c);
	} while((c=getc(fin))!=EOF && c!='$');
	curCdBlock = CodeStoreString(curCdBlock, ", -1)");
}

static void
ScanCodeBlock(void)
{
	int startline = yyline;
	/* keutzer
	char c;
	*/
	int c;
	if(curCdBlock==NULL) {
		curCdBlock = CodeGetBlock();
		curCdBlock = CodeMarkLine(curCdBlock,yyline);
	}
	while((c=getc(fin))!=EOF) {
		if (c=='}')
			return;
		else if (c=='$') ScanTreeReference();
		else curCdBlock = CodeStoreChar(curCdBlock, c);
		if (c=='\n') yyline++;
		if (c=='"') ScanString(getput);
		else if (c=='\'') ScanChar();
		else if (c=='/') {
			if ((c=getc(fin))=='*') {
				curCdBlock = CodeStoreChar(curCdBlock, '*');
				ScanComment(getput);
			}
			else ungetc(c, fin);
		}
		else if (c=='{') {
			ScanCodeBlock();
			curCdBlock = CodeStoreChar (curCdBlock, '}');
		}
	}
	yyerror2("{ on line %d has no closing }", startline);
	nerrors++;
}
