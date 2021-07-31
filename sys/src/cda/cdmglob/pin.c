#include <ctype.h>
#include "cdmglob.h"

#define NPINS	8192
#define NPP	64



static Pin	pins[NPINS];
static int	npins;


/*
 * since p_hash is an index into m_pins, pinamsearch() can't
 * be used after macargv() which rearranges m_pins.
 */

Pin *
pinamsearch (Macro *m, char *pname)
{
	register Pin	*p;
	register int	h, H;

	H = hash (pname, HASH);

	for (h = m->m_phash[H]; h >= 0; h = p->p_hash) {
		p = m->m_pins + h;
		if (Strsame (pname, p->p_name))
			return (p);
	}
	return (0);
}


/*
 * [re]define pin.
 * all pins must be named.
 * numbers are optional, and needn't be given each call.
 * directions and signal lists must be given identically each call.
 * multiple signals per pin permitted, but not multiple pins per signal.
 */

void
pinlookup (Macro *m, char *pname, Sig **s, int  ns, int num, int dir)
{
	Sig	**t;
	Pin	*p;
	int	h;

	if (m->m_use) {
		error ("%s: used before defined", m->m_name);
		return;
	}

	if (p = pinamsearch (m, pname)) {
		if (num)
			if (!p->p_num)
				p->p_num = num;
			else	if (num != p->p_num)
					goto redef;
		if (ns != p->p_nsigs)
			goto redef;
		for (t = p->p_sigs; ns > 0; ns--, s++, t++)
			if (*s != *t)
				goto redef;
		return;
	}
	if (num && pinumsearch (m, num))
		goto redef;

	if (npins >= NPINS)
		fatal ("too many pins");
	p = pins + npins;
	p->p_sigs = s;
	p->p_nsigs = ns;

	for (; ns > 0; ns--, s++)
		if ((*s)->s_type & (S_IN | S_OUT)) {
			error ("%s: signal already on pin", (*s)->s_name);
			return;
		}
		else if ((*s)->s_type & S_GLOB) {
			error ("%s: global signal on pin", (*s)->s_name);
			return;
		}
		else (*s)->s_type |= dir;
	npins++;
	if (!m->m_pins)
		m->m_pins = p;
	if (p != m->m_pins + m->m_npins++)
		fatal ("pinlookup: get help");
	p->p_name = strsave (pname);
	p->p_dir = dir;
	h = hash (pname, HASH);
	p->p_hash = m->m_phash[h];
	m->m_phash[h] = p - m->m_pins;

	p->p_num = num;

	return;

redef:
	error ("%s: %s: pin redefined", m->m_name, pname);
	return;
}


/*
 * try to match the pin name pattern "pname"
 * against the pin names known to macro "m".
 * m_psort is used instead of m_pins so that the resulting
 * matches come out sorted lexicographically by pin name.
 * pinmatch can't be used until macargv() builds m_psort.
 */

int
pinmatch (Macro *m, char *pname, Pin ***ppp)
{
	static Pin	*pp[NPP];
	char		**v;
	int		Npp, i, j, n, npp;

	if (0 >= (n = expand (pname, &v, 0)))
		return (0);
	for (i = Npp = 0; i < n; i++) {
		for (j = npp = 0; j < m->m_npins; j++)
			if (Gmatch (m->m_psort[j]->p_name, v[i]))
				if (Npp >= NPP)
					fatal ("pinmatch: too many pins");
				else	pp[npp++, Npp++] = m->m_psort[j];
		if (!npp)
			error ("%s: pin not matched", v[i]);
	}
	if (Npp)
		*ppp = pp;
	return (Npp);
}


Pin *
pinumsearch (Macro *m, int num)
{
	register Pin	*p = m->m_pins;
	register int	n = m->m_npins;

	for (; n > 0; n--, p++)
		if (num == p->p_num)
			return (p);
	return (0);
}
