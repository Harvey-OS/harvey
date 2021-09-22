/*
 * Ansitize, v.: to pollute code; converse of sanitize.
 */

#include "a.h"

void
usage(void)
{
	fprint(2, "usage: ansitize file.c\n");
	exits("usage");
}

char *cgrammar =
#include "cgrammar.c"
;

/*
 * Just enough of a type system to
 * analyze anonymous structures.
 */

enum { NONE, PTR, ARRAY, RUNE, CHAR, INT, UINTPTR, FN, ENUM, STRUCT, UNION, VOID };

typedef struct Arg Arg;
typedef struct Type Type;

struct Arg
{
	char *name;
	char *aname;
	Type *type;
	int istypedef;
};

struct Type
{
	int op;
	Type *sub;
	List *args;
	char *name;
};
#pragma varargck type "T" Type*

Type xvoidtype = { VOID, 0 };
Type *voidtype = &xvoidtype;

Type xchartype = { CHAR, 0 };
Type *chartype = &xchartype;

Type xinttype = { INT, 0 };
Type *inttype = &xinttype;

Type xstringtype = { PTR, &xchartype };
Type *stringtype = &xstringtype;

Type xrunetype = { RUNE, 0 };
Type *runetype = &xrunetype;

Type xuintptrtype = { UINTPTR, 0 };
Type *uintptrtype = &xuintptrtype;

Hash *structhash;
Hash *ast2typehash;
Hash *anonfixhash;
Hash *astdeclhash;
Hash *runestrings;
Hash *runestringnames;
Hash *renamehash;
Hash *reconstructhash;
Hash *ctypenames;

Alist alldecls;
Alist includedirs;
Alist preloads;

void	typeast(AST*);
void	dostruct(AST*, Type*, Name);

char *inputdir;

int		chatty;

int
typefmt(Fmt *fmt)
{
	int first;
	Arg *a;
	Type *t;
	
	t = va_arg(fmt->args, Type*);
	if(t == nil)
		return fmtstrcpy(fmt, "T");
	switch(t->op){
	default:
	case NONE:
		return fmtstrcpy(fmt, "???");
	case PTR:
		return fmtprint(fmt, "ptr to %T", t->sub);
	case ARRAY:
		return fmtprint(fmt, "array of %T", t->sub);
	case INT:
		return fmtstrcpy(fmt, "int");
	case UINTPTR:
		return fmtstrcpy(fmt, "uintptr");
	case RUNE:
		return fmtstrcpy(fmt, "Rune");
	case FN:
		fmtprint(fmt, "fn:(");
		first = 1;
		forlist(a, t->args){
			if(first)
				first = 0;
			else
				fmtprint(fmt, ", ");
			fmtprint(fmt, "%T", a->type);
		}
		return fmtprint(fmt, ")->%T", t->sub);
	case ENUM:
		return fmtprint(fmt, "enum %s", t->name);
	case STRUCT:
		return fmtprint(fmt, "struct %s", t->name);
	case UNION:
		return fmtprint(fmt, "union %s", t->name);
	case VOID:
		return fmtprint(fmt, "void");
	case CHAR:
		return fmtprint(fmt, "char");
	}
}

Arg*
findarg(List *l, char *name)
{
	Arg *a;
	
	for(; l; l=l->tl){
		a = l->hd;
		if(a->name && strcmp(a->name, name) == 0)
			return a;
	}
	return nil;
}

Arg*
mkarg(char *name, Type *t)
{
	Arg *a;
	
	a = malloc(sizeof *a);
	a->name = name;
	a->type = t;
	return a;
}

Type*
mktype(int op, Type *sub, List *args)
{
	Type *t;
	
	t = mallocz(sizeof *t, 1);
	t->op = op;
	t->sub = sub;
	t->args = args;
	return t;
}

Type*
ptrto(Type *t)
{
	return mktype(PTR, t, nil);
}

Type*
arrayof(Type *t)
{
	return mktype(ARRAY, t, nil);
}

Type*
fnof(Type *t, List *args)
{
	return mktype(FN, t, args);
}

Type*
structnamed(int su, Name tag)
{
	Type *t;
	
	if((t = hashget(structhash, tag)) == nil){
		t = mktype(su, nil, nil);
		t->name = tag;
		hashput(structhash, tag, t);
	}
	return t;
}

int
su(AST *ast)
{
	if(matchrule(ast, "structunion", "'union", nil))
		return UNION;
	return STRUCT;
}

Type*
isvoid(Type *t)
{
	if(t && t->op == VOID)
		return t;
	return nil;
}

Type*
ischar(Type *t)
{
	if(t && t->op == CHAR)
		return t;
	return nil;
}

Type*
isrune(Type *t)
{
	if(t && t->op == RUNE)
		return t;
	return nil;
}

Type*
isarray(Type *t)
{
	if(t && t->op == ARRAY)
		return t->sub;
	return nil;
}

Type*
isptr(Type *t)
{
	if(t && (t->op == PTR || t->op == ARRAY))
		return t->sub;
	return nil;
}

Type*
isuintptr(Type *t)
{
	if(t && t->op == UINTPTR)
		return t;
	return nil;
}

Type*
isfn(Type *t)
{
	if(t && t->op == FN)
		return t->sub;
	return nil;
}

Type*
issu(Type *t)
{
	if(t && (t->op == STRUCT || t->op == UNION))
		return t;
	return nil;
}

Type*
asttype(AST *a)
{
	return hashget(ast2typehash, a);
}

Type*
setasttype(AST *a, Type *t)
{
	hashput(ast2typehash, a, t);
	return t;
}

/*
 * Find the leftmost node in the AST tree with non-nil s or pre.
 */
AST*
findpre(AST *ast)
{
	int i;
	AST *a;
	
	if(ast->s || ast->pre)
		return ast;

	for(i=0; i<ast->nright; i++)
		if((a = findpre(*ast->right[i])) != nil)
			return a;
	return nil;
}

/*
 * Tentative typing.
 */
