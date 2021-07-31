#pragma lib "/$M/lib/ape/libap.a"

#undef assert
#ifdef NDEBUG
#define assert(ignore) ((void)0)
#else
#ifdef __cplusplus
extern "C" {
#endif

extern int _assert(char *f, unsigned l);

#ifdef __cplusplus
}
#endif
#define assert(e) ((void)((e)||_assert(__FILE__, __LINE__)))
#endif /* NDEBUG */
