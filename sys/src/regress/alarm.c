/*
 * This file is part of Harvey.
 *
 * Copyright (C) 2015  Giacomo Tesio
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

/* Test alarm(2):
 * - alarms replace each other (the last one wins)
 * - alarms are received as notes
 * - alarms are received (more or less) on time
 *
 * Given we have two clocks at work here, we can use one to
 * check the other:
 * - nsec() is an high resolution clock from RDTSC instruction
 *   (see sysproc.c, portclock.c and tod.c in sys/src/9/port)
 * - sys->ticks incremented by hzclock (in portclock.c) that
 *   is used by alarm (and sleep and the kernel scheduler)
 */

int verbose = 0;
int64_t alarmReceived;

int
printFirst(void *v, char *s)
{
	/* just not exit, please */
	if(strcmp(s, "alarm") == 0){
		alarmReceived = nsec();
		if(verbose)
			fprint(2, "%d: noted: %s at %lld\n", getpid(), s, nsec());
		atnotify(printFirst, 0);
		return 1;
	}
	return 0;
}

int
failOnSecond(void *v, char *s)
{
	/* just not exit, please */
	if(strcmp(s, "alarm") == 0){
		print("FAIL\n");
		exits("FAIL");
	}
	return 0;
}

void
main(void)
{
	int64_t a2000, a500;
	if (!atnotify(printFirst, 1) || !atnotify(failOnSecond, 1)){
		fprint(2, "%r\n");
		exits("atnotify fails");
	}

	alarm(2000);
	a2000 = nsec();
	alarm(500);
	a500 = nsec();
	while(sleep(5000) < 0)
		;

	if(verbose)
		fprint(2, "%d: set alarm(2000)@%lld then alarm(500)@%lld; received after %lld nanosecond\n", getpid(), a2000, a500, alarmReceived-a500);

	if(alarmReceived-a500 > 700 * 1000 * 1000){
		print("FAIL\n");
		exits("FAIL");
	}
	
	print("PASS\n");
	exits("PASS");
}
