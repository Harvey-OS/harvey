#include <u.h>
#include <libc.h>
#include <lock.h>

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

void
lockinit()
{
	int n;

	arch = C_fcr0();
	switch(arch) {
	case POWER:
		n = segattach(SG_CEXEC, "lock", (void*)Lockaddr, Pagesize);
		if(n < 0) {
			arch = MAGNUM;
			break;
		}
		memset((void*)Lockaddr, 0, Pagesize);
		break;
	case MAGNUM:
	case MAGNUMII:
	case R4K:
		break;
	default:
		abort();
	}
	
}

void
lock(Lock *lk)
{
	int *hwsem;
	int hash;

	switch(arch) {
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

	switch(arch) {
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
