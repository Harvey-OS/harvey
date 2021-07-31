#include <alef.h>
#include "stack.h"

void
Stack.init(Stack *s)
{
	s->tos = 0;
}

int
Stack.empty(Stack *s)
{
	return s->tos == 0;
}

void
Stack.push(Stack *s, T v)
{
	s->data[s->tos++] = v;
}

T
Stack.pop(Stack *s)
{
	return s->data[--s->tos];
}

void
printT(T v)
{
	typeof v {
	case int:
		print("%d\n", v+1);
		break;
	case byte*:
		print("%s\n", v);
		break;
	}
}
