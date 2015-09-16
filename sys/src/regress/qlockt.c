#include <u.h>
#include <libc.h>

QLock l;
int64_t start, timedout;

int
handlealarm(void *v, char *s)
{
	/* just not exit, please */
	if(strcmp(s, "alarm") == 0){
		fprint(2, "%d: noted: %s\n", getpid(), s);
		return 1;
	}
	return 0;
}

void
failOnLock()
{
	int locked = -1;
	if((locked = qlockt(&l, 1000))){
		print("FAIL\n");
		exits("FAIL\n");
	}
	fprint(2, "%d: lock waiter: qlockt returned %d\n", getpid(), locked);
	timedout = nsec();
	while(rendezvous(&start, (void*)0x11111) == (void*)~0)
		fprint(2, "lock waiter pid = %d, rendezvous interrupted, continue\n", getpid());
}

void
main(void)
{
	int pid;
	int64_t elapsed;

	if (!atnotify(handlealarm, 1)){
		fprint(2, "%r\n");
		exits("atnotify fails");
	}

	rfork(RFNOTEG|RFREND);
	
	switch((pid = rfork(RFMEM|RFPROC|RFNOWAIT)))
	{
		case 0:
			sleep(1000);	/* wait for the parent to qlock */
			start = nsec();
			failOnLock();
			exits(0);
		case -1:
			fprint(2, "%r\n");
			exits("rfork fails");
		default:
			qlock(&l);
			fprint(2, "lock holder pid = %d, lock waiter pid = %d\n", getpid(), pid);

			while(rendezvous(&start, (void*)0x11111) == (void*)~0)
				fprint(2, "lock holder pid = %d, rendezvous interrupted, continue\n", getpid());
				
			qunlock(&l);
			elapsed = (timedout - start) / (1000 * 1000);
			fprint(2, "lock holder released lock, elapsed = %d\n", elapsed);
			if(elapsed > 900 && elapsed < 1100) /* 10% error on timeout is acceptable */
			{
				print("PASS\n");
				exits("PASS");
			}
			print("FAIL\n");
			exits("FAIL");
	}
}
