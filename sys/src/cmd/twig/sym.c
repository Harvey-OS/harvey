/*
 * Symbol manipulation routines
 */
#include "common.h"
#include "code.h"
#include "sym.h"
#include "machine.h"
#include "y.tab.h"
#include "mem.h"

#define SAFE	1			/* generates code to do redundant checks */
#define MAX_UNIQUE MAX_NODE_VALUE	/* must fit in a short */

typedef struct SymbolList {
	SymbolEntry *first, *last;
	int count;
} SymbolList;
static void AppendToList(SymbolList *, SymbolEntry *);

/*
 * The range of the unique numbers assigned to each type of
 * symbol is [min,max)
 */
struct ranges {
	int	min;
	int	max;
	int	limit;
} uniques[] = {
	{0, 0, MAX_UNIQUE},	/* A_UNDEFINED -- not used */
	{0, 0, MAX_NODE_VALUE},	/* A_NODE */
	{0, 0, MAX_UNIQUE},	/* A_LABEL */
	{0, 0, MAX_UNIQUE},	/* A_COST */
	{0, 0, MAX_UNIQUE},	/* A_ACTION */
};

int treeIndex = 0;

static SymbolList lists[] = {
	{ NULL, NULL, 0},	/* A_UNDEFINED -- never used */
	{ NULL, NULL, 0}, /* A_NODE */
	{ NULL, NULL, 0}, /* A_LABEL */
	{ NULL, NULL, 0}, /* A_COST */
	{ NULL, NULL, 0}, /* A_ACTION */
	{ NULL, NULL, 0}, /* A_CONST */
};

struct _mem sym_mem, lab_mem, tree_mem;
SymbolEntry *hash_table[HASHSIZE];
SymbolEntry *startSymbol;

static int
RawHash(char *s)
{
	int sum = 0, i = 0;

	while(*s)
		sum += (++i)*(0377&*s++);
	return sum;
}


/*
 * Allocate a new symbol_entry structure for the given structure and
 * return it.  There is no check of whether the symbol is already defined or
 * not.  The caller is expected to fill in the uninitialized fields.
 */
SymbolEntry *
SymbolAllocate(char *s)
{
	SymbolEntry *sp;
	static char *arena;
	static int alen;
	int n;

	sp = (SymbolEntry *) mem_get(&sym_mem);
	assert(sp!=NULL);

#ifdef SAFE
	assert (SymbolLookup(s) == NULL);
#endif
	n = strlen(s) + 1;
	if(n > alen){
		alen = 10000;
		if(n > alen)
			alen = n;
		arena = malloc(alen);
		assert(arena != NULL);
	}
	sp->name = arena;
	alen -= n;
	arena += n;
	strcpy (sp->name, s);
	sp->hash = RawHash(s);
	sp->next = NULL;
	sp->attr = A_UNDEFINED;
	sp->unique = -1;
	return(sp);
}

void
SymbolFree(SymbolEntry *symp)
{
	assert (symp->next == NULL);
	free(symp);
}

SymbolEntry *
SymbolLookup(char *s)
{
	int hash_value = RawHash(s);
	register SymbolEntry * hp = hash_table[hash_value%HASHSIZE];

	while(hp!=NULL) {
		if(hp->hash == hash_value &&
				strcmp (s, hp->name)==0) break;
		hp = hp->next;
	}
	return(hp);
}

/*
 * enter the symbol into the symbol table.  It believes everything in the
 * symp argument.
 */
void
SymbolEnter(SymbolEntry *symp, int attr)
{
	struct symbol_entry *hp;

#ifdef SAFE
	assert (symp!=NULL);
	assert (symp->hash == RawHash(symp->name));
	assert (symp->attr == A_UNDEFINED);
#endif

	hp = hash_table[symp->hash%HASHSIZE];
	symp->next = hp;
	hash_table[symp->hash%HASHSIZE] = symp;
	symp->attr = attr;
	if (HAS_UNIQUE(attr)){
		struct ranges *rp = &uniques[attr];
		int new_unique;
		if(symp->unique == -1)
			symp->unique = new_unique = rp->max++;
		else {
			new_unique = symp->unique;
			if(new_unique >= rp->max)
				rp->max = new_unique+1;
			else if(new_unique < rp->min)
				rp->min = new_unique;
		}
		if(new_unique > rp->limit)
			sem_error("number assigned to %s(%d) attr %d out of range",
				symp->name, new_unique, attr);
		if(new_unique < 0)
			sem_error("number assigned to %s (%d) is negative",
				symp->name, new_unique);
		assert(rp->max >= rp->min);
	}
	if (HAS_LIST(attr)) {
		AppendToList(&lists[attr], symp);
	}
}

static void
AppendToList(SymbolList *list, SymbolEntry *sp)
{
	sp->link = NULL;
	if(list->first==NULL)
		list->first = sp;
	else (list->last)->link = sp;
	list->last = sp;
	list->count++;
}

