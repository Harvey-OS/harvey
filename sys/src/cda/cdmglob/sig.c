#include <ctype.h>
#include "cdmglob.h"

#define NSIGS	32768


#ifdef jhc
Sig	unconnected = { "/unconnected", S_GLOB, 0 };
#endif

static Sig	sigs[NSIGS];
static int	nsigs;


int
global (char *name)
{
	if (*name == '/')
		return (1);
	if (*name == 'V' || *name == 'v')
		if (name[1] == 'D' || name[1] == 'd') {
			if (name[2] == 'D' || name[2] == 'd')
				if (!name[3])
					return (1);
		}
		else if (name[1] == 'S' || name[1] == 's') {
			if (name[2] == 'S' || name[2] == 's')
				if (!name[3])
					return (1);
		}
	if (*name == 'V' || *name == 'v')
		if (name[1] == 'C' || name[1] == 'c') {
			if (name[2] == 'C' || name[2] == 'c')
				if (!name[3])
					return (1);
		}
	if (*name == 'G' || *name == 'g')
		if (name[1] == 'N' || name[1] == 'n') {
			if (name[2] == 'D' || name[2] == 'd')
				if (!name[3])
					return (1);
		}
	return (0);
}


int
sigexpand (Macro *m, char *sname, Sig ***ss)
{
	Sig	*s;
	char	**v;
	int	i, n, nn;

	if (0 >= (n = expand (sname, &v, 1)))
		return (0);
	alloc (*ss, n, Sig *);
	for (nn = i = 0; i < n; i++)
		if (s = siglookup (m, v[i]))
			(*ss)[nn++] = s;
	return (nn);
}


Sig *
siglookup (Macro *m, char *sname)
{
	register Sig	*s;
	register int	h, H;

	H = hash (sname, HASH);

	for (h = m->m_shash[H]; h >= 0; h = s->s_hash) {
		s = m->m_sigs + h;
		if (Strsame (sname, s->s_name))
			return (s);
	}

	if (nsigs >= NSIGS)
		fatal ("too many signals");
	s = sigs + nsigs++;
	if (!m->m_sigs)
		m->m_sigs = s;
	if (s != m->m_sigs + m->m_nsigs++)
		fatal ("siglookup: get help");

	s->s_name = strsave (sname);
	s->s_type = global (sname) ? S_GLOB : 0;
	s->s_hash = m->m_shash[H];
	m->m_shash[H] = s - m->m_sigs;
	return (s);
}
