#include <u.h>
#include <libc.h>
#include "dat.h"

Menu initop =
{
	0, 0,

	"VGA setup",
	"Mouse type",
	"Ethernet card",
	"SCSI Device",
	"SoundBlaster",
	"SoundBlaster CD-ROM",
	"File system console",
	"Save configuration",
};

Menu fscons =
{
	1, 0,

	"None",
	"CGA",
	"COM1",
	"COM2",
};
char *fsconsstr[] = { "", "cga", "0", "1" };

Menu fsconsbaud =
{
	1, 0,

	"19200",
	"9600",
	"4800",
	"2400",
	"1200",
};

Menu mouset =
{
	0, 0,

	"COM1",
	"COM2",
	"PS2",
};
char *mstrs[] = { "0", "1", "ps2" };

Menu monitor =
{
	0, 0,

	"vga          - Basic monitor or laptop LCD",
	"multisync65  - Video bandwidth up to 65MHz",
	"multisync75  - Video bandwidth up to 75MHz",
	"multisync135 - Video bandwidth up to 135MHz",
};

Menu resol =
{
	0, 0,

	"640x480x1",
	"640x480x8",
	"800x600x1",
	"800x600x8",
	"1024x768x1",
	"1024x768x8",	
	"1376x1024x8",
};

Menu ethers =
{
	0, 0,

	"None",
	"NE2000 - this card comes with Gateway & TI laptops",
	"NE4100 - The National/IBM PCMCIA ethernet",
	"WD8003 - includes SMC Elites & WD8013-based cards",
	"3C509  - the 3COM ISA/EISA card",
	"3C589  - the 3COM PCMCIA card",
};

enum
{
	NE2000	= 1,
	NE4100,
	WD8003,
	E3C509,
	E3C589,
};

Menu C589ps = 
{
	1, 0,

	"Port 0x240",
	"Port 0x250",
	"Port 0x3E0",
	"Port 0x3F0",
};
int C589psn[] = { 0x240, 0x250, 0x3E0, 0x3F0 };

Menu C589irqs = 
{
	0, 0,

	"IRQ 10",
	"IRQ 11",
	"IRQ 12",
};
int C589irqn[] = { 10, 11, 12 };

Menu WD8003sizs =
{
	1, 0,

	" 8 Kb",
	"16 Kb",
	"32 Kb",
	"64 Kb",
};
int WD8003sizn[] = { 8*1024, 16*1024, 32*1024, 64*1024 };

Menu WD8003irqs =
{
	1, 0,

	"IRQ 2",
	"IRQ 3",
	"IRQ 4",
	"IRQ 5",
	"IRQ 7",
	"IRQ 10",
	"IRQ 11",
	"IRQ 15",
};
int WD8003irqn[] = { 2, 3, 4, 5, 7, 10, 11, 15 };

Menu WD8003ports =
{
	4, 0,

	"Port 0x200",
	"Port 0x220",
	"Port 0x240",
	"Port 0x260",
	"Port 0x280",
	"Port 0x2A0",
	"Port 0x2C0",
	"Port 0x2E0",
	"Port 0x300",
	"Port 0x320",
	"Port 0x340",
	"Port 0x360",
	"Port 0x380",
	"Port 0x3A0",
	"Port 0x3C0",
	"Port 0x3E0"
};
int WD8003pn[] =
{ 0x200, 0x220, 0x240, 0x260, 0x280, 0x2A0, 0x2C0, 0x2E0,
  0x300, 0x320, 0x340, 0x360, 0x380, 0x3A0, 0x3C0, 0x3E0 };

Menu NE2000ports =
{
	0, 0,

	"Port 0x300",
	"Port 0x320",
	"Port 0x340",
	"Port 0x360",
};
int NE2000portn[] = { 0x300, 0x320, 0x340, 0x360 };

Menu NE2000irqs =
{
	2, 0,

	"IRQ 5",
	"IRQ 9",
	"IRQ 10",
};
NE2000irqn[] = { 5, 9, 10 };

Menu NE4100ports =
{
	1, 0,

	"Port 0x300",
	"Port 0x320",
	"Port 0x340",
	"Port 0x360",
};
int NE4100portn[] = { 0x300, 0x320, 0x340, 0x360 };

