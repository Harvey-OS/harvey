#include <u.h>
#include <libc.h>
#include <bio.h>
#include "chain.h"
#include "debug.h"
#include "tap.h"
#include "lebo.h"
#include "bebo.h"
#include "jtag.h"
#include "icert.h"
#include "mpsse.h"
#include "/sys/src/9/kw/arm.h"
#include "mmu.h"

/* 
 *	Read, paying special atention to anything called 
 *	embedded-ice/debug of the arm core, normally
 *	there is a chapter called Debug or Debugging and some
 *	details sprinkled around.
 *	- ARM9E-S "Technical Reference Manual"
 *	- ARM7TDMI-S "Core Technical Reference Manual"
 *	- Open On-Chip Debugger thesis
 *	"Design and Implementation of an On-Chip Debug for Embedded
 *	Target Systems"
 *	- Application note 205 "Writing JTAG Sequences for Arm 9 Processors"
 *	The bit ordering on the last one is confusing/wrong, but the
 *	procedures for doing things clarify a lot.
 */


/*
 *	BUG: print pack and unpack could be generated automatically 
 *	except for the wild endianness of the instr. Minilanguage?
 */

static char *
printech1(EiceChain1 *ec1, char *s, int ssz)
{
	char *e, *te;

	te = s + ssz -1;
	
	e = seprint(s, te, "rwdata: %#8.8ux\n", ec1->rwdata);
	e = seprint(e, te, "wptandbkpt: %1.1d\n", ec1->wptandbkpt);
	e = seprint(e, te, "sysspeed: %1.1d\n", ec1->sysspeed);
	e = seprint(e, te, "instr: %#8.8ux\n", ec1->instr);
	return e;
}

static int
packech1(Chain *ch, EiceChain1 *ec1)
{
	EiceChain1	lec;
	uchar unused;

	unused = 0;

	lec.instr = hmsbputl(&ec1->instr);
	hbeputl(&lec.rwdata, ec1->rwdata);

	putbits(ch, &lec.rwdata, RWDataSz);
	putbits(ch, &unused, 1);
	putbits(ch, &lec.wptandbkpt, WpTanDBKptSz);
	
	putbits(ch, &ec1->sysspeed, SysSpeedSz);
	putbits(ch, &lec.instr, InstrSz);

	return 0;
}

static int
unpackech1(EiceChain1 *ec1, Chain *ch)
{
	uchar unused;

	unused = 0;


	getbits(&ec1->rwdata, ch, RWDataSz);
	getbits(&unused, ch, 1);
	getbits(&ec1->wptandbkpt, ch, WpTanDBKptSz);
	getbits(&ec1->sysspeed, ch, SysSpeedSz);
	getbits(&ec1->instr, ch, InstrSz);

	ec1->instr = msbhgetl(&ec1->instr);
	ec1->rwdata = lehgetl(&ec1->rwdata);

	return 0;
}


static char *
printech2(EiceChain2 *ec2, char *s, int ssz)
{
	char *e, *te;

	te = s + ssz -1;
	
	e = seprint(s, te, "data: %#8.8ux\n", ec2->data);
	e = seprint(e, te, "addr: %#2.2ux\n", ec2->addr);
	e = seprint(e, te, "rw: %1.1d\n", ec2->rw);
	return e;
}

static int
valregaddr(uchar addr)
{
	if(addr <= CtlMskReg)
		return 1;
	if( (addr&Wp0)  <= CtlMskReg ||  (addr&Wp1)  <= CtlMskReg)
		return 1;
	return 0;
}

static int
packech2(Chain *ch, EiceChain2 *ec2)
{
	EiceChain2	lec;

	if(!valregaddr(ec2->addr))
		return -1;

	hleputl(&lec.data, ec2->data);
	
	putbits(ch, &lec.data, DataSz);
	putbits(ch, &ec2->addr, AddrSz);
	putbits(ch, &ec2->rw, RWSz);
	return 0;
}

static int
unpackech2(EiceChain2 *ec2, Chain *ch)
{

	getbits(&ec2->data, ch, DataSz);
	getbits(&ec2->addr, ch, AddrSz);
	getbits(&ec2->rw, ch, RWSz);

	if(!valregaddr(ec2->addr))
		return -1;

	ec2->data = lehgetl(&ec2->data);

	return 0;
}


