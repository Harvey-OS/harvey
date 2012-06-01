#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "../port/error.h"
#include <tos.h>
#include "ureg.h"

/* from linux */
#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

void
arch_prctl(Ar0*ar, va_list list)
{
	uintptr va;
	int code;
	code = va_arg(list, int);
	va = va_arg(list, uintptr);
	if (up->attr & 128) print("%d:arch_prctl code %x va %p: ", up->pid, code, va);
	if (code < ARCH_SET_GS || code > ARCH_GET_GS)
		error("Bad code!");
	/* always make sure it's a valid address, no matter what the command */
	validaddr((void *)va, 8, code > ARCH_SET_FS);
	
	if (code > ARCH_SET_FS) {
		uvlong val; 
		val = rdmsr(code == ARCH_GET_FS ? 0xC0000100 : 0xC0000101);
		memmove((void *)va, &val, sizeof(uvlong));
		if (up->attr & 128) print("get %#p\n", (void *)val);
	} else {
		if (code == ARCH_SET_GS)
			error("Can't set GS yet");
		wrmsr(code == ARCH_SET_FS ? 0xC0000100 : 0xC0000101, va);
		if (up->attr & 128) print("set\n");
	}

	ar->i = 0;
}

