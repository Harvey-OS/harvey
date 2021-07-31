#pragma lib "ape/libap.a"

extern "C" {
	extern char* memccpy(char*, const char*, int, int);
	extern char* memchr(const char*, int, int);
	extern int memcmp(const char*, const char*, int);
	extern char* memcpy(char*, const char*, int);
	extern void* memset(void*, int, int);
}
