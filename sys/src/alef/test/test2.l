#include <alef.h>

chan (int) path;

void
test(int k)
{
	int i;

	for(;;) {
		i = <-path;
		if(i < 0)
			break;	
		print("%d %d\n", k, i);
	}
	print("exit\n");
}

void
main(void)
{
	int i;

	alloc path;

	task test(12345);

	for(i = 0; i < 10; i++)
		path<-= i;

	path <- = -1;
}
