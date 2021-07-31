#include <u.h>
#include <libc.h>
#include <libg.h>
#include <panel.h>
#include "pldefs.h"
typedef struct Message Message;
struct Message{
	char *text;
	Point minsize;
};
void pl_textmsg(Bitmap *b, Rectangle r, Font *f, char *s){
	char *start, *end;	/* of line */
	Point where;
	int lwid, c, wid;
	where=r.min;
	wid=r.max.x-r.min.x;
	do{
		start=s;
		lwid=0;
		end=s;
		do{
			for(;*s!=' ' && *s!='\0';s=pl_nextrune(s)) lwid+=pl_runewidth(f, s);
			if(lwid>wid) break;
			end=s;
			for(;*s==' ';s=pl_nextrune(s)) lwid+=pl_runewidth(f, s);
		}while(*s!='\0');
		if(end==start)	/* can't even fit one word on line! */
			end=s;
		c=*end;
		*end='\0';
		string(b, where, f, start, S|D);
		*end=c;
		where.y+=font->height;
		s=end;
		while(*s==' ') s=pl_nextrune(s);
	}while(*s!='\0');
}
Point pl_foldsize(Font *f, char *s, int wid){
	char *start, *end;	/* of line */
	Point size;
	int lwid, ewid;
	size=Pt(0,0);
	do{
		start=s;
		lwid=0;
		end=s;
		ewid=lwid;
		do{
			for(;*s!=' ' && *s!='\0';s=pl_nextrune(s)) lwid+=pl_runewidth(f, s);
			if(lwid>wid) break;
			end=s;
			ewid=lwid;
			for(;*s==' ';s=pl_nextrune(s)) lwid+=pl_runewidth(f, s);
		}while(*s!='\0');
		if(end==start){	/* can't even fit one word on line! */
			ewid=lwid;
			end=s;
		}
		if(ewid>size.x) size.x=ewid;
		size.y+=font->height;
		s=end;
		while(*s==' ') s=pl_nextrune(s);
	}while(*s!='\0');
	return size;
}
void pl_drawmessage(Panel *p){
	pl_textmsg(p->b, pl_box(p->b, p->r, PASSIVE), font, ((Message *)p->data)->text);
}
int pl_hitmessage(Panel *g, Mouse *m){
	USED(g, m);
	return 0;
}
void pl_typemessage(Panel *g, Rune c){
	USED(g, c);
}
Point pl_getsizemessage(Panel *p, Point children){
	Message *mp;
	USED(children);
	mp=p->data;
	return pl_boxsize(pl_foldsize(font, mp->text, mp->minsize.x), PASSIVE);
}
void pl_childspacemessage(Panel *p, Point *ul, Point *size){
	USED(p, ul, size);
}
void plinitmessage(Panel *v, int flags, int wid, char *msg){
	Message *mp;
	mp=v->data;
	v->flags=flags|LEAF;
	v->draw=pl_drawmessage;
	v->hit=pl_hitmessage;
	v->type=pl_typemessage;
	v->getsize=pl_getsizemessage;
	v->childspace=pl_childspacemessage;
	mp->text=msg;
	mp->minsize=Pt(wid, font->height);
	v->kind="message";
}
Panel *plmessage(Panel *parent, int flags, int wid, char *msg){
	Panel *v;
	v=pl_newpanel(parent, sizeof(Message));
	plinitmessage(v, flags, wid, msg);
	return v;
}