Menu NE4100irqs =
{
	2, 0,

	"IRQ 5",
	"IRQ 9",
	"IRQ 10",
};
NE4100irqn[] = { 5, 9, 10 };

Menu WD8003mems =
{
	2, 0,

	"mem 0xC8000",
	"mem 0xCC000",
	"mem 0xD0000",
	"mem 0xD4000",
	"mem 0xD8000",
	"mem 0xDC000",
};
WD8003memn[] = { 0xC8000, 0xCC000, 0xD0000, 0xD4000, 0xD8000, 0xDC000 };

Menu scsis =
{
	0, 0,

	"None",
	"aha1542  - Adaptec 154x[BC]",
	"bus745   - Buslogic 7[45]7[SD]",
	"ultra14f - Ultrastor [13]4f",
	"bus4201  - Buslogic 7[45]7[SD] in 32-bit mode",
};

enum
{
	AHA1542 = 1,
	BUS754,
	ULTRA14F,
	BUS4201
};

Menu aha1542ports =
{
	4, 0,

	"Port 0x130",
	"Port 0x134",
	"Port 0x230",
	"Port 0x234",
	"Port 0x330",
	"Port 0x334"
};
int aha1542portn[] = { 0x130, 0x134, 0x230, 0x234, 0x330, 0x334 };

Menu ultra14fports =
{
	6, 0,

	"Port 0x130",
	"Port 0x140",
	"port 0x210",
	"Port 0x230",
	"Port 0x240",
	"Port 0x310",
	"Port 0x330",
	"Port 0x340",
};
int ultra14fportn[] =
{ 0x130, 0x140, 0x210, 0x230, 0x240, 0x310, 0x330, 0x340 };

Menu sbports =
{
	1, 0,

	"None",
	"Port 0x220",
	"Port 0x240",
	"Port 0x260",
	"Port 0x280",
};
int sbtab[] = { 0, 0x220, 0x240, 0x260, 0x280 };

Menu sbirqs =
{
	2, 0,

	"IRQ 2",
	"IRQ 5",
	"IRQ 7",
	"IRQ 10",
};
int sbirqn[] = { 2, 5, 7, 10 };

Menu sbdma16s =
{
	0, 0,

	"DMA CH5",
	"DMA CH6",
	"DMA CH7",
};
int dma16sbn[] =  { 5, 6, 7 };

Menu cdroms =
{
	1, 0,

	"none",
	"panasonic",
	"matsushita",
	"mitsumi",
};

Menu panasonics =
{
	0, 0,

	"Port 0x230",
	"Port 0x250",
	"Port 0x270",
	"Port 0x290",
};

Menu mitsumis =
{
	2, 0,

	"Port 0x320",
	"Port 0x330",
	"Port 0x340",
	"Port 0x350",
	"Port 0x360",
	"Port 0x370",
};
int cdport[4][10] =
{
	{ 0 },
	{ 0x230, 0x250, 0x270, 0x290 },			/* Panasonic */
	{ 0x230, 0x250, 0x270, 0x290 },			/* Panasonic */
	{ 0x320, 0x330, 0x340, 0x350, 0x360, 0x370 },	/* Mitsumi */
};


int cdrom, cdromp, eport, eirq, esize, emem, sport, cons, consbaud;
int vgares, vgamon, mtype, etype, stype, portsb, irqsb, dma8sb, dma16sb;

Menu p9iniok = 
{
	0, 0,

	"Redo",
	"Save",
};

