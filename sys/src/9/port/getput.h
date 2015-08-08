
uint16_t get16be(uint8_t* p);
uint32_t get24be(uint8_t* p);
uint32_t get32be(uint8_t* p);
uint64_t get64be(uint8_t* p);

void put16be(uint8_t* p, int x);
void put24be(uint8_t* p, int x);
void put32be(uint8_t* p, uint32_t x);
void put64be(uint8_t* p, uint64_t x);

uint16_t get16le(uint8_t* p);
uint32_t get24le(uint8_t* p);
uint32_t get32le(uint8_t* p);
uint64_t get64le(uint8_t* p);

void put16le(uint8_t* p, int x);
void put24le(uint8_t* p, int x);
void put32le(uint8_t* p, uint32_t x);
void put64le(uint8_t* p, uint64_t x);