int tentative;
typedef struct Error Error;
struct Error
{
	Error *outer;
	jmp_buf jb;
};
Error *err;

void
_pusherror(Error *e)
{
	e->outer = err;
	err = e;
	tentative = 1;
}

#define waserror(e) (_pusherror((e)), setjmp((e)->jb))

void
poperror(Error *e)
{
	err = e->outer;
	tentative = (err!=nil);
}

void
kaboom(void)
{
	Error *e;

	if(err == nil)
		abort();
	
	e = err;
	err = e->outer;
	tentative = (err!=nil);
	longjmp(e->jb, 1);
}

/*
 * Just enough of a symbol table to determine types of valid expressions.
 */
typedef struct Scope Scope;
struct Scope
{
	Alist al;
	Scope *outer;
	Hash *h;
	int nau;	/* number of anonymous unions */
	int nas;	/* number of anonymous structures */
	Type *type;
};

Scope global;
Scope *scope = &global;

void
pushscope(Scope *s)
{
	memset(s, 0, sizeof *s);
	s->outer = scope;
	s->h = mkhash();
	scope = s;
}

void
popscope(Scope *s)
{
	scope = s->outer;
}

Type*
looktype(Name name)
{
	Arg *a;
	Scope *s;
	
	for(s=scope; s; s=s->outer)
		if((a = hashget(s->h, name)) != nil)
			return a->type;
	if(0){
		print("no %s in %p\n", name, scope);
		for(s=scope; s; s=s->outer)
			print("\tin %p\n", s);
	}
	return nil;
}

int
istypedef(Name name)
{
	Arg *a;
	Scope *s;
	
	for(s=scope; s; s=s->outer)
		if((a = hashget(s->h, name)) != nil)
			return a->istypedef;
	return 0;
}

/*
 * Declaration context.
 */
typedef struct Decl Decl;
struct Decl
{
	Decl *outer;
	Type *type;
	Type *lasttype;
	int istypedef;
	int anonymous;
	int ndeclared;
};

Decl globdecl;	/* so that decl is never nil */
Decl *decl = &globdecl;

void
pushdecl(Decl *d)
{
	memset(d, 0, sizeof *d);
	d->outer = decl;
	decl = d;
}

void
popdecl(Decl *d)
{
	decl = d->outer;
}

/*
 * AST helpers.
 */
static char Enoname[] = "no-name";

Name
idname(AST *ast)
{
	if(ast && ast->sym && ast->sym->name && strcmp(ast->sym->name, "id") == 0)
		return nameof((*ast->right[0])->s);
	if(ast->nright > 0)
		return idname(*ast->right[0]);
	return Enoname;
}

/*
 * Declarations
 */
void
declare(Name name, Type *t, AST *ast)
{
	char *p;
	Name x;
	Arg *a;
	
	if((!name && !t)){
		if (0)
		fprint(2, "declare %s %T %p in %p %p\n", name, t,
			getcallerpc(&name), scope, &scope->al);
		return;
	}
	if(name == Enoname)
		abort();
	decl->lasttype = t;
	a = mkarg(name, t);
	a->istypedef = decl->istypedef;
//fprint(2, "declare %s %T, %s typedef\n", a->name, a->type, a->istypedef ? "is" : "not");
	if(name)
		hashput(scope->h, name, a);
	else{
		if(t->name == nil){
			if(t->op == UNION){
				++scope->nau;
				if(scope->nau == 1)
					a->aname = "u";
				else
					a->aname = smprint("u%d", scope->nau-1);
			}else
				a->aname = smprint("_%d", ++scope->nas);
		}else
			a->aname = smprint("_%s", t->name);
		if(issu(scope->type) && scope->type->name){
			p = smprint("%s.%s", scope->type->name, a->aname);
			if((x = hashget(renamehash, nameof(p))) != nil)
				a->aname = x;
		}
		if((x = hashget(renamehash, nameof(a->aname))) != nil)
			a->aname = x;
	}
	hashput(astdeclhash, a, ast);
	listadd(&scope->al, a);
	listadd(&alldecls, a);
}

void
redeclare(List *l)
{
	Arg *a;
	
	forlist(a, l)
		declare(a->name, a->type, hashget(astdeclhash, a));
}

/*
 * Pull out the list of function arguments.
 */
List*
fnargs(AST *ast)
{
	Scope sc;

	pushscope(&sc);
	typeast(ast);
	popscope(&sc);
	return sc.al.first;
}

/*
 * Process a single declarator.
 */
Type*
typedecor(AST *ast, Type *t)
{
	int i;

	if(matchrule(ast, "sudecor", "decor", nil)){
		t = typedecor(*ast->right[0], t);
		setasttype(ast, t);
		return t;
	}
	
	if(!match(ast, "decor") && !match(ast, "abdecor") && !match(ast, "abdec1"))
		return asttype(ast);
	
	if(matchrule(ast, "abdecor", nil)){
		setasttype(ast, t);
		declare(nil, t, ast);
	}else if(matchrule(ast, "decor", "tag", nil)){
		declare(idname(ast), t, ast);
	}else if(matchrule(ast, "abdecor", "'*", "qname*", "abdecor", nil)
		|| matchrule(ast, "decor", "'*", "qname*", "decor", nil))
		t = typedecor(*ast->right[2], ptrto(t));
	else if(matchrule(ast, "abdecor", "abdec1", nil))
		t = typedecor(*ast->right[0], t);
	else if(matchrule(ast, "abdec1", "abdec1", "'(", "fnarg,?", "')", nil)
		|| matchrule(ast, "decor", "decor", "'(", "fnarg,?", "')", nil))
		t = typedecor(*ast->right[0], fnof(t, fnargs(*ast->right[2])));
	else if(matchrule(ast, "abdec1", "abdecor", "'[", "expr?", "']", nil)
		|| matchrule(ast, "decor", "decor", "'[", "expr?", "']", nil))
		t = typedecor(*ast->right[0], arrayof(t));
	else if(matchrule(ast, "abdec1", "'(", "abdecor", "')", nil)
		|| matchrule(ast, "decor", "'(", "decor", "')", nil))
		t = typedecor(*ast->right[1], t);
	else{
		print("weird declarator - %A\n", ast);
		print("%s:", ast->sym->name);
		for(i=0; i<ast->nright; i++)
			print(" %s", (*ast->right[i])->sym->name);
		print("\n");
	}
	
	setasttype(ast, t);
	return t;
}