static char *
printwp(EiceWp *wp, char *s, int ssz)
{
	char *e, *te;

	te = s + ssz -1;
	
	e = seprint(s, te, "addrval: %#8.8ux\n", wp->addrval);
	e = seprint(e, te, "addmsk: %#8.8ux\n", wp->addrmsk);
	e = seprint(e, te, "dataval: %#8.8ux\n", wp->dataval);
	e = seprint(e, te, "datamsk: %#8.8ux\n", wp->datamsk);

	e = seprint(e, te, "ctlval: %#4.4ux\n", wp->ctlval);
	e = seprint(e, te, "ctlmsk: %#2.2ux\n", wp->ctlmsk);
	return e;
}

static char *
printregs(EiceRegs *regs, char *s, int ssz)
{
	char *e, *te, i;

	te = s + ssz -1;
	
	e = seprint(s, te, "debug: %#2.2ux\n", regs->debug);
	e = seprint(e, te, "debsts: %#2.2ux\n", regs->debsts);
	e = seprint(e, te, "veccat: %#2.2ux\n", regs->veccat);
	e = seprint(e, te, "debcomctl: %#2.2ux\n", regs->debcomctl);
	e = seprint(e, te, "debcomdata: %#8.8ux\n", regs->debcomdata);

	for(i = 0; i<nelem(regs->wp); i++)
		e = printwp(regs->wp + i, e, ssz+(s-e));
	return e;
}

static u32int
unpackcpuid(uchar *v)
{
	u32int id;

	id = lehgetl(v);
	return id;
}

static void
prcpuid(u32int id)
{

	fprint(2, "ID: %#8.8ux\n", id);
	fprint(2, "Must be 1: %d\n", id&1);
	fprint(2, "Manufacturer id: %#lux\n", MANID(id));
	fprint(2, "Part no: %#lux\n", PARTNO(id));
	fprint(2, "Version: %#lux\n", VERS(id));
	return;
}



int
setinst(JMedium *jmed, uchar inst, int nocommit)
{
	int res, com;
	ShiftTDesc req;
	uchar buf[1+ShiftMarg];

	memset(buf, 0, 5);
	buf[0] = inst;
	com = 0;

	dprint(Djtag, "Setting jtag instr: %#2.2ux, commit: %d\n",
			inst, !nocommit);
	if(nocommit)
		com = ShiftNoCommit;
	req.reg = TapIR;
	req.buf = buf;
	req.nbits = InLen;
	req.op = ShiftOut|com;

	res = tapshift(jmed, &req, nil, jmed->tapcpu);
	if(res < 0){
		werrstr("setinst: shifting instruction %#2.2ux for jtag", inst);
		return -1;
	}
	return res;
}

int
icesetreg(JMedium *jmed, int regaddr, u32int val)
{
	Chain ch;
	ShiftTDesc req;
	EiceChain2 ec2;
	char debugstr[128];
	int res;

	dprint(Dice, "Set eice 2 reg[%#2.2ux] to %#8.8ux\n", regaddr, val);
	ec2.data = val;
	ec2.addr = regaddr;
	ec2.rw = 1;
	memset(&ch, 0, sizeof ch);


	res = packech2(&ch, &ec2);
	if(res < 0)
		return -1;
	if(debug[Dchain] || debug[DAll]){
		printech2(&ec2, debugstr, sizeof debugstr);
		fprint(2, "%s", debugstr);
		printchain(&ch);
	}
	req.reg = TapDR;
	req.buf = ch.buf;
	req.nbits = EiceCh2Len;
	req.op = ShiftOut;
	res = tapshift(jmed, &req, nil, jmed->tapcpu);
	if(res < 0)
		return -1;

	return 0;
}


