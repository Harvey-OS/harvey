#include <u.h>
#include <libc.h>
#include <libg.h>
#include <stdio.h>
#include "menu.h"
#include "event.h"
#include "thing.h"

Line *save;
Dict *files;
File *this,*editfile(char *);
#define DL	(this->dl)
Mouse mouse,pen;
Point cursor,dotp,grid;
Rectangle minbb;
Bitmap screen,*stage;
Font *tiny,*defont;
int moved,showgrey,invis=1;
int getmouse(void),dobut1(void),dobut2(void),dobut3(void),docursor(int);
int typing(char *),show(void);
Cursor blank;
Line *inlist(FILE*);
int putline(FILE*,Line *,int);

cursoroff(void)
{
	cursorswitch(&blank);
}

cursoron(void)
{
	cursorswitch(0);
}

uchar _dot[] = {
	0xe0, 0xa0, 0xe0
};
Blitmap dot = {(ulong *)_dot, 0, 1, 0, {0, 0, 3, 3}};

drawdot(void)
{
	static there;
	bitblt(stage,sub(dotp,Pt(1,1)),mapblit(&dot),dot.r,there?SUB:ADD);
	dombb(&minbb,raddp(dot.r,sub(dotp,Pt(1,1))));
	there = 1-there;
}

dogrid(void)
{
	grid.x = (mouse.xy.x+3)&~7;	/* this really will go away */
	grid.y = (mouse.xy.y+3)&~7;	/* honest! */
	if (showgrey)
		grid = mul(sub(grid,origin()),2);
}

getgrid(void)
{
	getmouse();
	dogrid();
	return mouse.buttons;
}

filter(Rectangle r)
{
	if (showgrey) {
		int i;
		r.min.x &= ~1;
		r.max.x = (r.max.x+1)&~1;
		r.min.y &= ~1;
		r.max.y = (r.max.y+1)&~1;
		/*if (r.min.x < 0)
			r.min.x = 0;
		if (r.min.y < 0)
			r.min.y = 0;
		if (r.max.x > (i=stage->r.max.x))
			r.max.x = i;
		if (r.max.y > (i=stage->r.max.y))
			r.max.y = i;
		filter01(&screen,add(origin(),div(r.min,2)),stage,r);
		minbb = MAXBB;*/
	}
}

clear(void)
{
	bitblt(&screen,screen.r.min,&screen,screen.r,0);
	if (showgrey)
		bitblt(stage,stage->r.min,stage,stage->r,0);
}

MyEvent *
getevent(void)
{
	Event e;
	static MyEvent me;
	switch (event(&e)) {
	case Ekeyboard:
		me.type = KBD;
		me.data[0] = e.kbdc;
		me.len = 1;
		break;
	case Emouse:
		while (ecanmouse())
			event(&e);
		me.type = MOUSE;
		mouse = e.mouse;
		break;
	}
	return &me;
}

getmouse(void)
{
	while (getevent()->type != MOUSE)
		;
}

void
ereshaped(Rectangle r)
{
	screen.r = inset(r,3);
	show();
}

main(int argc,char *argv[])
{
	char *fname;
	MyEvent *e;
	binit(0,0,"graw");
	/*if (screen.ldepth)
		showgrey = 1;*/
	einit(Emouse|Ekeyboard);
	screen.r = inset(screen.r,3);
	this = newfile(intern(""));
	files = (Dict *) calloc(1,sizeof(Dict));
	for (argv++; argc > 1 && argv[0][0] == '-'; argc--, argv++)
		switch (argv[0][1]) {
		case 'g':
			showgrey = 0;
			break;
		case 'f':
			fname = argv[1];
			argc--; argv++;
			break;
		}
	if (fname == 0)
		if (screen.ldepth == 0)
			fname = "/lib/font/bit/misc/latin1.6x10.font";
		else
			fname = "/lib/font/bit/courier/latin1.5.font";
	/* get option stuff out of the way before reading files */
	defont = font;
	if (tiny == 0 && (tiny = rdfontfile(fname,screen.ldepth)) == 0) {
		tiny = defont;
	}
	/*if (showgrey)
		stage = balloc(Rpt(Pt(0,0),mul(sub(screen.r.max,screen.r.min),2)),0);
	else*/
		stage = &screen;
	for (; argc > 1; argc--, argv++)
		editfile(argv[0]);
	show();
	while (1) {
		bflush();
		switch ((e = getevent())->type) {
		case MOUSE:
			dogrid();
			if (mouse.buttons&1)
				dobut1();
			else if (mouse.buttons&2)
				dobut2();
			else if (mouse.buttons&4)
				dobut3();
			else if (!eqpt(cursor,grid)) {
				if (ptinrect(grid,stage->r))
					docursor(0);
				else
					docursor(1);
				filter(minbb);
			}
			break;
		case KBD:
			typing(e->data);
			break;
		default:
			print("huh?");
			abort();
		}
	}
}

