#include <alef.h>

aggr Node
{
	int a;
	int b;
};
int fun(Node*);

Node *
main(void)
{
	Node *n;

	alloc n;
	rescue {
		unalloc n;
		return nil;
	}

	n->a = 10;
	if(fun(n) == 0)
		raise;

	return n;
}