int
mkplan9ini(char *inifile, char *bootfile, char *rootdir)
{
	int i, c, j;
	Saved s1, s2;
	char *lines[NP9INI], buf[NWIDTH];


	for(i = 0; i < NP9INI; i++)
		lines[i] = malloc(NWIDTH);

	i = 0;
	if(ethers.selected) {
		sprint(lines[i], "ether0=type=%s", ethers.items[etype]);
		*strchr(lines[i], ' ') = '\0';
		if(eport != 0) {
			sprint(buf, " port=0x%lux", eport);
			strcat(lines[i], buf);
		}
		if(emem != 0) {
			sprint(buf, " mem=0x%lux", emem);
			strcat(lines[i], buf);
		}
		if(eirq != 0) {
			sprint(buf, " irq=%d", eirq);
			strcat(lines[i], buf);
		}
		if(esize != 0) {
			sprint(buf, " size=%d", esize);
			strcat(lines[i], buf);
		}
		i++;
	}

	if(mouset.selected)
		sprint(lines[i++], "mouseport=%s", mstrs[mtype]);

	if(monitor.selected) {
		sprint(lines[i], "monitor=%s", monitor.items[vgamon]);
		*strchr(lines[i], ' ') = '\0';
		i++;
	}

	if(resol.selected)
		sprint(lines[i++], "vgasize=%s", resol.items[vgares]);

	if(scsis.selected) {
		sprint(lines[i], "scsi0=type=%s", scsis.items[stype]);
		*strchr(lines[i], ' ') = '\0';
		if(sport != 0) {
			sprint(buf, " port=0x%lux", sport);
			strcat(lines[i], buf);
		}
		i++;
	}

	if(sbports.selected) {
		sprint(lines[i], "audio0=type=sb16 port=0x%lux", sbtab[portsb]);
		if(sbirqs.selected) {
			sprint(buf, " irq=%d", sbirqn[irqsb]);
			strcat(lines[i], buf);
		}
		if(sbdma16s.selected) {
			sprint(buf, " dma=%d", dma16sbn[dma16sb]);
			strcat(lines[i], buf);
		}
		i++;
	}

	if(cdroms.selected) {
		sprint(lines[i], "cdrom0=type=%s", cdroms.items[cdrom]);
		if(mitsumis.selected || panasonics.selected) {
			sprint(buf, " port=0x%lux", cdport[cdrom][cdromp]);
			strcat(lines[i], buf);
		}
		i++;
	}
	if(fscons.selected) {
		sprint(lines[i], "console=%s", fsconsstr[cons]);
		if(fsconsbaud.selected) {
			sprint(buf, " baud=%s", fsconsbaud.items[consbaud]);
			strcat(lines[i], buf);
		}
		i++;
	}

	if(bootfile)
		sprint(lines[i++], "bootfile=%s", bootfile);
	if(rootdir)
		sprint(lines[i++], "rootdir=%s", rootdir);

	lines[i] = 0;
	infobox(20, 10, lines, &s1);
	c = menu(23, 10+i+1, &p9iniok, &s2);
	restore(&s2);
	restore(&s1);
	if(c == 1) {
		c = create(inifile, OWRITE, 0777);
		if(c < 0)
			warn(inifile);
		else {
			for(j = 0; j < i; j++) {
				fprint(c, "%s\r\n", lines[j]);
				free(lines[j]);
			}
			close(c);
		}
		return 1;
	}
	for(j = 0; j < i; j++)
		free(lines[j]);
	return 0;
}

char *nodefl[] =
{
	"You must select a SCSI controller for",
	"the installation to proceed.",
	"Press any key to continue ...",
	0
};

char *scsireboot[] =
{
	"To make the system recognize the SCSI",
	"controller you must now reboot. Press",
	"any key to make the system reboot.",
	"",
	"Plan 9 should reboot automatically.",
	"",
	"If Plan 9 fails to restart you have",
	"probably specified the SCSI controller",
	"incorrectly. You must make a new copy",
	"of the boot disk and repeat this",
	"procedure with the correct parameters.",
	0
};

char *mking[] = 
{
	"Writing configuration file to A:\\plan9.ini",
	"Please wait ...",
	0,
};

char floppyboot[] = "bootfile=fd!0!9dos\r\n";