/*
 * Assign types to structure derefs.
 */
char*
resolveanon(AST *ast, Type *t, Name name)
{
	Arg *a;
	char *p;
	
	assert(issu(t));

	if((a = findarg(t->args, name)) != nil){
		if(ast)
			setasttype(ast, a->type);
		return name;
	}
	forlist(a, t->args){
		if(a->type == nil || a->name != nil)
			continue;
		if(a->type->name == name){
			if(ast)
				setasttype(ast, a->type);
			return a->aname;
		}
		if(issu(a->type) && (p = resolveanon(ast, a->type, name)) != nil)
			return nameof(smprint("%s.%s", a->aname, p));
	}
	return nil;
}

/*
 * Figure out the type of a structure -> or . expression.
 * Allow anonymous structs/unions and record path through them.
 */
void
dostruct(AST *ast, Type *t, Name name)
{
	char *p;
	Arg *a;
	
	if((a = findarg(t->args, name)) != nil){
		setasttype(ast, a->type);
		return;
	}
	if((p = resolveanon(ast, t, name)) != nil)
		hashput(anonfixhash, ast, p);
	else{
		fprint(2, "%A: cannot find '%s' in %T\n", ast, name, t);
		if(1){
			fprint(2, "%T\n", t);
			forlist(a, t->args)
				fprint(2, "\t%s: %T\n", a->name, a->type);
		}
	}
}

int
tryinclude(char *dir, char *file)
{
	char *s;
	AST *ast;
	
	s = smprint("%s/%s", dir, file);
	if(access(s, 0) < 0)
		return -1;
	ast = parsefile(s);
	if(ast == nil)
		fprint(2, "error parsing %s\n", s);
	typeast(ast);
	return 0;
}

void
doinclude(char *line)
{
	int c;
	char *p, *q;
	char *dir;
	
	p = strpbrk(line, "\"<");
	if(p == nil){
	bad:
		fprint(2, "unrecognized #include line: %s\n", line);
		return;
	}
	c = *p=='"' ? '"' : '>';
	q = strchr(p+1, c);
	if(q == nil)
		goto bad;
	*q = 0;

	if(*(p+1) == '/'){
		if(tryinclude("", p+1) >= 0){
			*q = c;
			return;
		}
		goto end;
	}
	if(*p == '"' && inputdir && tryinclude(inputdir, p+1) >= 0){
		*q = c;
		return;
	}
	forlist(dir, includedirs.first)
		if(tryinclude(dir, p+1) >= 0){
			*q = c;
			return;
		}
end:
	*q = c;
	c = *++q;
	*q = 0;
	fprint(2, "cannot #include %s\n", p);
	*q = c;
}

/*
 * Assign types to variables.
 */