u32int
icegetreg(JMedium *jmed, int regaddr)
{
	Chain ch;
	ShiftTDesc req;
	int res;
	EiceChain2 ec2;
	char debugstr[128];

	ec2.data = 0;
	ec2.addr = regaddr;
	ec2.rw = 0;
	memset(&ch, 0, sizeof ch);

	res = packech2(&ch, &ec2);
	if(res < 0)
		return -1;
	if(debug[Dchain] || debug[DAll]){
		printech2(&ec2, debugstr, sizeof debugstr);
		fprint(2, "%s", debugstr);
		printchain(&ch);
	}
	req.reg = TapDR;
	req.buf = ch.buf;
	req.nbits = EiceCh2Len;
	req.op = ShiftOut;
	res = tapshift(jmed, &req, nil, jmed->tapcpu);
	if(res < 0)
		return -1;
	req.reg = TapDR;
	req.buf = ch.buf;
	req.nbits = EiceCh2Len;
	req.op = ShiftOut|ShiftIn;
	res = tapshift(jmed, &req, nil, jmed->tapcpu);
	if(res < 0)
		return -1;
	ch.b = 0;
	ch.e = EiceCh2Len;
	res = unpackech2(&ec2, &ch);
	if(res < 0)
		return -1;
	if(debug[Dice] || debug[DAll]){
		printchain(&ch);
		printech2(&ec2, debugstr, sizeof debugstr);
		fprint(2, "Get eice 2 reg[%#2.2ux] to %#8.8ux\n", regaddr, ec2.data);
	}
	return ec2.data;
}

int
setchain(JMedium *jmed, int commit, uchar chain)
{
	ShiftTDesc req;
	uchar data[1+ShiftMarg];
	int res, com;

	dprint(Dice, "Set chain %d, chain before %d\n", chain, jmed->currentch);
	if(jmed->currentch == chain)
		return 0;
	res = setinst(jmed, InScanN, 1);
	if(res < 0)
		return -1;
	data[0] = chain;
	com = 0;
	if(!commit)
		com = ShiftNoCommit;
	req.reg = TapDR;
	req.buf = data;
	req.nbits = 5;
	req.op = ShiftPauseIn|ShiftOut|com;
	res = tapshift(jmed, &req, nil, jmed->tapcpu);
	if(res < 0)
		return -1;
	res = setinst(jmed, InTest, 1);
	if(res < 0)
		return -1;
	jmed->currentch = chain;
	return 0;
}


int
armgofetch(JMedium *jmed, u32int instr, int sysspeed)
{
	char debugstr[128];
	ShiftTDesc req;
	EiceChain1 ec1;
	Chain ch;
	int res;

	ec1.instr = instr;
	ec1.sysspeed = sysspeed;
	ec1.wptandbkpt = 0;
	ec1.rwdata = 0;
	memset(&ch, 0, sizeof (Chain));
	res = packech1(&ch, &ec1);
	if(res < 0)
		return -1;
	if(debug[Dchain] || debug[DAll]){
		printech1(&ec1, debugstr, sizeof debugstr);
		fprint(2, "%s", debugstr);
		printchain(&ch);
	}
	req.reg = TapDR;
	req.buf = ch.buf;
	req.nbits = ch.e-ch.b;
	req.op = ShiftOut|ShiftPauseOut|ShiftPauseIn;
	res = tapshift(jmed, &req, nil, jmed->tapcpu);
	if(res < 0)
		return -1;
	return 0;

}

int
armgetexec(JMedium *jmed, int nregs, u32int *regs, u32int inst)
{
	ShiftTDesc req;
	int i, res, flags;
	uchar *buf;
	ShiftRDesc *replies;

	dprint(Darm, "Exec: %#8.8ux\n", inst);
	replies = mallocz(nregs*sizeof(ShiftRDesc), 1);
	if(replies == nil)
		sysfatal("read, no mem %r");
	buf = mallocz(EiceCh1Len+ShiftMarg, 1);	
	if(buf == nil)
		sysfatal("read, no mem %r");
	armgofetch(jmed, inst, 0);
	/* INST in decode stage */
	armgofetch(jmed, ARMNOP, 0);
	/* INST in execute stage */
	armgofetch(jmed, ARMNOP, 0);
	/* INST in access stage */
	flags = ShiftAsync|ShiftIn|ShiftPauseIn|ShiftPauseOut;
	for(i = 0; i < nregs; i++){
		req.reg = TapDR;
		req.buf = buf;
		req.nbits = EiceCh1Len;
		req.op = flags;
		res = tapshift(jmed, &req,  &replies[i], jmed->tapcpu);
		if(res < 0)
			return -1;
	}
	for(i = 0; i < nregs; i++){
		res = jmed->rdshiftrep(jmed, buf, &replies[i]);
		if(res < 0)
			return -1;
		regs[i] = *(u32int *)buf;
		dprint(Darm, "Read [%d]: %#8.8ux\n", i, regs[i]);
	}	
	armgofetch(jmed, ARMNOP, 0);
	/* INST in writeback stage */
	armgofetch(jmed, ARMNOP, 0);
	/* INST end, pipeline full of NOPs */

	free(buf);
	free(replies);
	return 0;
}

