#pragma lib "/$M/lib/ape/libap.a"

#undef assert
#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
#ifdef __cplusplus
extern "C" {
#endif

extern void _assert(char *, unsigned);

#ifdef __cplusplus
}
#endif
#define assert(e) {if(!(e))_assert(__FILE__, __LINE__);}
#endif /* NDEBUG */
