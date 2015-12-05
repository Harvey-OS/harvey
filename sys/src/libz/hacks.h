#  define z_off_t int64_t
#  define exit(n) exits("error: " #n)
#  define lseek seek

#  define O_RDONLY OREAD
#  define O_WRONLY OWRITE
#  define O_RDWR ORDWR
#  define O_ACCMODE	0x003
#  define O_NONBLOCK	0x004
#  define O_APPEND	0x008
#  define O_CREAT	0x100
#  define O_TRUNC	0x200
#  define O_EXCL	0x400

extern int uopen(const char* path, int mode, int mask);