uchar cross[] = {
	0x80, 0x80, 0x41, 0x00, 0x22, 0x00, 0x14, 0x00, 0x08, 0x00,
	0x14, 0x00, 0x22, 0x00, 0x41, 0x00, 0x80, 0x80
};
uchar box[] = {
	0xff, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0xff, 0x80
};
uchar dots[] = {
	0x9c, 0x80, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x80, 0x9c, 0x80,
	0x9c, 0x80, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x80
};
Blitmap icon = {(ulong *) cross, 0, 1, 0, {0, 0, 9, 9}};

drawcursor(void)
{
	static there;
	if (!invis) {
		bitblt(stage,sub(cursor,Pt(4,4)),mapblit(&icon),icon.r,there?SUB:ADD);
		there=1-there;
		dombb(&minbb,raddp(icon.r,sub(cursor,Pt(4,4))));
	}
}

curswitch(uchar *x)
{
	drawcursor();
	icon.base = (ulong *) x;
	drawcursor();
}

docursor(int i)
{
	if (cursor.x == grid.x && cursor.y == grid.y)
		return;
	drawcursor();
	cursor = grid;	/* this only happens here */
	if (i)
		cursoron();
	else
		cursoroff();
	invis = i;
	drawcursor();
	moved = 1;
}

NMitem prims[] = {
/* 	"line", "lines", 0, 0, 0, 0, WIRE,	 */
	"box", "draw a box", 0, 0, 0, 0, BOX,
	"dots", "guess!", 0, 0, 0, 0, DOTS,
	"macro", "enclose a macro", 0, 0, 0, 0, MACRO,
/*	"measure", "a box that tells its size", 0, 0, 0, 0, MEASURE,	*/
	0
};
NMenu mprims = { prims };

/*
NMitem objs[] = {
	"define",	"sweep a box to define a new object",	0, 0, 0, 0, MASTER,
	"enter",	"edit the selected object",		0, 0, 0, 0, ENTER,
	"exit",		"update edited master",			0, 0, 0, 0, EXIT,
	0
};
NMenu mobjs = { objs };
 */

char *imenu[400];
NMitem item = {0, 0, 0, 0, 0, 0, INST};
NMitem *instgen(int i,NMitem *unused)
{
	item.text = imenu[i];
	return &item;
}
NMenu minst = { 0, &instgen};

NMitem toplevel[] = {
/*	"boxes",	"toggle drawing primitive",		0, 0, 0, 0, BOX, */
	"onesies",	"draw one something",			&mprims, 0, 0, 0, 0,
/*	"master",	"select object editing entry",		&mobjs, 0, 0, 0, 0,*/
	"inst",		"deposit an instance",			&minst, 0, 0, 0, 0,
	"sweep",	"selects items enclosed by a box",	0, 0, 0, 0, SWEEP,
	"slash",	"cut and snip wires too",		0, 0, 0, 0, SLASH,
	"cut",		"put selected items in buffer",		0, 0, 0, 0, CUT,
	"paste",	"paste copy of buffer",			0, 0, 0, 0, PASTE,
	"snarf",	"cut without deleting original",	0, 0, 0, 0, SNARF,
	"scroll",	"move viewing window",			0, 0, 0, 0, SCROLL,
	0
};
NMenu mtop = { toplevel };

