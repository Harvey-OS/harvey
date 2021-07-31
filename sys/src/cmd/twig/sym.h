/* symbol table definitions */
extern int symbol_undefined;

/*
 * A LabelData structures are associated with label symbols.  They
 * indicate that a tree is labelled with the symbol
 */
typedef struct LabelData	LabelData;

struct LabelData {
	Node *tree;
	int treeIndex;
	int lineno;
	SymbolEntry *label;	/* back pointer */
	LabelData *next;
};

struct treeassoc {
	int tree;
	struct treeassoc *next;
};

struct symbol_entry {
	int hash;
	char *name;
	short attr;
#		define  A_UNDEFINED		0
#		define	A_NODE			1
#		define  A_LABEL			2
#		define	A_COST			3
#		define	A_ACTION		4
#						define MAXCHAINS     A_CONST	
#						define HAS_UNIQUE(x) (x<MAXCHAINS)
#		define	A_CONST			5
#						define HAS_LIST(x) (x<=A_CONST)
#		define	A_ERROR			10
#		define	A_KEYWORD		11
	short unique;
	struct symbol_entry *link;
	struct symbol_entry *next;
	union {
		int keyword;
		int cvalue;	/* a constant's value */
		int arity;	/* information stored for A_NODE type */
		LabelData *lp;
		struct {
			int arity;
			Code *code;
		} predData;
		struct {
			struct treeassoc *assoc;
			Code *code;
		} ca;				/* data for cost/action symbols */
	} sd;
};

SymbolEntry	*SymbolAllocate(char *);
void		SymbolFree(SymbolEntry *);
SymbolEntry	*SymbolLookup(char *);
void		SymbolEnter(SymbolEntry *, byte);
void		SymbolCheckNodeValues(void);
SymbolEntry	*SymbolEnterKeyword(char *, int);
void		SymbolInit(void);
void		SymbolMap(int (*)(SymbolEntry *));
void		SymbolFinish(void);
void		SymbolEnterList(SymbolEntry *, int);
LabelData	*SymbolEnterTreeIntoLabel(SymbolEntry *, struct node *, SymbolEntry *, SymbolEntry *, int);
void		SymbolDump(void);
void		SymbolGenerateWalkerCode(void);
void		SymbolWritePath(Node *);
char		*SymbolGenUnique(void);

extern SymbolEntry ErrorSymbol;
