#ifndef __STDARG
#define __STDARG

typedef char *va_list;

#define va_start(list, start) list = (char *)(&(start)+1)
#define va_end(list)
#define va_arg(list, mode)\
	(sizeof(mode)==1?\
		((mode*)(list += 4))[-1]:\
	sizeof(mode)==2?\
		((mode*)(list += 4))[-1]:\
	sizeof(mode)>4?\
		((mode*)(list = (char*)((long)(list+7) & ~7) + sizeof(mode)))[-1]:\
		((mode*)(list += sizeof(mode)))[-1])

#endif /* __STDARG */
