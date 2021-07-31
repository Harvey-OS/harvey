#include	<all.h>

void
main(int argc, char *argv[])
{
	int i;

	binit(0, 0, "road");
	einit(Emouse|Ekeyboard);
	etimer(Etimer, 1500);

	map.glog = "/tmp/gpslog";
	map.gpsfd = -1;
	map.rscale = 8;

	map.logsca[0] = 3.162278e-03;
	for(i=1; i<nelem(map.logsca); i++)
		map.logsca[i] = map.logsca[i-1] *
			sqrt(sqrt(10));	/* 4 scales per power of 10 */

	ARGBEGIN {
	case 'r':
	case 's':
		map.rscale = atol(ARGF());
		break;
	case 'g':
		map.gpsenabled = 1;
		break;
	} ARGEND

	map.screend = screen.ldepth;
	logoinit();
	initloc(&map.here);

	for(i=0; i<Nline; i++)
		map.p1.line[i][0] = 0;
	ereshaped(screen.r);

	for(;;)
		doevent();
}

void
doevent(void)
{
	ulong etype;
	Loc loc;
	int i;

	etype = eread(Etimer|Emouse|Ekeyboard, &ev);
	while(etype & Etimer) {
		if(map.gpsenabled)
			trackgps();
		break;
	}
	while(etype & Emouse) {
		if(ev.mouse.buttons & But1) {
			loc = invmap(But1);
			lookup1(loc, 2);
			drawmap(map.here);
			break;
		}
		if(ev.mouse.buttons & But2) {
			loc = invmap(But2);
			lookup2(loc, 2);
			drawmap(map.here);
			break;
		}
		if(ev.mouse.buttons & But3) {
			loc = invmap(But3);
			drawmap(loc);
			break;
		}
		break;
	}
	while(etype & Ekeyboard) {
		i = ev.kbdc;
		switch(i) {
		default:
			if(cmdi < nelem(cmd)-1)
				cmd[cmdi++] = i;
			break;

		case '\n':
			cmd[cmdi] = 0;
			docmd();
			cmdi = 0;
			break;

		case '\b':
			if(cmdi > 0)
				cmdi--;
			break;

		case 4:
			doexit();
		}
		cmd[cmdi] = 0;
		sprint(map.p1.line[3], "%S", cmd);
		drawpage(&map.p1);
		break;
	}
}

void
ereshaped(Rec r)
{
	long x, y, width, height;

	screen.r = bscreenrect(&screen.clipr);
	r = screen.clipr;
	if(map.rscale < 0)
		map.rscale = 0;
	if(map.rscale >= Nscale)
		map.rscale = Nscale-1;

	x = r.max.x - r.min.x;
	if(x <= 0)
		x = 1;
	width = 3*x/2;

	y = r.max.y - r.min.y;
	if(y <= 0)
		y = 1;
	height = 3*y/2;

	if(width != map.width || height != map.height) {
		map.width = width;
		map.height = height;
		if(map.off)
			bfree(map.off);
		map.off = balloc(Rect(0,0,width,height), map.screend!=0);
		if(map.off == 0)
			panic("balloc map");
	}

	map.p1.loc.x = r.min.x + Inset;
	map.p1.loc.y = r.min.y + Inset;
	if(map.p1.bit)
		bfree(map.p1.bit);
	map.p1.bit = balloc(Rect(0,0, 2*width/3 - 2*Inset, (Nline+1)*font->height), 0);

	map.screenr.min.x = r.min.x + Inset;
	map.screenr.min.y = r.min.y + (Nline+1)*font->height + Inset;
	map.screenr.max.x = r.max.x - Inset;
	map.screenr.max.y = r.max.y - Inset;

	map.loc.lat = 0;
	map.loc.lng = 0;
	drawmap(map.here);
}

void
logoinit(void)
{
	int t, i, x, z;

	map.poi = balloc(Rect(0,0, Msize,Msize), 0);
	if(map.poi == 0)
		panic("balloc poi");

	t = Msize/2;
	z = ~Z3;
	segment(map.poi, Pt(t-4,t), Pt(t+4+1,t), z, DorS);
	segment(map.poi, Pt(t,t-4), Pt(t,t+4+1), z, DorS);

	map.com = balloc(Rect(0,0, Msize,Msize), 0);
	if(map.com == 0)
		panic("balloc com");

	t = Msize/2;
	segment(map.com, Pt(t-3,t-3), Pt(t+3+1,t+3+1), z, DorS);
	segment(map.com, Pt(t-3,t+3), Pt(t+3+1,t-3-1), z, DorS);

	map.pla = balloc(Rect(0,0, Msize,Msize), map.screend!=0);
	if(map.pla == 0)
		panic("balloc pla");

	t = Msize/2;
	x = 6;
	for(i=-2; i<=2; i++) {
		z = Z1;
		if(map.screend == 0)
			z = Z3;
		z = ~z;
		segment(map.pla, Pt(t,t), Pt(t+i,t+x), z, DorS);
		segment(map.pla, Pt(t,t), Pt(t+i,t-x), z, DorS);
		segment(map.pla, Pt(t,t), Pt(t+x,t+i), z, DorS);
		segment(map.pla, Pt(t,t), Pt(t-x,t+i), z, DorS);
	}
}

void
docmd(void)
{
	int c, n, flag;
	char *cmd;
	Loc loc;

	cmd = convcmd();
	flag = 0;
	while(*cmd == ' ')
		cmd++;
	c = *cmd++;
	while(*cmd == ' ')
		cmd++;
	switch(c) {
	case 'h':
		for(c=0; c<50; c++)
			if(histo[c]) {
				print("%2d %ld\n", c, histo[c]);
				histo[c] = 0;
			}
		break;

	case 'q':
		if(*cmd)
			break;
		doexit();

	case 'g':
		map.gpsenabled = !map.gpsenabled;
		break;

	case 'p':
		getplace(cmd, &loc);
		drawmap(loc);
		break;

	case 'f':
		getfiles(cmd);
		break;

	case 'l':
		getlatlng(cmd, &loc);
		drawmap(loc);
		break;

	case 's':	/* scale */
		c = *cmd++;
		if(c == '+' || c == '-') {
			flag = c;
			c = *cmd++;
		}
		n = 1;
		if(c >= '0' && c <= '9') {
			n = 0;
			for(;;) {
				n = n*10 + (c-'0');
				c = *cmd++;
				if(c < '0' || c > '9')
					break;
			}
		}
		if(flag == '+')
			n = map.rscale+n;
		if(flag == '-')
			n = map.rscale-n;
		map.rscale = n;
		ereshaped(screen.r);
		break;

	case '/':
		cursorswitch(&thinking);
		search(cmd);
		drawmap(map.here);
		cursorswitch(0);
		break;
	}
}