void
typeast(AST *ast)
{
	int i, worked, pushedscope, pusheddecl;
	Decl dc;
	Name tag, name;
	Scope sc;
	Type *t;
	Error e;
	Alist al;

	if(ast == nil)
		return;

	if(ast->sym == nil){	/* merge! */
		for(i=0; i<ast->nright; i++){
			/*
			 * check it, but leave no trace.
			 */
			pushscope(&sc);
			pushdecl(&dc);
			al = alldecls;
			worked = 0;
			if(!waserror(&e)){
				typeast(*ast->right[i]);
				poperror(&e);
				worked = 1;
			}
			popscope(&sc);
			popdecl(&dc);
			alldecls = al;
			
			if(worked)
				goto unmerge;
// fprint(2, "not %B\n", *ast->right[i]);
		}
		
		fprint(2, "warning: no choices for unmerge\n");
		i = 0;
	unmerge:
// fprint(2, "using %B\n", *ast->right[i]);
		*ast->right[0] = *ast->right[i];
		ast->nright = 1;
		typeast(*ast->right[0]);
		return;		
	}
	
	if(match(ast, "include")){
		doinclude((*ast->right[0])->s);
		return;
	}

	/* function definition - handle timing of child typing specially */
	if(matchrule(ast, "fndef", "typeclass", "decor", "decl*", "block", nil)){
		pushdecl(&dc);
		typeast(*ast->right[0]);
		typeast(*ast->right[1]);
		pushscope(&sc);
		typeast(*ast->right[2]);
		if(isfn(t = asttype(*ast->right[1])))
			redeclare(t->args);
		typeast(*ast->right[3]);
		popscope(&sc);
		popdecl(&dc);
		return;
	}

	/* empty list for structure declarator - anonymous element */
	if(matchrule(ast, "sudecor,?", nil)){
		decl->ndeclared++;
		declare(nil, decl->type, ast);
		return;
	}

	/* declarator - handle with special declarator parser */
	if(match(ast, "decor") || match(ast, "sudecor") || match(ast, "abdecor")){
		decl->ndeclared++;
		typedecor(ast, decl->type);
		return;
	}

	/* enum declarator - always integer */
	if(match(ast, "edecl")){
		declare(idname(*ast->right[0]), inttype, ast);
		setasttype(ast, inttype);
		return;
	}
	
	/* blocks and structure definitions start new scope */
	pushedscope = 0;
	if(match(ast, "block") || match(ast, "abtype")
	|| matchrule(ast, "typespec", "structunion", "tag?", "'{", "sudecl+", "'}", nil)){
		pushedscope = 1;
		pushscope(&sc);
		if(match(ast, "typespec")){
			tag = idname(*ast->right[1]);
			if(tag != Enoname)
				sc.type = structnamed(su(*ast->right[0]), tag);
		}
	}

	/* new declaration contexts */
	pusheddecl = 0;
	if(match(ast, "decl") || match(ast, "sudecl") || match(ast, "abtype")){
		pusheddecl = 1;
		pushdecl(&dc);
	}
	
	/* type the children */
	for(i=0; i<ast->nright; i++)
		typeast(*ast->right[i]);

	/* pop any contexts */
	if(pusheddecl)
		popdecl(&dc);
	if(pushedscope)
		popscope(&sc);

	/* default: use first typed child as type of expr */
	for(i=0; i<ast->nright; i++){
		if((t = asttype(*ast->right[i])) != nil){
			setasttype(ast, t);
			break;
		}
	}

	/* infer types of expressions */

	/* names get looked up - check typedefness too. */
	if(match(ast, "name") || match(ast, "typename")){
		name = idname(*ast->right[0]);
		if(tentative && match(ast, "typename") != istypedef(name)){
			if(0) fprint(2, "kaboom: %s is%s a typedef\n", 
				match(ast, "typename") ? "not" : "", name);
			kaboom();
		}
		t = looktype(name);
		if(t == nil && chatty)
			fprint(2, "unknown type for %s\n", name);
		if(strcmp(name, "Rune") == 0)
			t = runetype;
		else if(strcmp(name, "uintptr") == 0)
			t = uintptrtype;
		setasttype(ast, t);
	}

	if(match(ast, "expr,"))
		setasttype(ast, asttype(*ast->right[ast->nright-1]));
	if(match(ast, "expr")){
		t = asttype(ast);
		if(matchrule(ast, "expr", "number", nil))
			setasttype(ast, inttype);
		else if(matchrule(ast, "expr", "string+", nil) || matchrule(ast, "expr", "Lstring+", nil))
			setasttype(ast, stringtype);
		else if(matchrule(ast, "expr", "char", nil) || matchrule(ast, "expr", "Lchar", nil))
			setasttype(ast, inttype);
		else if(matchrule(ast, "expr", "'&", "expr", nil))
			setasttype(ast, ptrto(t));
		else if(matchrule(ast, "expr", "'*", "expr", nil) && isptr(t))
			setasttype(ast, t->sub);
		else if(matchrule(ast, "expr", "expr", "'[", "cexpr", "']", nil) && isptr(t))
			setasttype(ast, t->sub);
		else if(matchrule(ast, "expr", "expr", "'(", "expr,?", "')", nil)){
			if(isfn(isptr(t)))
				t = t->sub;
			if(isfn(t))
				setasttype(ast, t->sub);
		}else if(matchrule(ast, "expr", "expr", "'->", "tag", nil)){
			if(!issu(isptr(t))){
				if(chatty)
					fprint(2, "not pointer to structure: (%T) %A\n", t, ast);
			}else
				dostruct(ast, t->sub, idname(*ast->right[2]));
		}
		else if(matchrule(ast, "expr", "expr", "'.", "tag", nil)){
			if(!issu(t)){
				if(chatty)
					fprint(2, "not structure: (%T) %A\n", t, ast);
			}else
				dostruct(ast, t, idname(*ast->right[2]));
		}
	}

	/* record whether decl is a typedef */
	if(match(ast, "'typedef"))
		decl->istypedef = 1;

	/* type names specify type */
	if(match(ast, "tname")){
		if(matchrule(ast, "tname", "'void", nil))
			setasttype(ast, voidtype);
		else if(matchrule(ast, "tname", "'char", nil))
			setasttype(ast, chartype);
		else
			setasttype(ast, inttype);
	}

	/* save type class */
	if(match(ast, "typeclass"))
		decl->type = asttype(ast);

	if(match(ast, "abtype"))
		setasttype(ast, dc.lasttype);
		
	/* process enum, struct, union declarations */
	if(matchrule(ast, "typespec", "enum", "tag", nil)
	|| matchrule(ast, "typespec", "enum", "tag?", "'{", "edecl,", "comma?", "'}", nil)){
		if(idname(*ast->right[1]) != Enoname){
			setasttype(ast, t = mktype(ENUM, nil, nil));
			t->name = idname(*ast->right[1]);
		}
	}
	if(matchrule(ast, "typespec", "structunion", "tag", nil)){
		tag = nameof(idname(*ast->right[1]));
		t = structnamed(su(*ast->right[0]), tag);
		setasttype(ast, t);
	}
	if(matchrule(ast, "typespec", "structunion", "tag?", "'{", "sudecl+", "'}", nil)){
		tag = idname(*ast->right[1]);
		if(tag == Enoname)
			t = mktype(su(*ast->right[0]), nil, nil);
		else
			t = structnamed(su(*ast->right[0]), tag);
		assert(pushedscope);
		t->args = sc.al.first;
		setasttype(ast, t);
	}
	if(chatty > 1)
		fprint(2, "%A: %T\n", ast, asttype(ast));
}

/*
 * Name anonymous structure declarations inside structures.
 */
void
anonstructs(void)
{
	Arg *a;
	AST *ast;

	forlist(a, alldecls.first){
		if(a->name == nil)
		if((ast = hashget(astdeclhash, a)) != nil)
		if(match(ast, "sudecor,?")){
			ast->s = a->aname;
			ast->pre = " ";
		}
	}
}

/*
 * Rewrite references via anonymous structure declarations
 * to use the new names.
 */
void
anonrefs(AST *ast)
{
	int i;
	char *p;
	
	if(ast == nil)
		return;

	for(i=0; i<ast->nright; i++)
		anonrefs(*ast->right[i]);

	if(matchrule(ast, "expr", "expr", "'->", "tag", nil)
	 || matchrule(ast, "expr", "expr", "'.", "tag", nil))
	if((p = hashget(anonfixhash, ast)) != nil){
		(*ast->right[2])->s = p;
	}
}

/*
 * Is this ast, a declarator, inside a fndef?
 */
AST*
infndef(AST *ast)
{
	for(; ast; ast=ast->up ? *ast->up : nil){
		if(match(ast, "decl"))
			return nil;
		if(match(ast, "fndef"))
			return ast;
	}
	return nil;
}

/*
 * Rewrite function definitions to name anonymous arguments _%d.
 */
