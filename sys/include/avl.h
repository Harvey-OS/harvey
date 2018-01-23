/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#pragma	lib	"libavl.a"
#pragma src "/sys/src/libavl"

typedef struct Avl	Avl;
typedef struct Avltree	Avltree;
typedef struct Avlwalk	Avlwalk;

#pragma incomplete Avltree
#pragma incomplete Avlwalk

struct Avl
{
	Avl	*p;		/* parent */
	Avl	*n[2];		/* children */
	int	bal;		/* balance bits */
};

Avl	*avlnext(Avlwalk *walk);
Avl	*avlprev(Avlwalk *walk);
Avlwalk	*avlwalk(Avltree *tree);
void	deleteavl(Avltree *tree, Avl *key, Avl **oldp);
void	endwalk(Avlwalk *walk);
void	insertavl(Avltree *tree, Avl *new, Avl **oldp);
Avl	*lookupavl(Avltree *tree, Avl *key);
Avltree	*mkavltree(int(*cmp)(Avl*, Avl*));
Avl*	searchavl(Avltree *tree, Avl *key, int neighbor);
