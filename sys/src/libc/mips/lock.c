#include <u.h>
#include <libc.h>

enum
{
	Pagesize	= 4096,
	Semperpg	= Pagesize/(16*sizeof(uint)),
	Lockaddr	= 0x60000000,

	POWER		= 0x320,
	MAGNUM		= 0x330,
	MAGNUMII	= 0x340,
	R4K		= 0x500,
};

static	int arch;
extern	int C_3ktas(int*);
extern	int C_4ktas(int*);
extern	int C_fcr0(void);

static void
lockinit(void)
{
	void *v;

	if(arch != 0)
		return;	/* allow multiple calls */
	arch = C_fcr0();
	switch(arch) {
	case POWER:
		v = (void*)Lockaddr;
		if(segattach(SG_CEXEC, "lock", v, Pagesize) == (void*)-1) {
			arch = MAGNUM;
			break;
		}
		memset(v, 0, Pagesize);
		break;
	case MAGNUM:
	case MAGNUMII:
	case R4K:
		break;
	default:
		arch = R4K;
		break;
	}
}

void
lock(Lock *lk)
{
	int *hwsem;
	int hash;

retry:
	switch(arch) {
	case 0:
		lockinit();
		goto retry;
	case MAGNUM:
	case MAGNUMII:
		while(C_3ktas(&lk->val))
			sleep(0);
		return;
	case R4K:
		for(;;){
			while(lk->val)
				;
			if(C_4ktas(&lk->val) == 0)
				return;
		}
		break;
	case POWER:
		/* Use low order lock bits to generate hash */
		hash = ((int)lk/sizeof(int)) & (Semperpg-1);
		hwsem = (int*)Lockaddr+hash;

		for(;;) {
			if((*hwsem & 1) == 0) {
				if(lk->val)
					*hwsem = 0;
				else {
					lk->val = 1;
					*hwsem = 0;
					return;
				}
			}
			while(lk->val)
				;
		}
	}
}

int
canlock(Lock *lk)
{
	int *hwsem;
	int hash;

retry:
	switch(arch) {
	case 0:
		lockinit();
		goto retry;
	case MAGNUM:
	case MAGNUMII:
		if(C_3ktas(&lk->val))
			return 0;
		return 1;
	case R4K:
		if(C_4ktas(&lk->val))
			return 0;
		return 1;
	case POWER:
		/* Use low order lock bits to generate hash */
		hash = ((int)lk/sizeof(int)) & (Semperpg-1);
		hwsem = (int*)Lockaddr+hash;

		if((*hwsem & 1) == 0) {
			if(lk->val)
				*hwsem = 0;
			else {
				lk->val = 1;
				*hwsem = 0;
				return 1;
			}
		}
		return 0;
	}
}

void
unlock(Lock *lk)
{
	lk->val = 0;
}

int
_tas(int *p)
{
	int *hwsem;
	int hash;

retry:
	switch(arch) {
	case 0:
		lockinit();
		goto retry;
	case MAGNUM:
	case MAGNUMII:
		return C_3ktas(p);
	case R4K:
		return C_4ktas(p);
	case POWER:
		/* Use low order lock bits to generate hash */
		hash = ((int)p/sizeof(int)) & (Semperpg-1);
		hwsem = (int*)Lockaddr+hash;

		if((*hwsem & 1) == 0) {
			if(*p)
				*hwsem = 0;
			else {
				*p = 1;
				*hwsem = 0;
				return 0;
			}
		}
		return 1;
	}
}
