#ifndef __STDARG
#define __STDARG

typedef char *va_list;

#define va_start(list, start) list = (sizeof(start)<8 ? (char *)((long long *)&(start)+1) : \
(char *)(&(start)+1))
#define va_end(list)
#define va_arg(list, mode)\
	((sizeof(mode) == 1)?\
		((mode*)(list += 8))[-8]:\
	(sizeof(mode) == 2)?\
		((mode*)(list += 8))[-4]:\
	(sizeof(mode) == 4)?\
		((mode*)(list += 8))[-2]:\
		((mode*)(list += sizeof(mode)))[-1])

#endif /* __STDARG */
