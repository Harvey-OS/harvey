typedef	struct Lruhead	Lruhead;
typedef struct Lru	Lru;

struct Lru
{
	Lru	*lprev;
	Lru	*lnext;
};

void	lruinit(Lru*);
void	lruadd(Lru*, Lru*);
void	lruref(Lru*, Lru*);
void	lruderef(Lru*, Lru*);
