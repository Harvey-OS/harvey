/*
 * See Aho, Corasick Comm ACM 18:6, and Hoffman and O'Donell JACM 29:1
 * for detail of the matching algorithm
 */

/* turn this flag on if your machine has a fast byte string zero operation */
/*#define BZERO	1*/
int mtDebug = 0;
int treedebug = 0;
extern int _machstep();
extern short int mtStart;
extern short int mtTable[], mtAccept[], mtMap[], mtPaths[], mtPathStart[];
#ifdef LINE_XREF
	extern short int mtLine[];
#endif
extern NODEPTR mtGetNodes(), mtAction();
extern skeleton *_allocskel();
extern __match *_allocmatch();
extern __partial *_allocpartial();

#define MPBLOCKSIZ 10000
__match *_mpblock[MPBLOCKSIZ], **_mpbtop;

/*
 * See sym.h in the preprocessor for details
 * Basically used to support eh $%n$ construct.
 */
__match **
_getleaves (mp, skp)
	register __match *mp; register skeleton *skp;
{
	skeleton *stack[MAXDEPTH];
	skeleton **stp = stack;
	register short int *sip = &mtPaths[mtPathStart[mp->tree]];
	register __match **mmp = _mpbtop;
	__match **mmp2 = mmp;
	if((_mpbtop += *sip++ + 1) > &_mpblock[MPBLOCKSIZ]) {
		printf ("ich: %d\n", _mpbtop-_mpblock);
		assert(0);
	}

	for(;;)
		switch(*sip++) {
		case ePUSH:
			*stp++ = skp;
			skp = skp->leftchild;
			break;

		case eNEXT:
			skp = skp->sibling;
			break;

		case eEVAL:
			if ((mp = skp->succ[M_DETAG(*sip++)])==NULL) abort();
			*mmp++ = mp;
			break;

		case ePOP:
			skp = *--stp;
			break;

		case eSTOP:
			*mmp = NULL;
			return (mmp2);
		}
}

_evalleaves (mpp)
	register __match **mpp;
{
	for (; *mpp!=NULL; mpp++) {
		register __match *mp = *mpp;
		_do (mp->skel, mp, 1);
	}
}


_do(sp, winner, evalall)
	skeleton *sp; register __match *winner; int evalall;
{
	register __match **mpp;
	register skeleton *skel = winner->skel;
	if(winner==NULL) {
		_prskel(sp, 0);
		puts ("no winner");
		return;
	}
	if(winner->mode==xDEFER || (evalall && winner->mode!=xTOPDOWN))
		REORDER (winner->lleaves);
	mtSetNodes (skel->parent, skel->nson,
		skel->root=mtAction (winner->tree, winner->lleaves, sp));
}

skeleton *
_walk(sp, ostate)
	register skeleton *sp;
	int ostate;
{
	int state, nstate, nson;
	register __partial *pp;
	register __match *mp, *mp2;
	register skeleton *nsp, *lastchild = NULL;
	NODEPTR son, root;

	root = sp->root;
	nson = 1; sp->mincost = INFINITY;
	state = _machstep (ostate, root, mtValue(root));

	while((son = mtGetNodes(root, nson))!=NULL) {
		nstate = _machstep (state, NULL, MV_BRANCH(nson));
		nsp = _allocskel();
		nsp->root = son;
		nsp->parent = root;
		nsp->nson = nson;
		_walk(nsp, nstate);
		if(COSTLESS(nsp->mincost, INFINITY)) {
			assert (nsp->winner->mode==xREWRITE);
			if (mtDebug || treedebug) {
				printf ("rewrite\n");
				_prskel (nsp, 0);
			}
			_do(nsp, nsp->winner, 0);
			_freeskel(nsp);
			continue;
		}
		_merge (sp, nsp);
		if (lastchild==NULL) sp->leftchild = nsp;
		else lastchild->sibling = nsp;
		lastchild = nsp;
		nson++;
	}

	for (pp = sp->partial; pp < &sp->partial[sp->treecnt]; pp++)
		if (pp->bits&01==1) {
			mp = _allocmatch();
			mp->tree = pp->treeno;
			_addmatches (ostate, sp, mp);
		}
	if(son==NULL && nson==1)
		_closure (state, ostate, sp);

	sp->rightchild = lastchild;
	if (root==NULL) {
		COST c; __match *win; int i; nsp = sp->leftchild;
		c = INFINITY;
		win = NULL;
		for (i = 0; i < MAXLABELS; i++) {
			mp = nsp->succ[i];
			if (mp!=NULL && COSTLESS (mp->cost, c))
				{ c = mp->cost; win = mp; }
		}
		if (mtDebug || treedebug)
			_prskel (nsp, 0);
		_do (nsp, win, 0);
	}
	if (mtDebug)
		_prskel (sp, 0);
	return(sp);
}

