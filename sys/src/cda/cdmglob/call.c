#include "cdmglob.h"

#define NCC	128

extern Macro	*mcur;
extern char	*filename;
extern int	lineno, lsl_flag;



/*
 * link actual paramters to formal paramters.
 * "ss" are the actuals.
 * the formals are either a nonzero pin number "num",
 * or a pin name pattern "form".
 */

int
callargv (Call **cc, int ncc, Sig **ss, int nss, int num, char *form)
{
	Macro	*m = cc[0]->c_type;
	Pin	*PP[1], *p, **pp;
	int	argno, formno, i, j, k, nforms, npp;
	Sig	*act, **pact;

#ifdef jhc
	int	cn, msn, pn, psn, sn;
#endif

	if (num) {
		if (!(p = pinumsearch (m, num))) {
			error ("%s: %d: no such pin #", m->m_name, num);
			return (1);
		}
		/*
		 * when both a pin name and number are given,
		 * make sure they are consistent.
		 */
		if (*form &&
		    (1 != (npp = pinmatch (m, form, &pp) ||
		    pp[0]->p_num != num))) {
			error ("%d,%s: ambiguous connection", num, form);
			return (1);
		}
		*(pp = PP) = p;
		npp = 1;
	}
	else	npp = pinmatch (m, form, &pp);

	for (i = nforms = 0; i < npp; nforms += pp[i++]->p_nsigs);

	if (callcount (ncc, nss, nforms))
		return (1);

#ifdef jhc
	for (sn = cn = 0; cn < ncc; cn++) {
		for (pn = 0; pn < npp; pn++) {
			msn = pp[pn]->p_sigs - m->m_argv;
			for (psn = 0; psn < pp[pn]->p_nsigs; psn++, msn++) {
				if (sn >= nss)
					sn = 0;
				act = ss[sn++];
				pact = cc[cn]->c_argv + msn;
				if (!*pact) {
					*pact = act;
					continue;
				}
				if (*pact == act)	/* for dave */
					warn ("%s: identically reconnected",
					    pp[pn]->p_name);
				else {
					error (
					    "%s: multiply connected (%s & %s)",
					    pp[pn]->p_name,
					    pact[0]->s_name, act->s_name);
					return (1);
				}
			}
		}
	}
#else
	for (formno = i = 0; i < npp; i++) {
		argno = pp[i]->p_sigs - m->m_argv;
		for (j = 0; j < pp[i]->p_nsigs; j++, argno++, formno++)
			for (k = 0; k < ncc; k++) {
				act = ss[nss == nforms ? formno : k];
				pact = cc[k]->c_argv + argno;
				if (!*pact) {
					*pact = act;
					continue;
				}
				if (*pact == act)	/* for dave */
					warn ("%s: identically reconnected",
					    pp[i]->p_name);
				else {
					error (
					    "%s: multiply connected (%s & %s)",
					    pp[i]->p_name,
					    pact[0]->s_name, act->s_name);
					return (1);
				}
			}
	}
#endif

	return (0);
}


/*
 * return nonzero iff there's a signal connected to each pin.
 */

int
callcheck (Call *c)
{
	Macro	*m = c->c_type;
	Pin	*p = m->m_pins;
	int	i, n = m->m_npins, ok = 1;

	for (i = 0; n > 0; n--, i += p++->p_nsigs)
		if (!c->c_argv[i]) {
#ifdef jhc
			extern Sig	unconnected;
			if(m->m_def) {
				filename = c->c_file;	/* fake out error0() */
				lineno = c->c_lineno;
				ok = 0;
				error ("%s (%s): %s: unconnected pin",
			    	    c->c_name, m->m_name, p->p_name);
			} else
				c->c_argv[i] = &unconnected;

#else
			filename = c->c_file;	/* fake out error0() */
			lineno = c->c_lineno;
			ok = 0;
			error ("%s (%s): %s: unconnected pin",
			    c->c_name, m->m_name, p->p_name);
#endif
		}
	return (ok);
}


