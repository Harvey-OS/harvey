#ifndef BOX_H
#define BOX_H

class ConnList;
class BoxList;
class Macro;

class Box:public Thing {
public:
	Rectangle R;
	Text *part;
	char *suffix;		/* index of suffix in part */
	ConnList *pins;
	Box();
	Box(Rectangle);
	inline ~Box();
	Rectangle mbb(Rectangle r)	{return r.mbb(R);}
	int namepart(Text *t);
	int namepin(Text *t);
	int namekahrs(Text *t);
	virtual void put(FILE *);
	void putm(BoxList *,FILE *);
	void getname(WireList *);
};

class BoxList:public Vector {
public:
	BoxList(int nn):Vector(nn)	{}
	Box *box(int i)	{return (Box *) a[i];}
	int parts(Text *t);
	int pins(Text *t);
	int kahrs(Text *t);
	void nets(WireList *);
	void expand(Vector *);
	void absorb(BoxList *);
	void inside(Macro *);
};

class Macro:public Box {
public:
	BoxList *b;
	Macro(Rectangle);
	inline ~Macro();
	Box *box(int i) {return (Box *) b->box(i);}
	void put(FILE *);
};
#endif
