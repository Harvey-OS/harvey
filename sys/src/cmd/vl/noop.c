#include	"l.h"

/*
 * flag: insert nops to prevent three consecutive stores.
 * workaround for 24k erratum #48, costs about 10% in text space,
 * so only enable this if you need it.  test cases are "hoc -e '7^6'"
 * and "{ echo moon; echo plot } | scat".
 */
enum {
	Mips24k	= 0,
};

static int
isdblwrdmov(Prog *p)
{
	if(p == nil)
		return 0;
	switch(p->as){
	case AMOVD:
	case AMOVDF:
	case AMOVDW:
	case AMOVFD:
	case AMOVWD:
	case AMOVV:
	case AMOVVL:
	case AMOVVR:
	case AMOVFV:
	case AMOVDV:
	case AMOVVF:
	case AMOVVD:
		return 1;
	}
	return 0;
}

static int
ismove(Prog *p)
{
	if(p == nil)
		return 0;
	switch(p->as){
	case AMOVB:
	case AMOVBU:
	case AMOVF:
	case AMOVFW:
	case AMOVH:
	case AMOVHU:
	case AMOVW:
	case AMOVWF:
	case AMOVWL:
	case AMOVWR:
	case AMOVWU:
		return 1;
	}
	if(isdblwrdmov(p))
		return 1;
	return 0;
}

static int
isstore(Prog *p)
{
	if(p == nil)
		return 0;
	if(ismove(p))
		switch(p->to.type) {
		case D_OREG:
		case D_EXTERN:
		case D_STATIC:
		case D_AUTO:
		case D_PARAM:
			return 1;
		}
	return 0;
}

static int
iscondbranch(Prog *p)
{
	if(p == nil)
		return 0;
	switch(p->as){
	case ABEQ:
	case ABFPF:
	case ABFPT:
	case ABGEZ:
	case ABGEZAL:
	case ABGTZ:
	case ABLEZ:
	case ABLTZ:
	case ABLTZAL:
	case ABNE:
		return 1;
	}
	return 0;
}

static int
isbranch(Prog *p)
{
	if(p == nil)
		return 0;
	switch(p->as){
	case AJAL:
	case AJMP:
	case ARET:
	case ARFE:
		return 1;
	}
	if(iscondbranch(p))
		return 1;
	return 0;
}

static void
nopafter(Prog *p)
{
	p->mark |= LABEL|SYNC;
	addnop(p);
}

/*
 * workaround for 24k erratum #48, costs about 0.5% in space.
 * inserts a NOP before the last of 3 consecutive stores.
 * double-word stores complicate things.
 */
static int
no3stores(Prog *p)
{
	Prog *p1;

	if(!isstore(p))
		return 0;
	p1 = p->link;
	if(!isstore(p1))
		return 0;
	if(isdblwrdmov(p) || isdblwrdmov(p1)) {
		nopafter(p);
		nop.store.count++;
		nop.store.outof++;
		return 1;
	}
	if(isstore(p1->link)) {
		nopafter(p1);
		nop.store.count++;
		nop.store.outof++;
		return 1;
	}
	return 0;
}

/*
 * keep stores out of branch delay slots.
 * this is costly in space (the other 9.5%), but makes no3stores effective.
 * there is undoubtedly a better way to do this.
 */
void
storesnosched(void)
{
	Prog *p;

	for(p = firstp; p != P; p = p->link)
		if(isstore(p))
			p->mark |= NOSCHED;
}

int
triplestorenops(void)
{
	int r;
	Prog *p, *p1;

	r = 0;
	for(p = firstp; p != P; p = p1) {
		p1 = p->link;
//		if (p->mark & NOSCHED)
//			continue;
		if(ismove(p) && isstore(p)) {
			if (no3stores(p))
				r++;
			/*
			 * given storenosched, the next two
			 * checks shouldn't be necessary.
			 */
			/*
			 * add nop after first MOV in `MOV; Bcond; MOV'.
			 */
			else if(isbranch(p1) && isstore(p1->link)) {
				nopafter(p);
				nop.branch.count++;
				nop.branch.outof++;
				r++;
			}
			/*
			 * this may be a branch target, so insert a nop after,
			 * in case a branch leading here has a store in its
			 * delay slot and we have consecutive stores here.
			 */
			if(p->mark & (LABEL|SYNC) && !isnop(p1)) {
				nopafter(p);
				nop.branch.count++;
				nop.branch.outof++;
				r++;
			}
		} else if (isbranch(p))
			/*
			 * can't ignore delay slot of a conditional branch;
			 * the branch could fail and fall through.
			 */
			if (!iscondbranch(p) && p1)
				p1 = p1->link;	/* skip its delay slot */
	}
	return r;
}

