static int
ise820biosmap(uchar *p)
{
	return memcmp(p, "APM\0", 4) == 0 || memcmp(p, "MAP\0", 4) == 0;
}

uintptr
finde820map(void)
{
	uchar *p;

	p = (uchar *)BIOSTABLES;
	if(ise820biosmap(p)){
		print("e820 map\n");
		return (uintptr)p;
	}
	p += 0x10;		/* compatible with some old bootstraps */
	if(ise820biosmap(p)){
		print("e820 map at old address %#p\n", p);
		return (uintptr)p;
	}
	return 0;
}

/* populates mmap from e820 map */
void
readlsconf(uintptr e820)
{
	int n;
	uchar *p;
	MMap *map, *nmap;
	uvlong addr, len;

	p = (uchar*)e820;
	for(n = 0; n < nelem(mmap) && *p != 0; n++)
		if(memcmp(p, "APM\0", 4) == 0)
			p += 20;			/* ignore APM */
		else if(memcmp(p, "MAP\0", 4) == 0){
			/* translate e820 MMap with size of "MAP\0" to mmap[] */
			map = (MMap*)p;
			switch(map->type){
			default:
				if(v_flag)
					print("type %ud", map->type);
				break;
			case 1:
				if(v_flag)
					print("Memory");
				break;
			case 2:
				if(v_flag)
					print("reserved");
				break;
			case 3:
				if(v_flag)
					print("ACPI Reclaim Memory");
				break;
			case 4:
				if(v_flag)
					print("ACPI NVS Memory");
				break;
			}
			addr = map->base;
			len = map->len;
			if(v_flag)
				print("\t%#16.16lluX %#16.16lluX (%llud)\n",
					addr, addr+len, len);

			if(nmmap < nelem(mmap)){
				nmap = &mmap[nmmap++];
				memmove(nmap, map, sizeof *map);
				nmap->size = sizeof(MMap) - sizeof map->size;
			}
			p += sizeof(MMap);
		} else				/* not a table from bios.s */
			break;
}