void
anonfnargs(void)
{
	int n;
	Arg *a;
	AST *ast;
	Arg *fna;
	
	forlist(a, alldecls.first){
		if(!isfn(a->type))
			continue;
		if(!infndef(hashget(astdeclhash, a)))
			continue;
		n = 0;
		forlist(fna, a->type->args){
			if(fna->name == nil && !isvoid(fna->type)){
				ast = hashget(astdeclhash, fna);
				ast->s = smprint(" _%d", ++n);
			}
		}
	}
}

/*
 * Rewrite *pa for passing to a function expecting type t.
 */
AST**
rewriteptr(AST **pa, Type *t)
{
	AST *a, *aa;
	Type *at;
	char *p;
	
	a = *pa;
	at = asttype(a);
	if(issu(isptr(at)) && issu(isptr(t)))
	if(at->sub != t->sub)
	if((p = resolveanon(nil, at->sub, t->sub->name)) != nil){
		aa = findpre(a);
		if(matchrule(a, "expr", "name", nil) 
		|| matchrule(a, "expr", "expr", "'->", "tag", nil)
		|| matchrule(a, "expr", "expr", "'.", "tag", nil)){
			aa->pre = smprint("%s&", aa->pre ? aa->pre : "");
			a->post = smprint("->%s%s", p, a->post ? a->post : "");
		}
		else if(matchrule(a, "expr", "'&", "expr", nil))
			a->post = smprint(".%s%s", p, a->post ? a->post : "");
		else{
			aa->pre = smprint("%s&(", aa->pre ? aa->pre : "");
			a->post = smprint(")->%s%s", p, a->post ? a->post : "");
		}
	}
	return pa;
}

/*
 * Promote ptr arguments for function calls and assignments
 */
void
promoteptrs(AST *ast)
{
	int i;
	Type *t, *tt;
	void **args;
	AST *a;
	
	if(ast == nil)
		return;
	for(i=0; i<ast->nright; i++)
		promoteptrs(*ast->right[i]);

	if(matchrule(ast, "expr", "expr", "'(", "expr,?", "')", nil)){
		t = asttype(*ast->right[0]);
		if(t == nil)
			return;
		a = *ast->right[2];
		if(a->nright){
			a = *a->right[0];
			args = listflatten(t->args);
			for(i=listlen(t->args)-1; i>=0 && a && a->nright>0; 
					i--, a=(a->nright==3 ? *a->right[0]: nil)){
				tt = ((Arg*)args[i])->type;
				a->right[a->nright-1] = rewriteptr(a->right[a->nright-1], tt);
			}
		}
	}
	if(matchrule(ast, "expr", "expr", "'=", "expr", nil))
		ast->right[2] = rewriteptr(ast->right[2], asttype(*ast->right[0]));
}

/*
 * Rewrite Unicode names to use plain ASCII.
 * The only Unicode names in the Plan 9 sources is the 
 * occasional Greek letter, so we handle that specially.
 */
int
hasunicode(char *name)
{
	char *p;
	
	for(p=name; *p; p++)
		if((uchar)*p >= Runeself)
			return 1;
	return 0;
}

struct {
	char *s;
	Rune r;
} runenames[] = {
	{"Alpha", 945-32},
	{"Beta", 946-32},
	{"Gamma", 947-32},
	{"Delta", 948-32},
	{"Epsilon", 949-32},
	{"Zeta", 950-32},
	{"Eta", 951-32},
	{"Theta", 952-32},
	{"Iota", 953-32},
	{"Kappa", 954-32},
	{"Lambda", 955-32},
	{"Mu", 956-32},
	{"Nu", 957-32},
	{"Xi", 958-32},
	{"Omicron", 959-32},
	{"Pi", 960-32},
	{"Rho", 961-32},
	{"VSigma", 962-32},
	{"Sigma", 963-32},
	{"Tau", 964-32},
	{"Upsilon", 965-32},
	{"Phi", 966-32},
	{"Chi", 967-32},
	{"Psi", 968-32},
	{"Omega", 969-32},
	{"alpha", 945},
	{"beta", 946},
	{"gamma", 947},
	{"delta", 948},
	{"epsilon", 949},
	{"zeta", 950},
	{"eta", 951},
	{"theta", 952},
	{"iota", 953},
	{"kappa", 954},
	{"lambda", 955},
	{"mu", 956},
	{"nu", 957},
	{"xi", 958},
	{"omicron", 959},
	{"pi", 960},
	{"rho", 961},
	{"vsigma", 962},
	{"sigma", 963},
	{"tau", 964},
	{"upsilon", 965},
	{"phi", 966},
	{"chi", 967},
	{"psi", 968},
	{"omega", 969}
};

char*
ununicode(char *name)
{
	int i, j;
	Fmt fmt;
	Rune *r;
	
	r = runesmprint("%s", name);
	fmtstrinit(&fmt);
	for(i=0; r[i]; i++){
		if(r[i] < Runeself){
			fmtrune(&fmt, r[i]);
			continue;
		}
		/* C99 says you can do this!
		fmtprint(&fmt, "\u%04X", r[i]);
		*/
		for(j=0; j<nelem(runenames); j++){
			if(runenames[j].r == r[i]){
				fmtstrcpy(&fmt, runenames[j].s);
				goto continue2;
			}
		}
		fmtprint(&fmt, "_%04X", r[i]);
	continue2:;
	}
	return fmtstrflush(&fmt);
}

void
unicodenames(AST *ast)
{
	int i;
	Name name;
	
	if(ast == nil)
		return;
	for(i=0; i<ast->nright; i++)
		unicodenames(*ast->right[i]);
	if(match(ast, "id")){
		name = idname(ast);
		if(hasunicode(name))
			(*ast->right[0])->s = ununicode(name);
	}
}

/*
 * Parse a single character in a string or char constant.
 */
static char *escaper = "abfnrtv\'\"\\0";
static char *escapee = "\a\b\f\n\r\t\v\'\"\\\0";

