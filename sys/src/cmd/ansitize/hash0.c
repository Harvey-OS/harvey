typedef struct Hashiter	Hashiter;
typedef struct Hash Hash;
#define nil 0

struct Hashiter
{
	int	i;
	int	j;
	Hash	*h;
};

typedef struct Xel Xel;
struct Xel
{
	Xel *next;
	void *key;
	void *value;
};

struct Hash
{
	int nel;		/* number of elements */
	Xel **tab;		/* hash chain heads */
	int size;		/* hashsize[size] heads in tab */
};

extern int hashsize[];

static Xel*
hashiternext(Hashiter *hi)
{
	int j;
	Xel *x;
	Hash *h;
	
	h = 0;
	for(;;){
		for(j=0, x=h->tab[hi->i]; j<hi->j && x!=nil; j++, x=x->next)
			;
		if(x){
			hi->j++;
			return x;
		}
		hi->j = 0;
		hi->i++;
	}
	/* stupid 8c */
//	return 0;
}