static short int _nodetab[MAXNDVAL], _labeltab[MAXLABELS];

/*
 * Convert the start state which has a large branching factor into
 * a index table.  This must be called before the matcher is used.
 */
_matchinit()
{
	short int *sp;
	struct pair { short int val, where } *pp;
	for (sp=_nodetab; sp < &_nodetab[MAXNDVAL]; sp++) *sp = HANG;
	for (sp=_labeltab; sp < &_labeltab[MAXLABELS]; sp++) *sp = HANG;
	sp = &mtTable[mtStart];
	assert (*sp==TABLE);
	for (pp = (struct pair *) ++sp; pp->val!=EOF; pp++) {
		if (MI_NODE(pp->val))
			_nodetab[M_DETAG(pp->val)] = pp->where;
		else if (MI_LABEL(pp->val))
			_labeltab[M_DETAG(pp->val)] = pp->where;
	}
}

int
_machstep(state, root, input)
	short int state, input;
	NODEPTR root;
{
	register short int *stp = &mtTable[state];
	int start = 0;
	if (state==HANG)
		return (input==(MV_BRANCH(1)) ? mtStart : HANG);
rescan:
	if (stp==&mtTable[mtStart]) {
		if (MI_NODE(input)) return(_nodetab[M_DETAG(input)]);
		if (MI_LABEL(input)) return(_labeltab[M_DETAG(input)]);
	}
	
	for(;;) {
		if(*stp==ACCEPT) stp += 2;

		if(*stp==TABLE) {
			stp++;
			while(*stp!=EOT) {
				if(input==*stp) {
					return(*++stp);
				}
				stp += 2;
			}
			stp++;
		}
		if(*stp!=FAIL) {
			if (start) return (HANG);
			else { stp = &mtTable[mtStart]; start = 1;  goto rescan;}
		} else {
			stp++;
			stp = &mtTable[*stp];
			goto rescan;
		}
	}
}

_addmatches(ostate, sp, np)
	int ostate;
	register skeleton *sp;
	register __match *np;
{
	int label;
	int state, qstate;
	register __match *mp;
	static short int quick[128][MAXLABELS];
	static short int qtag[128][MAXLABELS];

        label = mtMap[np->tree];

	/*
	 * The following lines replace the line:
	 *
	 *	state = _machstep(ostate, NULL, MV_LABEL(label));
	 *
	 * Statistics show that a large percentage (approx 2/3) of calls to
	 * _machstep occur in this function.  The lines below attempt
	 * to reduce the number of these calls by using an unsophisticated
	 * but apparently adequate caching scheme.
	 */
	qstate = ((ostate>>2)+2)&0177;
	if(ostate != qtag[qstate][label]) {
		state = _machstep(ostate, NULL, MV_LABEL(label));
		quick[qstate][label] = state;
		qtag[qstate][label] = ostate;
	} else state = quick[qstate][label];

	/*
	 * this is a very poor substitute for good design of the DFA.
	 * What we need is a special case that allows any label to be accepted
	 * by the start state but we don't want the start state to recognize
	 * them after a failure.
	 */
	if (ostate!=mtStart  && state==HANG) {
		_freematch(np);
		return;
	}

	np->lleaves = _getleaves (np, sp);
	np->skel = sp;
        if((np->mode = mtEvalCost(np, np->lleaves, sp))==xABORT)
	{
		_freematch(np);
		return;
	}

	if ((mp = sp->succ[label])!=NULL) {
		if (!COSTLESS (np->cost, mp->cost))
			{ _freematch(np); return; }
		else _freematch(mp);
	}
	if(COSTLESS(np->cost,sp->mincost)) {
		if(np->mode==xREWRITE) {
			sp->mincost = np->cost; sp->winner = np; }
		else { sp->mincost = INFINITY; sp->winner = NULL; }
	}
	sp->succ[label] = np;
	_closure(state, ostate, sp);
}

