#include "common.h"
#include "code.h"
#include "sym.h"
#include "machine.h"
#include "mem.h"

int gen_table2;		/* generate type two tables */
struct machine_node *machine = NULL;
static int depth;
static void	machineGen2(struct machine_node *);
static int	MachineNumber(struct machine_node *, int);

struct _mem match_mem, mach_mem;

void
MachInit(void)
{
	mem_init(&mach_mem, sizeof(struct machine_node));
	mem_init(&match_mem, sizeof(struct match));
}

static struct machine_node *
get_state(void)
{
	struct machine_node *mp = mem_get(&mach_mem);
	assert (mp!=NULL);
	mp->inp.value = -1;
	mp->nst = NULL;
	mp->link = NULL;
	mp->fail = NULL;
	mp->match = NULL;
	return(mp);
}

void
add_match(struct machine_node *s, LabelData *tp, int depth)
{
	struct match *mp = mem_get(&match_mem);

	assert (mp!=NULL);
	mp->next = s->match;
	mp->depth = depth;
	mp->tree = tp;
	s->match = mp;
}
	
/*
 * Build a trie from the path strings.  Failure transition are added later.
 */
void
cgotofn(LabelData *tp)
{
	struct machine_input inp;
	int ret;
	register struct machine_node *s;

	if (machine==NULL) {
		/* first time thru */
		machine = get_state();
	}
	s = machine;
	depth = -1;

	assert (tp->tree!=NULL);
	path_setup (tp->tree);

	for(;;) {
		ret = path_getsym(&inp);

		/* no more paths */
		if (ret == EOF)
			break;

		if (ret == NULL) {
			/*
			 * the end of the path.  Add a match to the
			 * accepting state.
			 */
			assert ((depth&01)==0); /* make sure it's even */
			add_match(s, tp, depth >> 1);
			s = machine;
			depth = -1;
			ntrees++;
			continue;
		}

		while (!MI_EQUAL(s->inp, inp)) {
			if (!MI_DEFINED(s->inp.value) || s->link == NULL) {
				if (MI_DEFINED(s->inp.value)) {
					/*
					 * The trie must be split here
					 */
					s->link = get_state();
					s = s->link;
				}
				/*
				 * Fill in the state
				 */
				s->inp = inp;
				s->nst = get_state();
				s = s->nst;
				depth++;
				break;
			} else s = s->link;
		}
		if (MI_EQUAL(s->inp, inp)) {
			s = s->nst;
			depth++;
		}
	}
}

/* print out the machine for debugging */
void
machine_print(struct machine_node *mp)
{
	register struct machine_node *mp2;
	struct machine_node *fail;
	struct match *match, *p;

	if (mp==NULL)
		return;
	printf("%x %d:", mp, mp->index);
	if((fail = mp->fail))
		printf("\tfail %x", fail);
	if((match = mp->match)) {
		printf("\taccept ");
		for(p = match; p!=NULL; p = p->next) {
			LabelData *tp = p->tree;
			printf("%s ", (tp->label)->name);
		}
	}
	putchar('\n');
	for(mp2=mp; mp2!=NULL; mp2 = mp2->link) {
		if (MI_DEFINED(mp2->inp.value)) {
			if (MI_BRANCH(mp->inp.value))
				printf(" %d -> ", mp2->inp.value);
			else
				printf(" [%s] -> ", (mp2->inp.sym)->name);
			printf("%x", mp2->nst);
		}
		assert(mp2->fail==fail);
		assert(mp2->match==match);
	}
	if(MI_DEFINED(mp->inp.value))
		putchar('\n');
	for (mp2 = mp; mp2!=NULL; mp2=mp2->link) {
		machine_print(mp2->nst);
	}
}

/*
 * This routine augments the machine with failure transition.
 * The trie is examined in breadth first order.
 *	See Aho, Corasick Comm ACM 18:6 for details
 */
