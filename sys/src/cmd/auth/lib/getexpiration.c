#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>
#include "authcmdlib.h"

/*
 * get the date in the format yyyymmdd
 */
Tm
getdate(char *d)
{
	Tm date;
	int i;

	date.year = date.mon = date.mday = 0;
	date.hour = date.min = date.sec = 0;
	for(i = 0; i < 8; i++)
		if(!isdigit(d[i]))
			return date;
	date.year = (d[0]-'0')*1000 + (d[1]-'0')*100 + (d[2]-'0')*10 + d[3]-'0';
	date.year -= 1900;
	d += 4;
	date.mon = (d[0]-'0')*10 + d[1]-'0' - 1;
	d += 2;
	date.mday = (d[0]-'0')*10 + d[1]-'0';
	date.yday = 0;
	return date;
}

long
getexpiration(char *db, char *u)
{
	char buf[Maxpath];
	char prompt[128];
	char cdate[32];
	Tm date;
	ulong secs, now;
	int n, fd;

	/* read current expiration (if any) */
	snprint(buf, sizeof buf, "%s/%s/expire", db, u);
	fd = open(buf, OREAD);
	buf[0] = 0;
	if(fd >= 0){
		n = read(fd, buf, sizeof(buf)-1);
		if(n > 0)
			buf[n-1] = 0;
		close(fd);
	}

	if(buf[0]){
		if(strncmp(buf, "never", 5)){
			secs = atoi(buf);
			memmove(&date, localtime(secs), sizeof(date));
			snprint(buf, sizeof buf, "%4.4d%2.2d%2.2d",
				date.year+1900, date.mon+1, date.mday);
		} else
			buf[5] = 0;
	} else
		strcpy(buf, "never");
	snprint(prompt, sizeof prompt,
		"Expiration date (YYYYMMDD or never)[return = %s]: ", buf);

	now = time(0);
	for(;;){
		readln(prompt, cdate, sizeof cdate, 0);
		if(*cdate == 0)
			return -1;
		if(strcmp(cdate, "never") == 0)
			return 0;
		date = getdate(cdate);
		secs = tm2sec(&date);
		if(secs > now && secs < now + 2*365*24*60*60)
			break;
		print("expiration time must fall between now and 2 years from now\n");
	}
	return secs;
}
