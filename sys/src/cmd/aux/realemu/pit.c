#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

enum {
	AC0 = 0,
	AC1,
	AC2,
	Actl,

	Readback = 3,

	RBC0 = 1<<1,
	RBC1 = 1<<2,
	RBC2 = 1<<3,
	RBlatchstatus = 1<<4,
	RBlatchcount = 1<<5,

	AMlatchcount = 0,
	AMloonly,
	AMhionly,
	AMlohi,

	OM0 = 0,
	OM1,
	OM2,
	OM3,
	OM4,
	OM5,
	OM2b,
	OM3b,
};

static void
latchstatus(Pit *ch)
{
	print_func_entry();
	if(ch->rlatched) {
		print_func_exit();
		return;
	}
	ch->rlatch[0] = ch->bcd | ch->omode<<1 | ch->amode<<4 | ch->count0<<6 | ch->out<<7;
	ch->rcount = 0;
	ch->rlatched = 1;
	print_func_exit();
}

static void
latchcount(Pit *ch)
{
	print_func_entry();
	unsigned long w;

	if(ch->rlatched) {
		print_func_exit();
		return;
	}
	w = ch->count & 0xFFFF;
	if(ch->bcd)
		w = (w % 10) + (((w/10) % 10)<<4) + (((w/100) % 10)<<8) + (((w/1000) % 10)<<12);
	ch->rlatch[0] = w & 0xFF;
	ch->rlatch[1] = (w >> 8) & 0xFF;
	ch->rcount = 0;
	ch->rlatched = 1;
	switch(ch->amode){
	case AMhionly:
		ch->rcount++;
		break;
	case AMlohi:
		ch->rlatched++;
		break;
	}
	print_func_exit();
}

static void
setcount(Pit *ch)
{
	print_func_entry();
	unsigned long w;

	w = (unsigned long)ch->wlatch[0] | (unsigned long)ch->wlatch[1] << 8;
	if(ch->bcd)
		w = (w & 0xF) + 10*((w >> 4)&0xF) + 100*((w >> 8)&0xF) + 1000*((w >> 12)&0xF);
	ch->count = w;
	ch->count0 = 0;
	print_func_exit();
}

static int
deccount(Pit *ch, long long *cycles)
{
	print_func_entry();
	if(ch->count0){
		*cycles = 0;
		print_func_exit();
		return 0;
	} else {
		long long passed, remain;

		passed = *cycles;
		if(ch->count == 0){
			ch->count = ch->bcd ? 9999 : 0xFFFF;
			passed--;
		}
		if(passed <= ch->count){
			remain = 0;
			ch->count -= passed;
		} else {
			remain = passed - ch->count;
			ch->count = 0;
		}
		*cycles = remain;
		print_func_exit();
		return ch->count == 0;
	}
}

void
setgate(Pit *ch, unsigned char gate)
{
	print_func_entry();
	if(ch->gate == 0 && gate)
		ch->gateraised = 1;
	ch->gate = gate;
	print_func_exit();
}

static void
clockpit1(Pit *ch, long long *cycles)
{
	print_func_entry();
	switch(ch->omode){
	case OM0:	/* Interrupt On Terminal Count */
		if(ch->count0){
			setcount(ch);
			ch->out = 0;
		Next:
			--*cycles;
			print_func_exit();
			return;
		}
		if(ch->gate && deccount(ch, cycles)){
			ch->out = 1;
			print_func_exit();
			return;
		}
		break;

	case OM1:	/* Hardware Re-triggerable One-shot */
		if(ch->gateraised){
			ch->gateraised = 0;
			setcount(ch);
			ch->out = 0;
			goto Next;
		}
		if(deccount(ch, cycles) && ch->out == 0){
			ch->out = 1;
			print_func_exit();
			return;
		}
		break;

	case OM2:	/* Rate Generator */
	case OM2b:
		ch->out = 1;
		if(ch->count0){
			setcount(ch);
			goto Next;
		}
		if(ch->gate == 0)
			break;
		if(ch->gateraised){
			ch->gateraised = 0;
			setcount(ch);
			goto Next;
		}
		if(deccount(ch, cycles)){
			setcount(ch);
			ch->out = 0;
			print_func_exit();
			return;
		}
		break;

	case OM3:	/* Square Wave Generator */
	case OM3b:
		if(ch->count0){
			setcount(ch);
			goto Next;
		}
		if(ch->gate == 0)
			break;
		if(ch->gateraised){
			ch->gateraised = 0;
			setcount(ch);
			goto Next;
		}
		if(deccount(ch, cycles)){
			setcount(ch);
			ch->out ^= 1;
			print_func_exit();
			return;
		}
		break;

	case OM4:	/* Software Triggered Strobe */
		ch->out = 1;
		if(ch->count0){
			setcount(ch);
			goto Next;
		}
		if(ch->gate && deccount(ch, cycles)){
			ch->out = 0;
			print_func_exit();
			return;
		}
		break;

	case OM5:	/* Hardware Triggered Strobe */
		ch->out = 1;
		if(ch->gateraised){
			ch->gateraised = 0;
			setcount(ch);
			goto Next;
		}
		if(deccount(ch, cycles)){
			ch->out = 0;
			print_func_exit();
			return;
		}
		break;
	}
	*cycles = 0;
	print_func_exit();
}

