%term ERROR
%{
#include "common.h"
#include "code.h"
#include "sym.h"
#include "machine.h"
#include <stdarg.h>

#define UNDEFINED -1
#define GIVENUP	-2

int	yyparse(void);

int debug_flag = 0;
int Dflag = 0;
int tflag = 0;
int sflag = 0;
int line_xref_flag = 0;
int ntrees = 0;
int nerrors = 0;
int fatalerrors = 0;
int tree_lineno;
Biobuf	*bin;
Biobuf	*bout;
Biobuf	*symfile;

Code *epilogue;

SymbolEntry ErrorSymbol;
%}
%union {
	Node *y_nodep;
	SymbolEntry *y_symp;
	Code *y_code;
	int y_int;
}
%start pattern_spec
%term K_NODE K_LABEL K_PROLOGUE K_CONST K_INSERT
%term K_COST K_ACTION
%token <y_symp> ID
%token <y_int> NUMBER
%token <y_code> CBLOCK
%type <y_nodep> tree tree_list
%type <y_symp> id_list label assoc_list assoc assoc_list2 assoc2
%type <y_symp> const_list const_def
%type <y_int> arity_spec constant
%type <y_symp> action cost
%%

pattern_spec: declarations patternsOrInserts = 
	{ if (nerrors==0) machine_build(); };

declarations: declarations decl
		| decl;

decl:	K_NODE assoc_list ';' = { SymbolEnterList ($2, A_NODE); }

	| K_NODE assoc_list2 ';'
		= {
			SymbolEnterList($2, A_NODE);
			SymbolCheckNodeValues();
		}

	| K_CONST const_list ';' = { SymbolEnterList ($2, A_CONST); }

	| K_LABEL id_list ';' = { SymbolEnterList ($2, A_LABEL); }

	| K_PROLOGUE CBLOCK ';' = { CodeWrite(bout, $2); CodeFreeBlock($2); }

	| K_COST ID CBLOCK ';'
		= { $2->sd.ca.code = $3; $2->sd.ca.assoc = NULL;
		 SymbolEnter ($2, A_COST); }

	| K_ACTION ID CBLOCK ';'
		= { $2->sd.ca.code = $3; $2->sd.ca.assoc = NULL;
		 SymbolEnter ($2, A_ACTION); }
 
	| K_PROLOGUE error ';'
	| K_ACTION error ';'
	| K_COST error ';'
	;

/*
 * The rule id_list, assoc_list, and const_list build lists of identifiers.
 * The field "next" is used as a link.  This field is also used by the symbol
 * table manager and thus the next field may not be used unless the identifiers
 * have not already been defined.
 */

id_list:
	id_list ID = {
				if(CheckIsUndefined($2)) {
					$2->next = $1;
					$$ = $2;
				} else $$ = $1;
			}
	| ID { if(CheckIsUndefined($1)) $$ = $1; else $$ = NULL; }
	| id_list error;

/*
 * Assoc_list2 allows user assignment of node values
 */
assoc_list2:
	assoc_list2 assoc2
		= {
			if($2->attr==A_ERROR)
				$$ = $1;
			else { $2->next = $1; $$ = $2; }
		}
	| assoc2 { $$ = $1->attr==A_ERROR ? NULL : $1; }
	| assoc_list2 error;

assoc_list:
	assoc_list assoc = { 
					if($2->attr==A_ERROR) $$ = $1;
			        	else { $2->next = $1; $$ = $2; }
				}
	| assoc { $$ = $1->attr==A_ERROR ? NULL : $1; }
	| assoc_list error;

assoc:	ID arity_spec
		= {
			if (CheckIsUndefined($1)) {
				$1->sd.arity = $2; $$ = $1;
			} else $$ = &ErrorSymbol;
		};

assoc2:
	ID arity_spec '=' constant {
				if(CheckIsUndefined($1)) {
					$1->unique = $4; $1->sd.arity = $2;
					$$ = $1;
				} else $$ = &ErrorSymbol;
			};

arity_spec:
	'(' constant ')' = { $$ = $2; };
	| '(' '*' ')' = { $$=GIVENUP; };
	| = { $$ = UNDEFINED; };

const_list:
	const_list const_def = {
				if ($2->attr==A_ERROR) $$ = $1;
				else { $2->next = $1; $$ = $2; }
			}
	| const_def { $$ = $1->attr==A_ERROR ? NULL : $1; }
	| const_list error;

const_def:
	ID '=' constant = {
				if(CheckIsUndefined($1)) {
					$1->sd.cvalue = $3; $$ = $1;
				} else $$ = &ErrorSymbol;
		};