void
noops(void)
{
	Prog *p, *p1, *q, *q1;
	int o, curframe, curbecome, maxbecome;

	/*
	 * find leaf subroutines
	 * become sizes
	 * frame sizes
	 * strip NOPs
	 * expand RET
	 * expand BECOME pseudo
	 */

	if(debug['v'])
		Bprint(&bso, "%5.2f noops\n", cputime());
	Bflush(&bso);

	curframe = 0;
	curbecome = 0;
	maxbecome = 0;
	curtext = 0;

	q = P;
	for(p = firstp; p != P; p = p->link) {

		/* find out how much arg space is used in this TEXT */
		if(p->to.type == D_OREG && p->to.reg == REGSP)
			if(p->to.offset > curframe)
				curframe = p->to.offset;

		switch(p->as) {
		case ATEXT:
			if(curtext && curtext->from.sym) {
				curtext->from.sym->frame = curframe;
				curtext->from.sym->become = curbecome;
				if(curbecome > maxbecome)
					maxbecome = curbecome;
			}
			curframe = 0;
			curbecome = 0;

			p->mark |= LABEL|LEAF|SYNC;
			if(p->link)
				p->link->mark |= LABEL;
			curtext = p;
			break;

		/* too hard, just leave alone */
		case AMOVW:
			if(p->to.type == D_FCREG ||
			   p->to.type == D_MREG) {
				p->mark |= LABEL|SYNC;
				break;
			}
			if(p->from.type == D_FCREG ||
			   p->from.type == D_MREG) {
				p->mark |= LABEL|SYNC;
				addnop(p);
				addnop(p);
				nop.mfrom.count += 2;
				nop.mfrom.outof += 2;
				break;
			}
			break;

		/* too hard, just leave alone */
		case ACASE:
		case ASYSCALL:
		case AWORD:
		case ATLBWR:
		case ATLBWI:
		case ATLBP:
		case ATLBR:
			p->mark |= LABEL|SYNC;
			break;

		case ANOR:
			if(p->to.type == D_REG && p->to.reg == REGZERO)
				p->mark |= LABEL|SYNC;
			break;

		case ARET:
			/* special form of RET is BECOME */
			if(p->from.type == D_CONST)
				if(p->from.offset > curbecome)
					curbecome = p->from.offset;

			if(p->link != P)
				p->link->mark |= LABEL;
			break;

		case ANOP:
			q1 = p->link;
			q->link = q1;		/* q is non-nop */
			q1->mark |= p->mark;
			continue;

		case ABCASE:
			p->mark |= LABEL|SYNC;
			goto dstlab;

		case ABGEZAL:
		case ABLTZAL:
		case AJAL:
			if(curtext != P)
				curtext->mark &= ~LEAF;

		case AJMP:
		case ABEQ:
		case ABGEZ:
		case ABGTZ:
		case ABLEZ:
		case ABLTZ:
		case ABNE:
		case ABFPT:
		case ABFPF:
			p->mark |= BRANCH;

		dstlab:
			q1 = p->cond;
			if(q1 != P) {
				while(q1->as == ANOP) {
					q1 = q1->link;
					p->cond = q1;
				}
				if(!(q1->mark & LEAF))
					q1->mark |= LABEL;
			} else
				p->mark |= LABEL;
			q1 = p->link;
			if(q1 != P)
				q1->mark |= LABEL;
			break;
		}
		q = p;
	}

	if(curtext && curtext->from.sym) {
		curtext->from.sym->frame = curframe;
		curtext->from.sym->become = curbecome;
		if(curbecome > maxbecome)
			maxbecome = curbecome;
	}

	if(debug['b'])
		print("max become = %d\n", maxbecome);
	xdefine("ALEFbecome", STEXT, maxbecome);

	curtext = 0;
	for(p = firstp; p != P; p = p->link) {
		switch(p->as) {
		case ATEXT:
			curtext = p;
			break;
		case AJAL:
			if(curtext != P && curtext->from.sym != S && curtext->to.offset >= 0) {
				o = maxbecome - curtext->from.sym->frame;
				if(o <= 0)
					break;
				/* calling a become or calling a variable */
				if(p->to.sym == S || p->to.sym->become) {
					curtext->to.offset += o;
					if(debug['b']) {
						curp = p;
						print("%D calling %D increase %d\n",
							&curtext->from, &p->to, o);
					}
				}
			}
			break;
		}
	}

	for(p = firstp; p != P; p = p->link) {
		o = p->as;
		switch(o) {
		case ATEXT:
			curtext = p;
			autosize = p->to.offset + 4;
			if(autosize <= 4)
			if(curtext->mark & LEAF) {
				p->to.offset = -4;
				autosize = 0;
			}

			q = p;
			if(autosize) {
				q = prg();
				q->as = AADD;
				q->line = p->line;
				q->from.type = D_CONST;
				q->from.offset = -autosize;
				q->to.type = D_REG;
				q->to.reg = REGSP;

				q->link = p->link;
				p->link = q;
			} else
			if(!(curtext->mark & LEAF)) {
				if(debug['v'])
					Bprint(&bso, "save suppressed in: %s\n",
						curtext->from.sym->name);
				Bflush(&bso);
				curtext->mark |= LEAF;
			}

			if(curtext->mark & LEAF) {
				if(curtext->from.sym)
					curtext->from.sym->type = SLEAF;
				break;
			}

			q1 = prg();
			q1->as = AMOVW;
			q1->line = p->line;
			q1->from.type = D_REG;
			q1->from.reg = REGLINK;
			q1->to.type = D_OREG;
			q1->from.offset = 0;
			q1->to.reg = REGSP;

			q1->link = q->link;
			q->link = q1;
			break;

		case ARET:
			nocache(p);
			if(p->from.type == D_CONST)
				goto become;
			if(curtext->mark & LEAF) {
				if(!autosize) {
					p->as = AJMP;
					p->from = zprg.from;
					p->to.type = D_OREG;
					p->to.offset = 0;
					p->to.reg = REGLINK;
					p->mark |= BRANCH;
					break;
				}

				p->as = AADD;
				p->from.type = D_CONST;
				p->from.offset = autosize;
				p->to.type = D_REG;
				p->to.reg = REGSP;

				q = prg();
				q->as = AJMP;
				q->line = p->line;
				q->to.type = D_OREG;
				q->to.offset = 0;
				q->to.reg = REGLINK;
				q->mark |= BRANCH;

				q->link = p->link;
				p->link = q;
				break;
			}
			p->as = AMOVW;
			p->from.type = D_OREG;
			p->from.offset = 0;
			p->from.reg = REGSP;
			p->to.type = D_REG;
			p->to.reg = 2;

			q = p;
			if(autosize) {
				q = prg();
				q->as = AADD;
				q->line = p->line;
				q->from.type = D_CONST;
				q->from.offset = autosize;
				q->to.type = D_REG;
				q->to.reg = REGSP;

				q->link = p->link;
				p->link = q;
			}

			q1 = prg();
			q1->as = AJMP;
			q1->line = p->line;
			q1->to.type = D_OREG;
			q1->to.offset = 0;
			q1->to.reg = 2;
			q1->mark |= BRANCH;

			q1->link = q->link;
			q->link = q1;
			break;

		become:
			if(curtext->mark & LEAF) {

				q = prg();
				q->line = p->line;
				q->as = AJMP;
				q->from = zprg.from;
				q->to = p->to;
				q->cond = p->cond;
				q->link = p->link;
				q->mark |= BRANCH;
				p->link = q;

				p->as = AADD;
				p->from = zprg.from;
				p->from.type = D_CONST;
				p->from.offset = autosize;
				p->to = zprg.to;
				p->to.type = D_REG;
				p->to.reg = REGSP;

				break;
			}
			q = prg();
			q->line = p->line;
			q->as = AJMP;
			q->from = zprg.from;
			q->to = p->to;
			q->cond = p->cond;
			q->link = p->link;
			q->mark |= BRANCH;
			p->link = q;

			q = prg();
			q->line = p->line;
			q->as = AADD;
			q->from.type = D_CONST;
			q->from.offset = autosize;
			q->to.type = D_REG;
			q->to.reg = REGSP;
			q->link = p->link;
			p->link = q;

			p->as = AMOVW;
			p->from = zprg.from;
			p->from.type = D_OREG;
			p->from.offset = 0;
			p->from.reg = REGSP;
			p->to = zprg.to;
			p->to.type = D_REG;
			p->to.reg = REGLINK;

			break;
		}
	}
	if (Mips24k)
		storesnosched();

	curtext = P;
	q = P;		/* p - 1 */
	q1 = firstp;	/* top of block */
	o = 0;		/* count of instructions */
	for(p = firstp; p != P; p = p1) {
		p1 = p->link;
		o++;
		if(p->mark & NOSCHED){
			if(q1 != p){
				sched(q1, q);
			}
			for(; p != P; p = p->link){
				if(!(p->mark & NOSCHED))
					break;
				q = p;
			}
			p1 = p;
			q1 = p;
			o = 0;
			continue;
		}
		if(p->mark & (LABEL|SYNC)) {
			if(q1 != p)
				sched(q1, q);
			q1 = p;
			o = 1;
		}
		if(p->mark & (BRANCH|SYNC)) {
			sched(q1, p);
			q1 = p1;
			o = 0;
		}
		if(o >= NSCHED) {
			sched(q1, p);
			q1 = p1;
			o = 0;
		}
		q = p;
	}

	if (Mips24k)
		triplestorenops();
}

void
addnop(Prog *p)
{
	Prog *q;

	q = prg();
	q->as = ANOR;
	q->line = p->line;
	q->from.type = D_REG;
	q->from.reg = REGZERO;
	q->to.type = D_REG;
	q->to.reg = REGZERO;

	q->link = p->link;
	p->link = q;
}

void
nocache(Prog *p)
{
	p->optab = 0;
	p->from.class = 0;
	p->to.class = 0;
}