void
clockpit(Pit *pit, long long cycles)
{
	print_func_entry();
	Pit *ch;
	int i;

	if(cycles <= 0) {
		print_func_exit();
		return;
	}
	for(i = 0; i<Actl; i++){
		ch = pit + i;
		if(ch->wlatched){
			long long c;

			switch(ch->omode){
			case OM3:
			case OM3b:
				c = cycles * 2;
				break;
			default:
				c = cycles;
			}
			while(c > 0)
				clockpit1(ch, &c);
		}
		ch->gateraised = 0;
	}
	print_func_exit();
}

unsigned char
rpit(Pit *pit, unsigned char addr)
{
	print_func_entry();
	Pit *ch;
	unsigned char data;

	if(addr >= Actl) {
		print_func_exit();
		return 0;
	}
	ch = pit + addr;
	if(ch->rlatched){
		data = ch->rlatch[ch->rcount & 1];
		ch->rlatched--;
	} else {
		data = 0;
		switch(ch->amode){
		case AMloonly:
			data = ch->count & 0xFF;
			break;
		case AMhionly:
			data = (ch->count >> 8) & 0xFF;
			break;
		case AMlohi:
			data = (ch->count >> ((ch->rcount & 1)<<3)) & 0xFF;
			break;
		}
	}
	ch->rcount++;
	if(0) fprint(2, "rpit %p: %.2x %.2x\n", pit, (int)addr, (int)data);
	print_func_exit();
	return data;
}

void
wpit(Pit *pit, unsigned char addr, unsigned char data)
{
	print_func_entry();
	Pit *ch;

	if(0) fprint(2, "wpit %p: %.2x %.2x\n", pit, (int)addr, (int)data);
	if(addr > Actl) {
		print_func_exit();
		return;
	}
	if(addr == Actl){
		unsigned char sc, amode, omode, bcd;

		bcd = (data & 1);
		omode = (data >> 1) & 7;
		amode = (data >> 4) & 3;
		sc = (data >> 6) & 3;
	
		if(sc == Readback){
			ch = nil;
			for(;;){
				if(data & RBC0){
					ch = pit;
					break;
				}
				if(data & RBC1){
					ch = pit + 1;
					break;
				}
				if(data & RBC2){
					ch = pit + 2;
					break;
				}
				break;
			}
			if(ch == nil) {
				print_func_exit();
				return;
			}
			if((data & RBlatchcount) == 0)
				latchcount(ch);
			if((data & RBlatchstatus) == 0)
				latchstatus(ch);
			print_func_exit();
			return;
		}

		ch = pit + sc;
		if(amode == AMlatchcount){
			latchcount(ch);
			print_func_exit();
			return;
		}
		ch->bcd = bcd;
		
		ch->amode = amode;
		ch->omode = omode;

		ch->rlatched = 0;
		ch->rcount = 0;
		ch->rlatch[0] = 0;
		ch->rlatch[1] = 0;

		ch->wlatched = 0;
		ch->wcount = 0;
		ch->wlatch[0] = 0;
		ch->wlatch[1] = 0;

		ch->count0 = 1;
		ch->out = !!omode;
		print_func_exit();
		return;
	}

	ch = pit + addr;
	switch(ch->amode){
	case AMloonly:
	case AMhionly:
		ch->wlatch[ch->amode - AMloonly] = data;
		ch->wcount++;
		break;
	case AMlohi:
		ch->wlatch[ch->wcount++ & 1] = data;
		if(ch->wcount < 2) {
		print_func_exit();
		return;
		}
		break;
	}
	ch->wlatched = ch->wcount;
	ch->wcount = 0;
	ch->count0 = 1;
	print_func_exit();
}
