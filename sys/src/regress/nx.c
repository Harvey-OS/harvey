#include <u.h>
#include <libc.h>
#define RET 0xc3

int success;
int cases;

void
handler(void *v, char *s)
{
	success++;
	exits(nil);
}

void
callinsn(char *name, char *buf)
{
	void (*f)(void);
	if (notify(handler)){
		fprint(2, "%r\n");
		exits("notify fails");
	}


	f = (void *)buf;
	f();
	print("FAIL %s\n", name);
	exits("FAIL");
}

void
writeptr(char *name, void *ptr)
{
	if (notify(handler)){
		fprint(2, "%r\n");
		exits("notify fails");
	}

	*(uintptr_t*)ptr = 0xdeadbeef;
	print("FAIL %s\n", name);
	exits("FAIL");
}

void
main(void)
{
	char *str = "hello world";
	char stk[128];
	char *mem;

	switch(rfork(RFMEM|RFPROC)){
	case -1:
		sysfatal("rfork");
	case 0:
		stk[0] = RET;
		callinsn("exec stack", stk);
	default:
		cases++;
		waitpid();
	}

	switch(rfork(RFMEM|RFPROC)){
	case -1:
		sysfatal("rfork");
	case 0:
		mem = malloc(128);
		mem[0] = RET;
		callinsn("exec heap", mem);
	default:
		cases++;
		waitpid();
	}

	switch(rfork(RFMEM|RFPROC)){
	case -1:
		sysfatal("rfork");
	case 0:
		writeptr("write code", (void*)&main);
	default:
		cases++;
		waitpid();
	}

	switch(rfork(RFMEM|RFPROC)){
	case -1:
		sysfatal("rfork");
	case 0:
		writeptr("write rodata", (void*)str);
	default:
		cases++;
		waitpid();
	}

	if(success == cases){
		print("PASS\n");
		exits(nil);
	}
	print("FAIL\n");
	exits("FAIL");
}
