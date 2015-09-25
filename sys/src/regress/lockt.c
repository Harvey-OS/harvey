/*
 * This file is part of Harvey.
 *
 * Copyright (C) 2015 Giacomo Tesio <giacomo@tesio.it>
 *
 * Harvey is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * Harvey is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Harvey.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <u.h>
#include <libc.h>

QLock rl;
Rendez rStart;
Rendez rCompleted;
int64_t elapsedInWaiter, resInWaiter;

int verbose = 0;

int
failOnTimeout(void *v, char *s)
{
	if(strncmp(s, "alarm", 4) == 0){
		if(verbose)
			print("%d: noted: %s\n", getpid(), s);
		print("FAIL: timeout\n");
		exits("FAIL");
	}
	return 0;
}

void
spawnWaiter(Lock *l)
{
	int pid;
	int64_t start, res;

	switch((pid = rfork(RFMEM|RFPROC|RFNOWAIT)))
	{
		case 0:
			/* wait for the alwaysLocked to be locked by the main process */
			qlock(&rl);
			while(resInWaiter == 0xff)
				rsleep(&rStart);

			start = nsec();
			resInWaiter = lockt(l, 6000);
			elapsedInWaiter = (nsec() - start) / (1000 * 1000);
			if(verbose)
				print("lockt returned %d, elapsed = %d ms\n", res, elapsedInWaiter);

			rwakeup(&rCompleted);
			qunlock(&rl);

			exits(0);
			break;
		case -1:
			print("spawnWaiter: %r\n");
			exits("rfork fails");
			break;
		default:
			if(verbose)
				print("spawn waiter %d\n", pid);
			break;
	}
}

void
main(void)
{
	int64_t start, elapsed, res;
	static Lock l;

	rfork(RFNOTEG|RFREND);
	rStart.l = &rl;
	rCompleted.l = &rl;
	resInWaiter = 0xff;

	spawnWaiter(&l);
	lock(&l);

	alarm(20000);	/* global timeout, FAIL if reached */
	if (!atnotify(failOnTimeout, 1)){
		fprint(2, "%r\n");
		exits("atnotify fails");
	}

	/* verify that lockt returns 0 on timeout */
	start = nsec();
	res = lockt(&l, 1000);
	elapsed = (nsec() - start) / (1000 * 1000);
	if(verbose)
		print("lockt returned %d, elapsed = %d ms\n", res, elapsed);
	if(res != 0 || elapsed < 900 || elapsed > 1300){
		print("FAIL: lockt timeout\n");
		exits("FAIL");
	}

	/* verify that lockt returns 1 if the lock is released and
	 * it can take it
	 */
	resInWaiter = -1;
	qlock(&rl);
	rwakeupall(&rStart);
	qunlock(&rl);
	sleep(1200);
	unlock(&l);

	qlock(&rl);
	while(elapsedInWaiter == 0)
		rsleep(&rCompleted);
	qunlock(&rl);
	if(resInWaiter != 1 || elapsedInWaiter < 1100 || elapsedInWaiter > 1500){
		print("FAIL: lockt delayed acquisition\n");
		exits("FAIL");
	}

	print("PASS\n");
	exits("PASS");
}