_closure(state, ostate, skp)
	int state, ostate;
	skeleton *skp;
{
	register struct ap { short int tree, depth; } *ap;
	short int *sp = &mtTable[state];
	register __match *mp;

	if(state==HANG || *sp!=ACCEPT) return(NULL);

	for(ap = (struct ap *) &mtAccept[*++sp]; ap->tree!=-1; ap++)
		if (ap->depth==0) {
			mp = _allocmatch();	
			mp->tree = ap->tree;
			_addmatches (ostate, skp, mp);
		} else {
			register __partial *pp, *lim = &skp->partial[skp->treecnt];
			for(pp=skp->partial; pp < lim; pp++)
				if(pp->treeno==ap->tree)
					break;
			if(pp==lim) {
				skp->treecnt++;
				pp->treeno = ap->tree;
				pp->bits = (1<<(ap->depth));
			} else pp->bits |= (1<<(ap->depth));
		}
}

_merge (old, new)
	skeleton *old, *new;
{
	register __partial *op = old->partial, *np = new->partial;
	int nson = new->nson;
	register __partial *lim = np + new->treecnt;
	if (nson==1) {
		old->treecnt = new->treecnt;
		for(; np < lim; op++, np++) {
			op->treeno = np->treeno;
			op->bits = np->bits/2;
		}
	} else {
		__partial *newer = _allocpartial();
		register __partial *newerp = newer;
		register int cnt;
		lim = op+old->treecnt;
		for(cnt=new->treecnt; cnt-- ; np++) {
			for(op = old->partial; op < lim; op++)
				if (op->treeno == np->treeno) {
					newerp->treeno = op->treeno;
					newerp++->bits = op->bits & (np->bits/2);
					break;
				}
		}
		_freepartial(old->partial);
		old->partial = newer;
		old->treecnt = newerp-newer;
	}
}
 
/* Save and Allocate Matches */
#define BLKF	100
__match *freep = NULL;
static int _count = 0;
static __match *_blockp = NULL;
#ifdef CHECKMEM
int x_matches, f_matches;
#endif

__match *
_allocmatch()
{
	__match *mp;

	if(freep!=NULL) {
		mp = freep;
		freep = ((__match *)((struct freeb *) freep)->next);
#ifdef CHECKMEM
		f_matches--;
#endif
	} else {
		if(_count==0) {
			_blockp = (__match *) malloc (BLKF*sizeof (__match));
			_count = BLKF;
#ifdef CHECKMEM
			x_matches += BLKF;
#endif
		}
		mp = _blockp++;
		_count--;
	}
	return(mp);
}

_freematch(mp)
	__match *mp;
{
	((struct freeb *) mp)->next = (struct freeb *) freep;
	freep = mp;
#ifdef CHECKMEM
	f_matches++;
#endif
}

static __partial *pfreep = NULL;
static int pcount = 0;
static __partial *pblock = NULL;
#ifdef CHECKMEM
static int x_partials, f_partials;
#endif

