#include <u.h>
#include <libc.h>
#include <libp.h>

QLock *tl;

void	iallocinit(void);
void	*ialloc(ulong);
void	test(char*);
void	startproc(void(*)(char*), char *);

void main(void){
	sharedata();
	iallocinit();
	lockinit();
	tl = ialloc(sizeof *tl);
	startproc(test, "child");
	test("parent");
	wait(0);
}

void test(char *who){
	qlock(tl);
	print("%s got it -- ", who);
	print("canqlock = %d\n", canqlock(tl));
	sleep(2*1000);
	print("%s releasing lock\n", who);
	qunlock(tl);
}

void startproc(void (*f)(char *), char *name){
	switch(fork()){
	case -1:
		print("can't fork");
		exits("fork");
	case 0:
		break;
	default:
		return;
	}
	f(name);
	exits(0);
}

void iallocinit(void){}

void *ialloc(ulong n){
	void *p;

	p = malloc(n);
	if(p)
		memset(p, 0, n);
	return p;
}
