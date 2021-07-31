typedef struct Can	Can;

void	*canAlloc(Can **can, ulong size, int zero);
void	*canGrow(Can **can, void *op, ulong osize, ulong size, int zero);
void	freeCan(Can *can);