__partial *
_allocpartial()
{
	__partial *pp;
	if (pfreep!=NULL) {
		pp = pfreep;
		pfreep = *(__partial **) pp;
#ifdef CHECKMEM
		f_partials--;
#endif
	} else {
		if (pcount==0) {
			pblock=(__partial *)malloc(BLKF*MAXTREES*sizeof(__partial));
			pcount = BLKF;
#ifdef CHECKMEM
			x_partials += BLKF;
#endif
		}
		pp = pblock;
		pblock += MAXTREES;
		pcount--;
	}
	return(pp);
}

_freepartial(pp)
	__partial *pp;
{
	* (__partial **)pp = pfreep;
	pfreep = pp;
#ifdef CHECKMEM
	f_partials++;
#endif
}


/* storage management */
static skeleton *sfreep = NULL;
static int scount = 0;
static skeleton * sblock = NULL;

skeleton *
_allocskel()
{
	skeleton *sp;
	int i;
	if(sfreep!=NULL) {
		sp = sfreep;
		sfreep = sp->sibling;
	} else {
		if(scount==0) {
			sblock = (skeleton *) malloc (BLKF * sizeof (skeleton));
			scount = BLKF;
		}
		sp = sblock++;
		scount--;
	}
	sp->sibling = NULL; sp->leftchild = NULL;
	for (i=0; i < MAXLABELS; sp->succ[i++] = NULL);
	sp->treecnt = 0;
	sp->partial = _allocpartial();
	return(sp);
}

_freeskel(sp)
	skeleton *sp;
{
	int i;
	__match *mp;
	if(sp==NULL)
		return;
	if(sp->leftchild)
		_freeskel(sp->leftchild);
	if(sp->sibling)
		_freeskel(sp->sibling);
	_freepartial (sp->partial);
	for (i=0; i < MAXLABELS; i++)
		if ((mp = sp->succ[i])!=NULL) _freematch (mp);
	sp->sibling = sfreep;
	sfreep = sp;
}

_match()
{
	skeleton *sp;
	sp = _allocskel();
	sp->root = NULL;
	_mpbtop = _mpblock;
	_freeskel(_walk(sp, HANG));
#ifdef CHECKMEM
	if(f_matches+_count!=x_matches) {
		printf(";#m %d %d %d\n", f_matches, _count, x_matches);
		assert(0);
	}
	if(f_partials+pcount!=x_partials) {
		printf(";#p %d %d %d\n", f_partials, pcount, x_partials);
		assert(0);
	}
#endif
}

NODEPTR
_mtG(root,i)
	NODEPTR root;
	int i;
{
	int *p = &i;
	while(*p!=-1)
		root = mtGetNodes(root, *p++);
	return(root);
}

/* diagnostic routines */

_prskel (skp, lvl)
	skeleton *skp; int lvl;
{
	int i; __match *mp;
	__partial *pp;
	if(skp==NULL) return;
	for (i = lvl; i > 0; i--) { putchar (' '); putchar (' '); }
	printf ("###\n");
	for (i = lvl; i > 0; i--) { putchar (' '); putchar (' '); }
	for (i = 0; i < MAXLABELS; i++)
		if ((mp=skp->succ[i])!=NULL)
#ifdef LINE_XREF
			printf ("[%d<%d> %d]", mp->tree,
				mtLine[mp->tree], mp->cost);
#else
			printf ("[%d %d]", mp->tree, mp->cost);
#endif
	putchar('\n');
	for (i = lvl; i > 0; i--) { putchar (' '); putchar (' '); }
	for (i = 0, pp=skp->partial; i < skp->treecnt; i++, pp++)
#ifdef LINE_XREF
			printf("(%d<%d> %x)", pp->treeno, mtLine[pp->treeno],
				pp->bits);
#else
			printf ("(%d %x)", pp->treeno, pp->bits);
#endif
	putchar('\n');
	fflush(stdout);
	_prskel (skp->leftchild, lvl+2);
	_prskel (skp->sibling, lvl);
}
