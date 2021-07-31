#include <u.h>
#include <libc.h>
#include <bio.h>

#include "pci.h"
#include "vga.h"

Biobuf stdout;

static int iflag, lflag, pflag, rflag;
static char *usage =
	"usage: aux/vga [ -BcdilpvV ] [ -b bios-id ] [ -m monitor ] [ -x db ] [ mode [ virtualsize ] ]\n";

static char *dbname = "/lib/vgadb";
static char monitordb[128];

static void
dump(Vga* vga)
{
	Ctlr *ctlr;
	Attr *attr;
	
	if(vga->mode)
		dbdumpmode(vga->mode);

	for(attr = vga->attr; attr; attr = attr->next)
		Bprint(&stdout, "vga->attr: %s=%s\n", attr->attr, attr->val);

	for(ctlr = vga->link; ctlr; ctlr = ctlr->link){
		if(ctlr->dump == 0)
			continue;

		trace("%s->dump\n", ctlr->name);
		if(ctlr->flag && ctlr->flag != Fsnarf){
			printitem(ctlr->name, "flag");
			printflag(ctlr->flag);
			Bprint(&stdout, "\n");
		}
		(*ctlr->dump)(vga, ctlr);
		ctlr->flag |= Fdump;
	}
	Bprint(&stdout, "\n");
}

void
resyncinit(Vga* vga, Ctlr* ctlr, ulong on, ulong off)
{
	Ctlr *link;

	trace("%s->resyncinit on 0x%8.8luX off 0x%8.8luX\n",
		ctlr->name, on, off);

	for(link = vga->link; link; link = link->link){
		link->flag |= on;
		link->flag &= ~off;
		if(link == ctlr)
			continue;

		if(link->init == 0 || (link->flag & Finit) == 0)
			continue;
		link->flag &= ~Finit;
		trace("%s->init 0x%8.8luX\n", link->name, link->flag);
		(*link->init)(vga, link);
	}
}

void
sequencer(Vga* vga, int on)
{
	static uchar seq01;
	static int state = 1;
	char *s;

	if(on)
		s = "on";
	else
		s = "off";
	trace("sequencer->enter %s\n", s);
	if(on){
		if(vga)
			seq01 = vga->sequencer[0x01];
		if(state == 0){
			seq01 |= 0x01;
			vgaxo(Seqx, 0x01, seq01);
			vgaxo(Seqx, 0x00, 0x03);
		}
	}
	else{
		vgaxo(Seqx, 0x00, 0x01);
		seq01 = vgaxi(Seqx, 0x01);
		vgaxo(Seqx, 0x01, seq01|0x20);
	}
	state = on;
	trace("sequencer->leave %s\n", s);
}

static void
linear(Vga* vga)
{
	char buf[256];

	/*
	 * Set up for linear addressing: try to allocate the
	 * kernel memory map then read the base-address back.
	 * vga->linear is a compatibility hack.
	 */
	if(vga->linear == 0){
		vga->ctlr->flag &= ~Ulinear;
		return;
	}
	if(vga->ctlr->flag & Ulinear){
		sprint(buf, "0x%lux 0x%lux", vga->apz ? vga->apz : vga->vmz, vga->vma);
		vgactlw("linear", buf);
		vgactlr("addr", buf);
		trace("linear->addr %s\n", buf);
		vga->vmb = strtoul(buf, 0, 0);
	}
	else
		vgactlw("linear", "0");
}

char*
chanstr[32+1] = {
[1]	"k1",
[2]	"k2",
[4]	"k4",
[8]	"m8",
[16]	"r5g6b5",
[24]	"r8g8b8",
[32]	"x8r8g8b8",
};

