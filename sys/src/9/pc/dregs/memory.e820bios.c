enum
{
	SMAP = ('S'<<24)|('M'<<16)|('A'<<8)|'P',
	Ememory = 1,
	Ereserved = 2,
	Carry = 1,
};

/*
 * load emap[] from bios calls; does not set BIOSTABLES.
 *
 * should no longer be needed; 9boot or expand will have extracted the
 * E820 map and APM info and stashed it in low memory at BIOSTABLES.
 */
int
e820bios(void)
{
	int i;
	ulong cont;
	Emap *e;
	Ureg u;

	if(getconf("*norealmode") || getconf("*noe820scan"))
		return -1;
	cont = 0;
	for(i=0; i<nelem(emap); i++){
		memset(&u, 0, sizeof u);
		u.ax = 0xE820;
		u.bx = cont;
		u.cx = 20;
		u.dx = SMAP;
		u.es = (PADDR(RMBUF)>>4)&0xF000;
		u.di = PADDR(RMBUF)&0xFFFF;
		u.trap = 0x15;
		realmode(&u);		/* bios call, int 15 */
		cont = u.bx;
		if((u.flags&Carry) || u.ax != SMAP || u.cx != 20)
			break;
		e = &emap[nemap++];
		*e = *(Emap*)RMBUF;
		if(u.bx == 0)
			break;
	}
	return 0;
}

