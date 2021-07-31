#ifndef __STDARG
#define __STDARG

typedef char *va_list;

#define va_start(list, start) list = (char *)(&(start)+1)
#define va_end(list)
#define va_arg(list, mode)\
	((sizeof(mode) <= 4)?\
		((list += 4), (mode*)list)[-1]:\
	(signof(mode) != signof(double))?\
		((list += sizeof(mode)), (mode*)list)[-1]:\
		((list = (char*)((unsigned long)(list+7) & ~7) + sizeof(mode)), (mode*)list)[-1])

#endif /* __STDARG */
