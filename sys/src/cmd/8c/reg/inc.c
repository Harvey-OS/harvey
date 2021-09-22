#pragma incomplete union kick
#pragma incomplete struct zot
typedef union Fred Fred;
#pragma incomplete Fred
typedef struct zot Zot;

struct zot {
	int	x, y;
};

void f(struct zot *);
struct zot *g(void);
void kk(Zot*);

void
h(int k)
{
	struct zot *p;

	p = g();
	p->x = k;
	kk(p);
	f(p);
}