enum {EDIT, READ, WRITE, EXIT, NEW};
char *fmenu[100] = {"edit", "read", "write", "exit", "new", 0};
Menu commands = {fmenu};

Thing type;

dobut1(void)
{
	int code,hits;
	drawdot();
	unselect(&DL,ALL);
	code = selpt(DL,grid);
	hits = code&ALLHIT;
	if (!moved || !code) {
		if (code && !moved)
			unselect(&DL,ALL);
		addathing(type);
	}
	else if ((hits & BOXHIT) && hits != BOXHIT)
		unselect(&DL,BOXHIT|ALLBOX);
	else if (code == (LINEHIT|LINEBOX|STRHIT|STRBOX))
		unselect(&DL,LINEHIT|LINEBOX|ALLBOX);
	drag(1,0);
	dotp = cursor;
	drawdot();
	snap(&DL);
	moved = 0;
}

typing(char *s)
{
	int hit;
	Line *l;
	unselect(&DL,ALL);
	if (((hit=selpt(DL,dotp))&STRHIT) == 0) {
		DL = newstring(dotp,textcode(DL,dotp),DL);
		drawstring((String *)DL,stage,ADD);
	}
	else if (*s == '\n') {	/* carriage return - tough call on non-manhattan lines */
		drawdot();
		dotp.y += ((hit & BOXHIT) ? GRID : 2*GRID)<<showgrey;
		drawdot();
		if (*++s != 0)
			typing(s);
	}

	if (hit&ALLBOX)
		unselect(&DL,ALLBOX);
	for (l = DL; l; l = l->next)
		if (l->type == STRING && l->sel) {
			addchars((String *)l,s);
			break;
		}
	filter(minbb);
	*s = 0;
}

show(void)
{
	Line *l;
	clear();
	cursor = Pt(-1,-1);
	for (l = DL; l; l = l->next)
		l->sel = 1;		/* goes away quickly */
	minbb = MAXBB;
	draw(DL,stage,ADD,1);
	filter(minbb);
	unselect(&DL,ALL);
	drawdot();
	alphkeys(this->d,imenu);
}

Point
and(Point a,Point b)
{
	a.x &= b.x;
	a.y &= b.y;
	return a;
}

dobut2(void)
{
	Line *l;
	NMitem *m;
	cursoron();
	m = mhit(&mtop,2,1);
	cursoroff();
	if (m) switch (m->data) {
	case LINE:
		type = LINE;
		curswitch(cross);
		break;
	case MACRO:
		type = MACRO;
		curswitch(box);
		break;
	case BOX:
		type = BOX;
		curswitch(box);
		break;
	case DOTS:
		type = DOTS;
		curswitch(dots);
		break;
	case INST:
		unselect(&DL,ALL);
		docursor(0);
		DL = newinst(grid,m->text,DL);
		drawinst((Inst *)DL,stage,ADD);
		filter(minbb);
		drag(0,0);
		snap(&DL);
		mbb(DL,this->d);
		while (getgrid())
			;
		break;
	case CUT:
		if ((l = cut(&DL)) != 0) {
			draw(l,stage,SUB,0);
			filter(minbb);
			removel(save);
			snap(&DL);
			save = l;
		}
		break;
	case PASTE:
		unselect(&DL,ALL);
		if ((l = copy(save)) != 0) {
			draw(l,stage,ADD,0);
			filter(minbb);
			append(&DL,l);
		}
		drag(0,0);
		snap(&DL);
		while (getgrid())
			;
		break;
	case SNARF:
		if ((l = copy(DL)) != 0) {
			removel(save);
			save = l;
		}
		break;
	case SWEEP:
		unselect(&DL,ALL);
		selbox(DL,mygetrect(1));
		if (IN(grid,stage->r)) {
			drag(0,0);
			snap(&DL);
		}
		break;
	case SLASH:
		unselect(&DL,ALL);
		slashbox(DL,gridrect(mygetrect(1)));
		if (IN(grid,stage->r)) {
			drag(0,0);
			snap(&DL);
		}
		break;
	case SCROLL:
		if (IN(grid,stage->r))
			drag(0,1);
		break;
	}
}