int
lchartorune(Rune *r, char *t, int wide)
{
	int i, n;
	char *p;
	char buf[10];
	
	if(*t != '\\')
		return chartorune(r, t);
	t++;
	if(*t == 'x'){
		t++;
		if(wide)
			n = 4;
		else
			n = 2;
		for(i=0; i<n; i++)
			if(t[i]==0)
				goto error;
		memmove(buf, t, n);
		buf[n] = 0;
		*r = strtol(buf, &p, 16);
		if(p != buf+n)
			goto error;
		return 2+n;
	}
	if(*t == '0'){
		t++;
		if(wide)
			n = 6;
		else
			n = 3;
		for(i=0; i<n && '0' <= *t && *t <= '7'; i++)
			;
		n = i;
		memmove(buf, t, n);
		buf[n] = 0;
		*r = strtol(buf, &p, 8);
		if(p != buf+n)
			goto error;
		return 2+n;
	}
	if((p=strchr(escaper, *t)) != nil){
		*r = escapee[p-escaper];
		return 2;
	}
	chartorune(r, t);
	fprint(2, "unknown escape sequence \\%C\n", *r);

error:
	*r = Runeerror;
	return 1;
}

/*
 * Parse rune strings.
 */
Rune*
crunestring(char *s, int *len)
{
	int nr;
	Rune *r;
	
	if(*s++ != 'L' || *s++ != '\"')
		return nil;
	nr = 0;
	r = nil;
	while(*s != '\"'){
		if(*s == 0)
			return nil;
		if(nr%16 == 0)
			r = realloc(r, (nr+16)*sizeof(Rune));
		s += lchartorune(&r[nr], s, 1);
		if(r[nr] == Runeerror)
			return nil;
		nr++;
	}
	if(nr%16 == 0)
		r = realloc(r, (nr+1)*sizeof(Rune));
	r[nr++] = 0;
	*len = nr;
	return r;
}

/*
 * Format a single rune without using rune extensions.
 */
void
runeprint1(char *s, Rune r, int wid)
{
	char *p;
	
	if(r == 0)
		strcpy(s, "0");
	else if(r < Runeself && (p = strchr(escapee, r)) != nil)
		sprint(s, "'\\%c'", escaper[p-escapee]);
	else if(r < Runeself && r >= ' ')
		sprint(s, "'%c'", r);
	else if(r < ' ')
		sprint(s, "0x%.*ux/*^%C*/", wid, r, r+'@');
	else
		sprint(s, "0x%.*ux/*%C*/", wid, r, r);
}


/*
 * Rewrite Rune character constants L'x' into
 * either (Rune)'x' or (Rune)0x1234.
 */
void
runechars(AST *ast)
{
	int i;
	char *s, *t;
	Rune r;
	char buf[20];
	
	if(ast == nil)
		return;
	for(i=0; i<ast->nright; i++)
		runechars(*ast->right[i]);

	if(match(ast, "Lchar")){
		s = (*ast->right[0])->s;
		t = s;
		if(*t++ != 'L' || *t++ != '\'' || *t == 0){
		bad:
			fprint(2, "bad rune character: %s\n", s);
			return;
		}
		t += lchartorune(&r, t, 1);
		if(r == Runeerror)
			goto bad;
		if(*t++ != '\'' || *t != 0)
			goto bad;
		runeprint1(buf, r, 4);
		(*ast->right[0])->s = strdup(buf);
	}
	if(match(ast, "char")){
		s = (*ast->right[0])->s;
		t = s;
		if(*t++ != '\'' || *t == 0){
		cbad:
			fprint(2, "bad character: %s\n", s);
			return;
		}
		t += lchartorune(&r, t, 0);
		if(r == Runeerror)
			goto cbad;
		if(*t++ != '\'' || *t != 0)
			goto cbad;
		runeprint1(buf, r, 2);
		(*ast->right[0])->s = strdup(buf);
	}
}

/*
 * Rewrite Rune strings.
 * If the string is an array initializer, like Rune x[] = L"hi",
 * then we can replace it with a constant array { 'h', 'i', 0 }.
 * If the string is being used as an expression, then we
 * must work harder.  We create a new static Rune Lhi[] = { 'h', 'i', 0 },
 * and then rewrite the L"hi" into Lhi.
 * Rune string concatenation L"hello " L"world" is not handled.
 */

char*
runestringinit(Rune *r, int len)
{
	int i;
	char buf[20];	
	Fmt fmt;
	
	fmtstrinit(&fmt);
	fmtprint(&fmt, "{");
	for(i=0; i<len; i++){
		if(i>0)
			fmtprint(&fmt, ",");
		if(i==len-1 && r[i] == 0)
			fmtprint(&fmt, " ");
		runeprint1(buf, r[i], 4);
		fmtprint(&fmt, "%s", buf);
	}
	fmtprint(&fmt, "}");
	return fmtstrflush(&fmt);
}

struct {
	int c;
	char *name;
} runenametab[] = {
	'\0',	"L_nul",
	'\r',	"L_cr",
	'\n',	"L_nl",
	'\t',	"L_tab",
	'/',	"L_slash",
	'.',	"L_dot",
	'-',	"L_dash",
	'+',	"L_plus",
	'#',	"L_sharp",
	'!',	"L_bang",
	'?',	"L_quest",
	'<',	"L_lt",
	'>',	"L_gt",
	'&',	"L_amp",
	'@',	"L_at",
	'$',	"L_dollar",
	'%',	"L_pcnt",
	'^',	"L_carat",
	'*',	"L_star",
	'(',	"L_lparen",
	')',	"L_rparen",
	'_',	"L_under",
	'=',	"L_equal",
	'{',	"L_lbrace",
	'}',	"L_rbrace",
	'[',	"L_lbrack",
	']',	"L_rbrack",
	'|',	"L_pipe",
	'\\',	"L_backslash",
	'~',	"L_twiddle",
	'`',	"L_backtick",
	':',	"L_colon",
	';',	"L_semi",
	'"',	"L_quote",
	'\'',	"L_tick",
	',',	"L_comma",
};

