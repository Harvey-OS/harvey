#pragma	lib	"libbin.a"
#pragma	src	"/sys/src/libbin"

typedef struct Bin	Bin;

#pragma incomplete Bin

void	*binalloc(Bin **, uintptr size, int zero);
void	*bingrow(Bin **, void *op, uintptr osize, uintptr size, int zero);
void	binfree(Bin **);