void
scsiini(void)
{
	int fd, sel;
	Saved s1, s2;
	char buf[128], port[32];

	for(;;) {
		stype = menu(5, 5, &scsis, &s1);
		if(stype != 0 && scsis.selected)
			break;
		infobox(8, 8, nodefl, &s2);
		getc();
		restore(&s2);
		restore(&s1);
	}

	sport = 0;
	aha1542ports.selected = 0;
	switch(stype) {
	case AHA1542:
		sel = menu(7, 7, &aha1542ports, &s2);
		restore(&s2);
		if(aha1542ports.selected == 0) {
			sport = 0;
			break;
		}
		sport = aha1542portn[sel];
		break;
	case ULTRA14F:
		sel = menu(7, 7, &ultra14fports, &s2);
		restore(&s2);
		if(ultra14fports.selected == 0) {
			sport = 0;
			break;
		}
		sport = ultra14fportn[sel];
	}
	restore(&s1);

	infobox(10, 10, mking, &s1);
	fd = create("/n/a/plan9.ini", OWRITE, 0444);
	if(fd < 0)
		warn("/n/a/plan9.ini");

	if(write(fd, floppyboot, strlen(floppyboot)) < 0)
		warn("/n/a/plan9.ini");

	sprint(buf, "scsi0=type=%s", scsis.items[stype]);
	*strchr(buf, ' ') = '\0';
	if(sport != 0) {
		sprint(port, " port=0x%lux", sport);
		strcat(buf, port);
	}
	strcat(buf, "\r\n");
	if(write(fd, buf, strlen(buf)) < 0)
		warn("/n/a/plan9.ini");
	close(fd);
	restore(&s1);

	infobox(5, 5, scsireboot, &s1);
	reboot(1);
}

char *allinfo[] =
{
	"You must enter information about all",
	"devices every time you use this menu",
	"to configure a device. This is not an",
	"editor; it makes a new PLAN9.INI file",
	"each time.",
	0,
};

void
configure(char *inifile, char *bootfile, char *rootdir)
{
	int sel, aion;
	Saved ai, st, s1, s2;

	aion = 1;
	infobox(38, 3, allinfo, &ai);

	for(;;) {
		sel = menu(15, 7, &initop, &st);
		if(aion) {
			aion = 0;
			restore(&ai);
		}
		switch(sel) {
		case 0:
			vgares = menu(19, 11, &resol, &s1);
			restore(&s1);
			if(resol.selected == 0)
				break;
			vgamon = menu(19, 11, &monitor, &s1);
			restore(&s1);
			break;
		case 1:
			mtype = menu(17, 10, &mouset, &s1);
			restore(&s1);
			break;
		case 2:
			etype = menu(17, 11, &ethers, &s1);
			restore(&s1);
			if(etype == 0) {
				ethers.selected = 0;
				break;
			}
			eport = 0;
			emem = 0;
			esize = 0;
			eirq = 0;
			switch(etype) {
			case NE4100:
				sel = menu(17, 11, &NE4100ports, &s1);
				restore(&s1);
				if(NE4100ports.selected == 0)
					break;
				eport = NE4100portn[sel];
				sel = menu(17, 11, &NE4100irqs, &s1);
				restore(&s1);
				if(NE4100irqs.selected == 0)
					break;
				eirq = NE4100irqn[sel];
				break;
			case NE2000:
				sel = menu(17, 11, &NE2000ports, &s1);
				restore(&s1);
				if(NE2000ports.selected == 0)
					break;
				eport = NE2000portn[sel];
				sel = menu(17, 11, &NE2000irqs, &s1);
				restore(&s1);
				if(NE2000irqs.selected == 0)
					break;
				eirq = NE2000irqn[sel];
				break;
			case WD8003:
				sel = menu(20, 4, &WD8003ports, &s1);
				restore(&s1);
				if(WD8003ports.selected == 0)
					break;
				eport = WD8003pn[sel];
				sel = menu(17, 11, &WD8003irqs, &s1);
				restore(&s1);
				if(WD8003irqs.selected == 0)
					break;
				eirq = WD8003irqn[sel];
				sel = menu(17, 11, &WD8003mems, &s1);
				restore(&s1);
				if(WD8003mems.selected == 0)
					break;
				emem = WD8003memn[sel];
				sel = menu(17, 11, &WD8003sizs, &s1);
				restore(&s1);
				if(WD8003sizs.selected == 0)
					break;
				esize = WD8003sizn[sel];
				break;
			case E3C589:
				sel = menu(17, 11, &C589ps, &s1);
				restore(&s1);
				if(C589ps.selected == 0)
					break;
				eport = C589psn[sel];
				sel = menu(17, 11, &C589irqs, &s1);
				restore(&s1);
				if(C589irqs.selected == 0)
					break;
				eirq = C589irqn[sel];
				break;
			case E3C509:
				break;
			}
			break;
		case 3:
			stype = menu(17, 11, &scsis, &s1);
			restore(&s1);
			if(stype == 0) {
				scsis.selected = 0;
				break;
			}

			sport = 0;
			aha1542ports.selected = 0;
			switch(stype) {
			case AHA1542:
				sel = menu(20, 13, &aha1542ports, &s1);
				restore(&s1);
				if(aha1542ports.selected == 0) {
					sport = 0;
					break;
				}
				sport = aha1542portn[sel];
				break;
			case ULTRA14F:
				sel = menu(20, 13, &ultra14fports, &s1);
				restore(&s1);
				if(ultra14fports.selected == 0) {
					sport = 0;
					break;
				}
				sport = ultra14fportn[sel];
			}
			break;
		case 4:
			portsb = menu(17, 13, &sbports, &s1);
			restore(&s1);
			if(portsb == 0 || sbports.selected == 0) {
				sbports.selected = 0;
				break;
			}
			irqsb = menu(17, 13, &sbirqs, &s1);
			restore(&s1);
			if(sbirqs.selected == 0)
				break;
			dma16sb = menu(17, 13, &sbdma16s, &s1);
			restore(&s1);
			break;
		case 5:
			cdrom = menu(17, 14, &cdroms, &s1);
			if(cdrom == 0 || cdroms.selected == 0) {
				cdroms.selected = 0;
				restore(&s1);
				break;
			}
			switch(cdrom) {
			case 1:
			case 2:
				cdromp = menu(19, 15, &panasonics, &s2);
				break;
			case 3:	
				cdromp = menu(19, 15, &mitsumis, &s2);
				break;
			}
			restore(&s2);
			restore(&s1);
			break;
		case 6:
			cons = menu(17, 13, &fscons, &s1);
			if(cons == 0 || fscons.selected == 0) {
				restore(&s1);
				fscons.selected = 0;
				break;
			}
			if(cons == 1) {
				restore(&s1);
				break;
			}
			consbaud = menu(22, 16, &fsconsbaud, &s2);
			restore(&s2);
			restore(&s1);
			break;
		case 7:
			if(mkplan9ini(inifile, bootfile, rootdir)) {
				restore(&st);
				return;
			}
		}
		restore(&st);
	}
}

