#include "all.h"
#include "io.h"
#include "mem.h"

#include "../ip/ip.h"
#include "etherif.h"

extern int ether21140reset(Ether*);
extern int etherelnk3reset(Ether*);
extern int etheri82557reset(Ether*);

static struct
{
	char*	type;
	int	(*reset)(Ether*);
} etherctlr[] =
{
	{ "21140", ether21140reset, },
	{ "2114x", ether21140reset, },
	{ "3C509", etherelnk3reset, },
	{ "elnk3", etherelnk3reset, },
	{ "i82557", etheri82557reset, },
	{ 0, },
};

static Ether etherif[MaxEther];

void
etheriq(Ether* ether, Msgbuf* mb)
{
	ilock(&ether->rqlock);
	if(ether->rqhead)
		ether->rqtail->next = mb;
	else
		ether->rqhead = mb;
	ether->rqtail = mb;
	mb->next = 0;
	iunlock(&ether->rqlock);

	wakeup(&ether->rqr);
}

static int
isinput(void* arg)
{
	return ((Ether*)arg)->rqhead != 0;
}

static void
etheri(void)
{
	Ether *ether;
	Ifc *ifc;
	Msgbuf *mb;
	Enpkt *p;

	ether = getarg();
	ifc = &ether->ifc;
	print("ether%di: %E %I\n", ether->ctlrno, ether->ifc.ea, ether->ifc.ipa);	
	(*ether->attach)(ether);

	for(;;) {
		while(!isinput(ether))
			sleep(&ether->rqr, isinput, ether);

		ilock(&ether->rqlock);
		if(ether->rqhead == 0) {
			iunlock(&ether->rqlock);
			continue;
		}
		mb = ether->rqhead;
		ether->rqhead = mb->next;
		iunlock(&ether->rqlock);

		p = (Enpkt*)mb->data;
		switch(nhgets(p->type)){
		case Arptype:
			arpreceive(p, mb->count, ifc);
			break;
		case Iptype:
			ipreceive(p, mb->count, ifc);
			ifc->rxpkt++;
			ifc->work[0].count++;
			ifc->work[1].count++;
			ifc->work[2].count++;
			ifc->rate[0].count += mb->count;
			ifc->rate[1].count += mb->count;
			ifc->rate[2].count += mb->count;
			break;
		}
		mbfree(mb);
	}
}

static void
ethero(void)
{
	Ether *ether;
	Ifc *ifc;
	Msgbuf *mb;
	int len;

	ether = getarg();
	ifc = &ether->ifc;
	print("ether%do: %E %I\n", ether->ctlrno, ifc->ea, ifc->ipa);	

	for(;;) {
		for(;;) {
			mb = recv(ifc->reply, 0);
			if(mb != nil)
				break;
		}

		if(mb->data == 0) {
			print("ether%do: pkt nil cat=%d free=%d\n",
				ether->ctlrno, mb->category, mb->flags&FREE);
			if(!(mb->flags & FREE))
				mbfree(mb);
			continue;
		}

		len = mb->count;
		if(len > ETHERMAXTU) {
			print("ether%do: pkt too big - %d\n", ether->ctlrno, len);
			mbfree(mb);
			continue;
		}
		if(len < ETHERMINTU) {
			memset(mb->data+len, 0, ETHERMINTU-len);
			len = ETHERMINTU;
		}
		memmove(((Enpkt*)(mb->data))->s, ifc->ea, sizeof(ifc->ea));

		ilock(&ether->tqlock);
		if(ether->tqhead)
			ether->tqtail->next = mb;
		else
			ether->tqhead = mb;
		ether->tqtail = mb;
		mb->next = 0;
		iunlock(&ether->tqlock);

		(*ether->transmit)(ether);

		ifc->work[0].count++;
		ifc->work[1].count++;
		ifc->work[2].count++;
		ifc->rate[0].count += len;
		ifc->rate[1].count += len;
		ifc->rate[2].count += len;
		ifc->txpkt++;
	}
}

Msgbuf*
etheroq(Ether* ether)
{
	Msgbuf *mb;

	mb = nil;

	ilock(&ether->tqlock);
	if(ether->tqhead){
		mb = ether->tqhead;
		ether->tqhead = mb->next;
	}
	iunlock(&ether->tqlock);

	return mb;
}

