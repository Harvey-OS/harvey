#pragma	src	"/sys/src/alef/lib/libregexp"
#pragma	lib	"/$M/lib/alef/libregexp.a"

/*
 *	Sub expression matches
 */
aggr Resub{
	union
	{
		byte *sp;
		Rune *rsp;
		uint	qsp;	/* for acme */
	};
	union
	{
		byte *ep;
		Rune *rep;
		uint	qep;	/* for acme */
	};
};

/*
 *	character class, each pair of rune's defines a range
 */
aggr Reclass{
	Rune	*end;
	Rune	spans[64];
};

/*
 *	Machine instructions
 */
aggr Reinst{
	int	type;
	union	{
		Reclass	*cp;		/* class pointer */
		Rune	r;		/* character */
		int	subid;		/* sub-expression id for RBRA and LBRA */
		Reinst	*right;		/* right child of OR */
	};
	union {	/* regexp relies on these two being in the same union */
		Reinst *left;		/* left child of OR */
		Reinst *next;		/* next instruction for CAT & LBRA */
	};
};

/*
 *	Reprogram definition
 */
aggr Reprog{
	Reinst	*startinst;	/* start pc */
	Reclass	class[16];	/* .data */
	Reinst	firstinst[5];	/* .text */
};

Reprog	*regcomp(byte*);
Reprog	*regcomplit(byte*);
Reprog	*regcompnl(byte*);
void	regerror(byte*);
int	regexec(Reprog*, byte*, Resub*, int);
void	regsub(byte*, byte*, Resub*, int);
int	rregexec(Reprog*, Rune*, Resub*, int);
void	rregsub(Rune*, Rune*, Resub*, int);
