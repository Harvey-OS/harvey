#include <u.h>
#include <libc.h>
#include <ureg.h>
#include "dat.h"
#include "fns.h"
#include "linux.h"

enum {
	FUTEX_WAIT,
	FUTEX_WAKE,
	FUTEX_FD,
	FUTEX_REQUEUE,
	FUTEX_CMP_REQUEUE,
};

uintptr futex(va_list list)
{
	ulong *addr;
	unsigned char op;
	int val;
	void *ptime;
	ulong *addr2;
	int val3;
	int err, val2;
	vlong timeout;

	addr = va_arg(list, ulong *);
	op = va_arg(list, int);
	val = va_arg(list, int);
	ptime = va_arg(list, void *);
	addr2 = va_arg(list, ulong *);
	val3 = va_arg(list, int);

	trace("sys_futex(%p, %d, %d, %p, %p, %d)", addr, op, val, ptime, addr2, val3);

	switch(op){
	case FUTEX_WAIT:
		trace("futex(): FUTEX_WAIT addr=%p", addr);
		print("futex(): FUTEX_WAIT addr=%p, val %d\n", addr, *addr);

		err = 0;
		timeout = 0;
#ifdef NOTYET
		if(ptime != nil){
			struct linux_timespec *ts = ptime;
			vlong now;

			wakeme(1);
			now = nsec();
			if(current->restart->syscall){
				timeout = current->restart->futex.timeout;
			} else {
				timeout = now + (vlong)ts->tv_sec * 1000000000LL + ts->tv_nsec;
			}
			if(now < timeout){
				current->timeout = timeout;
				setalarm(timeout);
			} else {
				err = -ETIMEDOUT;
			}
		}
#endif
		if(err == 0){
			if(*addr != val){
				err = -EWOULDBLOCK;
			} 
		}
#ifdef NOTYET
		if(ptime != nil){
			current->timeout = 0;
			wakeme(0);
		}
		if(err == -ERESTART)
			current->restart->futex.timeout = timeout;
#endif

		break;

#ifdef NOT
	case FUTEX_WAKE:
		trace("sys_futex(): FUTEX_WAKE futex=%p addr=%p", fu, addr);
		err = fu ? wakeq(fu, val < 0 ? 0 : val) : 0;
		break;

	case FUTEX_CMP_REQUEUE:
		trace("sys_futex(): FUTEX_CMP_REQUEUE futex=%p addr=%p", fu, addr);
		if(*addr != val3){
			err = -EAGAIN;
			break;
	case FUTEX_REQUEUE:
			trace("sys_futex(): FUTEX_REQUEUE futex=%p addr=%p", fu, addr);
		}
		err = fu ? wakeq(fu, val < 0 ? 0 : val) : 0;
		if(err > 0){
			val2 = (int)ptime;

			/* BUG: fu2 has to be in the same segment as fu */
			if(a = addr2area(seg, (ulong)addr2)){
				for(fu2 = a->futex; fu2; fu2 = fu2->next){
					if(fu2->addr == addr2){
						err += requeue(fu, fu2, val2);
						break;
					}
				}
			}
		}
		break;

#endif

	default:
		err = -ENOSYS;
	}

out:
	return err;
}