int
armsetexec(JMedium *jmed, int nregs, u32int *regs, u32int inst)
{
	ShiftTDesc req;
	int i, res, flags;
	uchar *buf;

	ShiftRDesc *replies;

	dprint(Darm, "Exec: %#8.8ux\n", inst);
	replies = mallocz(nregs*sizeof(ShiftRDesc), 1);
	if(replies == nil)
		sysfatal("read, no mem %r");
	buf = mallocz(EiceCh1Len+ShiftMarg, 1);
	if(buf == nil)
		sysfatal("readregs, no mem %r");
	armgofetch(jmed, inst, 0);
	/* STMIA in decode stage */
	armgofetch(jmed, ARMNOP, 0);
	/* STMIA in execute stage */
	armgofetch(jmed, ARMNOP, 0);
	/* STMIA in memory acess stage */
	flags = ShiftOut|ShiftPauseIn|ShiftPauseOut;
	for(i = 0; i < nregs; i++){
		*(u32int *)buf = regs[i];
		dprint(Darm, "Set [%d]: %#8.8ux\n", i, regs[i]);
		req.reg = TapDR;
		req.buf = buf;
		req.nbits = EiceCh1Len;
		req.op = flags;
		res = tapshift(jmed, &req,  nil, jmed->tapcpu);
		if(res < 0)
			return -1;
	}
	armgofetch(jmed, ARMNOP, 0);
	/* STMIA in writeback stage */
	armgofetch(jmed, ARMNOP, 0);
	/* STMIA end, pipeline full of NOPs */

	free(buf);
	free(replies);
	return 0;
}

/* reads r0-r15 to regs if bits of mask are 1 */
static int
armgetregs(JMedium *jmed, u32int mask, u32int *regs)
{
	int ireps, nregs, r, i;
	u32int *regrd;

	nregs = 0;
	ireps = 0;
	for(i = 0; i < 16; i++){
		if( (1 << i) & mask)
			nregs++;
	}
	regrd = mallocz(nregs*sizeof(u32int), 1);
	if(regrd == nil)
		sysfatal("read, no mem %r");

	r = armgetexec(jmed, nregs, regrd, ARMSTMIA|mask);
	if(r < 0)
		return -1;

	for(i = 0; i < 16; i++){
		if( (1 << i) & mask){
			regs[i] = regrd[ireps++];
			dprint(Darm, "rd register r%d: %#8.8ux\n", i, regs[i]);
		}
	}
	free(regrd);
	return 0;
}
/* sets r0-r15 to regs if bits of mask are 1 */
int
armsetregs(JMedium *jmed, int mask, u32int *regs)
{
	int ireps, nregs, r, i;
	u32int *regrw;

	nregs = 0;
	ireps = 0;
	for(i = 0; i < 16; i++){
		if( (1 << i) & mask)
			nregs++;
	}
	regrw = mallocz(nregs*sizeof(u32int), 1);
	if(regrw == nil)
		sysfatal("read, no mem %r");
	for(i = 0; i < 16; i++){
		if( (1 << i) & mask){
			regrw[ireps++] = regs[i];
			dprint(Darm, "wr register r%d: %#8.8ux\n", i, regs[i]);
		}
	}
	r = armsetexec(jmed, nregs, regrw, ARMLDMIA|mask);
	if(r < 0)
		return -1;

	free(regrw);
	return 0;
}

static void
armprctxt(ArmCtxt *ctxt)
{
	int i;

	fprint(2, "Arm is in debug: %d\nregs\n", ctxt->debug);
	for(i = 0; i < 16; i++)
		fprint(2, "r%d: %#8.8ux\n", i, ctxt->r[i]);
	fprint(2, "cpsr: %#8.8ux\n", ctxt->cpsr);
	fprint(2, "spsr: %#8.8ux\n", ctxt->spsr);
}

