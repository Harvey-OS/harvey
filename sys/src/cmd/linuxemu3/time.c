#include <u.h>
#include <libc.h>
#include <ureg.h>
#include <tos.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

struct linux_timezone
{
	int		tz_minuteswest;
	int		tz_dsttime;
};

static struct linux_timezone systz;

void
inittime(void)
{
	Tm *t;

	boottime = nsec();

	systz.tz_minuteswest = 0;
	systz.tz_dsttime = 0;

	if(t = localtime(time(nil)))
		systz.tz_minuteswest = t->tzoff / 60;
}

int sys_time(long *p)
{
	return time(p);
}

int sys_clock_gettime(int clock, void *t)
{
	struct linux_timespec *ts = t;
	vlong x;

	trace("sys_clock_gettime(%d, %p)", clock, t);
	x = nsec();
	ts->tv_sec = (long)(x/1000000000LL);
	ts->tv_nsec = (long)(x%1000000000LL);
	return 0;
}

int sys_gettimeofday(void *tvp, void *tzp)
{
	struct linux_timeval *tv = tvp;
	struct linux_timezone *tz = tzp;
	vlong t;

	trace("sys_gettimeofday(%p, %p)", tvp, tzp);

	t = nsec();
	tv->tv_sec = (long)(t/1000000000LL);
	tv->tv_usec = (long)((t%1000000000LL)/1000);

	if(tz)
		*tz = systz;

	return 0;
}

int sys_nanosleep(void *rqp, void *rmp)
{
	struct linux_timespec *req = rqp;
	struct linux_timespec *rem = rmp;
	vlong t, now;
	int err;

	trace("sys_nanosleep(%p, %p)", rqp, rmp);

	if(req == nil)
		return -EFAULT;
	if(req->tv_sec < 0 || req->tv_nsec < 0 || req->tv_nsec >= 1000000000LL)
		return -EINVAL;

	now = nsec();
	if(current->restart->syscall){
		t = current->restart->nanosleep.timeout;
	} else {
		t = now + req->tv_sec*1000000000LL + req->tv_nsec;
	}

	if(now < t){
		if(notifyme(1))
			err = -1;
		else {
			err = sleep((t - now) / 1000000LL);
			notifyme(0);
		}
		if(err < 0){
			now = nsec();
			if(now < t){
				current->restart->nanosleep.timeout = t;
				if(rem != nil){
					t -= now;
					rem->tv_sec = (long)(t/1000000000LL);
					rem->tv_nsec = (long)(t%1000000000LL);
				}
				return -ERESTART;
			}
		}
	}

	return 0;
}

int proctimes(Uproc *p, ulong *t)
{
	char buf[1024], *f[12];
	int fd, n;

	t[0] = t[1] = t[2] = t[3] = 0;
	snprint(buf, sizeof(buf), "/proc/%d/status", p->kpid);
	if((fd = open(buf, OREAD)) < 0)
		return mkerror();
	if((n = read(fd, buf, sizeof(buf)-1)) <= 0){
		close(fd);
		return mkerror();
	}
	close(fd);
	buf[n] = 0;
	if(getfields(buf, f, 12, 1, "\t ") != 12)
		return -EIO;
	t[0] = atoi(f[2])*HZ / 1000;
	t[1] = atoi(f[3])*HZ / 1000;
	t[2] = atoi(f[4])*HZ / 1000;
	t[3] = atoi(f[5])*HZ / 1000;
	return 0;
}

struct linux_tms
{
	long	tms_utime;
	long	tms_stime;
	long	tms_cutime;
	long	tms_cstime;
};

int sys_times(void *m)
{
	struct linux_tms *x = m;
	ulong t[4];
	int err;

	trace("sys_times(%p)", m);

	if(x != nil){
		if((err = proctimes(current, t)) < 0)
			return err;
		x->tms_utime = t[0];
		x->tms_stime = t[1];
		x->tms_cutime = t[2];
		x->tms_cstime = t[3];
	}
	return (HZ*(nsec() - boottime)) / 1000000000LL;
}