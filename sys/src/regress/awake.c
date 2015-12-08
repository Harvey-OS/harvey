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
main(void)
{
	int64_t start, elapsed, wkup, res;
	int32_t sem;

	semacquire(&sem, 0);

	alarm(20000);	/* global timeout, FAIL if reached */
	if (!atnotify(failOnTimeout, 1)){
		fprint(2, "%r\n");
		exits("atnotify fails");
	}

	awake(60000);	/* this will be handled by the kernel, see pexit */

	/* verify that rendezvous are interrupted */
	wkup = awake(1000);
	start = nsec();
	rendezvous(&elapsed, (void*)0x12345);
	elapsed = (nsec() - start) / (1000 * 1000);
	if(verbose)
		print("rendezvous interrupted, elapsed = %d ms\n", elapsed);
	if(!awakened(wkup) || elapsed < 900 || elapsed > 1300){
		print("FAIL: rendezvous\n");
		exits("FAIL");
	}

	/* verify that tsemacquire are NOT interrupted */
	wkup = awake(700);
	start = nsec();
	res = tsemacquire(&sem, 1500);
	elapsed = (nsec() - start) / (1000 * 1000);
	if(verbose)
		print("tsemacquire(&sem, 1500) returned %lld, elapsed = %d ms\n", res, elapsed);
	if(res != 0 || elapsed < 1300){
		print("FAIL: tsemacquire\n");
		exits("FAIL");
	}

	/* verify that sleeps are NOT interrupted */
	wkup = awake(700);
	start = nsec();
	sleep(1500);
	elapsed = (nsec() - start) / (1000 * 1000);
	if(verbose)
		print("sleep(1500), elapsed = %d ms\n", elapsed);
	if(elapsed < 1300){
		print("FAIL: sleep\n");
		exits("FAIL");
	}

	/* verify that semacquires are interrupted */
	/* MEMENTO: this requires deeper modifications to semacquire() in
	 * 9/port/sysproc.c, but it will enable tsemacquire to be moved to
	 * libc (one less syscall!)
	wkup = awake(1000);
	start = nsec();
	if(verbose)
		print("semacquire(&sem, 1)...\n");
	res = semacquire(&sem, 1);
	elapsed = (nsec() - start) / (1000 * 1000);
	if(verbose)
		print("semacquire(&sem, 1): returned %lld, elapsed = %d ms\n", res, elapsed);
	if(!awakened(wkup) || res != -1 || elapsed < 900 || elapsed > 1300){
		print("FAIL: semacquire\n");
		exits("FAIL");
	}
	*/

	/* do not forgivewkp the awake(60000): the kernel must handle it */
	print("PASS\n");
	exits("PASS");
}
