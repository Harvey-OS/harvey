#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"


void
hinv(void)
{
	int i, s, mem;
	CPUconf *cp;
	uvlong x;

	for(s = 0; s < Maxslots; s++)
		switch(MCONF->board[s].type){
		case BTip19:
			cp = &CPUREGS->conf[s*Maxcpus];
			uvmove(&cp->rev, &x);
			print("	IP19 rev %d in slot %d\r\n", x.lo, s);
			for(i = 0; i < Maxcpus; i++){
				uvmove(&cp->cachesz, &x);
				print("\t\t%d speed %d cachesz %dKB\n",
					MCONF->board[s].cpus[i].vpid,
					MCONF->board[s].cpus[i].speed,
					(1<<(22-x.lo))/1024);
				cp++;
			}
			break;
		case BTip21:
			print("	IP21 in slot %d\r\n", s);
			break;
		case BTio4:
			print("	IO4 in slot %d window %d\r\n", s, MCONF->board[s].winnum);
			break;
		case BTio5:
			print("	IO5 in slot %d\r\n", s);
			break;
		case BTmc3:
			mem = 0;
			for(i = 0; i < Maxmc3s; i++)
				if(MCONF->board[s].banks[i].enable)
					mem += MCONF->board[s].banks[i].size;
			print("	%dMB MC3 in slot %d\r\n", mem*8, s);
			break;
		}
}

ulong
expr(char *p, char **pp, ulong def)
{
	ulong val;

	if(*p == '.'){
		val = def;
		p++;
	} else
		val = strtoul(p, &p, 0);
	switch(*p){
	case '+':
		p++;
		val += expr(p, &p, def);
		break;
	case '-':
		p++;
		val -= expr(p, &p, def);
		break;
	case '*':
		p++;
		val *= expr(p, &p, def);
		break;
	}
	*pp = p;
	return val;
}

void
debugger(void)
{
	char line[128];
	char *p;
	ulong iter, i, set, val;
	uchar *cp;
	ushort *sp;
	ulong *lp;
	uvlong *vp;
	uvlong x;
	static ulong addr;

	val = 0;
	print("M(STATUS) = %lux\n", machstatus());
	spllo();
	for(;;){
		print("\r\n> ");
		getline(line, sizeof line);
		p = line;
		if(*p == 'l'){
			conf.nmach = 2;
			launchinit();
			continue;
		}
		addr = expr(p, &p, addr);
		if(*p == ','){
			p++;
			iter = expr(p, &p, 0);
		} else
			iter = 1;

		switch(*p){
		case '/':
			p++;
			set = 0;
			break;
		case '%':
			p++;
			val = expr(p, &p, addr);
			set = 1;
			break;
		default:
			continue;
		}

		for(i = 0; i < iter; i++){
			if((i&3) == 0)
				print("\r\n0x%lux/", addr);
			switch(*p){
			case 'b':
				cp = (uchar*)addr;
				if(set)
					*cp = val;
				print("\t%2.2ux", *cp);
				addr += sizeof(uchar);
				break;
			case 'x':
				sp = (ushort*)addr;
				if(set)
					*sp = val;
				print("\t%4.4ux", *sp);
				addr += sizeof(ushort);
				break;
			case 'd':
				sp = (ushort*)addr;
				if(set)
					*sp = val;
				print("\t%d", *sp);
				addr += sizeof(ushort);
				break;
			case 'D':
				lp = (ulong*)addr;
				if(set)
					*lp = val;
				print("\t%d", *lp);
				addr += sizeof(ulong);
				break;
			case 'V':
				vp = (uvlong*)addr;
				if(set){
					x.hi = 0;
					x.lo = val;
					uvmove(&x, vp);
				}
				uvmove(vp, &x);
				print("\t%8.8lux%8.8lux", x.hi, x.lo);
				addr += sizeof(uvlong);
				break;
			case 'X':
			default:
				lp = (ulong*)addr;
				if(set)
					*lp = val;
				print("\t%8.8lux", *lp);
				addr += sizeof(ulong);
				break;
			}
		}
	}
}
