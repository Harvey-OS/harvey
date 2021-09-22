void
add_to_meminit(void)
	ulong maxmem, lost;
	char *p;

	if(p = getconf("*maxmem"))
		maxmem = strtoul(p, 0, 0);	/* only used by ramscan */
	else
		maxmem = 0;
}

/*
 * only called if e820scan fails, which should never happen on a PC built
 * in 2002 or later.
 */
static void
ramscan(ulong maxmem)
{
	ulong *k0, kzero, map, maxkpa, maxpa, pa, *pte, *table, *va, vbase, x;
	int nvalid[NMemType];

	/*
	 * The bootstrap code has has created a prototype page
	 * table which maps the first MemMin of physical memory to KZERO.
	 * The page directory is at m->pdb and the first page of
	 * free memory is after the per-processor MMU information.
	 */
	pa = MemMin;

	/*
	 * Check if the extended memory size can be obtained from the CMOS.
	 * If it's 0 then it's either not known or >= 64MB. Always check
	 * at least 24MB in case there's a memory gap (up to 8MB) below 16MB;
	 * in this case the memory from the gap is remapped to the top of
	 * memory.
	 * The value in CMOS is supposed to be the number of KB above 1MB.
	 */
	if(maxmem == 0){
		x = (nvramread(0x18)<<8)|nvramread(0x17);
		if(x == 0 || x >= (63*KB))
			maxpa = MemMax;
		else
			maxpa = MB+x*KB;
		if(maxpa < 24*MB)
			maxpa = 24*MB;
	}else
		maxpa = maxmem;
	maxkpa = (uint)-KZERO;	/* 2^32 - KZERO */

	/*
	 * March up memory from MemMin to maxpa 1MB at a time,
	 * mapping the first page and checking the page can
	 * be written and read correctly. The page tables are created here
	 * on the fly, allocating from low memory as necessary.
	 */
	k0 = (ulong*)KZERO;
	kzero = *k0;
	map = 0;
	x = 0x12345678;
	memset(nvalid, 0, sizeof(nvalid));

	/*
	 * Can't map memory to KADDR(pa) when we're walking because
	 * can only use KADDR for relatively low addresses.
	 * Instead, map each 4MB we scan to the virtual address range
	 * MemMin->MemMin+4MB while we are scanning.
	 */
	vbase = MemMin;
	while(pa < maxpa){
		/*
		 * Map the page. Use mapalloc(&rmapram, ...) to make
		 * the page table if necessary, it will be returned to the
		 * pool later if it isn't needed.  Map in a fixed range (the second 4M)
		 * because high physical addresses cannot be passed to KADDR.
		 */
		va = (void*)(vbase + pa%(4*MB));
		table = &m->pdb[PDX(va)];
		if(pa%(4*MB) == 0){
			if(map == 0 && (map = mapalloc(&rmapram, 0, BY2PG, BY2PG)) == 0)
				break;
			memset(KADDR(map), 0, BY2PG);
			*table = map|PTEWRITE|PTEVALID;
			memset(nvalid, 0, sizeof(nvalid));
		}
		table = KADDR(PPN(*table));
		pte = &table[PTX(va)];

		*pte = pa|PTEWRITE|PTEUNCACHED|PTEVALID;
		mmuflushtlb(PADDR(m->pdb));
		/*
		 * Write a pattern to the page and write a different
		 * pattern to a possible mirror at KZERO. If the data
		 * reads back correctly the chunk is some type of RAM (possibly
		 * a linearly-mapped VGA framebuffer, for instance...) and
		 * can be cleared and added to the memory pool. If not, the
		 * chunk is marked uncached and added to the UMB pool if <16MB
		 * or is marked invalid and added to the UPA pool.
		 */
		*va = x;
		*k0 = ~x;
		if(*va == x){
			nvalid[MemRAM] += MB/BY2PG;
			mapfree(&rmapram, pa, MB);

			do{
				*pte++ = pa|PTEWRITE|PTEVALID;
				pa += BY2PG;
			}while(pa % MB);
			mmuflushtlb(PADDR(m->pdb));
			/* memset(va, 0, MB); so damn slow to memset all of memory */
		}
		else if(pa < 16*MB){
			nvalid[MemUMB] += MB/BY2PG;
			mapfree(&rmapumb, pa, MB);

			do{
				*pte++ = pa|PTEWRITE|PTEUNCACHED|PTEVALID;
				pa += BY2PG;
			}while(pa % MB);
		}
		else{
			nvalid[MemUPA] += MB/BY2PG;
			mapfree(&rmapupa, pa, MB);

			*pte = 0;
			pa += MB;
		}
		/*
		 * Done with this 4MB chunk, review the options:
		 * 1) not physical memory and >=16MB - invalidate the PDB entry;
		 * 2) physical memory - use the 4MB page extension if possible;
		 * 3) not physical memory and <16MB - use the 4MB page extension
		 *    if possible;
		 * 4) mixed or no 4MB page extension - commit the already
		 *    initialised space for the page table.
		 */
		if(pa%(4*MB) == 0 && pa >= 32*MB && nvalid[MemUPA] == (4*MB)/BY2PG){
			/*
			 * If we encounter a 4MB chunk of missing memory
			 * at a sufficiently high offset, call it the end of
			 * memory.  Otherwise we run the risk of thinking
			 * that video memory is real RAM.
			 */
			break;
		}
		if(pa <= maxkpa && pa%(4*MB) == 0){
			table = &m->pdb[PDX(KADDR(pa - 4*MB))];
			if(nvalid[MemUPA] == (4*MB)/BY2PG)
				*table = 0;
			else if(nvalid[MemRAM] == (4*MB)/BY2PG &&
			    (m->cpuiddx & Pse))
				*table = (pa - 4*MB)|PTESIZE|PTEWRITE|PTEVALID;
			else if(nvalid[MemUMB] == (4*MB)/BY2PG &&
			    (m->cpuiddx & Pse))
				*table = (pa - 4*MB)|PTESIZE|PTEWRITE|PTEUNCACHED|PTEVALID;
			else{
				*table = map|PTEWRITE|PTEVALID;
				map = 0;
			}
		}
		mmuflushtlb(PADDR(m->pdb));
		x += 0x3141526;
	}
	/*
	 * If we didn't reach the end of the 4MB chunk, that part won't
	 * be mapped.  Commit the already initialised space for the page table.
	 */
	if(pa % (4*MB) && pa <= maxkpa){
		m->pdb[PDX(KADDR(pa))] = map|PTEWRITE|PTEVALID;
		map = 0;
	}
	if(map)
		mapfree(&rmapram, map, BY2PG);

	m->pdb[PDX(vbase)] = 0;
	mmuflushtlb(PADDR(m->pdb));

	mapfree(&rmapupa, pa, (uint)-pa);
	*k0 = kzero;
}
