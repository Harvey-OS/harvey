#include	"l.h"

void
noops(void)
{
	Prog *p, *q, *q1;

	if(debug['v'])
		Bprint(&bso, "%5.2f noops\n", cputime());
	Bflush(&bso);
	q = P;
	for(p = firstp; p != P; p = p->link) {
		switch(p->as) {
		case ATEXT:
			p->mark |= LEAF;
/*p->mark = 0;/**/
			curtext = p;
			break;

		case ABAL:
			curtext->mark = 0;
			break;

		case ANOP:
			q = q;
			continue;
			q1 = p->link;
			q->link = q1;		/* q is non-nop */
			q1->mark |= p->mark;
			continue;
		}
		q = p;
	}

	curtext = P;
	for(p = firstp; p != P; p = p->link) {
		switch(p->as) {
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
				q->as = ASUBO;
				q->line = p->line;
				q->from.type = D_CONST;
				q->from.offset = autosize;
				q->to.type = REGSP;

				q->link = p->link;
				p->link = q;
			} else
			if(!(curtext->mark & LEAF)) {
				if(debug['v'])
					Bprint(&bso, "save suppressed in: %s\n",
						curtext->from.sym->name);
				curtext->mark |= LEAF;
			}

			if(curtext->mark & LEAF) {
				if(curtext->from.sym)
					curtext->from.sym->type = SLEAF;
				break;
			}

			q1 = prg();
			q1->as = AMOV;
			q1->line = p->line;
			q1->from.type = REGLINK;
			q1->to.type = REGSP+D_INDIR;

			q1->link = q->link;
			q->link = q1;
			break;

		case ARTS:
			if(curtext->mark & LEAF) {
				if(!autosize) {
					p->as = AB;
					p->from = zprg.from;
					p->to.type = REGLINK+D_INDIR;
					p->to.offset = 0;
					break;
				}

				p->as = AADDO;
				p->from.type = D_CONST;
				p->from.offset = autosize;
				p->to.type = REGSP;

				q = prg();
				q->as = AB;
				q->line = p->line;
				q->to.type = REGLINK+D_INDIR;
				q->to.offset = 0;

				q->link = p->link;
				p->link = q;
				break;
			}

			p->as = AMOV;
			p->from.type = REGSP+D_INDIR;
			p->from.offset = 0;
			p->to.type = REGRET+1;

			q = p;
			if(autosize) {
				q = prg();
				q->as = AADDO;
				q->line = p->line;
				q->from.type = D_CONST;
				q->from.offset = autosize;
				q->to.type = REGSP;

				q->link = p->link;
				p->link = q;
			}

			q1 = prg();
			q1->as = AB;
			q1->line = p->line;
			q1->to.type = REGRET+1+D_INDIR;
			q1->to.offset = 0;

			q1->link = q->link;
			q->link = q1;
			break;
		}
	}
}
