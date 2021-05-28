#ifndef __STDARG
#define __STDARG

typedef	unsigned long long va_list;

/* stdarg - little-endian 64-bit */
#define va_start(list, start) list =\
	(sizeof(start) < 4?\
		(unsigned long long)((long*)&(start)+1):\
		(unsigned long long)(&(start)+1))
#define va_end(list)\
	USED(list)
#define va_arg(list, mode)\
	((sizeof(mode) == 1)?\
		((list += 4), (mode*)list)[-4]:\
	(sizeof(mode) == 2)?\
		((list += 4), (mode*)list)[-2]:\
	(sizeof(mode) == 4)?\
		((list += 4), (mode*)list)[-1]:\
		((list += sizeof(mode)+7), (list &= ~7), (mode*)list)[-1])

#endif /* __STDARG */
