#include "common.h"
#include "code.h"
#include "sym.h"
#include "mem.h"

struct _mem node_mem;

void
TreeFree(Node *root)
{
	if(root==NULL) return;
	TreeFree(root->children);
	TreeFree(root->siblings);
	free(root);
}

void
TreePrint(Node *tree, int flag)
{
	if(tree==NULL)
		return;
	printf("%s", (tree->sym)->name);
	putchar('(');
	TreePrint(tree->children, 0);
	putchar(')');
	if(tree->siblings!=NULL) {
		putchar(',');
		TreePrint(tree->siblings, 0);
	}
	if(flag) putchar('\n');
}

Node *
rev(Node *sl, Node *nl, int *nlleaves)
{
	Node *sl2;
	if(sl==NULL)
		return(nl);
	sl2 = sl->siblings;
	sl->siblings = nl;
	*nlleaves += sl->nlleaves;
	return (rev (sl2, sl, nlleaves));
}

Node *
TreeBuild(SymbolEntry *symp, Node *children)
{
	Node *np = (struct node *) mem_get(&node_mem);
	np->sym = symp;
	np->nlleaves = symp->attr==A_LABEL ? 1 : 0;
	np->children = rev (children, NULL, &np->nlleaves);
	return(np);
}

void
TreeInit(void)
{
	mem_init(&node_mem, sizeof(struct node));
}

int
cntnodes(Node *np)
{
	if(np==NULL) return(0);
	else return(1+cntnodes(np->children)+cntnodes(np->siblings));
}
