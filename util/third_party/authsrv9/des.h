typedef struct Desks Desks;
struct Desks
{
	uvlong keys[16];
};

void deskey56(Desks *ks, uchar *k56);
void deskey64(Desks *ks, uchar *k64);
void desenc(Desks *ks, void *buf);
void desdec(Desks *ks, void *buf);
