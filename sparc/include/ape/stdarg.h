#ifndef __STDARG
#define __STDARG

typedef char *va_list;

#define va_start(list, start) list = (char *)(&(start)+1)
#define va_end(list)
#define va_arg(list, mode) (sizeof(mode)==1 ? ((mode *) (list += 4))[-4] : \
sizeof(mode)==2 ? ((mode *) (list += 4))[-2] : ((mode *) (list += sizeof(mode)))[-1])

#endif /* __STDARG */
