#ifndef MASTER_H
#define MASTER_H

class Master:public Thing {
public:
	String *s;
	BoxList *boxes;
	BoxList *macros;	/* only one, but Lists fit in best */
	WireList *wires;
	TextList *labels;
	int hasmbb;		/* inhibit recursion */
	Rectangle bb;
	Master(String *);
	~Master();
	int eq(Thing *);
	Rectangle findmbb();
	void process();
	int get(FILE *f);
};

class Inst:public Box {
public:
	String *s;
	Master *m;
	Inst(Rectangle r):Box(r)	{}
	Inst(String *, Point);
	Rectangle mbb(Rectangle);
	void put(FILE *);
	void expand(Vector *);
};
#endif
