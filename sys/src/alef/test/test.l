#include <alef.h>

aggr Test
{
	int a;
	int b;
	int c;
};

chan(int) a, b;
chan(Test) c;

void
produce(chan(int) j, int c)
{
	int i;

	for(i = 0; i < 10; i++)
		j <-= c;
}

void
prodtest(chan(Test) j)
{
	int i;
	Test xx;

	for(i = 0; i < 10; i++) {
		xx.b = i;
		j <-= xx;
	}
}

void
main(void)
{
	int i, ai, bi;
	Test ci;

	alloc a, b, c;

	proc produce(a, 'a');
	task produce(b, 'b');
	proc prodtest(c);

	for(i = 0; i < 30; i++)
		alt {
		case ai=<-a:
			print("a: %c\n", ai);
			break;
	
		case bi=<-b:
			print("b: %c\n", bi);
			break;
	
		case ci=<-c:
			print("c: %d\n", ci.b);
			break;
		}
}
