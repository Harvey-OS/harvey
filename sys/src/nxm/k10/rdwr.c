#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"

#include "apic.h"

u16int l16get(u8int *p)
{
	u16int ret;

	DBG("l16get: %#p = ", p);
	ret = (((p)[1]<<8)|(p)[0]);
	DBG("l16get: %#x\n", ret);
	return ret;
}

u32int l32get(u8int *p)
{
	u32int ret;
	DBG("l32get: ");
	ret = (((u32int)l16get(p+2)<<16)|l16get(p));
	DBG("l32get: %#ulx\n", ret);
	return ret;
}

u64int l64get(u8int *p)
{
	u64int ret;
	DBG("l64get: ");
	ret = (((u64int)l32get(p+4)<<32)|l32get(p));
	DBG("l64get: %#ullx\n", ret);
	return ret;
}
