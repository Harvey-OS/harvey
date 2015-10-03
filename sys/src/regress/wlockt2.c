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

/* verify that wlockt returns 0 even on tight timeouts */

#define NPROC 50
RWLock alwaysLocked;	/* held by main process, writers will timeout */

int killerProc;	/* pid, will kill the other processes if starved */

int elapsed[NPROC];
int32_t completed;
QLock rl;
Rendez rStart;
Rendez rCompleted;

int verbose = 1;

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
			fprint(2, "stopAllAfter: %r\n");
			exits("rfork fails");
		default:
			if(verbose)
				print("killer proc spawn: pid %d\n", getpid());
			killerProc = pid;
			atexit(killKiller);
	}
}

int
handletimeout(void *v, char *s)
{
	if(strcmp(s, "timedout") == 0){
		if(verbose)
			print("%d: noted: %s\n", getpid(), s);
		print("FAIL: timedout\n");
		exits("FAIL");
	}
	return 0;
}

int
handlefail(void *v, char *s)
{
	if(strncmp(s, "fail", 4) == 0){
		if(verbose)
			print("%d: noted: %s\n", getpid(), s);
		print("FAIL: %s\n", s);
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
	while(alwaysLocked._readers == 0)
		rsleep(&rStart);
	qunlock(&rl);

	/* try to lock for ~1 ms */
	start = nsec();
	end = start;
	if(verbose)
		print("writer %d: started\n", getpid());
	if(wlockt(&alwaysLocked, 1)){
		if(verbose)
			print("writer %d failed: got the wlock\n", getpid());
		wunlock(&alwaysLocked);
		postnote(PNGROUP, getpid(), smprint("fail: writer %d got the wlock", getpid()));
	} else {
		end = nsec();
		if(verbose)
			print("writer %d: wlockt timedout in %lld ms\n", getpid(), (end - start) / (1000*1000));
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
				print("spawn writer %d\n", pid);
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
		fprint(2, "atnotify(handletimeout): %r\n");
		exits("atnotify fails");
	}
	if (!atnotify(handlefail, 1)){
		fprint(2, "atnotify(handlefail): %r\n");
		exits("atnotify fails");
	}

	if(verbose)
		print("main: ready to lock...\n");
	qlock(&rl);
	rlock(&alwaysLocked);
	if(verbose)
		print("main: got the alwaysLocked: wakeup all writers...\n");
	rwakeupall(&rStart);
	if(verbose)
		print("main: got the alwaysLocked: wakeup all writers... done\n");
	qunlock(&rl);

	qlock(&rl);
	if(verbose)
		print("main: waiting all writers to timeout...\n");
	while(completed < NPROC){
		rsleep(&rCompleted);
		if(verbose && completed < NPROC)
			print("main: awaked, but %d writers are still pending\n", NPROC - completed);
	}
	runlock(&alwaysLocked);
	qunlock(&rl);
	if(verbose)
		print("main: waiting all writers to timeout... done\n");

	average = 0;
	for(i = 0; i < NPROC; ++i){
		average += elapsed[i];
	}
	average = average / NPROC / (1000 * 1000);

	if(average < 100) /* we asked for 1ms... we are dumb, after all */
	{
		print("PASS\n");
		exits("PASS");
	}
	print("FAIL: average timeout too long %lld ms\n", average / (1000*1000));
	exits("FAIL");
}