void
SymbolCheckNodeValues(void)
{
	SymbolList *slist;
	SymbolEntry **stab, **spp, *sp;
	struct ranges *rp;
	int i, range;

	rp = &uniques[A_NODE];
	slist = &lists[A_NODE];
	range = rp->max - rp->min + 1;
	stab = (SymbolEntry **) Malloc(range*sizeof(SymbolEntry *));

	for(i=range, spp = stab; i-->0; spp++) *spp = NULL;

	for(sp = slist->first; sp; sp = sp->link) {
		SymbolEntry *sp2 = stab[sp->unique];
		if(sp2==NULL)
			stab[sp->unique] = sp;
		else
			sem_error("%s and %s have the same node value",
				sp2->name, sp->name);
	}
	free(stab);
}

SymbolEntry *
SymbolEnterKeyword(char *s, int value)
{
	SymbolEntry *sp = SymbolAllocate(s);
	sp->sd.keyword = value;
	SymbolEnter (sp, A_KEYWORD);
	return(sp);
}

void
SymbolInit(void)
{
	mem_init(&sym_mem,sizeof(SymbolEntry));
	mem_init(&lab_mem,sizeof(LabelData));
	mem_init(&tree_mem,sizeof(struct treeassoc));
	SymbolEnterKeyword ("node", K_NODE);
	SymbolEnterKeyword ("label", K_LABEL);
	SymbolEnterKeyword ("prologue", K_PROLOGUE);
	SymbolEnterKeyword ("const", K_CONST);
	SymbolEnterKeyword ("insert", K_INSERT);
	SymbolEnterKeyword ("cost", K_COST);
	SymbolEnterKeyword ("action", K_ACTION);
}

void
SymbolMap(int (*f)(SymbolEntry *))
{
	SymbolEntry **spp, *sp;
	for(spp = hash_table; spp < &hash_table[HASHSIZE]; spp++)
		for(sp= *spp; sp!=NULL; sp = sp->next)
			f(sp);
};

static int symcnt, labcnt, nodecnt;
static int
sym_count(SymbolEntry *sp)
{
	LabelData *lp;

	symcnt++;
	if(sp->attr == A_LABEL)
		for(lp = sp->sd.lp;lp!=NULL;lp=lp->next) {
			labcnt++;
			nodecnt += cntnodes(lp->tree);
		}
	return 0;
}

void
SymbolFinish(void)
{
	extern struct _mem node_mem;
	int symout = mem_outstanding(&sym_mem);
	int labout = mem_outstanding(&lab_mem);
	int nodeout = mem_outstanding(&node_mem);

	if(!(DB_MEM & debug_flag))
		return;
	SymbolMap(sym_count);
	fprint(2, "symbols defined=%d out=%d\n", symcnt, symout);
	fprint(2, "labdata def=%d out=%d\n", labcnt, labout);
	fprint(2, "node def=%d out=%d\n", nodecnt, nodeout);
	assert(symcnt==symout);
	assert(labcnt==labout);
	assert(nodecnt==nodeout);
}

void
SymbolEnterList(SymbolEntry *symp, int attr)
{
	/*
	 * enter in reverse order because the list was built in reverse
	 * order by the parser
	 */

	if(symp==NULL)
		return;
	SymbolEnterList(symp->next, attr);
	if(symp->attr!=A_UNDEFINED)
		sem_error ("multiple declaration ignored: %s", symp->name);
	SymbolEnter (symp, attr);
}

LabelData *
SymbolEnterTreeIntoLabel(SymbolEntry *symp, struct node *tree, SymbolEntry *costfunc, SymbolEntry *action, int lineno)
{
	LabelData *tp = (LabelData *) mem_get(&lab_mem);
	struct treeassoc *ap;

	if (symp==&ErrorSymbol) return(NULL);
#ifdef SAFE
	assert (tp!=NULL);
	assert (symp->attr==A_LABEL);
#endif

	tp->tree = tree;
	tp->treeIndex = treeIndex++;
	tp->lineno = lineno;
	tp->next = symp->sd.lp;
	tp->label = symp;
	symp->sd.lp = tp;

	if (action!=NULL) {
		ap = (struct treeassoc *) mem_get(&tree_mem);
		ap->tree = tp->treeIndex;
		ap->next = action->sd.ca.assoc;
		action->sd.ca.assoc = ap;
	}

	if (costfunc!=NULL) {
		ap = (struct treeassoc *) mem_get(&tree_mem);
		ap->tree = tp->treeIndex;
		ap->next = costfunc->sd.ca.assoc;
		costfunc->sd.ca.assoc = ap;
	}
	return(tp);
}

static void
WriteSymbols(SymbolEntry *sp, int mask)
{
	for(; sp; sp = sp->link)
		Bprint(symfile, "#define %s\t0x%ux\n", sp->name, sp->unique|mask);
}