void
main(int argc, char** argv)
{
	char *bios, buf[256], sizeb[256], *p, *vsize, *psize, *type;
	int virtual, len;
	Ctlr *ctlr;
	Vga *vga;

	Binit(&stdout, 1, OWRITE);

	bios = getenv("vgactlr");
	if((type = getenv("monitor")) == 0)
		type = "vga";
	psize = vsize = "640x480x8";

	ARGBEGIN{

	case 'b':
		bios = ARGF();
		break;

	case 'B':
		dumpbios(0x10000);
		exits(0);

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

	case 'r':
		/*
		 * rflag > 1 means "leave me alone, I know what I'm doing."
		 */
		rflag++;
		break;

	case 'v':
		vflag = 1;
		break;

	case 'V':
		vflag = 1;
		Vflag = 1;
		break;

	case 'x':
		dbname = ARGF();
		break;

	default:
		error(usage, argv0);

	}ARGEND

	virtual = 0;
	switch(argc){
	case 1:
		vsize = psize = argv[0];
		break;
	case 2:
		psize = argv[0];
		vsize = argv[1];
		virtual = 1;
		break;
	case 0:
		break;
	default:
		error(usage, argv0);
	}

	vga = alloc(sizeof(Vga));
	if(bios){
		if((vga->offset = strtol(bios, &p, 0)) == 0 || *p++ != '=')
			error("main: bad BIOS string format - %s\n", bios);
		len = strlen(p);
		vga->bios = alloc(len+1);
		strncpy(vga->bios, p, len);
		trace("main->BIOS %s\n", bios);
	}

	/*
	 * Try to identify the VGA card and grab
	 * registers. Print them out if requested.
	 */
	if(dbctlr(dbname, vga) == 0 || vga->ctlr == 0){
		Bprint(&stdout, "%s: controller not in %s\n", argv0, dbname);
		dumpbios(256);
		type = "vga";
		vsize = psize = "640x480x1";
		virtual = 0;
		vga->ctlr = &generic;
		vga->link = &generic;
	}

	trace("main->snarf\n");
	for(ctlr = vga->link; ctlr; ctlr = ctlr->link){
		if(ctlr->snarf == 0)
			continue;
		trace("%s->snarf\n", ctlr->name);
		(*ctlr->snarf)(vga, ctlr);
	}

	if(pflag)
		dump(vga);

	for(ctlr = vga->link; ctlr; ctlr = ctlr->link)
		if(ctlr->flag & Ferror)
			error("%r");

	if(iflag || lflag){
		if(getenv(type))
			sprint(monitordb, "/env/%s", type);
		else
			strcpy(monitordb, dbname);

		if((vga->mode = dbmode(monitordb, type, psize)) == 0)
			error("main: %s@%s not in %s\n", type, psize, monitordb);
		if(virtual){
			if((p = strchr(vsize, 'x')) == nil)
				error("bad virtual size %s\n", vsize);
			vga->virtx = atoi(vsize);
			vga->virty = atoi(p+1);
			if(vga->virtx < vga->mode->x || vga->virty < vga->mode->y)
				error("virtual size smaller than physical size\n");
			vga->panning = 1;
		}
		else{
			vga->virtx = vga->mode->x;
			vga->virty = vga->mode->y;
			vga->panning = 0;
		}

		trace("vmf %d vmdf %d vf1 %lud vbw %lud\n",
			vga->mode->frequency, vga->mode->deffrequency,
			vga->f[1], vga->mode->videobw);
		if(vga->mode->frequency == 0 && vga->mode->videobw != 0 && vga->f[1] != 0){
			/*
			 * boost clock as much as possible subject
			 * to video and memory bandwidth constraints
			 */
			ulong bytes, freq, membw;
			double rr;

			freq = vga->mode->videobw;
			if(freq > vga->f[1])
				freq = vga->f[1];

			rr = (double)freq/(vga->mode->ht*vga->mode->vt);
			if(rr > 85.0)		/* >85Hz is ridiculous */
				rr = 85.0;

			bytes = (vga->mode->x*vga->mode->y*vga->mode->z)/8;
			membw = rr*bytes;
			if(vga->membw != 0 && membw > vga->membw){
				membw = vga->membw;
				rr = (double)membw/bytes;
			}

			freq = rr*(vga->mode->ht*vga->mode->vt);
			vga->mode->frequency = freq;

			trace("using frequency %lud rr %.2f membw %lud\n",
				freq, rr, membw);
		}
		else if(vga->mode->frequency == 0)
			vga->mode->frequency = vga->mode->deffrequency;

		for(ctlr = vga->link; ctlr; ctlr = ctlr->link){
			if(ctlr->options == 0)
				continue;
			trace("%s->options\n", ctlr->name);
			(*ctlr->options)(vga, ctlr);
		}

		for(ctlr = vga->link; ctlr; ctlr = ctlr->link){
			if(ctlr->init == 0)
				continue;
			trace("%s->init\n", ctlr->name);
			(*ctlr->init)(vga, ctlr);
		}

		if(strcmp(vga->mode->chan, "") == 0){
			if(vga->mode->z < nelem(chanstr) && chanstr[vga->mode->z])
				strcpy(vga->mode->chan, chanstr[vga->mode->z]);
			else
				error("%s: unknown channel type to use for depth %d", vga->ctlr->name, vga->mode->z);
		}

		if(iflag || pflag)
			dump(vga);

		if(lflag){
			trace("main->load\n");
			if(vga->vmz && (vga->virtx*vga->virty*vga->mode->z)/8 > vga->vmz)
				error("%s: not enough video memory - %lud\n",
					vga->ctlr->name, vga->vmz);

			if(vga->ctlr->type)
				vgactlw("type", vga->ctlr->type);
			else if(p = strchr(vga->ctlr->name, '-')){
				strncpy(buf, vga->ctlr->name, p - vga->ctlr->name);
				buf[p - vga->ctlr->name] = 0;
				vgactlw("type", buf);
			}
			else
				vgactlw("type", vga->ctlr->name);
	
			/*
			 * The new draw device needs linear mode set
			 * before size.
			 */
			linear(vga);

			sprint(buf, "%ludx%ludx%d %s",
				vga->virtx, vga->virty,
				vga->mode->z, vga->mode->chan);
			if(rflag){
				vgactlr("size", sizeb);
				if(rflag < 2 && strcmp(buf, sizeb) != 0)
					error("bad refresh: %s != %s\n",
						buf, sizeb);
			}
			else
				vgactlw("size", buf);

			/*
			 * Turn off the display during the load.
			 */
			sequencer(vga, 0);

			for(ctlr = vga->link; ctlr; ctlr = ctlr->link){
				if(ctlr->load == 0)
					continue;
				trace("%s->load\n", ctlr->name);
				(*ctlr->load)(vga, ctlr);
			}

			sequencer(vga, 1);

			vgactlw("drawinit", "");

			if(vga->hwgc == 0 || cflag)
				vgactlw("hwgc", "off");
			else
				vgactlw("hwgc", vga->hwgc->name);

			if(vga->virtx != vga->mode->x || vga->virty != vga->mode->y){
				sprint(buf, "%dx%d", vga->mode->x, vga->mode->y);
				vgactlw("actualsize", buf);
				if(vga->panning)
					vgactlw("panning", "on");
			}

			if(pflag)
				dump(vga);
		}
	}

	trace("main->exits\n");
	exits(0);
}
