
#ifndef GENERICH
#include <generic.h>
#endif

#define vector(type) name2(type,vector)
#define vectordeclare(type)							\
extern GPT errorhandler(vector,type);						\
extern GPT set_handler(vector,type,GPT);					\
class vector(type) {								\
	type* v;								\
	int sz;									\
public:										\
	vector(type)(int s)							\
	{	if (s<=0) callerror(vector,type,1,"bad vector size");		\
		v = new type[sz=s];						\
	}									\
	~vector(type)() { delete[sz] v; }					\
	vector(type)(vector(type)&);						\
	vector(type)& operator=(vector(type)&);					\
	int size() { return sz; }						\
	void set_size(int);							\
	type& elem(int i) { return v[i]; }					\
	type& operator[](int i)							\
	{	if (i<0 || sz<=i)						\
			callerror(vector,type,2,"vector index out of range");	\
		return v[i];							\
	}									\
};

#define vectorimplement(type)							\
GPT errorhandler(vector,type) = genericerror;					\
vector(type)::vector(type)(vector(type)& a)					\
{										\
	register i = a.sz;							\
	sz = i;									\
	v = new type[i];							\
	register type* vv = &v[i];						\
	register type* av = &a.v[i];						\
	while (i--) *--vv = *--av;						\
}										\
										\
vector(type)& vector(type)::operator=(vector(type)& a)				\
{										\
	register i = a.sz;							\
	if (i != sz)								\
		callerror(vector,type,3,"different vector sizes in assignment");\
	register type* vv = &v[i];						\
	register type* av = &a.v[i];						\
	while (i--) *--vv = *--av;						\
	delete[i] v;								\
	return *this;								\
}										\
										\
void vector(type)::set_size(int s)						\
{										\
	if (s<=0) callerror(vector,type,4,"bad new vector size");		\
	type* nv = new type[s];							\
	register i = (s<=sz)?s:sz;						\
	register type* vv = &v[i];						\
	register type* av = &nv[i];						\
	while (i--) *--vv = *--av;						\
	delete[sz] v;								\
	v = nv;									\
	sz = s;									\
}										\
										\
GPT set_handler(vector,type, GPT a)						\
{										\
	GPT oo = errorhandler(vector,type);					\
	errorhandler(vector,type) = a;						\
	return oo;								\
}
	
	

#define stack(type) name2(type,stack)

#define stackdeclare(type)							\
extern GPT errorhandler(stack,type);						\
extern GPT set_handler(stack,type,GPT);						\
class stack(type) : vector(type) {						\
	int t;									\
public:										\
	stack(type)(int s) : (s) { t = 0; }					\
	stack(type)(stack(type)& a) : ((vector(type)&)a) { t = a.t; }		\
	void push(type& a)							\
	{	if (t==size()-1) callerror(stack,type,1,"stack overflow");	\
		elem(++t) = a;							\
	}									\
	type pop()								\
	{	if (t==0) callerror(stack,type,2,"stack underflow");		\
		return elem(t--);						\
	}									\
	type& top()								\
	{	if (t==0) callerror(stack,type,3,"stack empty");		\
		return elem(t);							\
	}									\
};

#define stackimplement(type)							\
GPT errorhandler(stack,type);							\
GPT set_handler(stack,type, GPT a)						\
{										\
	GPT oo = errorhandler(stack,type);					\
	errorhandler(stack,type) = a;						\
	return oo;								\
}