void
SymbolDump(void)
{
	WriteSymbols(lists[A_NODE].first, M_NODE);
	WriteSymbols(lists[A_LABEL].first, 0);
	WriteSymbols(lists[A_CONST].first, 0);
	Bprint(symfile, "#define MAXLABELS %d\n", uniques[A_LABEL].max);
	Bprint(symfile, "#define MAXTREES %d\n", treeIndex);
	Bprint(symfile, "#define MAXNDVAL %d\n", uniques[A_NODE].max);
}

void
SymbolGenerateWalkerCode(void)
{
	SymbolEntry *pp;
	int i;
	extern int line_xref_flag;
	struct treeassoc *tap;
	LabelData *lp;
	short int *mapTab;

	/*
	 * Write out the table of tree index to label correspondence
	 */
	mapTab = Malloc(treeIndex * sizeof(short int));
	for(pp = lists[A_LABEL].first; pp; pp = pp->link) {
		lp = pp->sd.lp;
		if(!lp)
			sem_error2("%s undefined", pp->name);
		for(;lp; lp = lp->next)
			mapTab[lp->treeIndex] = pp->unique;
	}

	Bprint(bout, "short mtMap[] = {\n");
	oreset();
	for(i = 0; i < treeIndex; i++)
		oputoct(mapTab[i]);
	Bprint(bout, "};\n");

	/* generate tree to line index table */
	if(line_xref_flag){
		for(pp = lists[A_LABEL].first; pp; pp = pp->link){
			lp = pp->sd.lp;
			for(; lp; lp = lp->next)
				mapTab[lp->treeIndex] = lp->lineno;
		}
		Bprint(bout, "short mtLine[] = {\n");
		oreset();
		for(i = 0; i<treeIndex; i++)
			oputint(mapTab[i]);
		Bprint(bout, "};\n");
	}

	/*
	 * Generate path table
	 */
	Bprint(bout, "short mtPaths[] = {\n");
	oreset();
	for(pp = lists[A_LABEL].first; pp; pp = pp->link) {
		lp = pp->sd.lp;
		if(!lp){
			sem_error2("%s undefined", pp->name);
			continue;
		}
		while(lp){
			mapTab[lp->treeIndex] = ointcnt();
			oputint(lp->tree->nlleaves);
			SymbolWritePath(lp->tree);
			oputint(eSTOP);
			lp = lp->next;
		}
	}
	Bprint(bout, " };\nshort int mtPathStart[] = {\n");
	oreset();
	for(i = 0; i < treeIndex; i++)
		oputint(mapTab[i]);
	Bprint(bout, "};\n");

	/*
	 * Code to perform the action of the trees
	 */
	Bprint(bout, "NODEPTR\nmtAction (int _t, __match **_ll, skeleton *_s)\n");
	Bprint(bout, "{ NODEPTR root = _s->root;\n");
	Bprint(bout, "switch (_t) {\n");
	for(pp = lists[A_ACTION].first; pp; pp = pp->link) {
		tap = pp->sd.ca.assoc;
		if(!tap) {
			sem_error2 ("%s not used", pp->name);
			continue;
		}
		for(; tap; tap=tap->next)
			Bprint(bout, "case %d:", tap->tree);
		Bprint(bout, "{\n");
		CodeWrite(bout, pp->sd.ca.code);
		Bprint(bout, "} break;\n");
	}
	Bprint(bout, "} return(_s->root);}\n");

	Bprint(bout, "short\n");
	Bprint(bout, "mtEvalCost(__match *_m, __match **_ll, skeleton *_s)\n");
	Bprint(bout, "{ NODEPTR root = _s->root;\n");
	Bprint(bout, "COST cost; cost = DEFAULT_COST;\n");
	Bprint(bout, "switch(_m->tree) {\n");
	for(pp = lists[A_COST].first; pp; pp = pp->link) {
		tap = pp->sd.ca.assoc;
		if(!tap){
			sem_error2 ("%s not used", pp->name);
			continue;
		}
		for(; tap; tap = tap->next)
			Bprint(bout, "case %d:", tap->tree);
		Bprint(bout, "{\n");
		CodeWrite (bout, pp->sd.ca.code);
		Bprint(bout, "} break;\n");
	}
	Bprint(bout, "}\n");
	Bprint(bout, "_m->cost = cost; return xDEFER;}\n");

}

void
SymbolWritePath(Node *root)
{
	Node *np;

	if(!root->nlleaves)
		return;
	if(root->sym->attr == A_LABEL) {
		oputint(eEVAL);
		oputoct(MV_LABEL(root->sym->unique));
		return;
	}
	oputint(ePUSH);
	for(np = root->children;;) {
		if(np->nlleaves > 0)
			SymbolWritePath(np);
		np = np->siblings;
		if(!np)
			break;
		oputint(eNEXT);
	}
	oputint(ePOP);
}

static int gensymndx = 0;

char *
SymbolGenUnique(void)
{
	static char name[7];

	name[0] = '$';
	sprint(&name[1], "%5d", gensymndx++);
	return name;
}
