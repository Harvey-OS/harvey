#include <u.h>
#include <libc.h>

#include "vga.h"

static int iflag, lflag, pflag;
static char *usage = "usage: %s [ -cdfilmpv ] [ mode ]\n";

static char *dbname = "/lib/vgadb";
static char monitordb[128];

static void
dump(Vga *vga)
{
	Ctlr *ctlr;

	if(vga->mode)
		dbdumpmode(vga->mode);

	for(ctlr = vga->link; ctlr; ctlr = ctlr->link){
		if(ctlr->dump == 0)
			continue;

		verbose("%s->dump\n", ctlr->name);
		if(ctlr->flag && ctlr->flag != Fsnarf){
			printitem(ctlr->name, "flag");
			print("%9luX\n", ctlr->flag);
		}
		(*ctlr->dump)(vga, ctlr);
		ctlr->flag |= Fdump;
	}
	print("\n");
}

void
resyncinit(Vga *vga, Ctlr *ctlr, ulong on, ulong off)
{
	Ctlr *link;

	verbose("%s->resyncinit\n", ctlr->name);

	for(link = vga->link; link; link = link->link){
		link->flag |= on;
		link->flag &= ~off;
		if(link == ctlr)
			continue;

		if(link->init == 0 || (link->flag & Finit) == 0)
			continue;
		link->flag &= ~Finit;
		(*link->init)(vga, link);
	}
}

void
sequencer(Vga *vga, int on)
{
	static uchar seq01;

	verbose("+sequencer %d\n", on);
	if(on){
		if(vga)
			seq01 = vga->sequencer[0x01];
		seq01 |= 0x01;
		vgaxo(Seqx, 0x01, seq01);
		vgaxo(Seqx, 0x00, 0x03);
	}
	else{
		vgaxo(Seqx, 0x00, 0x01);
		seq01 = vgaxi(Seqx, 0x01);
		vgaxo(Seqx, 0x01, seq01|0x20);
	}
	verbose("-sequencer %d\n", on);
}

void
main(int argc, char *argv[])
{
	char *size, *type, name[NAMELEN+1], *p;
	Ctlr *ctlr;
	Vga *vga;

	if((type = getenv("monitor")) == 0)
		type = "vga";
	size = "640x480x1";

	ARGBEGIN{

	case 'c':
		cflag = 1;
		break;

	case 'd':
		dflag = 1;
		break;

	case 'i':
		iflag = 1;
		break;

	case 'l':
		lflag = 1;
		break;

	case 'm':
		type = ARGF();
		break;

	case 'p':
		pflag = 1;
		break;

	case 'v':
		vflag = 1;
		break;

	default:
		error(usage, argv0);

	}ARGEND

	vga = alloc(sizeof(Vga));

	/*
	 * Try to identify the VGA card and grab
	 * registers. Print them out if requested.
	 */
	if(dbctlr(dbname, vga) == 0){
		print("controller not in %s\n", dbname);
		dumpbios();
		type = "vga";
		size = "640x480x1";
		vga->ctlr = &generic;
		vga->link = &generic;
	}

	for(ctlr = vga->link; ctlr; ctlr = ctlr->link){
		if(ctlr->snarf == 0)
			continue;
		(*ctlr->snarf)(vga, ctlr);
	}

	if(pflag)
		dump(vga);

	if(iflag || lflag){
		if(getenv(type))
			sprint(monitordb, "/env/%s", type);
		else
			strcpy(monitordb, dbname);
		if(argc)
			size = *argv;

		if((vga->mode = dbmode(monitordb, type, size)) == 0)
			error("%s@%s not in %s\n", type, size, monitordb);

		for(ctlr = vga->link; ctlr; ctlr = ctlr->link){
			if(ctlr->options == 0)
				continue;
			(*ctlr->options)(vga, ctlr);
		}

		for(ctlr = vga->link; ctlr; ctlr = ctlr->link){
			if(ctlr->init == 0)
				continue;
			(*ctlr->init)(vga, ctlr);
		}

		if(iflag || pflag)
			dump(vga);

		if(lflag){
			verbose("load\n");
			if(vga->vmb && (vga->mode->x*vga->mode->y*vga->mode->z/8 > vga->vmb))
				error("%s: not enough video memory - %lud\n",
					ctlr->name, vga->vmb);

			/*
			 * Turn off the display during the load.
			 */
			sequencer(vga, 0);

			if(vga->ctlr->type)
				vgactlw("type", vga->ctlr->type);
			else if(p = strchr(vga->ctlr->name, '-')){
				strncpy(name, vga->ctlr->name, p - vga->ctlr->name);
				name[p - vga->ctlr->name] = 0;
				vgactlw("type", name);
			}
			else
				vgactlw("type", vga->ctlr->name);

			vgactlw("size", vga->mode->size);

			for(ctlr = vga->link; ctlr; ctlr = ctlr->link){
				if(ctlr->load == 0)
					continue;
				(*ctlr->load)(vga, ctlr);
			}
			sequencer(vga, 1);

			if(vga->hwgc == 0 || cflag)
				vgactlw("hwgc", "off");
			else
				vgactlw("hwgc", vga->hwgc->name);

			if(pflag)
				dump(vga);
		}
	}

	verbose("exits\n");
	exits(0);
}
