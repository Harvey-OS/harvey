/* symbol table definitions */
extern int symbol_undefined;

/*
 * A LabelData structures are associated with label symbols.  They
 * indicate that a tree is labelled with the symbol
 */
typedef struct LabelData	LabelData;

struct LabelData {
	Node		*tree;
	int		treeIndex;
	int		lineno;
	SymbolEntry	*label;		/* back pointer */
	LabelData	*next;
};

struct treeassoc {
	int tree;
	struct treeassoc *next;
};

typedef enum{
	A_UNDEFINED,
	A_NODE,
	A_LABEL,
	A_COST,
	A_ACTION,
	A_CONST,

	A_ERROR	= 10,
	A_KEYWORD,
}Attr;

#define MAXCHAINS	A_CONST	
#define HAS_UNIQUE(x)	(x < MAXCHAINS)
#define HAS_LIST(x)	(x <= A_CONST)

struct symbol_entry {
	int		hash;
	char		*name;
	Attr		attr;
	short		unique;
	SymbolEntry	*link;
	SymbolEntry	*next;
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

SymbolEntry	*SymbolAllocate(char*);
void		SymbolFree(SymbolEntry*);
SymbolEntry	*SymbolLookup(char*);
void		SymbolEnter(SymbolEntry*, int);
void		SymbolCheckNodeValues(void);
SymbolEntry	*SymbolEnterKeyword(char*, int);
void		SymbolInit(void);
void		SymbolMap(int(*)(SymbolEntry*));
void		SymbolFinish(void);
void		SymbolEnterList(SymbolEntry*, int);
LabelData	*SymbolEnterTreeIntoLabel(SymbolEntry*, struct node*, SymbolEntry*, SymbolEntry*, int);
void		SymbolDump(void);
void		SymbolGenerateWalkerCode(void);
void		SymbolWritePath(Node *);
char		*SymbolGenUnique(void);

extern SymbolEntry ErrorSymbol;
