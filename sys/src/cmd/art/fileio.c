/*
 * commands to read and write files
 */
#include "art.h"
int readerr;
Flt getnum(Biobuf *f){	/* Fgetd would be fine but for error checking */
	int c;
	char numbuf[500], *nump=numbuf;
	do
		c=Bgetc(f);
	while(c==' ' || c=='\t' || c=='\n');
	if(c=='-'){
		*nump++=c;
		c=Bgetc(f);
	}
	if(c<'0' || '9'<c && c!='.'){
		readerr++;
		return 0;
	}
	do{
		*nump++=c;
		c=Bgetc(f);
	}while('0'<=c && c<='9' || c=='.');
	*nump='\0';
	return atof(numbuf);
}
char gsbuf[513];
char *getstring(Biobuf *f, int end){
	char *p=gsbuf;
	int c;
	while((c=Bgetc(f))!=end && c>=0) if(p!=&gsbuf[512])*p++=c;
	*p='\0';
	return gsbuf;
}
Dpoint getpt(Biobuf *f){
	Dpoint p;
	p.x=getnum(f);
	p.y=getnum(f);
	return p;
}
void input(char *name){
	Biobuf *f=Bopen(name, OREAD);
	int c, i, n, style;
	Dpoint p0, p1, p2, p[200];
	Flt r;
	Item *inp, *last;
	Typeface *svface;
	if(f==0){
		msg("Can't open %s", name);
		return;
	}
	ngrp=0;
	for(i=0;i!=NGROUP;i++) if(group[i]==0) grpmap[ngrp++]=i;
	readerr=0;
	inp=scene;
	while((c=Bgetc(f))>=0) switch(c){
	case ' ':
	case '\t':
	case '\n':
		break;
	default:
		msg("Ill-formed file");
	case '#':
		do
			c=Bgetc(f);
		while(c!='\n' && c>=0);
		break;
	case 'l':
		p0=getpt(f); if(readerr) goto Out;
		p1=getpt(f); if(readerr) goto Out;
		last=addline(inp, p0, p1);
		break;
	case 'b':
		p0=getpt(f); if(readerr) goto Out;
		p1=getpt(f); if(readerr) goto Out;
		last=addbox(inp, p0, p1);
		break;
	case 'c':
		p0=getpt(f); if(readerr) goto Out;
		r=getnum(f); if(readerr) goto Out;
		last=addcircle(inp, p0, r);
		break;
	case 'a':
		p0=getpt(f); if(readerr) goto Out;
		p1=getpt(f); if(readerr) goto Out;
		p2=getpt(f); if(readerr) goto Out;
		last=addarc(inp, p0, p1, p2);
		break;
	case 'g':
		i=getnum(f); if(readerr) goto Out;
		if(i<0 || ngrp<=i) goto Out;
		p0=getpt(f); if(readerr) goto Out;
		last=addgroup(inp, grpmap[i], p0);
		break;
	case 's':
		n=getnum(f); if(readerr) goto Out;
		if(n<2 || 200<n) goto Out;
		for(i=0;i!=n;i++){
			p[i]=getpt(f);
			if(readerr) goto Out;
		}
		last=additemv(inp, SPLINE, 0., 0, 0, 0, &splinefns, n, p, Dpt(0,0));
		break;
	case 't':
		p0=getpt(f); if(readerr) goto Out;
		svface=curface;
		setface(getstring(f, ' '));
		last=addtext(inp, p0, curface, getstring(f, '\n'));
		curface=svface;
		break;
	case 'G':
		i=getnum(f); if(readerr) goto Out;
		if(i<0 || ngrp<=i) goto Out;
		if(group[grpmap[i]]) goto Out;
		last=inp=group[grpmap[i]]=addhead();
		break;
	case ';':
		last=inp=scene;
		break;
	case 'S':
		style=0;
		do c=Bgetc(f); while(c>=0 && c==' ' || c=='\t');
		while(c>=0 && strchr("<>.-", c)){
			switch(c){
			case '<': style|=ARROW0; break;
			case '>': style|=ARROW1; break;
			case '-': style|=DASH; break;
			case '.': style|=DOT; break;
			}
			c=Bgetc(f);
		}
		last->style=style;
	}
Out:
	redraw();
	Bterm(f);
}
int grpmap[NGROUP];
int ngrp;
void output(char *name){
	int i, j;
	int f=create(name, OWRITE|OTRUNC, 0666);
	Item *p;
	if(f==-1){
		msg("Can't create %s", name);
		return;
	}
	ngrp=0;
	for(i=0;i!=NGROUP;i++) grpmap[i]=-1;
	mapgrps(scene);
	for(i=0;i!=ngrp;i++){
		for(j=0;j!=NGROUP && grpmap[j]!=i;j++);
		if(j==NGROUP)
			msg("write: grpmap phase error");
		else{
			fprint(f, "G %d\n", i);
			(*group[j]->fn->write)(group[j], f);
			fprint(f, ";\n");
		}
	}
	(*scene->fn->write)(scene, f);
	close(f);
}
void mapgrps(Item *ip){
	if(ip->type==HEAD)
		for(ip=ip->next;ip->type!=HEAD;ip=ip->next)
			mapgrps(ip);
	else if(ip->type==GROUP && goodgroup(ip->group) && grpmap[ip->group]==-1){
		grpmap[ip->group]=ngrp++;
		mapgrps(group[ip->group]);
	}
}