int
callcount (int ncalls, int nacts, int nforms)
{
#ifdef jhc
	int	n = ncalls * nforms;
#endif
	if (ncalls <= 0 || nacts <= 0 || nforms <= 0)
		return (1);
#ifdef jhc
	if (n < nacts || n % nacts ||
	    nforms > nacts && nforms % nacts ||
	    nacts > nforms && nacts % nforms)
#else
	if (nacts == nforms)
		return (0);
	if (ncalls == 1)
		if (nacts > 1)
			error ("arg count mismatch");
		else	return (0);
	else	if (nforms > 1 || nacts > 1 && nacts != ncalls)
#endif
			error ("arg/macro count mismatch");
		else	return (0);
	return (1);
}


int
callexpand (Macro *m, Call ***ccc, char	*name)
{
	static Call	*cc[NCC];

	char	**names;
	int	i, ncc;

	if (0 >= (ncc = expand (name, &names, 1)))
		return (0);
	if (ncc > NCC)
		fatal ("too many instances");

	for (*ccc = cc, i = 0; i < ncc; i++)
		cc[i] = callookup (m, names[i]);

	return (ncc);
}

void
callinline (Call *c)
{
	register Macro	*m = c->c_type;
	register Sig	*sigs;
	register char	*base;
	register int	i;
	Call		C;

	pushmem();

	if (*(base = C.c_name = c->c_name))
		(base += 1 + strlen (base))[-1] = '/';

	alloc (sigs, m->m_nsigs, Sig);
	bcopy ((char *) sigs, (char *) m->m_sigs, m->m_nsigs * sizeof (Sig));

	for (i = 0; i < m->m_argc; i++)
		sigs[m->m_argv[i] - m->m_sigs].s_name = c->c_argv[i]->s_name;

	for (i = 0; i < m->m_nsigs; i++)
		if (!(m->m_sigs[i].s_type & (S_GLOB | S_IN | S_OUT))) {
			(void) strcpy (base, m->m_sigs[i].s_name);
			sigs[i].s_name = strsave (C.c_name);
		}

	for (c = m->m_calls; c; c = c->c_next) {
		C.c_type = c->c_type;
		(void) strcpy (base, c->c_name);
		alloc (C.c_argv, C.c_argc = c->c_argc, Sig *);
		for (i = 0; i < c->c_argc; i++) {
#ifdef jhc
			extern Sig	unconnected;

			if (c->c_argv[i] == &unconnected)
				C.c_argv[i] = &unconnected;
			else
#endif
			C.c_argv[i] = sigs + (c->c_argv[i] - m->m_sigs);
		}
		if (c->c_type->m_def)
			callinline (&C);
		else	if (lsl_flag)
				lslcall (&C);
			else	wcall (&C);
	}

	if (base > C.c_name)
		base[-1] = 0;
	popmem();
	return;
}


Call *
callookup (Macro *m, char *cname)
{
	Call	*c;
	int	H, n;
	Sig	**v;

	H = hash (cname, HASH);

	for (c = mcur->m_chash[H]; c; c = c->c_hash)
		if (Strsame (cname, c->c_name))
			goto found;

	alloc (c, 1, Call);
	c->c_name = strsave (cname);
	c->c_type = m;
	c->c_next = mcur->m_calls;
	mcur->m_calls = c;
	v = alloc (c->c_argv, n = c->c_argc = m->m_argc, Sig *);
	for (; n > 0; n--, *v++ = 0);
	c->c_hash = mcur->m_chash[H];
	mcur->m_chash[H] = c;

found:
	c->c_file = filename;		/* for diagnostics */
	c->c_lineno = lineno;
	return (c);
}


void
callrelookup (Macro *m, Call *c)
{
	Call	*c2;
	int	H;

	H = hash (c->c_name, HASH);

	for (c2 = m->m_chash[H]; c2; c2 = c2->c_hash)
		if (Strsame (c->c_name, c2->c_name)) {
			filename = c->c_file;	/* fake out error0() */
			lineno = c->c_lineno;
			error ("%s: instance name repeated", c->c_name);
			return;
		}

	c->c_hash = m->m_chash[H];
	m->m_chash[H] = c;
	return;
}
