struct tbl_entry {
	struct tbl_entry *next;		/* free list */
	int *data;			/* data */
};

struct table {
	int size;			/* # entries */
	struct tbl_entry *entries;	/* the entries */
	struct tbl_entry *free;		/* the free list */
};

void *tbl_alloc(struct table *, int);
void tbl_free(struct table *, void *);
int tbl_index(struct table *, void *);
void *tbl_data(struct table *, int);
