#include <alef.h>

extern	byte	end[];

aggr Mem
{
	Lock;
	byte*	addr;
};

intern Mem bloc;

int
brk(void *p)
{
	uint n;

	n = (uint)p;
	n += 3;
	n &= ~3;

	bloc.lock();
	rescue {
		bloc.unlock();
		return -1;
	}

	if(bloc.addr == nil)
		bloc.addr = end;

	if(brk_((void*)n) < 0)
		raise;

	bloc.addr = (byte*)n;
	bloc.unlock();
	return 0;
}

void*
sbrk(uint n)
{
	void *base;

	n += 3;
	n &= ~3;

	bloc.lock();
	rescue {
		bloc.unlock();
		return (void*)-1;
	}

	if(bloc.addr == nil)
		bloc.addr = end;

	base = bloc.addr;
	if(brk_(bloc.addr+n) < 0)
		raise;

	bloc.addr += n;
	bloc.unlock();

	return base;
}
