#include <u.h>
#include <libc.h>

int32_t x = 1;

void
main(void)
{
	int i;
	for(i = 0; i < 2; i++)
		rfork(RFMEM|RFPROC);

	i = 0;
	while(i < 10){
		if(tsemacquire(&x, 100)){
			print("pid %d got it  %d\n", getpid(), i);
			sleep(100);
			semrelease(&x, 1);
			i++;
		} else {
			print("pid %d timeout %d\n", getpid(), i);
		}
	}
}
