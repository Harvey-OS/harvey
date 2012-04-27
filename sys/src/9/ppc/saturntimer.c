#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "msaturn.h"

enum {
	Timer_ctrl = Saturn + 0x0106,
	Timer0_load = Saturn + 0x0200,
	Timer0_cnt = Saturn + 0x0204,
	Timer1_load = Saturn + 0x0300,
	Timer1_cnt = Saturn + 0x0304,
	
	T0_event = RBIT(13, ushort),
	T0_ie = RBIT(14, ushort),
	T0_cen = RBIT(15, ushort),
	T1_event = RBIT(5, ushort),
	T1_ie = RBIT(6, ushort),
	T1_cen = RBIT(7, ushort),
};

static ulong ticks;
static Lock tlock;
static ushort timer_ctl;

void
saturntimerintr(Ureg *u, void*)
{
	ushort ctl = *(ushort*)Timer_ctrl, v = 0;

	if(ctl&T1_event){
		v = T1_event;
		ticks++;
	}
		
	*(ushort*)Timer_ctrl = timer_ctl|T0_event|v;
	intack();
	timerintr(u, 0);
}

void
timerinit(void)
{
	*(ushort*)Timer_ctrl = 0;
	*(ulong*)Timer0_load = m->bushz  / HZ;
	*(ulong*)Timer0_cnt = m->bushz / HZ;
	*(ulong*)Timer1_load = m->bushz;
	*(ulong*)Timer1_cnt = m->bushz;

	intrenable(Vectimer0, saturntimerintr, nil, "timer");

	timer_ctl = T0_cen|T0_ie|T1_cen;
	*(ushort*)Timer_ctrl = timer_ctl;
}

uvlong
fastticks(uvlong *hz)
{
	assert(*(ushort*)Timer_ctrl&T1_cen);
	if(*(ushort*)Timer_ctrl&T1_event){
		*(ushort*)Timer_ctrl = timer_ctl|T1_event;
		ticks++;
	}
		
	if (hz)
		*hz = m->bushz;

	return (uvlong)ticks*m->bushz+(uvlong)(m->bushz-*(ulong*)Timer1_cnt);
}

void
timerset(uvlong next)
{
	ulong offset;
	uvlong now;

	ilock(&tlock);
	*(ushort*)Timer_ctrl = T1_cen;

	now = fastticks(nil);
	offset = next - now;
	if((long)offset < 10000)
		offset = 10000;
	else if(offset > m->bushz)
		offset = m->bushz;

	*(ulong*)Timer0_cnt = offset;
	*(ushort*)Timer_ctrl = timer_ctl;
	assert(*(ushort*)Timer_ctrl & T1_cen);
	iunlock(&tlock);
}

