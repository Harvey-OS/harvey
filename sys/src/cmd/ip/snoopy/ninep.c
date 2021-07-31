#include <u.h>
#include <libc.h>
#include <ip.h>
#include <fcall.h>
#include "dat.h"
#include "protos.h"

static void
p_compile(Filter *f)
{
	sysfatal("unknown ninep field: %s", f->s);
}

static int
p_filter(Filter *, Msg *)
{
	return 0;
}

static int
p_seprint(Msg *m)
{
	Fcall f;
	char *p;

	memset(&f, 0, sizeof(f));
	f.type = 0;
	f.data = 0;	/* protection for %F */
	convM2S(m->ps, m->pe-m->ps, &f);
	if(f.type){
		p = m->p;
		m->p = seprint(m->p, m->e, "%F", &f);
		while(p < m->p){
			p = strchr(p, '\n');
			if(p == nil)
				break;
			*p = '\\';
		}
	} else
		dump.seprint(m);
	m->pr = nil;
	return 0;
}

Proto ninep =
{
	"ninep",
	p_compile,
	p_filter,
	p_seprint,
	nil,
	nil,
};
