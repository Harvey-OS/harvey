int sys_set_tid_address(int *tidptr);

u64int linux_mmap(u64int, u64int, u64int, u64int, u64int, u64int);
u64int linux_munmap(u64int, u64int);

/* trace */
void inittrace(void);
void exittrace(void);
void tprint(char *fmt, ...);
#pragma varargck argpos tprint 1
#define trace if(debug)tprint

uintptr calllinuxnoted(int);

