struct _mem {
        struct _mem *next;
        int size;       /* size of individual elements in bytes */
        int blkf;       /* blocking factor */
        int bsize;      /* block size */
        char *block;    /* block */
        int cnt;        /* count of free elem in block */
        char *freelist; /* free list */
        int totelem;    /* total number elements we have */
        int freecnt;    /* number of times mem_free was called */
};
void	mem_init(struct _mem *, int);
void	*mem_get(struct _mem *);
void	mem_free(struct _mem *, char *);
int	mem_outstanding(struct _mem *);
void	mem_epilogue(void);