void
reboot(int ask)
{
	int fd;
	char *av[10];

	if(access("#s/kfs", 0) == 0) {
		av[0] = "/bin/disk/kfscmd";
		av[1] = "halt";
		av[2] = 0;
		docmd(av, 0);
	}

	if(ask)
		getc();

	fd = open("#↑/zot", OWRITE);
	write(fd, "reboot", 6);
	close(fd);
	exits("reboot");
}

void
fixplan9(char *inifile, char *disk)
{	int fd, n;
	char *p, *q, buf[8192], bootfile[128];

	fd = open(inifile, OREAD);
	if(fd < 0) {
		warn(inifile);
		return;
	}
	n = read(fd, buf, sizeof(buf));
	close(fd);
	if(n < 0) {
		warn(inifile);
		return;
	}
	buf[n] = '\0';

	p = strstr(buf, "rootdir=");
	if(p != 0) {
		q = strchr(p, '\n');
		if(q != 0) {
			q++;
			memmove(p, q, (buf+n) - q);
			n -= q-p;
			buf[n] = '\0';
		}
	}	
	p = strstr(buf, "bootfile=");
	if(p != 0) {
		q = strchr(p, '\n');
		if(q != 0) {
			q++;
			memmove(p, q, (buf+n) - q);
			n -= q-p;
			buf[n] = '\0';
		}
	}

	strcat(buf, "\r\nbootfile=");
	strcpy(bootfile, "local");
	p = strrchr(disk, '/');
	if(p != 0) {
		p++;
		q = p;
		while(*q && (*q < '0' || *q > '7'))
			q++;
		if(*q)
			sprint(bootfile, "%c!%c", *p, *q);
	}
	strcat(buf, bootfile);
	strcat(buf, "\r\n");

	fd = create(inifile, OWRITE, 0664);
	if(fd < 0) {
		warn(inifile);
		return;
	}
	n = strlen(buf);
	if(write(fd, buf, n) != n)
		warn(inifile);
	close(fd);
}