dobut3(void)
{
	int i;
	char buf[80],*name;
	FILE *fp;
	Line *l;
	File *t;
	Point p,q;
	cursoron();
	i = menuhit(3,&mouse,&commands);
	cursoroff();
	switch (i) {
	case -1:
		break;
	case EDIT:
		buf[0] = 0;
		if (*getline("edit file: ",buf) && (this = editfile(buf)))
			show();
		break;
	case READ:
		buf[0] = 0;
		if (*getline("read file: ",buf) && readfile(buf)) {
			addaref(&DL,intern(buf));
			initfile(this);
			show();
		}
		break;
	case WRITE:
		strcpy(buf,this->name);
		if (*getline("write file: ",buf) && (fp = fopen(buf,"w")) != 0) {
			if (strcmp(buf,this->name) != 0) {	/* could have changed */
				if (lookup(this->name,files)) {
					this->name = intern(buf);
					rehash(files,files->n);
				}
				else {
					this->name = intern(buf);
					assign(this->name,this,files);
				}
				alphkeys(files,&fmenu[NEW+1]);
			}
			putline(fp,DL,0);
			putline(fp,DL,1);
			this->date = mtime(fileno(fp));
			fflush(fp);
			fclose(fp);
		}
		else
			complain("can't write ",buf);
		break;
	case EXIT:
		if (*getline("really quit? ",buf) == 'y')
			exits(0);
	case NEW:
		this = newfile(intern(""));
		show();
		filter(screen.r);
		break;
	default:		/* must be a file name */
		this = (File *) lookup(fmenu[i],files);
		show();
		break;
	}
}

File *
editfile(char *s)
{
	FILE *fp;
	Point p,q;
	Line *l=0;
	if ((fp = fopen(s,"r")) != 0) {
		if ((this = (File *) lookup(intern(s),files)) == 0) {
			this = newfile(intern(s));
			assign(this->name,this,files);
		}
		if (l = inlist(fp)) {
			this->date = mtime(fileno(fp));
			free(this->dl);
			this->dl = l;
			initfile(this);
			if (!showgrey) {
				p = origin();
				q = and(Pt(~GRIDMASK<<showgrey,~GRIDMASK<<showgrey),mbb(DL,this->d).min);
				allmove(DL,sub(p,q));
			}
			alphkeys(files,&fmenu[NEW+1]);
		}
		 fclose(fp);
	}
	else
		complain("can't edit ",s);
	return this;
}

readfile(char *s)
{
	FILE *fp;
	char *name,buf[80];
	File *t;
	sprint(buf,"/lib/graw/%s",s);
	if ((fp = fopen(s,"r")) != 0 || (fp = fopen(buf,"r")) != 0) {
		t = this;
		name = intern(s);
		if ((this = (File *) lookup(name,files)) == 0) {
			this = newfile(name);
			assign(name,this,files);
		}
		if (this->date < mtime(fileno(fp))) {
			this->dl = inlist(fp);
			this->date = mtime(fileno(fp));
			initfile(this);
		}
		this = t;
		fclose(fp);
		return 1;
	}
	else {
		complain("can't read ",s);
		return 0;
	}
}

drag(int but,int all)
{
	for (; mouse.buttons == but; getgrid()) {
		if (grid.x == cursor.x && grid.y == cursor.y)
			continue;
		draw(DL,stage,SUB,all);
		getgrid();
		if (all)
			allmove(DL,sub(grid,cursor));
		else
			move(DL,sub(grid,cursor));
		docursor(0);
		draw(DL,stage,ADD,all);
		filter(minbb);
	}
	type = LINE;
	curswitch(cross);
}

addathing(Thing t)
{
	switch (t) {
	case LINE:
		DL = newline(grid,DL);
		break;
	case BOX:
		DL = newbox(grid,DL);
		break;
	case DOTS:
		DL = newdots(grid,DL);
		break;
	case MACRO:
		DL = newmacro(grid,DL);
		break;
	case STRING:
		DL = newstring(grid,0,DL);
		break;
	}
	draw(DL,stage,ADD,0);
	filter(minbb);
}
