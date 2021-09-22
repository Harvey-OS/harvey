// Reserve a range to be ioalloced later.
// This is in particular useful for exchangable cards, such
// as pcmcia and cardbus cards.
int
ioreserve(int, int size, int align, char *tag)		/* unused */
{
	IOMap *map, **l;
	int i, port;

	lock(&iomap);
	// find a free port above 0x400 and below 0x1000
	port = 0x400;
	for(l = &iomap.map; *l; l = &(*l)->next){
		map = *l;
		if (map->start < 0x400)
			continue;
		i = map->start - port;
		if(i > size)
			break;
		if(align > 0)
			port = ((port+align-1)/align)*align;
		else
			port = map->end;
	}
	if(*l == nil){
		unlock(&iomap);
		return -1;
	}
	map = iomap.free;
	if(map == nil){
		print("ioalloc: out of maps");
		unlock(&iomap);
		return port;
	}
	iomap.free = map->next;
	map->next = *l;
	map->start = port;
	map->end = port + size;
	map->reserved = 1;
	strncpy(map->tag, tag, sizeof(map->tag));
	map->tag[sizeof(map->tag)-1] = 0;
	*l = map;

	archdir[0].qid.vers++;

	unlock(&iomap);
	return map->start;
}
