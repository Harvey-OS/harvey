#include <u.h>
#include <libc.h>
#include <libp.h>

char *procname = "parent";
Lock *tl;

void	iallocinit(void);
void	*ialloc(ulong);
void	test(void);
void	startproc(void(*)(void), char *);

void main(void){
	iallocinit();
	lockinit();
	tl = ialloc(sizeof *tl);
	startproc(test, "child");
	test();
	wait(0);
}

void test(void){
	lock(tl);
	print("%s got it -- ", procname);
	print("canlock = %d\n", canlock(tl));
	sleep(2*1000);
	print("%s releasing lock\n", procname);
	unlock(tl);
}

void startproc(void (*f)(void), char *name){
	switch(fork()){
	case -1:
		print("can't fork");
		exits("fork");
	case 0:
		break;
	default:
		return;
	}
	procname = name;
	f();
	exits(0);
}

enum{
	WD = 4,			/* size for alignment; must be pow of 2 */
};

static ulong b, base, top;

void iallocinit(void){
	extern int edata;

	b = base = edata + 25*1024;
	top = base + 16*1024;
	if(segattach(0, "shared", (void *)base, top-base) < 0){
		print("can't created shared mem");
		exits("seg");
	}
}

void *ialloc(ulong n){
	ulong ob;

	n = (n + WD) & ~ (WD - 1);
	ob = b;
	b += n;
	if(b > top){
		if(segbrk((void*)base, (void*)(b + 16 * 1024)) < 0){
			print("ialloc too much mem");
			exits("out of mem");
		}
	}
	memset((void *)ob, 0, n);
	return (void *)ob;
}
