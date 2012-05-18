#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>
#include "../boot/boot.h"

static long lusertime(char*);

char *timeserver = "#s/boot";

void
settime(int islocal, int afd, char *rp)
{
	int n, f;
	int timeset;
	Dir dir[2];
	char timebuf[64];

	print("time...");
	timeset = 0;
	if(islocal){
		/*
		 *  set the time from the real time clock
		 */
		f = open("#r/rtc", ORDWR);
		if(f >= 0){
			if((n = read(f, timebuf, sizeof(timebuf)-1)) > 0){
				timebuf[n] = '\0';
				timeset = 1;
			}
			close(f);
		}else do{
			strcpy(timebuf, "yymmddhhmm[ss]");
			outin("\ndate/time ", timebuf, sizeof(timebuf));
		}while((timeset=lusertime(timebuf)) <= 0);
	}
	if(timeset == 0){
		/*
		 *  set the time from the access time of the root
		 */
		f = open(timeserver, ORDWR);
		if(f < 0)
			return;
		if(mount(f, afd, "/tmp", MREPL, rp) < 0){
			warning("settime mount");
			close(f);
			return;
		}
		close(f);
		if(stat("/tmp", statbuf, sizeof statbuf) < 0)
			fatal("stat");
		convM2D(statbuf, sizeof statbuf, &dir[0], (char*)&dir[1]);
		sprint(timebuf, "%ld", dir[0].atime);
		unmount(0, "/tmp");
	}

	f = open("#c/time", OWRITE);
	if(write(f, timebuf, strlen(timebuf)) < 0)
		warning("can't set #c/time");
	close(f);
	print("\n");
}

#define SEC2MIN 60L
#define SEC2HOUR (60L*SEC2MIN)
#define SEC2DAY (24L*SEC2HOUR)

int
g2(char **pp)
{
	int v;

	v = 10*((*pp)[0]-'0') + (*pp)[1]-'0';
	*pp += 2;
	return v;
}

/*
 *  days per month plus days/year
 */
static	int	dmsize[] =
{
	365, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
static	int	ldmsize[] =
{
	366, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/*
 *  return the days/month for the given year
 */
static int *
yrsize(int y)
{

	if((y%4) == 0 && ((y%100) != 0 || (y%400) == 0))
		return ldmsize;
	else
		return dmsize;
}

/*
 *  compute seconds since Jan 1 1970
 */
static long
lusertime(char *argbuf)
{
	char *buf;
	ulong secs;
	int i, y, m;
	int *d2m;

	buf = argbuf;
	i = strlen(buf);
	if(i != 10 && i != 12)
		return -1;
	secs = 0;
	y = g2(&buf);
	m = g2(&buf);
	if(y < 70)
		y += 2000;
	else
		y += 1900;

	/*
	 *  seconds per year
	 */
	for(i = 1970; i < y; i++){
		d2m = yrsize(i);
		secs += d2m[0] * SEC2DAY;
	}

	/*
	 *  seconds per month
	 */
	d2m = yrsize(y);
	for(i = 1; i < m; i++)
		secs += d2m[i] * SEC2DAY;

	secs += (g2(&buf)-1) * SEC2DAY;
	secs += g2(&buf) * SEC2HOUR;
	secs += g2(&buf) * SEC2MIN;
	if(*buf)
		secs += g2(&buf);

	sprint(argbuf, "%ld", secs);
	return secs;
}
