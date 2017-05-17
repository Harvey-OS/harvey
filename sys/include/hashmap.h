
typedef struct Hashentry Hashentry;
typedef struct Hashtable Hashtable;
typedef struct Hashmap Hashmap;

struct Hashentry {
	uint64_t key;
	uint64_t val;
};

struct Hashtable {
	Hashentry *tab;
	size_t len;
	size_t cap; // always a power of 2.
};

struct Hashmap {
	Hashtable *cur;
	Hashtable *next;
	Hashtable tabs[2];
};

int hmapinit(Hashmap *ht);
int hmapfree(Hashmap *ht);
int hmapdel(Hashmap *ht, uint64_t key, uint64_t *valp);
int hmapget(Hashmap *ht, uint64_t key, uint64_t *valp);
int hmapput(Hashmap *ht, uint64_t key, uint64_t val);
int hmapstats(Hashmap *ht, size_t *lens, size_t nlens);
