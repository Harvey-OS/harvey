#include "symbols.h"

/* limits */
/*
 * The depth of a pattern must be 7 or less.  Otherwise the type of he
 * partial mask in skeleton must be changed
 */
#define MAXDEPTH	7

/* modes */
#define xTOPDOWN	3
#define xABORT		2
#define xREWRITE	1
#define xDEFER		0

/* macros */
#define tDO(m)	_do((m)->skel, (m), 1)
#define REWRITE	return(_m->cost = cost, xREWRITE)
#define TOPDOWN return(_m->cost = cost, xTOPDOWN)
#define ABORT return(xABORT)

/*
 * REORDER macro allows a knowledgeable user to change the order
 * of evaluation of the leaves.
 */
#ifndef REORDER
#define REORDER(list)	_evalleaves(list)
#endif
#define EVAL	REORDER(_ll)

#define mpush(s,m)	(m)->next = s, s = m

/* skeleton structure */
typedef struct skeleton		skeleton;
typedef struct __match		__match;
typedef struct __partial	__partial;
typedef int			labelset;	/* a bit vector of labels */

struct __partial {
	short		treeno;
	short		bits;
};

struct skeleton {
	skeleton	*sibling;
	skeleton	*leftchild;		/* leftmost child */
	skeleton	*rightchild;		/* rightmost child */
	NODEPTR		root;
	NODEPTR		parent;			/* our parent */
	int		nson;
	int		treecnt;
	__match		*succ[MAXLABELS];
	__partial	*partial;
	__match		*winner;
	COST		mincost;
};

struct __match {
	__match		**lleaves;	/* labelled leaves */
	skeleton	*skel;		/* back pointer to skeleton node */
	COST		cost;
	short		tree;
	short		mode;
};

/* ZEROCOST, and ADDCOST allows easy implementation of the common
 * operation of just equating the cost of a match to be the sum
 * of the costs of the labelled leaves.
 */
#ifdef ADDCOST
#define DEFAULT_COST sumleaves(_ll)
COST sumleaves(__match **list)
{
	COST cost;
	cost = ZEROCOST;
	for(; *list; list++)
		ADDCOST((cost),(*list)->cost);
	return cost;
}
#endif
