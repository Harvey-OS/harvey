#pragma	lib	"libbin.a"
#pragma	src	"/sys/src/libbin"

typedef struct Bin	Bin;

#pragma incomplete Bin

void	*binalloc(Bin **, ulong size, int zero);
void	*bingrow(Bin **, void *op, ulong osize, ulong size, int zero);
void	binfree(Bin **);
