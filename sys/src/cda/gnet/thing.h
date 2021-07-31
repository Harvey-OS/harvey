#ifndef THING_H
#define THING_H

class Vector;

class Thing {
public:
	Thing()	{}
	virtual int operator&(Rectangle)	{return 0;}
	virtual int eq(Thing *t)	{return this==t;}
	virtual void put(FILE *ouf)	{fprintf(ouf,"Thing::put\n");}
	virtual void draw()	{printf("Thing::draw\n");}
	virtual Rectangle mbb(Rectangle r)	{return r;}
	virtual void expand(Vector *)	{}
};

class String:public Thing {
public:
	char *s;
	String();
	~String()	{delete s;}
	String(char *ss);
	String *append(char *ss);
	int eq(Thing *);
	virtual void put(FILE *ouf)	{fprintf(ouf,"String(\"%s\")::put\n",s);}
	virtual void draw()	{printf("String(\"%s\")::draw\n",s);}
};

class Vector:public Thing {
public:
	Thing **a;
	int n,N;
	Vector(int nn)	{n = 0; N = nn; a = new Thing*[N];}
	void clear();
	~Vector()	{clear(); delete a;}
	Thing **operator[](int i)	{return &a[i];}
	Thing *lookup(Thing *);
	Thing *append(Thing *t);
	Thing *remove(int);
	void exchg(int, int);
	void put(FILE *);
	Rectangle mbb(Rectangle);
	void apply(int *, int (*)(...));
};
#endif
