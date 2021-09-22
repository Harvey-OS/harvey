typedef	char*	va_list;
#define va_start(list, start) list = (char*)&start + 4
#define va_end(list)
#define va_arg(list, mode)\
	(sizeof(mode)==1?\
		((mode*)(list += 4))[-4]:\
	sizeof(mode)==2?\
		((mode*)(list += 4))[-2]:\
	sizeof(mode)>4?\
		((mode*)(list = (char*)((long)(list + 7) & ~7) + sizeof(mode)))[-1]:\
		((mode*)(list += 4))[-1])
