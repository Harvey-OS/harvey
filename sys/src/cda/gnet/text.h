#ifndef TEXT_H
#define TEXT_H

class Text:public Thing {
public:
	Point p;
	String *s,*t;
	int used,style;
	Text(char *ss,char *tt)	{s = new String(ss); t = new String(tt);}
	Text(Point pp, String *ss, String *tt, int st)	{p = pp; s = ss; t = tt; used = 0; style = st;}
	~Text()		{delete s; delete t;}
	void merge(Text *);
};

class TextList:public Vector {
public:
	TextList(int nn):Vector(nn)	{}
	Text *text(int i)	{return (Text *) a[i];}
	void walk(Vector *, int (*)(...));	/* the workhorse */
	int prop(Text *);
};
#endif