char *
armsprctxt(ArmCtxt *ctxt, char *s, int ssz)
{
	char *e, *te;
	int i;

	te = s + ssz -1;

	e = seprint(s, te, "Arm is in debug: %d\nregs\n", ctxt->debug);
	for(i = 0; i < 16; i++)
		e = seprint(e, te, "r%d: %#8.8ux\n", i, ctxt->r[i]);
	e = seprint(e, te, "cpsr: %#8.8ux\n", ctxt->cpsr);
	e = seprint(e, te, "spsr: %#8.8ux\n", ctxt->spsr);
	return e;
}

static char dbgstr[4*1024];

int
armsavectxt(JMedium *jmed, ArmCtxt *ctxt)
{
	int res;

	res = setchain(jmed, 0, 1);
	if(res < 0)
		goto Error;
	
	res = armgetregs(jmed, 0xffff, ctxt->r);
	if(res < 0)
		goto Error;
	res = armgofetch(jmed, ARMMRSr0CPSR, 0);
	if(res < 0)
		goto Error;
	res = armgetexec(jmed, 1, &ctxt->cpsr, ARMSTMIA|0x0001);
	if(res < 0)
		goto Error;
	res = armgofetch(jmed, ARMMRSr0SPSR, 0);
	if(res < 0)
		goto Error;
	res = armgetexec(jmed, 1, &ctxt->spsr, ARMSTMIA|0x0001);
	if(res < 0)
		goto Error;
	res = mmurdregs(jmed, ctxt);
	if(res < 0){
		werrstr("mmurdregs %r");
		return -1;
	}
	if(debug[Dmmu] || debug[DAll]){
		printmmuregs(ctxt, dbgstr, sizeof dbgstr);
		dprint(Dfs, "MMU state:\n%s\n", dbgstr);
	}
	if(debug[Dctxt] || debug[DAll]){
		fprint(2, "Arm save ctxt\n");
		armprctxt(ctxt);
	}
	return 0;
Error:
	return -1;
}

static int
armjmpctxt(JMedium *jmed, ArmCtxt *ctxt)
{
	int res;

	if(debug[Dctxt] || debug[DAll]){
		fprint(2, "Arm jmp ctxt\n");
		armprctxt(ctxt);
	}
	res = armsetexec(jmed, 1, &ctxt->cpsr, ARMLDMIA|0x0001);
	if(res < 0)
		return -1;
	res = armgofetch(jmed, ARMMSRr0CPSR, 0);
	if(res < 0)
		return -1;
	 /*	it is quite important this is the last instr for PC calculations */
	ctxt->r[15] += ctxt->pcadjust;
	res = armsetregs(jmed, 0xffff, ctxt->r);
	if(res < 0)
		return -1;

	return 0;
}


int
armfastexec(JMedium *jmed, u32int inst)
{
	int res;

	dprint(Darm, "Exec: %#8.8ux\n", inst);
	res = armgofetch(jmed, ARMNOP, 0);
	if(res < 0)
		return -1;

	res = armgofetch(jmed, ARMNOP, 0);
	if(res < 0)
		return -1;
	res = armgofetch(jmed, ARMNOP, 0);
	if(res < 0)
		return -1;
	res = armgofetch(jmed, inst, 0);
	if(res < 0)
		return -1;
	res = armgofetch(jmed, ARMNOP, 1);
	if(res < 0)
		return -1;
	return 0;
}

int
icedebugstate(JMedium *jmed)
{
	int res;

	res = setchain(jmed, ChCommit, 2);
	if(res < 0)
		return 0;
	return icegetreg(jmed, DebStsReg) & DBGACK;
}

int
icewaitdebug(JMedium *jmed)
{
	int i, res;

	res = setchain(jmed, ChCommit, 2);
	if(res < 0)
		return -1;

	for(i = 0; i < 20; i ++){
		if(icedebugstate(jmed))
			break;
		sleep(100);
	}
	if( i == 20)
		return -1;

	icesetreg(jmed, DebugCtlReg, INTDIS|DBGACK); /* clear RQ */
	return 0;
}

