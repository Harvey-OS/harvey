# include </sys/include/stdio.h>
#ifndef BSD
#define getdtablesize()	(FOPEN_MAX)
#endif
