#ifndef SYSV
extern int yydebug;
#endif

void srand(long);
long time(long *);
int rand(void);

char *library = "/usr/cda/lib/library.paddle";

#define GOODEND exit(0)
#define BADEND exit(1)

#define create($1,$2,$3) creat($1,$3)