Name
makename(Rune *r, int len)
{
	static int did;
	int i;
	char *s, *t;
	Name x;
	
	if(!did){
		did = 1;
		for(i=0; i<nelem(runenametab); i++)
			hashput(runestringnames, nameof(runenametab[i].name), 
				runesmprint("%C", runenametab[i].c));
	}

	if(len == 1)	/* ""; 1 for NUL */
		return nameof("L_NUL");

	if(len == 2){
		for(i=0; i<nelem(runenametab); i++)
			if(r[0] == runenametab[i].c)
				return nameof(runenametab[i].name);
	}
	
	s = malloc(len*2+20);
	t = s;
	*t++ = 'L';
	*t++ = '_';
	for(i=0; i<len; i++){
		if(('A' <= r[i] && r[i] <= 'Z')
		|| ('a' <= r[i] && r[i] <= 'z')
		|| ('0' <= r[i] && r[i] <= '9')
		|| r[i] == '_')
			*t++ = r[i];
		if(r[i] == '\r'){
			*t++ = 'C';
			*t++ = 'R';
		}
		if(r[i] == '\n'){
			*t++ = 'N';
			*t++ = 'L';
		}
		if(r[i] == ' ')
			*t++ = '_';
	}
	*t = 0;
	x = nameof(s);
	if(hashget(runestringnames, x) == nil)
		return x;
	for(i=0;; i++){
		sprint(t, "%d", i);
		x = nameof(s);
		if(hashget(runestringnames, x) == nil)
			return x;
	}
}

char*
runestringcopy(Rune *r, int len, Alist *al)
{
	char *s;
	Name x;
	int i;
	
	/*
	 * If there's already a copy, reuse it.
	 */
	
	/* clumsy - nameof requires NUL-terminated char strings */
	s = malloc(len*4+1);
	for(i=0; i<len; i++)
		sprint(s+4*i, "%.4ux", r[i]);
	x = nameof(s);

	if((s = hashget(runestrings, x)) == nil){
		s = makename(r, len);
		listadd(al, smprint("static Rune %s[] = %s;\n", s, runestringinit(r, len)));
		hashput(runestrings, x, s);
		hashput(runestringnames, s, x);
	}
	return s;
}

int
isrunearraydecor(AST *ast)
{
	Type *t;
	
	t = asttype(ast);
	if(t == nil){
		fprint(2, "no type: %A\n", ast);
		return 0;
	}
	return isrune(isarray(t)) != nil;
}

void
dorunestrings(AST *ast, int init, Alist *al)
{
	int i, n;
	char *s;
	Rune *r;
	Fmt fmt;
	Alist xal;
	
	if(ast == nil)
		return;
	/*
	 * Are we being used as an initializer?
	 */
	if(matchrule(ast, "idecor", "decor", "'=", "init", nil) && isrunearraydecor(*ast->right[0]))
		init = 1;
	if(match(ast, "expr") && ast->up && !match(*ast->up, "init"))
		init = 0;

	/*
	 * Handle a single Lstring.
	 */
	if(match(ast, "Lstring")){
		s = (*ast->right[0])->s;
		r = crunestring(s, &n);
		if(r == nil){
			fprint(2, "bad rune string: %s\n", s);
			return;
		}
		if(init)
			(*ast->right[0])->s = runestringinit(r, n);
		else
			(*ast->right[0])->s = runestringcopy(r, n, al);
	}

	/*
	 * Each top-level statement gets its own list of strings.
	 */
	listreset(&xal);
	if(match(ast, "prog1"))
		al = &xal;

	for(i=0; i<ast->nright; i++)
		dorunestrings(*ast->right[i], init, al);

	/*
	 * Emit any necessary strings before this ast.
	 */
	if(match(ast, "prog1") && xal.first){
		ast = findpre(ast);
		fmtstrinit(&fmt);
		if(ast->pre)
			fmtstrcpy(&fmt, ast->pre);
		forlist(s, xal.first)
			fmtstrcpy(&fmt, s);
		ast->pre = fmtstrflush(&fmt);
	}
}

/*
 * Comment out pragma lines.
 */
void
commentpragmas(AST *ast)
{
	int i;
	
	if(ast == nil)
		return;
	for(i=0; i<ast->nright; i++)
		commentpragmas(*ast->right[i]);
		
	if(match(ast, "pragma"))
		(*ast->right[0])->s = smprint("/* %s */", (*ast->right[0])->s);
}

void
ifdefvarargs(AST *ast)
{
	int i;
	
	if(ast == nil)
		return;
	for(i=0; i<ast->nright; i++)
		ifdefvarargs(*ast->right[i]);
		
	if(match(ast, "vararg"))
		(*ast->right[0])->s = smprint("#ifdef VARARGCK\n%s#endif\n", (*ast->right[0])->s);
}

/*
 * plan 9 does not complain about casts from int -> ptr,
 * because there is no possibility of a loss in precision.
 * gcc and its ilk do complain, so insert uintptr casts.
 */
void
intptrcasts(AST *ast)
{
	int i;
	Type *t, *tt;
	
	if(ast == nil)
		return;
	for(i=0; i<ast->nright; i++)
		intptrcasts(*ast->right[i]);
		
	if(matchrule(ast, "expr", "'(", "abtype", "')", "expr", nil)){
		t = asttype(*ast->right[1]);
		tt = asttype(*ast->right[3]);
		if(isptr(t) && !isptr(tt) && !isuintptr(tt) 
		&& !matchrule(*ast->right[3], "expr", "number", nil))
			(*ast->right[3])->pre = "(uintptr)";
	}
}

/*
 * plan 9 tolower casts its argument to (uchar) automatically.
 * ansi c does not, arguably so that they can accept EOF as an argument.
 */
