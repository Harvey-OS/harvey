#include <alef.h>
#include "stack.h"

adt Foo
{
	int	x;
	int	y;

	Stack;
};

void	printT(T);

void
main()
{
	T v;
	Foo a;
	Stack[byte] *b;

	v = nil;

	a.init();
	v = (alloc T)10;
	a.push(v);
	print("sizeof v %d\n", sizeof v);
	a.push((alloc T)"hello world");

	while(!a.empty()) {
		v = a.pop();
		printT(v);
		unalloc v;
	}

	alloc b;

	b->init();
	b->push('a');
	b->push('b');
	print("%c\n", b->pop());
	print("%c\n", b->pop());
}