constant:
	NUMBER
	| ID = {
		if(!CheckIsDefined($1)) $$ = UNDEFINED;
		else if($1->attr!=A_CONST) {
			sem_error("non-constant id used");
			$$ = -1;
		} else $$ = $1->sd.cvalue;
	};

patternsOrInserts: patternsOrInserts pattern
	| patternsOrInserts insert
	| insert
	| pattern
	;

insert:	K_INSERT CBLOCK ';' = { epilogue = CodeAppend(epilogue, $2); }
	| K_INSERT error ';'
	;

pattern:
	label ':' tree cost action ';' = {
		    if ($1->attr==A_ERROR) {
			error(0, "\"label: tree\" pair ignored");
			TreeFree($3);
		    } else {
			if(nerrors==0)
				cgotofn(SymbolEnterTreeIntoLabel($1,
					$3, $4, $5, tree_lineno));
			if(debug_flag&DB_TREE)
				TreePrint($3, 1);
		    }
		}
	| error ';' ;

action:	'=' CBLOCK { SymbolEntry *sp = SymbolAllocate (SymbolGenUnique());
		sp->sd.ca.code = $2; sp->sd.ca.assoc = NULL;
		SymbolEnter(sp, A_ACTION); $$ = sp; }
	| '=' ID { if(CheckIsDefined($2)) {
			if ($2->attr!=A_ACTION) {
				sem_error ("non action id: %s", $2->name);
				$$ = &ErrorSymbol;
			} else $$ = $2;
		} else $$ = &ErrorSymbol; }
	| { $$ = NULL;};

cost:	CBLOCK { SymbolEntry *sp = SymbolAllocate (SymbolGenUnique());
		sp->sd.ca.code = $1; sp->sd.ca.assoc = NULL;
		SymbolEnter (sp, A_COST); $$ = sp;
		}
	| ID { if (CheckIsDefined($1)) {
			if ($1->attr!=A_COST) {
				sem_error ("non cost id: %s", $1->name);
				$$ = &ErrorSymbol;
			} else $$ = $1;
		} else $$ = &ErrorSymbol; }
	| { $$ = NULL; };

/* labels play the same role that non-terminals do in YACC */
label:	ID = {
		tree_lineno = yyline;	/* record line no */
	        if(!CheckIsDefined($1))
			$1->attr = A_ERROR;
		else if(!is_label($1)) {
			sem_error("non label id: %s", $1->name);
			$1->attr = A_ERROR;
		}
		$$ = $1;
	};

tree:	ID {CheckIsNodeOrPred($1);} '(' tree_list ')'
	= {
		int count;
		Node *ap; 
		/* check the arity of the node */
		for(count=0, ap = $4; ap!=NULL; ap=ap->siblings) count++;
		switch($1->attr) {
			case A_NODE:
				set_arity($1, &$1->sd.arity, count);
				break;
		}

		$$ = TreeBuild ($1, $4);
	}
	| ID
	= {
		CheckIsDefined($1);
		switch ($1->attr) {
			case A_NODE:
				set_arity($1, &$1->sd.arity, 0);
				break;
		}
		$$ = TreeBuild ($1, NULL);
	};

tree_list: tree_list ',' tree = {
			/*
			 * build sibling list in reverse order TreeBuild will
			 * put it right later.
			 */
			$3->siblings = $1;
			$$ = $3;
		}
	| tree = { $1->siblings = NULL; $$ = $1; }
	| error { $$ = NULL; };
	| tree_list error;

%%

static char *process_suffix(char *);

void
set_arity(SymbolEntry *symp, int *arityp, int count)
{
	if(*arityp!=GIVENUP) {
		if (*arityp==UNDEFINED)
			*arityp = count;
		else if (*arityp!=count) {
			sem_error("inconsistent arity for %s", symp->name);
			*arityp = GIVENUP;
		}
	}
}

int
is_node(SymbolEntry *symp)
{
	return(symp->attr==A_NODE);
}

int
is_label(SymbolEntry *symp)
{
	return(symp->attr==A_LABEL);
}

void
CheckIsNodeOrPred(SymbolEntry *symp)
{
	if (symp->attr==A_ERROR)
		return;
	if (symp->attr!=A_NODE)
		sem_error ("non-node identifier %s", symp->name);
}

int
CheckIsUndefined(SymbolEntry *symp)
{
	if (symp->attr==A_ERROR)
		return(0);
	if (symp->attr!=A_UNDEFINED) {
		sem_error ("multiple declaration: %s", symp->name);
		return(0);
	} else return(1);
}

int
CheckIsDefined(SymbolEntry *symp)
{
	if (symp->attr==A_ERROR)
		return(0);
	if (symp->attr==A_UNDEFINED) {
		sem_error ("undefined identifier: %s", symp->name);
		symp->attr=A_ERROR;
		return(0);
	} else return(1);
}

