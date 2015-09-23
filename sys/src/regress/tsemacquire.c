#include <u.h>
#include <libc.h>

int32_t nprocs = 32;
int32_t nloops = 100000;
int32_t x = 1;
int32_t incr;

void
tsemloop(void)
{
	int i;
	i = 0;
	while(i < nloops){
		if(tsemacquire(&x, 10)){
			if((i % 1000) == 0)
				sleep(10);
			incr++;
			i++;
			semrelease(&x, 1);
		} else {
			//print("pid %d timeout\n", getpid());
		}
	}
	exits(nil);
}

void
semloop(void)
{
	int i;
	i = 0;
	while(i < nloops){
		if(semacquire(&x, 1)){
			incr++;
			i++;
			semrelease(&x, 1);
		} else {
			sysfatal("semacquire failed");
		}
	}
}


void
main(void)
{
	int i;

	incr = 0;
	for(i = 0; i < nprocs; i++){
		switch(rfork(RFMEM|RFPROC)){
		case -1:
			sysfatal("rfork");
		case 0:
			tsemloop();
			exits(nil);
		default:
			break;
		}
	}
	for(i = 0; i < nprocs; i++)
		waitpid();
	print("tsemloop incr %d\n", incr);
	if(incr != nprocs*nloops){
		print("FAIL\n");
		exits("FAIL");
	}

	incr = 0;
	for(i = 0; i < nprocs; i++){
		switch(rfork(RFMEM|RFPROC)){
		case -1:
			sysfatal("rfork");
		case 0:
			semloop();
			exits(nil);
		default:
			break;
		}
	}
	for(i = 0; i < nprocs; i++)
		waitpid();
	print("semloop incr %d\n", incr);
	if(incr != nprocs*nloops){
		print("FAIL\n");
		exits("FAIL");
	}

	incr = 0;
	for(i = 0; i < nprocs; i++){
		switch(rfork(RFMEM|RFPROC)){
		case -1:
			sysfatal("rfork");
		case 0:
			if((i&1) == 0)
				semloop();
			else
				tsemloop();
			exits(nil);
		default:
			break;
		}
	}
	for(i = 0; i < nprocs; i++)
		waitpid();
	print("mixed incr %d\n", incr);
	if(incr != nprocs*nloops){
		print("FAIL\n");
		exits("FAIL");
	}

	print("PASS\n");
	exits(nil);
}
