#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"ureg.h"

IOQ dkbdq;
IOQ dprintq;

typedef struct Syslogbuf	Syslogbuf;
struct Syslogbuf
{
	char	*next;
	char	buf[4*1024];
};

Syslogbuf syslogbuf[4];

static int
dprint(char *fmt, ...)
{
	char buf[PRINTSIZE];
	int n;

	n = doprint(buf, buf+sizeof(buf), fmt, (&fmt+1)) - buf;
	dprintq.puts(&dprintq, buf, n);
	return n;
}

static int
dgetline(char *p, int n)
{
	int c;
	char *end, *start;
	char buf[1];

	start = p;
	end = p + n - 1;
	for(;;){
		c = getc(&dkbdq);
		if(c <= 0)
			continue;

		/* echo */
		buf[0] = c;
		if(c == '\r' || c == '\n')
			dprintq.puts(&dprintq, "\r\n", 2);
		else
			dprintq.puts(&dprintq, buf, 1);

		if(c == '\b'){
			if(p != start)
				p--;
			continue;
		}
		if(c == '\r' || c == '\n')
			break;
		if(p >= end)
			continue;
		*p++ = c;
	}
	*p = 0;
	return p - start;
}

static void
printlog(char *cpu)
{
	int i, c;
	char *p, *end;
	Syslogbuf *s;

	if(cpu == 0)
		i = 0;
	else
		i = atoi(cpu);
	if(i < 0 || i > 3)
		return;
	s = &syslogbuf[i];

	if(s->next < s->buf || s->next >= &s->buf[sizeof(s->buf)])
		s->next = s->buf;
	end = s->next;
	p = end + 1;
	if(p >= &s->buf[sizeof(s->buf)] || p < s->buf)
		p = s->buf;
	while(p != end){
		c = *p & 0xff;
		if(c == '\r' || c == '\n')
			dprintq.puts(&dprintq, "\r\n", 2);
		else if(c >= ' ' && c <= '~')
			dprintq.puts(&dprintq, p, 1);
		p++;
		if(p >= &s->buf[sizeof(s->buf)])
			p = s->buf;
	}
}

static void
printhex(char *start, char *len)
{
	int i;
	ulong n;
	ulong *p;

	if(start == 0){
		dprint("!	h <location> <howmany> - hex display\r\n");
		return;
	}
	n = strtoul(start, 0, 16);
	if((n & KZERO) == 0)
		return;
	p = (ulong *)n;

	if(len)
		n = strtoul(len, 0, 0);
	else
		n = 1;

	while(n > 0){
		dprint("%lux/", p);
		for(i = 0; i < 8 && n > 0; i++, n--)
			dprint(" %8.8lux", p[i]);
		dprint("\r\n");
		p += 8;
	}
}

static void
printinfo(void)
{
	Mach *mp;
	Proc *p;
	Page *pg;
	Ureg *ur;
	ulong l;
	int i;

	for(i = 0; i < 4; i++){
		if((active.machs&(1<<i)) == 0)
			continue;
		mp = (void*)(MACHADDR+i*BY2PG);
		l = (ulong)(mp->proc);
		dprint("mach[%d] proc/%lux\r\n", i, l);
		dprint("mach[%d] splpc/%lux\r\n", i, mp->splpc);

		if(l & KZERO){
			p = (Proc*)l;
			l = (ulong)(p->upage);
			if(l & KZERO){
				pg = (Page*)l;
				l = pg->pa;
				dprint("mach[%d] proc->upage->pa/%lux\r\n", i, l);
			}
		}
		l = (ulong)(mp->ur);
		dprint("mach[%d] ur/%lux", i, l);
		ur = (Ureg*)l;
		dprint(" &status/%lux &cause/%lux &sp/%lux &pc/%lux",
				&ur->status, &ur->cause, &ur->sp, &ur->pc);
		dprint("\r\n");
	}
}

void
debugger(void *arg)
{
	int n, level;
	char buf[256];
	char *field[3];

	USED(arg);

	/*
	 *  sched() until we are on processor 1
	 */
	for(;;){
		level = splhi();
		if(m->machno == 1)
			break;
		splx(level);
		sched();
	}

	/*
 	 *  grab a port for a console
	 */
	initq(&dprintq);
	initq(&dkbdq);
	duartspecial(3, &dprintq, &dkbdq, 9600);

	/*
	 *  take processor and process out of active groups
	 */
	active.machs &= ~(1<<1);
	splx(level);

	for(;;){
		dprint("kdb> ");
		dgetline(buf, sizeof(buf));
		memset(field, 0, sizeof(field));
		n = getfields(buf, field, 3, " ");
		if(n == 0)
			continue;
		switch(*field[0]){
		case 'l':
			printlog(field[1]);
			break;
		case 'h':
			printhex(field[1], field[2]);
			break;
		case 'i':
			printinfo();
			break;
		case 'q':
			firmware(cpuserver ? PROM_AUTOBOOT : PROM_REINIT);
			break;
		default:
			dprint("!commands are:\r\n");
			dprint("!	l <cpu#> - print cpu log\r\n");
			dprint("!	h <location> <howmany> - hex display\r\n");
			dprint("!	i - display some addresses\r\n");
			dprint("!	q - quit/rebooot\r\n");
			break;
		}
	}
}

void
syslog(char *str, int n)
{
	Syslogbuf *s;
	int level;

	level = splhi();
	s = &syslogbuf[m->machno];
	if(s->next < s->buf || s->next >= &s->buf[sizeof(s->buf)])
		s->next = s->buf;
	while(n-- > 0){
		*s->next = *str++;
		if(s->next >= &s->buf[sizeof(s->buf)-1])
			s->next = s->buf;
		else
			s->next++;
	}
	splx(level);
}
