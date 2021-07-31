/*
 * Command support & miscellaneous commands
 */
#include "art.h"
char *buttons2[]={
	"line",
	"circle",
	"arc",
	"box",
	"spline",
	"group",
	0
};
Menu menu2={buttons2};
char *buttons3[]={
	"delete",
	"heat",
	"copy",
	"edit",
	"open",
	"close",
	"flatten",
	0
};
Menu menu3={buttons3};
int narg;
Dpoint arg[NARG];
/*
 * body of command loop
 */
void command(void){
	for(;;){
		getmouse();
		if(button123() && hitmenubar()==-1){
			if(button1()) getarg();
			else if(button2()){
				switch(menuhit(2, &mouse, &menu2)){
				case 0: Oline(); break;
				case 1: Ocircle(); break;
				case 2: Oarc(); break;
				case 3: Obox(); break;
				case 4: Ospline(); break;
				case 5: Ogroup(); break;
				}
			}
			else if(button3()){
				switch(menuhit(3, &mouse, &menu3)){
				case 0: Odelete(); break;
				case 1: Oheat(); break;
				case 2: Ocopy(); break;
				case 3: Oedit(); break;
				case 4: Oopengrp(); break;
				case 5: Oclosegrp(); break;
				case 6: Oflattengrp(); break;
				}
			}
		}
	}
}
Cursor confirm3 = {
0, 0,
0x0F, 0xBF,
0x0F, 0xBF,
0xFF, 0xFF,
0xFF, 0xFF,
0xFF, 0xFF,
0xFF, 0xFE,
0xFF, 0xFE,
0xFF, 0xFE,
0xFF, 0xFE,
0xFF, 0xFF,
0x00, 0x07,
0xFF, 0xFF,
0xFF, 0xFF,
0xFF, 0xFF,
0xFF, 0xFE,
0xFF, 0xF8,

0x00, 0x0E,
0x07, 0x1F,
0x03, 0x17,
0x73, 0x6F,
0xFB, 0xCE,
0xDB, 0x8C,
0xDB, 0xC0,
0xFB, 0x6C,
0x77, 0xFC,
0x00, 0x00,
0x00, 0x01,
0x00, 0x03,
0x94, 0xA6,
0x63, 0x3C,
0x63, 0x18,
0x94, 0x90,
};
int confirm(char *action){
	int b;
	msg("button 3 to %s", action);
	cursorswitch(&confirm3);
	while(!button123()) getmouse();
	cursorswitch((Cursor *)0);
	msg(cmdname);
	b=button3();
	while(button123()) getmouse();
	return b;
}
void Oexit(void){
	if(confirm("exit")) exits("");
}
int curgrav=6;
char *mgravity[]={
	"gravity",
	" off",
	" .01",
	" .02",
	" .03",
	" .05",
	" .08",
	"*.13",
	" .21",
	" .34",
	0
};
int gval[]={0, 1, 2, 3, 5, 8, 13, 21, 34};
void Ogravity(int n){
	gravity=gval[n]/DPI;
	mgravity[curgrav+1][0]=' ';
	curgrav=n;
	mgravity[curgrav+1][0]='*';
}
char *mheating[]={
	"heating",
	"*heat new",
	0
};
int heatnew=1;
void Oheating(int n){
	switch(n){
	case 0:
		heatnew=!heatnew;
		mheating[1][0]=heatnew?'*':' ';
		break;
	}
}
void getarg(void){
	track(movenone, 0, 0);
	argument();
}
void argument(void){
	register i;
	marks();
	if(narg!=NARG) narg++;
	for(i=narg-1;i!=0;--i) arg[i]=arg[i-1];
	arg[0]=curpt;
	marks();
	msg(cmdname);
}
void Oselall(void){
	setselection(scene);
}
void Oanchor(void){
	marks();
	anchor=curpt;
	marks();
}
Scsave *scsp=scstack;
void Oopengrp(void){
	int i, g;
	Dpoint soffs;
	if(scsp==&scstack[NSCSTACK])
		msg("open: stack overflow");
	else if(!selection || selection->type!=GROUP || !goodgroup(selection->group))
		msg("open: must select a group");
	else{
		scsp->scene=scene;
		scsp->scoffs=scoffs;
		scsp++;
		g=selection->group;
		soffs=selection->p[0];
		setselection(0);
		scene=group[g];
		scoffs=dadd(scoffs, soffs);
		curpt=dsub(curpt, soffs);
		for(i=0;i!=narg;i++) arg[i]=dsub(arg[i], soffs);
	}
}
void Oclosegrp(void){
	Dpoint delta;
	int i;
	if(scsp==&scstack[0])
		msg("close: nothing open");
	else{
		setselection(0);
		--scsp;
		delta=dsub(scoffs, scsp->scoffs);
		curpt=dadd(curpt, delta);
		for(i=0;i!=narg;i++) arg[i]=dadd(arg[i], delta);
		scene=scsp->scene;
		scoffs=scsp->scoffs;
		redraw();
	}
}
void Oflattengrp(void){
	Item *ip;
	Dpoint offs;
	if(!selection || selection->type!=GROUP || !goodgroup(selection->group)){
		msg("flatten: must select a group");
		return;
	}
	offs=dadd(selection->p[0], scoffs);
	for(ip=group[selection->group]->next;ip->type!=HEAD;ip=ip->next)
		additemv(scene, ip->type, ip->r, ip->face, ip->text, ip->group, ip->fn,
			ip->np, ip->p, offs);
	ip=selection;
	setselection(0);
	delete(ip);
}
void Odelete(void){
	Item *s=selection;
	if(s){
		if(s==scene && !confirm("delete everything")) return;
		setselection(0);
		delete(s);
		if(s==scene) scene=addhead();
		redraw();
		reheat();
		realign();
		setselection(select());
	}
	else msg("delete: nothing selected");
}
Rune rcmd[]={1};
Rune *rcmdp=rcmd;
void typein(int c){
	switch(c){
	case '\n':
	case '\r':
		*rcmdp='\0';
		sprint(cmd, "%S", rcmd);
		typecmd();
	case '@':
		rcmdp=rcmd;
		*rcmdp='\0';
		break;
	case '\b':
		if(rcmdp!=rcmd)
			*--rcmdp='\0';
		break;
	case 027:	/* ctrl-w */
		while(rcmdp!=rcmd && !idchar(rcmdp[-1]))
			--rcmdp;
		while(rcmdp!=rcmd && idchar(rcmdp[-1]))
			--rcmdp;
		*rcmdp='\0';
		break;
	default:
		*rcmdp++=c;
		*rcmdp='\0';
		break;
	}
	rcmdp[0]='\1';
	rcmdp[1]='\0';
	sprint(cmd, "%S", rcmd);
	echo(cmd);
}
int idchar(int c){
	return 'a'<=c && c<='z' || 'A'<=c && c<='Z' || '0'<=c && c<='9' || c=='_' || c>0177;
}
char filename[NCMD];
void typecmd(void){
	register char *a=cmd;
	register c;
	Item *s;
	while(*a==' ' || *a=='\t') a++;
	c=*a++;
	if(*a=='\0') a=0;
	else if(*a!=' ' && *a!='\t'){
	What:
		msg("what?");
		return;
	}
	else{
		while(*a==' ' || *a=='\t') a++;
		if(*a=='\0') goto What;
	}
	switch(c){
	default:
		goto What;
	case 'f':				/* font select */
		if(a) setface(a);
		msg(curface->name);
		break;
	case 'D':				/* draw a clean display */
		redraw();
		break;
	case 'q':				/* quit */
		if(a) goto What;
		rectf(&screen, inset(screen.r, 3), Zero); /* width of 8.5 border is 3 */
		exits("");
	case 'r':				/* read a file */
		if(a) strcpy(filename, a);
		if(filename[0]=='\0') goto What;
		input(filename);
		break;
	case 'w':				/* write a file */
		if(a) strcpy(filename, a);
		if(filename[0]=='\0') goto What;
		output(filename);
		break;
	case 'c':
		if(a) goto What;
		Ocoolall();
		break;
	case 'a':
		if(a) goto What;
		Oselall();
		break;
	case 'd':
		if(a) goto What;
		Oanchor();
		break;
	case 't':				/* text */
		if(!a) goto What;
		s=addtext(scene, curpt, curface, a);
		draw(s, scoffs, DARK, S|D);
		setselection(s);
		break;
	}
}