int
armrdmemwd(JMedium *jmed, u32int addr, u32int *data, int sz)
{
	int res;
	u32int d, byte;

	res = setchain(jmed, ChCommit, 1);
	if(res < 0)
		sysfatal("setchain %r");
	byte = 0;
	if(sz == 1)
		byte = BYTEWDTH;
	/* load address in r0 */
	res = armsetexec(jmed, 1, &addr, ARMLDMIA|0x0001);
	if(res < 0)
		return -1;

	/* LDR r1, [r0] place data in r1 */
	res = armfastexec(jmed, byte|ARMLDRindr1xr0);
	if(res < 0)
		return -1;

	setinst(jmed, InRestart, 0);
	res = icewaitdebug(jmed);
	if(res < 0)
		return -1;
	res = setchain(jmed, ChCommit, 1);
	if(res < 0)
		sysfatal("setchain %r");
	armgetexec(jmed, 1, &d, byte|ARMSTMIA|0x0002);

	*data = d;
	dprint(Dmem, "Read data %#8.8ux addr %#8.8ux \n",
				d, addr);
	return sz;
}

int
armwrmemwd(JMedium *jmed, u32int addr, u32int data, int sz)
{
	int res;
	u32int byte;

	dprint(Dmem, "Write data %#8.8ux addr %#8.8ux \n",
				data, addr);
	res = setchain(jmed, ChCommit, 1);
	if(res < 0)
		sysfatal("setchain %r");
	byte = 0;
	if(sz == 1)
		byte = BYTEWDTH;
	/* load address in r0 */
	res = armsetexec(jmed, 1, &addr, ARMLDMIA|0x0001);
	if(res < 0)
		return -1;
	/* load data in r1 */
	res = armsetexec(jmed, 1, &data, byte|ARMLDMIA|0x0002);
	if(res < 0)
		return -1;

	/* STR [r0], r1 place data in [r0] */
	res = armfastexec(jmed, ARMSTRindxr0r1);
	if(res < 0)
		return -1;

	setinst(jmed, InRestart, 0);
	res = icewaitdebug(jmed);
	if(res < 0)
		return -1;
	return sz;
}


u32int
armidentify(JMedium *jmed)
{
	uchar *buf;
	ShiftTDesc req;
	u32int cpuid;
	int res, i;

	//if(jmed->motherb == Sheeva)
	//	jmed->resets(jmed->mdata, 0, 1);
	buf = malloc(sizeof(u32int)*0x100+	ShiftMarg);
	if(buf == nil)
		sysfatal("memory");

	for(i = 0; i < sizeof(u32int)*0x100; i++)
		buf[i] = i;	/* just a pattern to see in debugging */

	req.reg = TapDR;
	req.buf = buf;
	req.nbits = sizeof(u32int)*0x100;	/* Bug by 8? */
	req.op = ShiftOut|ShiftIn;
	res = tapshift(jmed, &req, nil, jmed->tapcpu);
	if(res < 0){
		free(buf);
		return ~0;
	}
	//if(jmed->motherb == Sheeva)
	//	jmed->resets(jmed->mdata, 0, 0);
	cpuid = unpackcpuid(buf);
	if(cpuid != jmed->taps[jmed->tapcpu].hwid){
		fprint(2, "cpuid: %#8.8ux != %#8.8ux\n", cpuid, jmed->taps[jmed->tapcpu].hwid);
		free(buf);
		return ~0;
	}
	prcpuid(cpuid);
	free(buf);
	return cpuid;
}

/* see if the fixed bypass value is ok */
int
armbpasstest(JMedium *jmed)
{
	ShiftTDesc req;
	uchar buf[2+ShiftMarg];
	int res;

	memset(buf, 0xff, sizeof(buf));
	req.reg = TapIR;
	req.buf = buf;
	req.nbits = 2+InLen;
	req.op = ShiftOut|ShiftIn;
	res = tapshift(jmed, &req, nil, jmed->tapcpu);
	if(res < 0)
		sysfatal("jmed->regshift %r");
	if((buf[0] & 0xf) !=  0x1){
		fprint(2, "bad bypass: %#2.2ux\n", buf[0]);
		return -1;
	}

	return 0;
}

