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

/* verify that qlockt returns 0 on timeout */

#define NPROC 50
QLock alwaysLocked;	/* held by main process, waiters will timeout */

int killerProc;	/* pid, will kill the other processes if starwed */

int elapsed[NPROC];
int32_t completed;
QLock rl;
Rendez rStart;
Rendez rCompleted;

int verbose = 0;

void
killKiller(void)
{
	postnote(PNPROC, killerProc, "interrupt");
}

void
stopAllAfter(int seconds)
{
	int pid;

	switch((pid = rfork(RFMEM|RFPROC|RFNOWAIT)))
	{
		case 0:
			if(verbose)
				print("killer proc started: pid %d\n", getpid());
			sleep(seconds * 1000);
			postnote(PNGROUP, killerProc, "timedout");
			if(verbose)
				print("killer proc timedout: pid %d\n", getpid());
			exits("FAIL");
		case -1:
			fprint(2, "%r\n");
			exits("rfork fails");
		default:
			killerProc = pid;
			atexit(killKiller);
	}
}

int
handletimeout(void *v, char *s)
{
	/* just not exit, please */
	if(strcmp(s, "timedout") == 0){
		if(verbose)
			print("%d: noted: %s\n", getpid(), s);
		exits("FAIL");
	}
	return 0;
}

int
handlefail(void *v, char *s)
{
	/* just not exit, please */
	if(strncmp(s, "fail", 4) == 0){
		if(verbose)
			print("%d: noted: %s\n", getpid(), s);
		print("FAIL");
		exits("FAIL");
	}
	return 0;
}

char *
waiter(int index)
{
	int64_t start, end;

	/* wait for the alwaysLocked to be locked by the main process */
	qlock(&rl);
	while(alwaysLocked.locked == 0)
		rsleep(&rStart);
	qunlock(&rl);

	/* try to lock for ~1000 ms */
	start = nsec();
	end = start;
	if(verbose)
		print("waiter %d: started\n", getpid());
	if(qlockt(&alwaysLocked, 1000)){
		if(verbose)
			print("waiter %d failed: got the qlock\n", getpid());
		qunlock(&alwaysLocked);
		postnote(PNGROUP, getpid(), smprint("fail: waiter %d got the qlock", getpid()));
	} else {
		end = nsec();
		if(verbose)
			print("waiter %d: qlockt timedout in %lld ms\n", getpid(), (end - start) / (1000*1000));
	}

	/* notify the main process that we have completed */
	qlock(&rl);
	elapsed[index] = end - start;
	++completed;
	rwakeup(&rCompleted);
	qunlock(&rl);

	return end != start ? nil : "FAIL";
}

void
spawnWaiter(int index)
{
	int pid;
	char * res;

	switch((pid = rfork(RFMEM|RFPROC|RFNOWAIT)))
	{
		case 0:
			res = waiter(index);
			exits(res);
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
	int i;
	int64_t average;

	rfork(RFNOTEG|RFREND);
	rStart.l = &rl;
	rCompleted.l = &rl;

	if(verbose)
		print("main: started with pid %d\n", getpid());

	for(i = 0; i < NPROC; ++i){
		spawnWaiter(i);
	}

	stopAllAfter(30);

	if (!atnotify(handletimeout, 1)){
		fprint(2, "%r\n");
		exits("atnotify fails");
	}
	if (!atnotify(handlefail, 1)){
		fprint(2, "%r\n");
		exits("atnotify fails");
	}

	qlock(&rl);
	qlock(&alwaysLocked);
	if(verbose)
		print("main: got the alwaysLocked: wakeup all waiters...\n");
	rwakeupall(&rStart);
	if(verbose)
		print("main: got the alwaysLocked: wakeup all waiters... done\n");
	qunlock(&rl);

	qlock(&rl);
	if(verbose)
		print("main: waiting all waiters to timeout...\n");
	while(completed < NPROC){
		rsleep(&rCompleted);
		if(verbose && completed < NPROC)
			print("main: awaked, but %d waiters are still pending\n", NPROC - completed);
	}
	qunlock(&alwaysLocked);
	qunlock(&rl);
	if(verbose)
		print("main: waiting all waiters to timeout... done\n");

	average = 0;
	for(i = 0; i < NPROC; ++i){
		average += elapsed[i];
	}
	average = average / NPROC / (1000 * 1000);

	if(average > 900 && average < 1300) /* 30% error on timeout is acceptable */
	{
		print("PASS\n");
		exits("PASS");
	}
	print("FAIL: average timeout too long %lld ms\n", average / (1000*1000));
	exits("FAIL");
}
