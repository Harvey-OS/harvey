#ifndef __STDARG
#define __STDARG

typedef	unsigned long long va_list;

/* stdarg - little-endian 64-bit */
/*
 * types narrower than long are widened to long then pushed.
 * types wider than long are pushed on vlong alignment boundaries.
 */
#define va_start(list, start) list =\
	(sizeof(start) < 4?\
		(va_list)((long*)&(start)+1):\
		(va_list)(&(start)+1))
#define va_end(list)\
	USED(list)
#define va_arg(list, mode)\
	((sizeof(mode) == 1)?\
		((list += 4), (mode*)list)[-4]:\
	(sizeof(mode) == 2)?\
		((list += 4), (mode*)list)[-2]:\
	(sizeof(mode) == 4)?\
		((list += 4), (mode*)list)[-1]:\
		((mode*)(list = (list + sizeof(mode)+7) & ~7))[-1])

#endif /* __STDARG */