char *ctypes[] = {
	"isalpha",
	"isupper",
	"islower",
	"isdigit",
	"isxdigit",
	"isspace",
	"ispunct",
	"isalnum",
	"isprint",
	"isgraph",
	"iscntrl",
	"isascii",
	"toupper",
	"tolower",
	"toascii"
};
void
tolowercasts(AST *ast)
{
	int i;
	static int did;
	AST *a, *apre;
	
	if(!did){
		did = 1;
		ctypenames = mkhash();
		for(i=0; i<nelem(ctypes); i++)
			hashput(ctypenames, nameof(ctypes[i]), ctypes[i]);
	}

	if(ast == nil)
		return;
	for(i=0; i<ast->nright; i++)
		tolowercasts(*ast->right[i]);
		
	if(matchrule(ast, "expr", "expr", "'(", "expr,?", "')", nil))
	if(matchrule(*ast->right[0], "expr", "name", nil))
	if(hashget(ctypenames, idname(*ast->right[0])))
	if(matchrule(a=*ast->right[2], "expr,?", "expr,", nil))
	if(matchrule(a=*a->right[0], "expr,", "expr", nil))
	if(ischar(asttype(a=*a->right[0]))){
		apre = findpre(a);
		if(matchrule(a, "expr", "name", nil)
		|| matchrule(a, "expr", "'*", "expr", nil)
		|| matchrule(a, "expr", "expr", "'[", "cexpr", "']", nil)
		|| matchrule(a, "expr", "expr", "'->", "tag", nil)
		|| matchrule(a, "expr", "expr", "'.", "tag", nil)
		|| matchrule(a, "expr", "expr", "'(", "expr,?", "')", nil))
			apre->pre = smprint("%s(uchar)", apre->pre ? apre->pre : "");
		else{
fprint(2, "no match %B\n", a);
			apre->pre = smprint("%s(uchar)(", apre->pre ? apre->pre : "");
			a->post = smprint(")%s", a->post ? a->post : "");
		}
	}
}

/*
 * plan 9 allows the C99 (Type){1,2,3} as a constructor.
 * some older compilers do not.  when the user has asked,
 * rewrite these into fn(1,2,3) where fn is a user-specified
 * function.
 */
void
initializercasts(AST *ast)
{
	Name x;
	int i;
	Type *t;
	
	if(ast == nil)
		return;
	for(i=0; i<ast->nright; i++)
		initializercasts(*ast->right[i]);
	
	if(matchrule(ast, "expr", "'(", "abtype", "')", "'{", "init,", "'}", nil))
	if(issu(t = asttype(*ast->right[1])))
	if((x = hashget(reconstructhash, nameof(t->name))) != nil){
		findpre(*ast->right[0])->s = "";
		findpre(*ast->right[2])->s = "";
		findpre(*ast->right[3])->s = "(";
		findpre(*ast->right[5])->s = ")";
		(*ast->right[1])->s = x;
	}
}

int
astsexprfmt(Fmt *fmt)
{
	int i;
	AST *a;
	
	a = va_arg(fmt->args, AST*);
	if(a == nil)
		return fmtprint(fmt, "Z");
	if(a->s)
		fmtprint(fmt, "'%s", a->s);
	else{
		fmtprint(fmt, "(%s", a->sym ? a->sym->name : "merge!");
		for(i=0; i<a->nright; i++)
			fmtprint(fmt, " %B", *a->right[i]);
		fmtprint(fmt, ")");
	}
	return 0;
}

void
loadconf(char *file)
{
	int n, nf;
	char *f[10];
	char *a, *p, *nextp, line[1000];
	
	a = readfile(file);
	n = 0;
	for(p=a; p && *p; p=nextp){
		n++;
		if((nextp = strchr(p, '\n')) != nil)
			*nextp++ = 0;
		if(p[0] == '#')
			continue;
		strecpy(line, line+sizeof line, p);
		nf = tokenize(line, f, nelem(f));
		if(nf == 0)
			continue;
		if(nf == 3 && strcmp(f[0], "rename") == 0){
			hashput(renamehash, nameof(f[1]), nameof(f[2]));
			continue;
		}
		if(nf == 3 && strcmp(f[0], "reconstruct") == 0){
			hashput(reconstructhash, nameof(f[1]), nameof(f[2]));
			continue;
		}
		fprint(2, "%s:%d: malformed config line\n", file, n);
	}
}

void
main(int argc, char **argv)
{
	char *p;
	AST *ast;

	renamehash = mkhash();
	reconstructhash = mkhash();

	ARGBEGIN{
	case 'D':
		chatty++;
		break;
	case 'c':
		loadconf(EARGF(usage()));
		break;
	case 'p':
		listadd(&preloads, EARGF(usage()));
		break;
	case 'I':
		listadd(&includedirs, EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND
	
	listadd(&includedirs, "/386/include");
	listadd(&includedirs, "/sys/include");

	if(argc != 1)
		usage();
	
	fmtinstall('A', astfmt);
	fmtinstall('B', astsexprfmt);
	fmtinstall('T', typefmt);	
	structhash = mkhash();
	global.h = mkhash();
	ast2typehash = mkhash();
	anonfixhash = mkhash();
	astdeclhash = mkhash();
	runestrings = mkhash();
	runestringnames = mkhash();
	/*
	 * load and parse the c files.
	 */
	parsegrammar(cgrammar);

	forlist(p, preloads.first){
		ast = parsefile(p);
		typeast(ast);
	}

	inputdir = strdup(argv[0]);
	if((p = strrchr(inputdir, '/')) != nil)
		*++p = 0;
	else
		inputdir = ".";
	ast = parsefile(argv[0]);
	typeast(ast);
	
	/*
	 * rewrite plan 9 constructs into ansi c.
	 */
	anonstructs();
	anonrefs(ast);
	promoteptrs(ast);
	anonfnargs();
	unicodenames(ast);
	runechars(ast);
	dorunestrings(ast, 0, nil);
	commentpragmas(ast);
	ifdefvarargs(ast);
	intptrcasts(ast);
	tolowercasts(ast);
	initializercasts(ast);
	

	/*
	 * to do:
	 *	turn tolower(x) into tolower((uchar)x)
	 */

	/*
	 * print the new c text.
	 */
	print("%A", ast);
	exits(0);
}
