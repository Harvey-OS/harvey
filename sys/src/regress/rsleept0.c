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

/* verify that rsleept returns 0 on timeout */

#define NPROC 50
QLock al;
Rendez neverAwakened;	/* sleepers will timeout */

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
		print("FAIL: timedout\n");
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
		print("FAIL: %s\n");
		exits("FAIL");
	}
	return 0;
}

char *
sleeper(int index)
{
	int64_t start, end;

	/* wait for the other sleepers to be ready to start */
	qlock(&rl);
	while(!neverAwakened.l)
		rsleep(&rStart);
	qunlock(&rl);

	/* try to sleep for ~1000 ms */
	start = nsec();
	end = start;
	if(verbose)
		print("sleeper %d: started\n", getpid());
	qlock(neverAwakened.l);
	if(rsleept(&neverAwakened, 1000)){
		if(verbose)
			print("sleeper %d failed: awakened by rwakeup\n", getpid());
		qunlock(neverAwakened.l);
		postnote(PNGROUP, getpid(), smprint("fail: sleeper %d awakened by rwakeup", getpid()));
	} else {
		end = nsec();
		if(verbose)
			print("sleeper %d: rsleept timedout in %lld ms\n", getpid(), (end - start) / (1000*1000));
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
spawnsleeper(int index)
{
	int pid;
	char * res;

	switch((pid = rfork(RFMEM|RFPROC|RFNOWAIT)))
	{
		case 0:
			res = sleeper(index);
			exits(res);
			break;
		case -1:
			print("spawnsleeper: %r\n");
			exits("rfork fails");
			break;
		default:
			if(verbose)
				print("spawn sleeper %d\n", pid);
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
		spawnsleeper(i);
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
	neverAwakened.l = &al;
	if(verbose)
		print("main: wakeup all sleepers...\n");
	rwakeupall(&rStart);
	if(verbose)
		print("main: wakeup all sleepers... done\n");
	qunlock(&rl);

	qlock(&rl);
	if(verbose)
		print("main: waiting all sleepers to timeout...\n");
	while(completed < NPROC){
		rsleep(&rCompleted);
		if(verbose && completed < NPROC)
			print("main: awaked, but %d sleepers are still pending\n", NPROC - completed);
	}
	qunlock(&rl);
	if(verbose)
		print("main: waiting all sleepers to timeout... done\n");

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