/* BUG: werrstr in the inner functions to indicate error, warnings... */


int
icewaitentry(JMedium *jmed, ArmCtxt *ctxt)
{
	int res;

	if(ctxt->debugreas == NoReas){
		werrstr("No debug activated");
		return -1;
	}
	res = setchain(jmed, ChCommit, 2);
	if(res < 0)
		return -1;
	res = icewaitdebug(jmed);
	if(res < 0)
		return -1;
	
	res = setchain(jmed, ChNoCommit, 1);
	if(res < 0)
		return -1;
	
	/*	BUG: should change to Arm from Thumb or Jazelle, adjust, keep PC.
	 *	If I ever come down to doing this, Remember to repeat Thumb inst
	 *	to be endian independant...
	  */
	res = armsavectxt(jmed, ctxt);
	if(res < 0)
		return -1;
	ctxt->debug = 1;

	ctxt->pcadjust = 0;	/* BUG: Thumb not impl */
	ctxt->r[15] -= 6*ArmInstSz;
	switch(ctxt->debugreas){
		case BreakReas:
		case VeccatReas:
			ctxt->pcadjust = -2*ArmInstSz;
		break;
		case DebugReas:
		break;
	}
	return 0;
}

int
iceenterdebug(JMedium *jmed, ArmCtxt *ctxt)
{
	int res;

	res = setchain(jmed, ChCommit, 2);
	if(res < 0)
		return -1;

	//if(jmed->motherb == Sheeva)
	//	jmed->resets(jmed->mdata, 0, 1);
	if(!(icegetreg(jmed, DebStsReg) & DBGACK)){
			icesetreg(jmed, DebugCtlReg, DBGRQ);	/* freeze */
	}

	//if(jmed->motherb == Sheeva)
	//	jmed->resets(jmed->mdata, 0, 0);
	ctxt->debugreas = DebugReas;
	res = icewaitentry(jmed, ctxt);
	if(res < 0)
		return -1;
	ctxt->exitreas = NoReas;
	return 0;
}


/*
 *	The branch is the way to exit debug permanently
 * 	no branch and you fall back to debug
 *	The context needs to be restored (some register needs
 *	to be written), don't know why
 *	but you will fall back to debug if not, no matter what doc says.
 *	The bypass is not mentioned to be necessary anywhere, but
 *	the feroceon will not restart without it.
 */

int
iceexitdebug(JMedium *jmed, ArmCtxt *ctxt)
{
	int res, i;
	uchar wctl;
	u32int sts;

	res = setchain(jmed, ChCommit, 1);
	if(res < 0)
		return -1;

	res = armjmpctxt(jmed, ctxt);
	if(res < 0)
		return -1;

	/* 4 entry to debug,  Store(does not count)  3 NOPS, Branch = -8 */
	res = armfastexec(jmed, ARMBRIMM|0xfffffb);
	if(res < 0)
		return -1;

	res = setchain(jmed, ChCommit, 2);
	if(res < 0)
		return -1;

	wctl = icegetreg(jmed, Wp0|CtlValReg);
	if(ctxt->exitreas != BreakReqReas)
		icesetreg(jmed, Wp0|CtlValReg, wctl&~EnableWPCtl);

	if(ctxt->exitreas != VeccatReqReas)
		icesetreg(jmed, VecCatReg, 0);
	icesetreg(jmed, DebugCtlReg, 0); /* clear all,*/
	setinst(jmed, InBypass, 0);
	setinst(jmed, InRestart, 0);
	sleep(100);
	res = setchain(jmed, ChCommit, 2);
	if(res < 0)
		return -1;

	for(i = 0; i < 10; i ++){
		sts = icegetreg(jmed, DebStsReg);
		if((sts&DBGACK) == 0)
			break;
		sleep(100);
	}
	if( i == 10)
		return -1;

	switch(ctxt->exitreas){
		case NoReas:
			ctxt->debugreas = NoReas;
		break;
		case VeccatReqReas:
			ctxt->debugreas = VeccatReas;
		break;
		case BreakReqReas:
			ctxt->debugreas = BreakReas;
		break;
		default:
			ctxt->debugreas = NoReas;
		break;
	}
	ctxt->debug = 0;
	return 0;
}