void
cfail(void)
{
	struct machine_node **stack, **stack2;
	struct machine_node **stackTop, **stack2Top, **temp;
	int stackCnt, stack2Cnt;
	struct machine_node *state;
	struct machine_input sym;
	register struct machine_node *s, *q;

	/* allocate stacks */
	stackTop = stack = (struct machine_node **) Malloc
				(ntrees*sizeof (struct machine_node *));
	stack2 = stack2Top = (struct machine_node **) Malloc
				(ntrees*sizeof (struct machine_node *));
	stackCnt = stack2Cnt = 0;
	s = machine;
	do {
		if (MI_DEFINED(s->inp.value)) {
			assert(++stackCnt <= ntrees);
			*stackTop++ = s->nst;
		}
		s = s->link;
	} while (s != NULL);

	for(;;) {
		if (stackCnt == 0) {
			/* swap stacks */
			if (stack2Cnt == 0)
				break;
			stackCnt = stack2Cnt; stack2Cnt = 0;
			stackTop = stack2Top;
			temp = stack; stack = stack2; stack2 = temp;
			stack2Top = stack2;
		}
		stackCnt--;
		s = *--stackTop;
		do {
			int startstate = 0;
			sym = s->inp;
			if (MI_DEFINED(sym.value)) {	/* g(s,c) != 0 */
			    assert (++stack2Cnt <= ntrees);
			    *stack2Top++ = (q=s->nst);
			    state = s->fail;	/* state = f(s) */
			    for(;;) {
				if (state == 0)	{
					state = machine;
					startstate++;
				}
				if (MI_EQUAL(state->inp, sym)) {
				    struct match *mp;

				    /* append any accepting matches */
				    for(mp=state->nst->match;mp;mp=mp->next)
					add_match(q,mp->tree, mp->depth);
				    mp = q->match;

				    do {
					/* f(q) = g(state,sym)
					 * for all q links */
					q->fail = state->nst;
					q->match = mp;
				    } while ((q = q->link) != 0);
				    break;
				}
				else if (state->link != 0)
				    state = state->link;
				else {
					state = state->fail;
					if (state==NULL&&startstate)
						break;
				}
			    }
			}
			s = s->link;
		} while (s!=NULL);
	}
}

/*
 * Called from the YACC rule pattern_spec
 */
void
machine_build(void)
{
	cfail();
	MachineNumber(machine, 0);
	if(debug_flag&DB_MACH) {
		fputs("\n-------\n", stderr);
		machine_print(machine);
	}
}

#define MAXACCEPTS 300
struct match *acceptTable[MAXACCEPTS];
struct match **acceptTableTop = acceptTable;
static short int *arityTable;
int nextAcceptIndex = 0;

/*
 * This is the first pass of the process to generate the
 * flattened machine used in the walker.  Called from machine_build
 */
static int
MachineNumber(struct machine_node *mach, int index)
{
	struct machine_node *mp;

	for(mp=mach; mp!=NULL; mp = mp->link)
		if(MI_DEFINED(mp->inp.value))
			index = MachineNumber(mp->nst, index);

	mach->index = index;
	if (mach->match!=NULL) index += 2;
	if(mach->link!=NULL || mach->nst!=NULL) {
		index += 2;
		for(mp = mach; mp!=NULL; mp = mp->link)
			if(mp->nst!=NULL) 
				index += 2;
	}

	if (mach->fail!=NULL)
		index += 2;
	index++;
	return(index);
}

/*
 * Build the flattened out machine used in the walker.
 * See machine.h for description
 */
void
MachineGen(void)
{
	struct match **atp, *ap;

	oreset();
	fputs("short mtTable[] = {\n", outfile);
	machineGen2(machine);
	fputs("};\n", outfile);

	fputs("short mtAccept[] = {\n", outfile);

	oreset();
	for(atp = acceptTable; atp < acceptTableTop; atp++) {
		for(ap = *atp; ap!=NULL; ap = ap->next) {
			register LabelData *tree = ap->tree;

			/* if you change any of the code below you must change
			 * the increment added to nextAcceptIndex in
			 * machineGen2
			 */
			oputint(tree->treeIndex);
			oputint(ap->depth);
		}
		oputint(-1);
	}
	fprintf(outfile, "};\nshort mtStart = %d;\n", machine->index);
}

static void
machineGen2(struct machine_node *mach)
{
	struct machine_node *mp;
	struct match *ap;

	if (mach==NULL)
		return;

	for(mp=mach; mp!=0; mp = mp->link)
		if(MI_DEFINED(mp->inp.value))
			machineGen2(mp->nst);

	assert(mach->index == ointcnt());
	if (mach->match!=NULL) {
		if(acceptTableTop >= &acceptTable[MAXACCEPTS])
			error(1, "out of acceptTable slots");
		*acceptTableTop++ = mach->match;
		oputint(ACCEPT);
		oputint(nextAcceptIndex);
		for(ap = mach->match; ap!=NULL; ap = ap->next)
			nextAcceptIndex += 2;
		nextAcceptIndex++;
	}
	if(mach->link!=NULL || mach->nst!=NULL) {
		if(MI_BRANCH(mach->inp.value)&&gen_table2) {
			oputint(TABLE2);
			for(mp = mach; mp!=NULL; mp = mp->link) {
				if(mp->nst!=NULL) {
					oputoct(mp->inp.value);
					oputint((mp->nst)->index);
				}
			}
		} else {
			oputint(TABLE);
			for(mp = mach; mp!=NULL; mp = mp->link) {
				if(mp->nst!=NULL) {
					oputoct(mp->inp.value);
					oputint((mp->nst)->index);
				}
			}
		}
		oputint(-1);
	}

	/* A failure transition or a -1 terminate every state */
	if (mach->fail!=NULL) {
		oputint(FAIL);
		oputint((mach->fail)->index);
	}
	oputint(-1);
}
