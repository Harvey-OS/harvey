
u16 get16be(u8 *p);
u32 get24be(u8 *p);
u32 get32be(u8 *p);
u64 get64be(u8 *p);

void put16be(u8 *p, int x);
void put24be(u8 *p, int x);
void put32be(u8 *p, u32 x);
void put64be(u8 *p, u64 x);

u16 get16le(u8 *p);
u32 get24le(u8 *p);
u32 get32le(u8 *p);
u64 get64le(u8 *p);

void put16le(u8 *p, int x);
void put24le(u8 *p, int x);
void put32le(u8 *p, u32 x);
void put64le(u8 *p, u64 x);
