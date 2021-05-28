/*
 * alternate implementation with kprocs (TODO)
 */

Rendez oqgotpkt;
Rendez txdmadone;
Rendez gotfifotxq;
Rendez txdone;

int
isdmaready(Ether *ether)
{
	Block *bp;
	Queue *q;

	q = ether->oq;
	if (!qcanread(q))
		return 0;
	bp = qget(q);
	qputback(q, bp);
	return (frp->tdfv & Wordcnt) >=
		ROUNDUP(padpktlen(BLEN(bp)), BY2WD) / BY2WD;
}

void
txdmaproc(Ether *ether)			/* owns dma chan 1 */
{
	unsigned ruplen;
	Block *bp;

	for(;;) {
		/* sleep until oq has pkt & tx fifo has room for it */
		sleep(&oqgotpkt, isdmaready, ether);

		bp = qget(ether->oq);
		ruplen = ROUNDUP(padpktlen(BLEN(bp)), BY2WD);

		dmastart(Chan1, &frp->tdfd, bp->rp, ruplen, Sinc, dmatxdone);
		sleep(&txdmadone);

		addfifotxq(ether, bp);
		wakeup(&gotfifotxq);	/* have work for temac now */
	}
}

int
istemacready(Ether *ether)
{
	Ctlr *ctlr;

	ctlr = ether->ctlr;
	return ether->fifotxq != nil && !ctlr->xmitting;
}

void
txtemacproc(Ether *ether)		/* owns temac transmitter */
{
	Block *bp;

	for(;;) {
		/* sleep until txq has pkt & tx is idle */
		sleep(&gotfifotxq, istemacready, ether);

		bp = ether->fifotxq;
		ether->fifotxq = bp->next;

		kicktemac(ether);
		sleep(&txdone);

		free(bp);
		wakeup(&oqgotpkt);	/* might be work for dma1 now */
	}
}

dmatxdone()
{
	// ...
	wakeup(&txdmadone);
	// ...
}

temactransmit(Ether *ether)
{
	// ...
	wakeup(&oqgotpkt);
	// ...
}

dmaintr()
{
	// ...
	wakeup(&oqgotpkt);		/* if txfifo now has room */
	wakeup(&txdmadone);
	// ...
}

fifointr()
{
	// ...
	wakeup(&txdone);
	// ...
}
