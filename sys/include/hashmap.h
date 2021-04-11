
typedef struct Hashentry Hashentry;
typedef struct Hashtable Hashtable;
typedef struct Hashmap Hashmap;

struct Hashentry {
	u64 key;
	u64 val;
};

struct Hashtable {
	Hashentry *tab;
	usize len;
	usize cap; // always a power of 2.
};

struct Hashmap {
	Hashtable *cur;
	Hashtable *next;
	Hashtable tabs[2];
};

int hmapinit(Hashmap *ht);
int hmapfree(Hashmap *ht);
int hmapdel(Hashmap *ht, u64 key, u64 *valp);
int hmapget(Hashmap *ht, u64 key, u64 *valp);
int hmapput(Hashmap *ht, u64 key, u64 val);
int hmapstats(Hashmap *ht, usize *lens, usize nlens);