static void
cmd_state(int, char*[])
{
	Ether *ether;
	Ifc *ifc;

	for(ether = &etherif[0]; ether < &etherif[MaxEther]; ether++){
		if(ether->mbps == 0)
			continue;

		ifc = &ether->ifc;
		if(!isvalidip(ifc->ipa))
			continue;

		print("ether stats %d\n", ether->ctlrno);
		print("	work =%7W%7W%7W pkts\n", ifc->work+0, ifc->work+1, ifc->work+2);
		print("	rate =%7W%7W%7W tBps\n", ifc->rate+0, ifc->rate+1, ifc->rate+2);
		print("	err  =    %3ld rc %3ld sum\n", ifc->rcverr, ifc->sumerr);
	}
}

void
etherstart(void)
{
	Ether *ether;
	Ifc *ifc;
	int anystarted;
	char buf[100], *p;

	anystarted = 0;
	for(ether = &etherif[0]; ether < &etherif[MaxEther]; ether++){
		if(ether->mbps == 0)
			continue;

		ifc = &ether->ifc;
		lock(ifc);
		getipa(ifc, ether->ctlrno);
		if(!isvalidip(ifc->ipa)){
			unlock(ifc);
			ether->mbps = 0;
			continue;
		}
		if(ifc->reply == 0){
			dofilter(ifc->work+0, C0a, C0b, 1);
			dofilter(ifc->work+1, C1a, C1b, 1);
			dofilter(ifc->work+2, C2a, C2b, 1);
			dofilter(ifc->rate+0, C0a, C0b, 1000);
			dofilter(ifc->rate+1, C1a, C1b, 1000);
			dofilter(ifc->rate+2, C2a, C2b, 1000);
			ifc->reply = newqueue(Nqueue);
		}
		unlock(ifc);

		sprint(ether->oname, "ether%do", ether->ctlrno);
		userinit(ethero, ether, ether->oname);
		sprint(ether->iname, "ether%di", ether->ctlrno);
		userinit(etheri, ether, ether->iname);

		ifc->next = enets;
		enets = ifc;

		anystarted++;
	}

	if(anystarted){
		cmd_install("state", "-- ether stats", cmd_state);
		arpstart();
		if((p = getconf("route")) && strlen(p) < sizeof(buf)-7){
			sprint(buf, "route %s", p);
			cmd_exec(buf);
		}
	}
}

static int
parseether(uchar *to, char *from)
{
	char nip[4];
	char *p;
	int i;

	p = from;
	while(*p == ' ')
		++p;
	for(i = 0; i < 6; i++){
		if(*p == 0)
			return -1;
		nip[0] = *p++;
		if(*p == 0)
			return -1;
		nip[1] = *p++;
		nip[2] = 0;
		to[i] = strtoul(nip, 0, 16);
		if(*p == ':')
			p++;
	}
	return 0;
}

void
etherinit(void)
{
	Ether *ether;
	int i, n, ctlrno;

	for(ctlrno = 0; ctlrno < MaxEther; ctlrno++){
		ether = &etherif[ctlrno];
		memset(ether, 0, sizeof(Ether));
		if(!isaconfig("ether", ctlrno, ether))
			continue;

		for(n = 0; etherctlr[n].type; n++){
			if(cistrcmp(etherctlr[n].type, ether->type))
				continue;
			ether->ctlrno = ctlrno;
			ether->tbdf = BUSUNKNOWN;
			for(i = 0; i < ether->nopt; i++){
				if(strncmp(ether->opt[i], "ea=", 3))
					continue;
				if(parseether(ether->ea, &ether->opt[i][3]) == -1)
					memset(ether->ea, 0, Easize);
			}	
			if((*etherctlr[n].reset)(ether))
				break;

			if(ether->irq == 2)
				ether->irq = 9;
			setvec(Int0vec + ether->irq, ether->interrupt, ether);
			memmove(ether->ifc.ea, ether->ea, sizeof(ether->ea));

			print("ether%d: %s: %dMbps port 0x%lux irq %ld",
				ctlrno, ether->type, ether->mbps, ether->port, ether->irq);
			if(ether->mem)
				print(" addr 0x%lux", ether->mem & ~KZERO);
			if(ether->size)
				print(" size 0x%lux", ether->size);
			print(": ");
			for(i = 0; i < sizeof(ether->ea); i++)
				print("%2.2ux", ether->ea[i]);
			print("\n");
		
			break;
		}
	}
}
