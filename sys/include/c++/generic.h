#ifndef GENERICH
#define GENERICH 1
#pragma lib "ape/libap.a"
#pragma lib "c++/libC.a"

#define __name2(a,b)		a ## b
#define __name3(a,b,c)		a ## b ## c
#define __name4(a,b,c,d)	a ## b ## c ## d
#define name2(a,b)		__name2(a,b)
#define name3(a,b,c)		__name3(a,b,c)
#define name4(a,b,c,d)		__name4(a,b,c,d)

#define declare(a,t) name2(a,declare)(t)
#define implement(a,t) name2(a,implement)(t)
#define declare2(a,t1,t2) name2(a,declare2)(t1,t2)
#define implement2(a,t1,t2) name2(a,implement2)(t1,t2)


extern genericerror(int,char*);
typedef int (*GPT)(int,char*);
#define set_handler(generic,type,x) name4(set_,type,generic,_handler)(x)
#define errorhandler(generic,type) name3(type,generic,handler)
#define callerror(generic,type,a,b) (*errorhandler(generic,type))(a,b)

#endif