void
yyerror(char *fmt, ...)
{
	char buf[4096];

	doprint(buf, buf + sizeof buf, fmt, &fmt+1);
	fprint(2, "line %d: %s", yyline, buf);
	if (token_buffer[0] != 0)
		fprint(2, " at \"%s\"\n", token_buffer);
}

void
yyerror2 (char *fmt, ...)
{
	char buf[4096];

	doprint(buf, buf + sizeof buf, fmt, &fmt+1);
	fprint(2, "line %d: %s\n", yyline, buf);
}

char *inFileName;
char *outFileName;
char prefixFile[100];
char *prefix_base = "/sys/lib/twig/walker";
char *suffix = "stdio";

void
usage(void)
{
	fprint(2, "usage: %s [-ACSlswx] file\n", argv0);
	exits("usage");
}

void
main(int argc, char *argv[])
{
	Biobuf *prefix;
	char *cp;
	int c;

	inFileName = NULL;

	ARGBEGIN{
	/* to see what each of these flags mean...
	 * see common.h */
	case 'd': {
		char *cp;
		for(cp = ARGF(); *cp!='\0'; cp++) 
			switch(*cp) {
			case 'l': debug_flag |= DB_LEX; break;
			case 'm': debug_flag |= DB_MACH; break;
			case 's': debug_flag |= DB_SYM; break;
			case 't': debug_flag |= DB_TREE; break;
			case 'M': debug_flag |= DB_MEM;	break;
			}
		}
		break;
	case 'D': Dflag++;		break;
	case 't': tflag++;		break;
	case 's': sflag++;		break;
	case 'A': suffix = "ansi";	break;
	case 'S': suffix = "stdio";	break;
	case 'C': suffix = "libc";	break;
	case 'w': suffix = process_suffix(ARGF()); break;
	case 'l': prefix_base = ARGF(); break;
	case 'x': line_xref_flag++; break;
	default:
		usage();
	}ARGEND

	if(argc != 1)
		usage();
	inFileName = argv[0];

	bin = Bopen(inFileName, OREAD);	
	if(!bin)
		error(1, "can't open %s", inFileName);
	cp = utfrrune(inFileName, L'.');
	if(!cp || strcmp(cp,".mt") != 0)
		error(1, "input file must have suffix .mt");
	bout = Bopen("walker.c", OWRITE);
	if(!bout)
		error(1, "can't open walker.c");
	if(!sflag){
		symfile = Bopen("symbols.h", OWRITE);
		if(!symfile)
			error(1, "can't open symbols.h");
	}

	ErrorSymbol.attr = A_LABEL;
	TreeInit();
	SymbolInit();
	LexInit();
	MachInit();
	yyparse();

	if(!sflag)
		SymbolDump();
	if(nerrors == 0) {
		if(!tflag) {
			sprint(prefixFile, "%s.%s", prefix_base, suffix);
			prefix = Bopen(prefixFile, OREAD);
			assert(prefix != NULL);
			if(Dflag)
				Bprint(bout, "#define DEBUG 1\n");
			if(line_xref_flag)
				Bprint(bout, "#define LINE_XREF 1\n");
			Bprint(bout, "#line 1 \"%s\"\n", prefixFile);
			while((c = Bgetc(prefix)) != Beof)
				Bputc(bout, c);
		}
		MachineGen();
		SymbolGenerateWalkerCode();
		CodeWrite(bout, epilogue);
		CodeFreeBlock(epilogue);
	}

	cleanup(0);
}

void
cleanup(int retcode)
{
	lexCleanup();
	if(!retcode) {
		SymbolFinish();
	}
	exits(retcode ? "errors" : 0);
}

void
error(int rc, char *fmt, ...)
{
	char buf[4096];

	doprint(buf, buf + sizeof buf, fmt, &fmt+1);
	fprint(2, "%s: %s\n", argv0, buf);
	if(rc)
		exits("errors");
}

void
sem_error(char *fmt, ...)
{
	char buf[4096];

	doprint(buf, buf + sizeof buf, fmt, &fmt+1);
	fprint(2, "line %d: %s\n", yyline, buf);
	nerrors++;
	fatalerrors++;
}

void
sem_error2(char *fmt, ...)
{
	char buf[4096];

	doprint(buf, buf + sizeof buf, fmt, &fmt+1);
	fprint(2, "%s\n", buf);
	nerrors++;
}

static char *
process_suffix(char *suf)
{
	extern int gen_table2;

	if(strcmp(suf,"exper") == 0){
		/* experimental walker */
		/* expect this to change alot */
		gen_table2++;
	}
	return suf;
}

void *
Malloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(!p)
		error(1, "out of memory");
	memset(p, 0, n);
	return p;
}
