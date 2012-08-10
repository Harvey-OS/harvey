typedef struct Uproc Uproc;
typedef struct Linuxstat Linuxstat;

struct Uproc {
	int		pid;
	int		tid;
	int		*cleartidptr;
};

struct Linuxstat {
        int     st_dev;     /* ID of device containing file */
        int     st_ino;  /* inode number */
        int     st_mode;        /* protection */
        int     st_nlink;   /* number of hard links */
        int     st_uid;  /* user ID of owner */
        int     st_gid;  /* group ID of owner */
        int     st_rdev;        /* device ID (if special file) */
        u64int  st_size;        /* total size, in bytes */
        int     st_blksize; /* blocksize for file system I/O */
        int     st_blocks;  /* number of 512B blocks allocated */
        int     st_atime;   /* time of last access */
        int     st_mtime;   /* time of last modification */
        int     st_ctime;   /* time of last status change */
};


Uproc uproc;

extern int debug;
