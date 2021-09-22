/*
 * a block allocator primarily for devether and ether drivers that
 * maintains and reuses a pool of max-size Blocks, per the better ether drivers.
 */

Block*
bpoolallocb(Blockpool *pool)
{
	Block *bp;
	static int whined;

	if (pool->bsize == 0)
		return nil;		/* pool is uninitialised */
	ilock(pool);
	if((bp = pool->first) != nil){
		pool->first = bp->next;
		bp->next = nil;
		ainc(&bp->ref);		/* prevent bp from being freed */
	}
	iunlock(pool);
	if (bp == nil && ++whined < 10)
		iprint("bpoolallocb out of pool Blocks\n");
	return bp;
}

void
bpoolfreeb(Blockpool *pool, Block *bp)
{
	bp->wp = bp->rp = (uchar *)ROUNDDN((ulong)bp->lim - pool->bsize,
		BLOCKALIGN);
	assert(bp->rp >= bp->base);
	assert(((ulong)bp->rp & (BLOCKALIGN-1)) == 0);
	bp->flag &= ~(Bipck | Budpck | Btcpck | Bpktck);

	ilock(pool);
	bp->next = pool->first;
	pool->first = bp;
	iunlock(pool);
	adec(&pool->nrbfull);
}

void
bpooladd(Blockpool *pool, int nblks)
{
	int n;
	Block *bp;
	static int inhere;

	if (pool->bsize == 0)			/* pool not initialised? */
		return;
	if (inhere++ > 0)
		panic("recursive bpooladd");
	if (up == nil)
		panic("bpooladd: nil up");
	for (n = nblks; n-- > 0; ){
		bp = (allocb)(pool->bsize);
		if(bp == nil)
			error(Enomem);
		bp->free = pool->free;
		freeb(bp);	
	}
	ilock(pool);
	pool->nrbfull += nblks;	/* don't let pool->nrbfull stay negative */
	pool->blocks += nblks;
	inhere--;
	iunlock(pool);
}

void
bpoolinit(Blockpool *pool, int blocks, int bsize, void (*free)(Block *))
{
	memset(pool, 0, sizeof *pool);
	pool->bsize = bsize;
	pool->free = free;
	bpooladd(pool, blocks);
}

void
bpoolremove(Blockpool *pool, int nblks)
{
	int n;
	Block *bp;

	ilock(pool);
	pool->blocks -= nblks;
	iunlock(pool);
	for (n = nblks; n-- > 0; ){
		bp = bpoolallocb(pool);
		if (bp == nil)
			break;
		adec(&bp->ref);		/* allow bp to be freed */
		bp->free = nil;
		freeb(bp);
	}
}

void
bpoolfree(Blockpool *pool)
{
	bpoolremove(pool, pool->blocks);
}
