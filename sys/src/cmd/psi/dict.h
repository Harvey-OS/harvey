struct	dictent
{
	struct	object	key ;
	struct	object	value ;
} ;

struct	dict
{
	enum	access	access ;
	int		max ;
	int		cur ;
	struct	dictent	*dictent ;
} ;
void dict_invalid(char *);

void ExecCharString(struct dict *, int, int, struct object);

extern	struct	dict	*Systemdict, *statusdict, *Internaldict;
extern	int		Op_nel, Sop_nel, nNop_nel, nSop_nel ;
extern	struct	oobject	Op_tab[] , Sop_tab[];
extern struct pstring Nop_tab[], NSop_tab[], se_tab[], sye_tab[];
extern struct pstring sproduct;
extern int T1Dirleng;
extern struct object T1Names, T1Dir;
